/* libguestfs
 * Copyright (C) 2009-2013 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/signal.h>

#include "cloexec.h"
#include "c-ctype.h"

#include "guestfs.h"
#include "guestfs-internal.h"
#include "guestfs-internal-actions.h"
#include "guestfs_protocol.h"

/* Per-handle data. */
struct backend_uml_data {
  pid_t pid;                    /* vmlinux PID. */
  pid_t recoverypid;            /* Recovery process PID. */

#define UML_UMID_LEN 16
  char umid[UML_UMID_LEN+1];    /* umid=<...> unique ID. */
};

static void print_vmlinux_command_line (guestfs_h *g, char **argv);
static char *make_cow_overlay (guestfs_h *g, const char *original);
static int kill_vmlinux (guestfs_h *g, struct backend_uml_data *data, int signum);

/* Test for features which are not supported by the UML backend.
 * Possibly some of these should just be warnings, not errors.
 */
static bool
uml_supported (guestfs_h *g)
{
  size_t i;
  struct drive *drv;

  if (g->enable_network) {
    error (g, _("uml backend does not support networking"));
    return false;
  }
  if (g->smp > 1) {
    error (g, _("uml backend does not support SMP"));
    return false;
  }

  ITER_DRIVES (g, i, drv) {
    if (drv->src.protocol != drive_protocol_file) {
      error (g, _("uml backend does not support remote drives"));
      return false;
    }
    if (drv->format && STRNEQ (drv->format, "raw")) {
      error (g, _("uml backend does not support non-raw-format drives"));
      return false;
    }
    if (drv->iface) {
      error (g,
             _("uml backend does not support drives with 'iface' parameter"));
      return false;
    }
    if (drv->disk_label) {
      error (g,
             _("uml backend does not support drives with 'label' parameter"));
      return false;
    }
  }

  return true;
}

