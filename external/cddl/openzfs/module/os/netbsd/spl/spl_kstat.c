/* SPDX-License-Identifier: BSD-2-Clause */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/kstat.h>

struct openzfs_kstat_entry {
	kstat_t	*kstat;
	uint_t	index;
};

/* Initialized by the module before any OpenZFS statistics are created. */
static kmutex_t kstat_config_lock;
static LIST_HEAD(, kstat) kstats;

int
spl_kstat_init(void)
{
	mutex_init(&kstat_config_lock, NULL, MUTEX_DEFAULT, NULL);
	LIST_INIT(&kstats);
	return (0);
}

void
spl_kstat_fini(void)
{
	KASSERT(LIST_EMPTY(&kstats));
	mutex_destroy(&kstat_config_lock);
}

/*
 * Handlers retain the sysctl tree read lock.  sysctl_teardown() takes the
 * writer lock, so deletion drains readers before freeing provider data.
 * Providers must not delete a kstat while holding its ks_lock.
 */
static int
kstat_update(kstat_t *ksp)
{
	return (ksp->ks_update != NULL ?
	    ksp->ks_update(ksp, KSTAT_READ) : 0);
}

static int
kstat_named_sysctl(SYSCTLFN_ARGS)
{
	struct openzfs_kstat_entry *entry = rnode->sysctl_data;
	kstat_t *ksp = entry->kstat;
	struct sysctlnode node = *rnode;
	kstat_named_t value;
	char *str = NULL;
	size_t len = 0;
	int error;

	if (newp != NULL)
		return (EPERM);

	mutex_enter(ksp->ks_lock);
	error = kstat_update(ksp);
	if (error == 0) {
		/*
		 * Some providers shorten ks_ndata to omit trailing zero
		 * buckets.  The allocated table still contains those buckets.
		 */
		value = ((kstat_named_t *)ksp->ks_data)[entry->index];
		if (value.data_type == KSTAT_DATA_STRING) {
			const char *src = KSTAT_NAMED_STR_PTR(&value);
			len = src != NULL ?
			    strnlen(src, KSTAT_NAMED_STR_BUFLEN(&value)) : 0;
			str = kmem_alloc(len + 1, KM_SLEEP);
			if (len != 0)
				memcpy(str, src, len);
			str[len] = '\0';
		}
	}
	mutex_exit(ksp->ks_lock);
	if (error != 0)
		return (error);

	switch (value.data_type) {
	case KSTAT_DATA_CHAR:
		node.sysctl_data = value.value.c;
		node.sysctl_size = sizeof (value.value.c);
		break;
	case KSTAT_DATA_INT32:
	case KSTAT_DATA_UINT32:
		node.sysctl_data = &value.value.ui32;
		node.sysctl_size = sizeof (value.value.ui32);
		break;
	case KSTAT_DATA_INT64:
	case KSTAT_DATA_UINT64:
		node.sysctl_data = &value.value.ui64;
		node.sysctl_size = sizeof (value.value.ui64);
		break;
	case KSTAT_DATA_LONG:
	case KSTAT_DATA_ULONG:
		node.sysctl_data = &value.value.ul;
		node.sysctl_size = sizeof (value.value.ul);
		break;
	case KSTAT_DATA_STRING:
		node.sysctl_data = str;
		node.sysctl_size = len + 1;
		break;
	default:
		return (EINVAL);
	}

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (str != NULL)
		kmem_free(str, len + 1);
	return (error);
}

struct kstat_output {
	struct lwp	*lwp;
	char		*buffer;
	size_t		capacity;
	size_t		length;
};

static int
kstat_output(struct kstat_output *out, const void *data, size_t len)
{
	int error = 0;

	if (len > SIZE_MAX - out->length)
		return (EOVERFLOW);
	if (out->buffer != NULL && out->length < out->capacity)
		error = sysctl_copyout(out->lwp, data,
		    out->buffer + out->length,
		    MIN(len, out->capacity - out->length));
	out->length += len;
	return (error);
}

/*
 * A complete text view also preserves names which cannot be native sysctl
 * components (long counter names, or histogram buckets such as "1024 ns").
 */
