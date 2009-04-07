/* libguestfs - the guestfsd daemon
 * Copyright (C) 2009 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef GUESTFSD_DAEMON_H
#define GUESTFSD_DAEMON_H

#include "../src/guestfs_protocol.h"

#include <stdarg.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

/* in guestfsd.c */
extern void xwrite (int sock, const void *buf, size_t len);
extern void xread (int sock, void *buf, size_t len);

extern int add_string (char ***argv, int *size, int *alloc, const char *str);
extern int count_strings (char **argv);
extern void sort_strings (char **argv, int len);
extern void free_strings (char **argv);
extern void free_stringslen (char **argv, int len);

extern int command (char **stdoutput, char **stderror, const char *name, ...);

/* in proto.c */
extern int proc_nr;
extern int serial;

/* in mount.c */
extern int root_mounted;

/* in stubs.c (auto-generated) */
extern void dispatch_incoming_message (XDR *);
extern guestfs_lvm_int_pv_list *parse_command_line_pvs (void);
extern guestfs_lvm_int_vg_list *parse_command_line_vgs (void);
extern guestfs_lvm_int_lv_list *parse_command_line_lvs (void);

/* in proto.c */
extern void main_loop (int sock);
extern void reply_with_error (const char *fs, ...);
extern void reply_with_perror (const char *fs, ...);
extern void reply (xdrproc_t xdrp, char *ret);

#define NEED_ROOT(errcode)						\
  do {									\
    if (!root_mounted) {						\
      reply_with_error ("%s: you must call 'mount' first to mount the root filesystem", __func__); \
      return (errcode);							\
    }									\
  }									\
  while (0)

#define ABS_PATH(path,errcode)						\
  do {									\
    if ((path)[0] != '/') {						\
      reply_with_error ("%s: path must start with a / character", __func__); \
      return (errcode);							\
    }									\
  } while (0)

/* NB:
 * (1) You must match CHROOT_IN and CHROOT_OUT even along error paths.
 * (2) You must not change directory!  cwd must always be "/", otherwise
 *     we can't escape our own chroot.
 * (3) All paths specified must be absolute.
 */
#define CHROOT_IN chroot ("/sysroot");
#define CHROOT_OUT chroot (".");

#endif /* GUESTFSD_DAEMON_H */