static int
launch_uml (guestfs_h *g, void *datav, const char *arg)
{
  struct backend_uml_data *data = datav;
  CLEANUP_FREE_STRINGSBUF DECLARE_STRINGSBUF (cmdline);
  int console_sock = -1, daemon_sock = -1;
  int r;
  int csv[2], dsv[2];
  CLEANUP_FREE char *kernel = NULL, *initrd = NULL, *appliance = NULL;
  int has_appliance_drive;
  CLEANUP_FREE char *appliance_cow = NULL;
  uint32_t size;
  CLEANUP_FREE void *buf = NULL;
  struct drive *drv;
  size_t i;
  struct hv_param *hp;
  char *term = getenv ("TERM");

  if (!uml_supported (g))
    return -1;

  if (!g->nr_drives) {
    error (g, _("you must call guestfs_add_drive before guestfs_launch"));
    return -1;
  }

  /* Assign a random unique ID to this run. */
  if (guestfs___random_string (data->umid, UML_UMID_LEN) == -1) {
    perrorf (g, "guestfs___random_string");
    return -1;
  }

  /* Locate and/or build the appliance. */
  if (guestfs___build_appliance (g, &kernel, &initrd, &appliance) == -1)
    return -1;
  has_appliance_drive = appliance != NULL;

  /* Create COW overlays for any readonly drives, and for the root.
   * Note that the documented syntax ubd0=cow,orig does not work since
   * kernel 3.3.  See:
   * http://thread.gmane.org/gmane.linux.uml.devel/13556
   */
  ITER_DRIVES (g, i, drv) {
    if (drv->readonly) {
      drv->priv = make_cow_overlay (g, drv->src.u.path);
      if (!drv->priv)
        goto cleanup0;
      drv->free_priv = free;
    }
  }

  if (has_appliance_drive) {
    appliance_cow = make_cow_overlay (g, appliance);
    if (!appliance_cow)
      goto cleanup0;
  }

  /* The socket that the daemon will talk to us on.
   */
  if (socketpair (AF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0, dsv) == -1) {
    perrorf (g, "socketpair");
    goto cleanup0;
  }

  /* The console socket. */
  if (!g->direct_mode) {
    if (socketpair (AF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0, csv) == -1) {
      perrorf (g, "socketpair");
      close (dsv[0]);
      close (dsv[1]);
      goto cleanup0;
    }
  }

  /* Construct the vmlinux command line.  We have to do this before
   * forking, because after fork we are not allowed to use
   * non-signal-safe functions such as malloc.
   */
#define ADD_CMDLINE(str) \
  guestfs___add_string (g, &cmdline, (str))
#define ADD_CMDLINE_PRINTF(fs,...) \
  guestfs___add_sprintf (g, &cmdline, (fs), ##__VA_ARGS__)

  ADD_CMDLINE (g->hv);

  /* UMID: *NB* This must be the first parameter on the command line
   * because of kill_vmlinux below.
   */
  ADD_CMDLINE_PRINTF ("umid=%s", data->umid);

  /* Set memory size. */
  ADD_CMDLINE_PRINTF ("mem=%dM", g->memsize);

  /* vmlinux appears to ignore this, but let's add it anyway. */
  ADD_CMDLINE_PRINTF ("initrd=%s", initrd);

  /* Make sure our appliance init script runs first. */
  ADD_CMDLINE ("init=/init");

  /* This tells the /init script not to reboot at the end. */
  ADD_CMDLINE ("guestfs_noreboot=1");

  /* Root filesystem should be mounted read-write (default seems to
   * be "ro").
   */
  ADD_CMDLINE ("rw");

  /* See also guestfs___appliance_command_line. */
  if (g->verbose)
    ADD_CMDLINE ("guestfs_verbose=1");

  ADD_CMDLINE ("panic=1");

  ADD_CMDLINE_PRINTF ("TERM=%s", term ? term : "linux");

  if (g->selinux)
    ADD_CMDLINE ("selinux=1 enforcing=0");
  else
    ADD_CMDLINE ("selinux=0");

  /* XXX This isn't quite right.  Multiple append args won't work. */
  if (g->append)
    ADD_CMDLINE (g->append);

  /* Add the drives. */
  ITER_DRIVES (g, i, drv) {
    if (!drv->readonly)
      ADD_CMDLINE_PRINTF ("ubd%zu=%s", i, drv->src.u.path);
    else
      ADD_CMDLINE_PRINTF ("ubd%zu=%s", i, (char *) drv->priv);
  }

  /* Add the ext2 appliance drive (after all the drives). */
  if (has_appliance_drive) {
    char drv_name[64] = "ubd";
    guestfs___drive_name (g->nr_drives, &drv_name[3]);

    ADD_CMDLINE_PRINTF ("ubd%zu=%s", g->nr_drives, appliance_cow);
    ADD_CMDLINE_PRINTF ("root=/dev/%s", drv_name);
  }

  /* Create the daemon socket. */
  ADD_CMDLINE_PRINTF ("ssl3=fd:%d", dsv[1]);
  ADD_CMDLINE ("guestfs_channel=/dev/ttyS3");

#if 0 /* XXX This could be made to work. */
#ifdef VALGRIND_DAEMON
  /* Set up virtio-serial channel for valgrind messages. */
  ADD_CMDLINE ("-chardev");
  ADD_CMDLINE_PRINTF ("file,path=%s/valgrind.log.%d,id=valgrind",
                      VALGRIND_LOG_PATH, getpid ());
  ADD_CMDLINE ("-device");
  ADD_CMDLINE ("virtserialport,chardev=valgrind,name=org.libguestfs.valgrind");
#endif
#endif

  /* Add any vmlinux parameters. */
  for (hp = g->hv_params; hp; hp = hp->next) {
    ADD_CMDLINE (hp->hv_param);
    if (hp->hv_value)
      ADD_CMDLINE (hp->hv_value);
    }

  /* Finish off the command line. */
  guestfs___end_stringsbuf (g, &cmdline);

  r = fork ();
  if (r == -1) {
    perrorf (g, "fork");
    if (!g->direct_mode) {
      close (csv[0]);
      close (csv[1]);
    }
    close (dsv[0]);
    close (dsv[1]);
    goto cleanup0;
  }

  if (r == 0) {                 /* Child (vmlinux). */
    /* Set up the daemon socket for the child. */
    close (dsv[0]);
    set_cloexec_flag (dsv[1], 0); /* so it doesn't close across exec */

    if (!g->direct_mode) {
      /* Set up stdin, stdout, stderr. */
      close (0);
      close (1);
      close (csv[0]);

      /* We set the FD_CLOEXEC flag on the socket above, but now (in
       * the child) it's safe to unset this flag so vmlinux can use the
       * socket.
       */
      set_cloexec_flag (csv[1], 0);

      /* Stdin. */
      if (dup (csv[1]) == -1) {
      dup_failed:
        perror ("dup failed");
        _exit (EXIT_FAILURE);
      }
      /* Stdout. */
      if (dup (csv[1]) == -1)
        goto dup_failed;

      /* Send stderr to the pipe as well. */
      close (2);
      if (dup (csv[1]) == -1)
        goto dup_failed;

      close (csv[1]);
    }

    /* Dump the command line (after setting up stderr above). */
    if (g->verbose)
      print_vmlinux_command_line (g, cmdline.argv);

    /* Put vmlinux in a new process group. */
    if (g->pgroup)
      setpgid (0, 0);

    setenv ("LC_ALL", "C", 1);

    execv (g->hv, cmdline.argv); /* Run vmlinux. */
    perror (g->hv);
    _exit (EXIT_FAILURE);
  }

  /* Parent (library). */
  data->pid = r;

  /* Fork the recovery process off which will kill vmlinux if the
   * parent process fails to do so (eg. if the parent segfaults).
   */
  data->recoverypid = -1;
  if (g->recovery_proc) {
    r = fork ();
    if (r == 0) {
      int i, fd, max_fd;
      struct sigaction sa;
      pid_t vmlinux_pid = data->pid;
      pid_t parent_pid = getppid ();

      /* Remove all signal handlers.  See the justification here:
       * https://www.redhat.com/archives/libvir-list/2008-August/msg00303.html
       * We don't mask signal handlers yet, so this isn't completely
       * race-free, but better than not doing it at all.
       */
      memset (&sa, 0, sizeof sa);
      sa.sa_handler = SIG_DFL;
      sa.sa_flags = 0;
      sigemptyset (&sa.sa_mask);
      for (i = 1; i < NSIG; ++i)
        sigaction (i, &sa, NULL);

      /* Close all other file descriptors.  This ensures that we don't
       * hold open (eg) pipes from the parent process.
       */
      max_fd = sysconf (_SC_OPEN_MAX);
      if (max_fd == -1)
        max_fd = 1024;
      if (max_fd > 65536)
        max_fd = 65536; /* bound the amount of work we do here */
      for (fd = 0; fd < max_fd; ++fd)
        close (fd);

      /* It would be nice to be able to put this in the same process
       * group as vmlinux (ie. setpgid (0, vmlinux_pid)).  However
       * this is not possible because we don't have any guarantee here
       * that the vmlinux process has started yet.
       */
      if (g->pgroup)
        setpgid (0, 0);

      /* Writing to argv is hideously complicated and error prone.  See:
       * http://git.postgresql.org/gitweb/?p=postgresql.git;a=blob;f=src/backend/utils/misc/ps_status.c;hb=HEAD
       */

      /* Loop around waiting for one or both of the other processes to
       * disappear.  It's fair to say this is very hairy.  The PIDs that
       * we are looking at might be reused by another process.  We are
       * effectively polling.  Is the cure worse than the disease?
       */
      for (;;) {
        if (kill (vmlinux_pid, 0) == -1)
          /* vmlinux's gone away, we aren't needed */
          _exit (EXIT_SUCCESS);
        if (kill (parent_pid, 0) == -1) {
          /* Parent's gone away, vmlinux still around, so kill vmlinux. */
          kill_vmlinux (g, data, SIGKILL);
          _exit (EXIT_SUCCESS);
        }
        sleep (2);
      }
    }

    /* Don't worry, if the fork failed, this will be -1.  The recovery
     * process isn't essential.
     */
    data->recoverypid = r;
  }

  if (!g->direct_mode) {
    /* Close the other end of the console socketpair. */
    close (csv[1]);

    console_sock = csv[0];      /* stdin of child */
    csv[0] = -1;
  }

  daemon_sock = dsv[0];
  close (dsv[1]);
  dsv[0] = -1;

  g->state = LAUNCHING;

  /* Wait for vmlinux to start and to connect back to us via
   * virtio-serial and send the GUESTFS_LAUNCH_FLAG message.
   */
  g->conn =
    guestfs___new_conn_socket_connected (g, daemon_sock, console_sock);
  if (!g->conn)
    goto cleanup1;

  /* g->conn now owns these sockets. */
  daemon_sock = console_sock = -1;

  /* We now have to wait for vmlinux to start up, the daemon to start
   * running, and for it to send the GUESTFS_LAUNCH_FLAG to us.
   */
  r = guestfs___recv_from_daemon (g, &size, &buf);

  if (r == -1) {
    guestfs___launch_failed_error (g);
    goto cleanup1;
  }

  if (size != GUESTFS_LAUNCH_FLAG) {
    guestfs___launch_failed_error (g);
    goto cleanup1;
  }

  if (g->verbose)
    guestfs___print_timestamped_message (g, "appliance is up");

  /* This is possible in some really strange situations, such as
   * guestfsd starts up OK but then vmlinux immediately exits.  Check
   * for it because the caller is probably expecting to be able to
   * send commands after this function returns.
   */
  if (g->state != READY) {
    error (g, _("vmlinux launched and contacted daemon, but state != READY"));
    goto cleanup1;
  }

  if (has_appliance_drive)
    guestfs___add_dummy_appliance_drive (g);

  return 0;

 cleanup1:
  if (!g->direct_mode && csv[0] >= 0)
    close (csv[0]);
  if (dsv[0] >= 0)
    close (dsv[0]);
  if (data->pid > 0) kill_vmlinux (g, data, SIGKILL);
  if (data->recoverypid > 0) kill (data->recoverypid, SIGKILL);
  if (data->pid > 0) waitpid (data->pid, NULL, 0);
  if (data->recoverypid > 0) waitpid (data->recoverypid, NULL, 0);
  data->pid = 0;
  data->recoverypid = 0;
  memset (&g->launch_t, 0, sizeof g->launch_t);

 cleanup0:
  if (daemon_sock >= 0)
    close (daemon_sock);
  if (console_sock >= 0)
    close (console_sock);
  if (g->conn) {
    g->conn->ops->free_connection (g, g->conn);
    g->conn = NULL;
  }
  g->state = CONFIG;
  return -1;
}

static bool
is_numeric (const char *name)
{
  if (!*name)
    return false;

  while (*name) {
    if (!c_isdigit (*name))
      return false;
    name++;
  }

  return true;
}

/* You can't just kill the parent vmlinux PID (data->pid) and have
 * the whole process go away.  You have to kill all related vmlinux
 * PIDs.  I think this is related to a 'FIXME' in the UML code (in
 * function 'kill_off_processes') which seems to indicate that it
 * doesn't kill any UML-userspace processes.
 *
 * We use the 'umid=' on the command line (by reading /proc/PID/cmdline)
 * to identify the related PIDs so we can kill them too.
 *
 * XXX Possibly insecure if a process spoofs umid= argument for some
 * reason.
 */
static int
kill_vmlinux (guestfs_h *g, struct backend_uml_data *data, int signum)
{
  DIR *dir;

  kill (data->pid, signum);

  /* Find the related processes. */
  dir = opendir ("/proc");
  if (dir == NULL) {
    perrorf (g, "opendir: /proc");
    return -1;
  }

  for (;;) {
    pid_t pid;
    struct dirent *d;
    FILE *fp;
    char procname[64];
    size_t i;
    char umid[UML_UMID_LEN+1];

    errno = 0;
    d = readdir (dir);
    if (d == NULL) break;

    /* Ignore anything which is not numeric. */
    if (! is_numeric (d->d_name))
      continue;

    if (sscanf (d->d_name, "%d", &pid) != 1)
      continue;

    if (pid <= 0)
      continue;

    snprintf (procname, sizeof procname, "/proc/%d/cmdline", pid);
    fp = fopen (procname, "r");
    if (fp == NULL)
      continue;                 /* Ignore it, there are many reasons
                                 * this could legitimately fail.
                                 */

    /* /proc/PID/cmdline is argv[0].. with each string separated by a \0.
     * As umid= parameter is always argv[1], search for the first \0
     * followed by 'u' 'm' 'i' 'd' '='.
     */
    for (;;) {
      int c = fgetc (fp);
      if (c == 0)
        goto found_0;
      if (c == EOF)
        goto not_found;
    }

  found_0:
    if (fgetc (fp) != 'u' ||
        fgetc (fp) != 'm' ||
        fgetc (fp) != 'i' ||
        fgetc (fp) != 'd' ||
        fgetc (fp) != '=')
      goto not_found;

    for (i = 0; i < UML_UMID_LEN; ++i) {
      int c = fgetc (fp);
      if (c == 0 || c == EOF)
        goto not_found;
      umid[i] = c;
    }
    umid[i] = '\0';

    if (STRNEQ (umid, data->umid))
      goto not_found;

    /* Found a related process - kill it! */
    kill (pid, signum);

  not_found:
    fclose (fp);
  }

  if (errno != 0) {
    perrorf (g, "readdir: /proc");
    closedir (dir);
    return -1;
  }

  if (closedir (dir) == -1) {
    perrorf (g, "closedir: /proc");
    return -1;
  }

  return 0;
}

/* Run uml_mkcow to create a COW overlay.  This works around a kernel
 * bug in UML option parsing.
 */
static char *
make_cow_overlay (guestfs_h *g, const char *original)
{
  CLEANUP_CMD_CLOSE struct command *cmd = guestfs___new_command (g);
  char *cow;
  int r;

  cow = safe_asprintf (g, "%s/cow%d", g->tmpdir, g->unique++);

  guestfs___cmd_add_arg (cmd, "uml_mkcow");
  guestfs___cmd_add_arg (cmd, cow);
  guestfs___cmd_add_arg (cmd, original);
  r = guestfs___cmd_run (cmd);
  if (r == -1) {
    free (cow);
    return NULL;
  }
  if (!WIFEXITED (r) || WEXITSTATUS (r) != 0) {
    guestfs___external_command_failed (g, r, "uml_mkcow", original);
    free (cow);
    return NULL;
  }

  return cow;                   /* caller must free */
}

/* This is called from the forked subprocess just before vmlinux runs,
 * so it can just print the message straight to stderr, where it will
 * be picked up and funnelled through the usual appliance event API.
 */
static void
print_vmlinux_command_line (guestfs_h *g, char **argv)
{
  size_t i = 0;
  int needs_quote;

  struct timeval tv;
  gettimeofday (&tv, NULL);
  fprintf (stderr, "[%05" PRIi64 "ms] ",
           guestfs___timeval_diff (&g->launch_t, &tv));

  while (argv[i]) {
    if (i > 0) fputc (' ', stderr);

    /* Does it need shell quoting?  This only deals with simple cases. */
    needs_quote = strcspn (argv[i], " ") != strlen (argv[i]);

    if (needs_quote) fputc ('\'', stderr);
    fprintf (stderr, "%s", argv[i]);
    if (needs_quote) fputc ('\'', stderr);
    i++;
  }

  fputc ('\n', stderr);
}

static int
shutdown_uml (guestfs_h *g, void *datav, int check_for_errors)
{
  struct backend_uml_data *data = datav;
  int ret = 0;
  int status;

  /* Signal vmlinux to shutdown cleanly, and kill the recovery process. */
  if (data->pid > 0) {
    debug (g, "sending SIGTERM to process %d", data->pid);
    kill_vmlinux (g, data, SIGTERM);
  }
  if (data->recoverypid > 0) kill (data->recoverypid, 9);

  /* Wait for subprocess(es) to exit. */
  if (data->pid > 0) {
    if (waitpid (data->pid, &status, 0) == -1) {
      perrorf (g, "waitpid (vmlinux)");
      ret = -1;
    }
    /* Note it's normal for the vmlinux process to exit with status
     * "killed by signal 15" (where 15 == SIGTERM).  So don't consider
     * that to be an error.
     */
    else if (!(WIFSIGNALED (status) && WTERMSIG (status) == SIGTERM) &&
             !(WIFEXITED (status) && WEXITSTATUS (status) == 0)) {
      guestfs___external_command_failed (g, status, g->hv, NULL);
      ret = -1;
    }
  }
  if (data->recoverypid > 0) waitpid (data->recoverypid, NULL, 0);

  data->pid = data->recoverypid = 0;

  return ret;
}

static int
get_pid_uml (guestfs_h *g, void *datav)
{
  struct backend_uml_data *data = datav;

  if (data->pid > 0)
    return data->pid;
  else {
    error (g, "get_pid: no vmlinux subprocess");
    return -1;
  }
}

/* UML appears to use a single major, and puts ubda at minor 0 with
 * each partition at minors 1-15, ubdb at minor 16, etc.  So the
 * maximum is 256/16 = 16.  However one disk is used by the appliance,
 * so it's one less than this.  I tested both 15 & 16 disks, and found
 * that 15 worked and 16 failed.
 */
static int
max_disks_uml (guestfs_h *g, void *datav)
{
  return 15;
}

static struct backend_ops backend_uml_ops = {
  .data_size = sizeof (struct backend_uml_data),
  .launch = launch_uml,
  .shutdown = shutdown_uml,
  .get_pid = get_pid_uml,
  .max_disks = max_disks_uml,
};

static void init_backend (void) __attribute__((constructor));
static void
init_backend (void)
{
  guestfs___register_backend ("uml", &backend_uml_ops);
}