static int
kstat_named_all_sysctl(SYSCTLFN_ARGS)
{
	kstat_t *ksp = rnode->sysctl_data;
	struct kstat_output out = {
		.lwp = l, .buffer = oldp, .capacity = *oldlenp
	};
	int error;

	if (newp != NULL)
		return (EPERM);
	if (namelen != 0)
		return (EINVAL);
	mutex_enter(ksp->ks_lock);
	error = kstat_update(ksp);
	for (uint_t i = 0; error == 0 && i < ksp->ks_ndata; i++) {
		kstat_named_t *kn = &((kstat_named_t *)ksp->ks_data)[i];
		char number[32];
		const char *value = number;
		size_t len;

		switch (kn->data_type) {
		case KSTAT_DATA_CHAR:
			value = kn->value.c;
			len = strnlen(value, sizeof (kn->value.c));
			break;
		case KSTAT_DATA_STRING:
			value = KSTAT_NAMED_STR_PTR(kn);
			len = value != NULL ?
			    strnlen(value, KSTAT_NAMED_STR_BUFLEN(kn)) : 0;
			break;
		case KSTAT_DATA_INT32:
			len = snprintf(number, sizeof (number), "%d", kn->value.i32);
			break;
		case KSTAT_DATA_UINT32:
			len = snprintf(number, sizeof (number), "%u", kn->value.ui32);
			break;
		case KSTAT_DATA_INT64:
			len = snprintf(number, sizeof (number), "%lld",
			    (long long)kn->value.i64);
			break;
		case KSTAT_DATA_UINT64:
			len = snprintf(number, sizeof (number), "%llu",
			    (unsigned long long)kn->value.ui64);
			break;
		case KSTAT_DATA_LONG:
			len = snprintf(number, sizeof (number), "%ld", kn->value.l);
			break;
		case KSTAT_DATA_ULONG:
			len = snprintf(number, sizeof (number), "%lu", kn->value.ul);
			break;
		default:
			error = EINVAL;
			continue;
		}
		error = kstat_output(&out, kn->name,
		    strnlen(kn->name, sizeof (kn->name)));
		if (error == 0)
			error = kstat_output(&out, "\t", 1);
		if (error == 0 && len != 0)
			error = kstat_output(&out, value, len);
		if (error == 0)
			error = kstat_output(&out, "\n", 1);
	}
	if (error == 0)
		error = kstat_output(&out, "", 1);
	mutex_exit(ksp->ks_lock);
	*oldlenp = out.length;
	return (error);
}

/*
 * Raw callbacks use ENOMEM to request a larger record buffer.  Retry the
 * same record without calling addr again: addr may advance an iterator.
 */
static int
kstat_raw_record(kstat_t *ksp, void *data, boolean_t header,
    char **bufp, size_t *sizep)
{
	int error;

	for (;;) {
		char *buf = *bufp;
		size_t size = *sizep;
		buf[0] = '\0';
		if (!header) {
			error = ksp->ks_raw_ops.data(buf, size, data);
		} else if (ksp->ks_raw_ops.headers != NULL) {
			error = ksp->ks_raw_ops.headers(buf, size);
		} else {
			struct seq_file f = {
				.sf_buf = buf, .sf_size = size
			};
			error = ksp->ks_raw_ops.seq_headers(&f);
			if (error == 0)
				error = f.sf_error;
		}
		if (error == 0 && strnlen(buf, size) == size)
			error = ENOMEM;
		if (error != ENOMEM || size == KSTAT_RAW_MAX)
			return (error);
		kmem_free(buf, size);
		*sizep = MIN(size * 2, KSTAT_RAW_MAX);
		*bufp = kmem_alloc(*sizep, KM_SLEEP);
	}
}

