/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_KSTAT_H_
#define	_NETBSD_SPL_KSTAT_H_

#include <sys/types.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#define	KSTAT_STRLEN		255
#define	KSTAT_RAW_MAX		(128 * 1024)
#define	KSTAT_TYPE_RAW		0
#define	KSTAT_TYPE_NAMED	1
#define	KSTAT_FLAG_VIRTUAL	0x01
#define	KSTAT_FLAG_NO_HEADERS	0x80
#define	KSTAT_READ		0
#define	KSTAT_WRITE		1

#define	KSTAT_DATA_CHAR		0
#define	KSTAT_DATA_INT32	1
#define	KSTAT_DATA_UINT32	2
#define	KSTAT_DATA_INT64	3
#define	KSTAT_DATA_UINT64	4
#define	KSTAT_DATA_LONG		5
#define	KSTAT_DATA_ULONG	6
#define	KSTAT_DATA_STRING	7

typedef struct kstat_named {
	char	name[KSTAT_STRLEN];
	uchar_t	data_type;
	union {
		char		c[16];
		int32_t		i32;
		uint32_t	ui32;
		int64_t		i64;
		uint64_t	ui64;
		long		l;
		ulong_t		ul;
		struct {
			union {
				char	*ptr;
				char	__pad[8];
			} addr;
			uint32_t len;
		} string;
	} value;
} kstat_named_t;

#define	KSTAT_NAMED_STR_PTR(k)		((k)->value.string.addr.ptr)
#define	KSTAT_NAMED_STR_BUFLEN(k)	((k)->value.string.len)

struct seq_file {
	char	*sf_buf;
	size_t	sf_size;
	size_t	sf_len;
	int	sf_error;
};

typedef struct kstat kstat_t;
typedef struct kstat_raw_ops {
	int	(*headers)(char *, size_t);
	int	(*seq_headers)(struct seq_file *);
	int	(*data)(char *, size_t, void *);
	void	*(*addr)(kstat_t *, loff_t);
} kstat_raw_ops_t;

struct openzfs_kstat_entry;
struct kstat {
	void		*ks_data;
	uint_t		ks_ndata;
	size_t		ks_data_size;
	int		(*ks_update)(kstat_t *, int);
	void		*ks_private;
	kmutex_t	*ks_lock;
	uchar_t		ks_type;
	uchar_t		ks_flags;
	kstat_raw_ops_t	ks_raw_ops;

	/* Private to the NetBSD implementation. */
	kmutex_t	ks_private_lock;
	char		ks_module[KSTAT_STRLEN];
	char		ks_name[KSTAT_STRLEN];
	char		ks_class[KSTAT_STRLEN];
	struct sysctllog	*ks_log;
	int		ks_sysctl_flags;
	struct openzfs_kstat_entry *ks_entries;
	size_t		ks_entries_size;
	size_t		ks_alloc_size;
	boolean_t	ks_installed;
	LIST_ENTRY(kstat) ks_link;
};

/* The solaris module continues to export the old kstat ABI. */
#define	kstat_create		openzfs_kstat_create
#define	kstat_install		openzfs_kstat_install
#define	kstat_delete		openzfs_kstat_delete
#define	kstat_named_init	openzfs_kstat_named_init
#define	kstat_set_string	openzfs_kstat_set_string
#define	kstat_set_raw_ops	openzfs_kstat_set_raw_ops
#define	kstat_set_seq_raw_ops	openzfs_kstat_set_seq_raw_ops
#define	seq_printf		openzfs_seq_printf
#define	spl_kstat_init		openzfs_kstat_init
#define	spl_kstat_fini		openzfs_kstat_fini

kstat_t *kstat_create(const char *, int, const char *, const char *,
    uchar_t, uint_t, uchar_t);
void kstat_install(kstat_t *);
void kstat_delete(kstat_t *);
void kstat_named_init(kstat_named_t *, const char *, uchar_t);
void kstat_set_string(char *, const char *);
void kstat_set_raw_ops(kstat_t *, int (*)(char *, size_t),
    int (*)(char *, size_t, void *), void *(*)(kstat_t *, loff_t));
void kstat_set_seq_raw_ops(kstat_t *, int (*)(struct seq_file *),
    int (*)(char *, size_t, void *), void *(*)(kstat_t *, loff_t));
void seq_printf(struct seq_file *, const char *, ...) __printflike(2, 3);
int spl_kstat_init(void);
void spl_kstat_fini(void);

#endif
