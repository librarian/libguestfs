/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _GUESTFS_PROTOCOL_H_RPCGEN
#define _GUESTFS_PROTOCOL_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef char *str;

struct guestfs_lvm_int_pv {
	char *pv_name;
	char pv_uuid[32];
	char *pv_fmt;
	quad_t pv_size;
	quad_t dev_size;
	quad_t pv_free;
	quad_t pv_used;
	char *pv_attr;
	quad_t pv_pe_count;
	quad_t pv_pe_alloc_count;
	char *pv_tags;
	quad_t pe_start;
	quad_t pv_mda_count;
	quad_t pv_mda_free;
};
typedef struct guestfs_lvm_int_pv guestfs_lvm_int_pv;

typedef struct {
	u_int guestfs_lvm_int_pv_list_len;
	guestfs_lvm_int_pv *guestfs_lvm_int_pv_list_val;
} guestfs_lvm_int_pv_list;

struct guestfs_lvm_int_vg {
	char *vg_name;
	char vg_uuid[32];
	char *vg_fmt;
	char *vg_attr;
	quad_t vg_size;
	quad_t vg_free;
	char *vg_sysid;
	quad_t vg_extent_size;
	quad_t vg_extent_count;
	quad_t vg_free_count;
	quad_t max_lv;
	quad_t max_pv;
	quad_t pv_count;
	quad_t lv_count;
	quad_t snap_count;
	quad_t vg_seqno;
	char *vg_tags;
	quad_t vg_mda_count;
	quad_t vg_mda_free;
};
typedef struct guestfs_lvm_int_vg guestfs_lvm_int_vg;

typedef struct {
	u_int guestfs_lvm_int_vg_list_len;
	guestfs_lvm_int_vg *guestfs_lvm_int_vg_list_val;
} guestfs_lvm_int_vg_list;

struct guestfs_lvm_int_lv {
	char *lv_name;
	char lv_uuid[32];
	char *lv_attr;
	quad_t lv_major;
	quad_t lv_minor;
	quad_t lv_kernel_major;
	quad_t lv_kernel_minor;
	quad_t lv_size;
	quad_t seg_count;
	char *origin;
	float snap_percent;
	float copy_percent;
	char *move_pv;
	char *lv_tags;
	char *mirror_log;
	char *modules;
};
typedef struct guestfs_lvm_int_lv guestfs_lvm_int_lv;

typedef struct {
	u_int guestfs_lvm_int_lv_list_len;
	guestfs_lvm_int_lv *guestfs_lvm_int_lv_list_val;
} guestfs_lvm_int_lv_list;

struct guestfs_mount_args {
	char *device;
	char *mountpoint;
};
typedef struct guestfs_mount_args guestfs_mount_args;

struct guestfs_touch_args {
	char *path;
};
typedef struct guestfs_touch_args guestfs_touch_args;

struct guestfs_cat_args {
	char *path;
};
typedef struct guestfs_cat_args guestfs_cat_args;

struct guestfs_cat_ret {
	char *content;
};
typedef struct guestfs_cat_ret guestfs_cat_ret;

struct guestfs_ll_args {
	char *directory;
};
typedef struct guestfs_ll_args guestfs_ll_args;

struct guestfs_ll_ret {
	char *listing;
};
typedef struct guestfs_ll_ret guestfs_ll_ret;

struct guestfs_ls_args {
	char *directory;
};
typedef struct guestfs_ls_args guestfs_ls_args;

struct guestfs_ls_ret {
	struct {
		u_int listing_len;
		str *listing_val;
	} listing;
};
typedef struct guestfs_ls_ret guestfs_ls_ret;

struct guestfs_list_devices_ret {
	struct {
		u_int devices_len;
		str *devices_val;
	} devices;
};
typedef struct guestfs_list_devices_ret guestfs_list_devices_ret;

struct guestfs_list_partitions_ret {
	struct {
		u_int partitions_len;
		str *partitions_val;
	} partitions;
};
typedef struct guestfs_list_partitions_ret guestfs_list_partitions_ret;

struct guestfs_pvs_ret {
	guestfs_lvm_int_pv_list physvols;
};
typedef struct guestfs_pvs_ret guestfs_pvs_ret;

struct guestfs_vgs_ret {
	guestfs_lvm_int_vg_list volgroups;
};
typedef struct guestfs_vgs_ret guestfs_vgs_ret;

struct guestfs_lvs_ret {
	guestfs_lvm_int_lv_list logvols;
};
typedef struct guestfs_lvs_ret guestfs_lvs_ret;

enum guestfs_procedure {
	GUESTFS_PROC_MOUNT = 1,
	GUESTFS_PROC_SYNC = 2,
	GUESTFS_PROC_TOUCH = 3,
	GUESTFS_PROC_CAT = 4,
	GUESTFS_PROC_LL = 5,
	GUESTFS_PROC_LS = 6,
	GUESTFS_PROC_LIST_DEVICES = 7,
	GUESTFS_PROC_LIST_PARTITIONS = 8,
	GUESTFS_PROC_PVS = 9,
	GUESTFS_PROC_VGS = 10,
	GUESTFS_PROC_LVS = 11,
	GUESTFS_PROC_dummy = 11 + 1,
};
typedef enum guestfs_procedure guestfs_procedure;
#define GUESTFS_MESSAGE_MAX 4194304
#define GUESTFS_PROGRAM 0x2000F5F5
#define GUESTFS_PROTOCOL_VERSION 1