static int
kstat_raw_sysctl(SYSCTLFN_ARGS)
{
	kstat_t *ksp = rnode->sysctl_data;
	struct kstat_output out = {
		.lwp = l, .buffer = oldp, .capacity = *oldlenp
	};
	size_t size = PAGE_SIZE;
	char *buf;
	int error;

	if (newp != NULL)
		return (EPERM);
	if (namelen != 0)
		return (EINVAL);

	buf = kmem_alloc(size, KM_SLEEP);
	mutex_enter(ksp->ks_lock);
	error = kstat_update(ksp);
	if (error != 0)
		goto out;
	if (ksp->ks_raw_ops.data == NULL) {
		if (ksp->ks_data_size != 0)
			error = kstat_output(&out, ksp->ks_data,
			    ksp->ks_data_size);
		goto out;
	}

	if (!(ksp->ks_flags & KSTAT_FLAG_NO_HEADERS) &&
	    (ksp->ks_raw_ops.headers != NULL ||
	    ksp->ks_raw_ops.seq_headers != NULL)) {
		error = kstat_raw_record(ksp, NULL, B_TRUE, &buf, &size);
		if (error == 0)
			error = kstat_output(&out, buf, strlen(buf));
		if (error != 0)
			goto out;
	}

	for (loff_t index = 0; ; index++) {
		void *data = ksp->ks_raw_ops.addr != NULL ?
		    ksp->ks_raw_ops.addr(ksp, index) :
		    (index == 0 ? ksp->ks_data : NULL);
		if (data == NULL)
			break;
		error = kstat_raw_record(ksp, data, B_FALSE, &buf, &size);
		if (error == 0)
			error = kstat_output(&out, buf, strlen(buf));
		if (error != 0)
			goto out;
	}
	error = kstat_output(&out, "", 1);
out:
	mutex_exit(ksp->ks_lock);
	kmem_free(buf, size);
	*oldlenp = out.length;
	return (error);
}

void
seq_printf(struct seq_file *f, const char *fmt, ...)
{
	va_list ap;
	int len;

	if (f->sf_error != 0)
		return;
	va_start(ap, fmt);
	len = vsnprintf(f->sf_buf + f->sf_len, f->sf_size - f->sf_len,
	    fmt, ap);
	va_end(ap);
	if (len < 0)
		f->sf_error = EINVAL;
	else if ((size_t)len >= f->sf_size - f->sf_len)
		f->sf_error = ENOMEM;
	else
		f->sf_len += len;
}

void
kstat_set_raw_ops(kstat_t *ksp, int (*headers)(char *, size_t),
    int (*data)(char *, size_t, void *), void *(*addr)(kstat_t *, loff_t))
{
	ksp->ks_raw_ops.headers = headers;
	ksp->ks_raw_ops.seq_headers = NULL;
	ksp->ks_raw_ops.data = data;
	ksp->ks_raw_ops.addr = addr;
}

void
kstat_set_seq_raw_ops(kstat_t *ksp, int (*headers)(struct seq_file *),
    int (*data)(char *, size_t, void *), void *(*addr)(kstat_t *, loff_t))
{
	kstat_set_raw_ops(ksp, NULL, data, addr);
	ksp->ks_raw_ops.seq_headers = headers;
}

void
kstat_set_string(char *dst, const char *src)
{
	(void) strlcpy(dst, src, KSTAT_STRLEN);
}

void
kstat_named_init(kstat_named_t *kn, const char *name, uchar_t type)
{
	memset(kn, 0, sizeof (*kn));
	kstat_set_string(kn->name, name);
	kn->data_type = type;
}

