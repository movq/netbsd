/* $NetBSD$ */

#ifndef _NETBSD_SPL_SYS_FM_PROTOCOL_H_
#define _NETBSD_SPL_SYS_FM_PROTOCOL_H_

/* The Solaris module retains its own fault-management implementation. */
#define fm_ena_format_get	zfs_fm_ena_format_get
#define fm_ena_generate		zfs_fm_ena_generate
#define fm_ena_generate_cpu	zfs_fm_ena_generate_cpu
#define fm_ena_generation_get	zfs_fm_ena_generation_get
#define fm_ena_id_get		zfs_fm_ena_id_get
#define fm_ena_increment		zfs_fm_ena_increment
#define fm_ena_time_get		zfs_fm_ena_time_get
#define fm_ereport_set		zfs_fm_ereport_set
#define fm_fmri_cpu_set		zfs_fm_fmri_cpu_set
#define fm_fmri_dev_set		zfs_fm_fmri_dev_set
#define fm_fmri_hc_create	zfs_fm_fmri_hc_create
#define fm_fmri_hc_set		zfs_fm_fmri_hc_set
#define fm_fmri_mem_set		zfs_fm_fmri_mem_set
#define fm_fmri_zfs_set		zfs_fm_fmri_zfs_set
#define fm_nva_xcreate		zfs_fm_nva_xcreate
#define fm_nva_xdestroy		zfs_fm_nva_xdestroy
#define fm_payload_set		zfs_fm_payload_set
#define i_fm_payload_set		zfs_i_fm_payload_set

#include_next <sys/fm/protocol.h>

#endif /* _NETBSD_SPL_SYS_FM_PROTOCOL_H_ */