enum guestfs_message_direction {
	GUESTFS_DIRECTION_CALL = 0,
	GUESTFS_DIRECTION_REPLY = 1,
};
typedef enum guestfs_message_direction guestfs_message_direction;

enum guestfs_message_status {
	GUESTFS_STATUS_OK = 0,
	GUESTFS_STATUS_ERROR = 1,
};
typedef enum guestfs_message_status guestfs_message_status;
#define GUESTFS_ERROR_LEN 256

struct guestfs_message_error {
	char *error;
};
typedef struct guestfs_message_error guestfs_message_error;

struct guestfs_message_header {
	u_int prog;
	u_int vers;
	guestfs_procedure proc;
	guestfs_message_direction direction;
	u_int serial;
	guestfs_message_status status;
};
typedef struct guestfs_message_header guestfs_message_header;

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_str (XDR *, str*);
extern  bool_t xdr_guestfs_lvm_int_pv (XDR *, guestfs_lvm_int_pv*);
extern  bool_t xdr_guestfs_lvm_int_pv_list (XDR *, guestfs_lvm_int_pv_list*);
extern  bool_t xdr_guestfs_lvm_int_vg (XDR *, guestfs_lvm_int_vg*);
extern  bool_t xdr_guestfs_lvm_int_vg_list (XDR *, guestfs_lvm_int_vg_list*);
extern  bool_t xdr_guestfs_lvm_int_lv (XDR *, guestfs_lvm_int_lv*);
extern  bool_t xdr_guestfs_lvm_int_lv_list (XDR *, guestfs_lvm_int_lv_list*);
extern  bool_t xdr_guestfs_mount_args (XDR *, guestfs_mount_args*);
extern  bool_t xdr_guestfs_touch_args (XDR *, guestfs_touch_args*);
extern  bool_t xdr_guestfs_cat_args (XDR *, guestfs_cat_args*);
extern  bool_t xdr_guestfs_cat_ret (XDR *, guestfs_cat_ret*);
extern  bool_t xdr_guestfs_ll_args (XDR *, guestfs_ll_args*);
extern  bool_t xdr_guestfs_ll_ret (XDR *, guestfs_ll_ret*);
extern  bool_t xdr_guestfs_ls_args (XDR *, guestfs_ls_args*);
extern  bool_t xdr_guestfs_ls_ret (XDR *, guestfs_ls_ret*);
extern  bool_t xdr_guestfs_list_devices_ret (XDR *, guestfs_list_devices_ret*);
extern  bool_t xdr_guestfs_list_partitions_ret (XDR *, guestfs_list_partitions_ret*);
extern  bool_t xdr_guestfs_pvs_ret (XDR *, guestfs_pvs_ret*);
extern  bool_t xdr_guestfs_vgs_ret (XDR *, guestfs_vgs_ret*);
extern  bool_t xdr_guestfs_lvs_ret (XDR *, guestfs_lvs_ret*);
extern  bool_t xdr_guestfs_procedure (XDR *, guestfs_procedure*);
extern  bool_t xdr_guestfs_message_direction (XDR *, guestfs_message_direction*);
extern  bool_t xdr_guestfs_message_status (XDR *, guestfs_message_status*);
extern  bool_t xdr_guestfs_message_error (XDR *, guestfs_message_error*);
extern  bool_t xdr_guestfs_message_header (XDR *, guestfs_message_header*);

#else /* K&R C */
extern bool_t xdr_str ();
extern bool_t xdr_guestfs_lvm_int_pv ();
extern bool_t xdr_guestfs_lvm_int_pv_list ();
extern bool_t xdr_guestfs_lvm_int_vg ();
extern bool_t xdr_guestfs_lvm_int_vg_list ();
extern bool_t xdr_guestfs_lvm_int_lv ();
extern bool_t xdr_guestfs_lvm_int_lv_list ();
extern bool_t xdr_guestfs_mount_args ();
extern bool_t xdr_guestfs_touch_args ();
extern bool_t xdr_guestfs_cat_args ();
extern bool_t xdr_guestfs_cat_ret ();
extern bool_t xdr_guestfs_ll_args ();
extern bool_t xdr_guestfs_ll_ret ();
extern bool_t xdr_guestfs_ls_args ();
extern bool_t xdr_guestfs_ls_ret ();
extern bool_t xdr_guestfs_list_devices_ret ();
extern bool_t xdr_guestfs_list_partitions_ret ();
extern bool_t xdr_guestfs_pvs_ret ();
extern bool_t xdr_guestfs_vgs_ret ();
extern bool_t xdr_guestfs_lvs_ret ();
extern bool_t xdr_guestfs_procedure ();
extern bool_t xdr_guestfs_message_direction ();
extern bool_t xdr_guestfs_message_status ();
extern bool_t xdr_guestfs_message_error ();
extern bool_t xdr_guestfs_message_header ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_GUESTFS_PROTOCOL_H_RPCGEN */