kstat_t *
kstat_create(const char *module, int instance, const char *name,
    const char *class, uchar_t type, uint_t ndata, uchar_t flags)
{
	kstat_t *ksp;
	size_t size;

	if (instance != 0 ||
	    (type != KSTAT_TYPE_RAW && type != KSTAT_TYPE_NAMED))
		return (NULL);
	if (class == NULL)
		class = "misc";
	if (strlen(module) >= KSTAT_STRLEN || strlen(name) >= KSTAT_STRLEN ||
	    strlen(class) >= KSTAT_STRLEN)
		return (NULL);
	if (type == KSTAT_TYPE_NAMED &&
	    ndata > SIZE_MAX / sizeof (kstat_named_t))
		return (NULL);
	size = type == KSTAT_TYPE_NAMED ?
	    ndata * sizeof (kstat_named_t) : ndata;
	ksp = kmem_zalloc(sizeof (*ksp), KM_SLEEP);
	mutex_init(&ksp->ks_private_lock, NULL, MUTEX_DEFAULT, NULL);
	ksp->ks_lock = &ksp->ks_private_lock;
	kstat_set_string(ksp->ks_module, module);
	kstat_set_string(ksp->ks_name, name);
	kstat_set_string(ksp->ks_class, class);
	ksp->ks_type = type;
	ksp->ks_flags = flags;
	ksp->ks_ndata = type == KSTAT_TYPE_RAW ? 1 : ndata;
	ksp->ks_data_size = size;
	if (!(flags & KSTAT_FLAG_VIRTUAL) && size != 0) {
		ksp->ks_alloc_size = size;
		ksp->ks_data = kmem_zalloc(size, KM_SLEEP);
	}
	mutex_enter(&kstat_config_lock);
	kstat_t *other;
	LIST_FOREACH(other, &kstats, ks_link) {
		if (strcmp(other->ks_module, module) == 0 &&
		    strcmp(other->ks_class, class) == 0 &&
		    strcmp(other->ks_name, name) == 0) {
			mutex_exit(&kstat_config_lock);
			if (ksp->ks_alloc_size != 0)
				kmem_free(ksp->ks_data, ksp->ks_alloc_size);
			mutex_destroy(&ksp->ks_private_lock);
			kmem_free(ksp, sizeof (*ksp));
			return (NULL);
		}
	}
	LIST_INSERT_HEAD(&kstats, ksp, ks_link);
	mutex_exit(&kstat_config_lock);
	return (ksp);
}

/*
 * Keep numeric paths rather than pointers to movable sysctl nodes.  This
 * helper supports up to eight components (kstat/module/.../class/name/field).
 * Native sysctl names have a 31-character limit; report unrepresentable
 * names instead of silently truncating them and aliasing another statistic.
 */
static int
kstat_node(kstat_t *ksp, int *path, u_int depth, const char *name,
    int type, sysctlfn func, void *data)
{
	const struct sysctlnode *parent = NULL;
	int error;

	if (depth >= 8)
		return (ENAMETOOLONG);
	for (u_int i = depth; i < 8; i++)
		path[i] = CTL_EOL;
	path[depth] = CTL_CREATE;
	/* The type is selected at runtime, so bypass the type-check macro. */
	error = (sysctl_createv)(&ksp->ks_log, 0, NULL, NULL,
	    CTLFLAG_READONLY, type, name, NULL, func, 0, data, 0,
	    path[0], path[1], path[2], path[3],
	    path[4], path[5], path[6], path[7], CTL_EOL);
	if (error != 0)
		return (error);
	/*
	 * createv's returned node pointer would become stale if a different
	 * subsystem added a sibling.  Resolve the number under the tree lock.
	 * kstat_config_lock serializes our installation and teardown.
	 */
	sysctl_lock(false);
	error = sysctl_locate(NULL, path, depth, &parent, NULL);
	if (error == 0) {
		error = ENOENT;
		for (u_int i = 0; i < parent->sysctl_clen; i++) {
			const struct sysctlnode *node = &parent->sysctl_child[i];
			if (strcmp(node->sysctl_name, name) == 0) {
				path[depth] = node->sysctl_num;
				error = 0;
				break;
			}
		}
	}
	sysctl_unlock();
	return (error);
}

static boolean_t
kstat_valid_name(const char *name)
{
	size_t len = strlen(name);

	if (len == 0 || len >= SYSCTL_NAMELEN ||
	    (name[0] >= '0' && name[0] <= '9'))
		return (B_FALSE);
	for (const char *p = name; *p != '\0'; p++) {
		if (!((*p >= 'a' && *p <= 'z') ||
		    (*p >= 'A' && *p <= 'Z') ||
		    (*p >= '0' && *p <= '9') || *p == '_' || *p == '-'))
			return (B_FALSE);
	}
	return (B_TRUE);
}

void
kstat_install(kstat_t *ksp)
{
	char module[KSTAT_STRLEN], *p, *part;
	int path[8], error;
	u_int depth = 0;
	uint_t count = ksp->ks_ndata;

	mutex_enter(&kstat_config_lock);
	KASSERT(!ksp->ks_installed);
	ksp->ks_installed = B_TRUE;
	error = kstat_node(ksp, path, depth++, "kstat",
	    CTLTYPE_NODE, NULL, NULL);
	if (error != 0)
		goto fail;
	kstat_set_string(module, ksp->ks_module);
	p = module;
	while ((part = strsep(&p, "/")) != NULL) {
		error = kstat_node(ksp, path, depth++, part,
		    CTLTYPE_NODE, NULL, NULL);
		if (error != 0)
			goto fail;
	}
	error = kstat_node(ksp, path, depth++, ksp->ks_class,
	    CTLTYPE_NODE, NULL, NULL);
	if (error != 0)
		goto fail;

	if (ksp->ks_type == KSTAT_TYPE_RAW) {
		error = kstat_node(ksp, path, depth, ksp->ks_name,
		    ksp->ks_raw_ops.data != NULL ?
		    CTLTYPE_STRING : CTLTYPE_STRUCT, kstat_raw_sysctl, ksp);
		if (error != 0)
			goto fail;
		goto done;
	}

	error = kstat_node(ksp, path, depth++, ksp->ks_name,
	    CTLTYPE_NODE, NULL, NULL);
	if (error != 0)
		goto fail;
	error = kstat_node(ksp, path, depth, "_all",
	    CTLTYPE_STRING, kstat_named_all_sysctl, ksp);
	if (error != 0)
		goto fail;
	if (count == 0)
		goto done;
	ksp->ks_entries_size =
	    count * sizeof (*ksp->ks_entries);
	ksp->ks_entries = kmem_zalloc(ksp->ks_entries_size, KM_SLEEP);
	for (uint_t i = 0; i < count; i++) {
		kstat_named_t *kn = &((kstat_named_t *)ksp->ks_data)[i];
		struct openzfs_kstat_entry *entry = &ksp->ks_entries[i];
		int type;

		if (!kstat_valid_name(kn->name) || strcmp(kn->name, "_all") == 0)
			continue;
		entry->kstat = ksp;
		entry->index = i;
		switch (kn->data_type) {
		case KSTAT_DATA_CHAR:
			type = CTLTYPE_STRUCT;
			break;
		case KSTAT_DATA_INT32:
		case KSTAT_DATA_UINT32:
			type = CTLTYPE_INT;
			break;
		case KSTAT_DATA_INT64:
		case KSTAT_DATA_UINT64:
			type = CTLTYPE_QUAD;
			break;
		case KSTAT_DATA_LONG:
		case KSTAT_DATA_ULONG:
			type = CTLTYPE_LONG;
			break;
		case KSTAT_DATA_STRING:
			type = CTLTYPE_STRING;
			break;
		default:
			continue;
		}
		error = kstat_node(ksp, path, depth, kn->name,
		    type, kstat_named_sysctl, entry);
		if (error != 0)
			printf("OpenZFS: cannot export kstat %s/%s/%s: %d\n",
			    ksp->ks_module, ksp->ks_name, kn->name, error);
	}
	goto done;
fail:
	printf("OpenZFS: cannot export kstat %s/%s: %d\n",
	    ksp->ks_module, ksp->ks_name, error);
	sysctl_teardown(&ksp->ks_log);
done:
	mutex_exit(&kstat_config_lock);
}

void
kstat_delete(kstat_t *ksp)
{
	if (ksp == NULL)
		return;
	mutex_enter(&kstat_config_lock);
	sysctl_teardown(&ksp->ks_log);
	LIST_REMOVE(ksp, ks_link);
	mutex_exit(&kstat_config_lock);
	if (ksp->ks_entries != NULL)
		kmem_free(ksp->ks_entries, ksp->ks_entries_size);
	if (ksp->ks_alloc_size != 0)
		kmem_free(ksp->ks_data, ksp->ks_alloc_size);
	mutex_destroy(&ksp->ks_private_lock);
	kmem_free(ksp, sizeof (*ksp));
}
