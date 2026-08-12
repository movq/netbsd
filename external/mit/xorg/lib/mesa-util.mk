#	$NetBSD$

# Sources for Mesa's private libmesa_util PIC archive.

.PATH:		${X11SRCDIR.Mesa}/src/util
.PATH:		${X11SRCDIR.Mesa}/../src/util
.PATH:		${X11SRCDIR.Mesa}/src/util/format
.PATH:		${X11SRCDIR.Mesa}/../src/util/format
.PATH:		${X11SRCDIR.Mesa}/src/util/blake3
.PATH:		${X11SRCDIR.Mesa}/src/c11/impl

CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/util

SRCS.mesa_util= \
	anon_file.c \
	bitscan.c \
	blob.c \
	build_id.c \
	cnd_monotonic.c \
	compress.c \
	thread_sched.c \
	crc32.c \
	dag.c \
	disk_cache.c \
	disk_cache_os.c \
	double.c \
	fast_idiv_by_const.c \
	float8.c \
	fossilize_db.c \
	futex.c \
	half_float.c \
	hash_table.c \
	helpers.c \
	u_idalloc.c \
	log.c \
	lut.c \
	memstream.c \
	mesa-blake3.c \
	os_time.c \
	os_file.c \
	os_file_notify.c \
	os_memory_fd.c \
	os_misc.c \
	os_socket.c \
	pb_slab.c \
	u_process.c \
	rwlock.c \
	ralloc.c \
	rand_xor.c \
	range_minimum_query.c \
	rb_tree.c \
	register_allocate.c \
	rgtc.c \
	set.c \
	simple_mtx.c \
	slab.c \
	softfloat.c \
	sparse_array.c \
	string_buffer.c \
	strndup.c \
	strtod.c \
	u_atomic.c \
	u_call_once.c \
	u_dl.c \
	u_dynarray.c \
	u_hash_table.c \
	u_queue.c \
	u_range_remap.c \
	u_string.c \
	u_thread.c \
	u_vector.c \
	u_math.c \
	u_mm.c \
	u_debug.c \
	u_debug_memory.c \
	u_cpu_detect.c \
	u_printf.c \
	u_worklist.c \
	vl_zscan_data.c \
	vma.c \
	u_sync_provider.c \
	mesa_cache_db.c \
	mesa_cache_db_multipart.c \
	u_debug_stack.c \
	u_debug_symbol.c \
	streaming-load-memcpy.c \
	u_qsort.cpp \
	texcompress_astc_luts.cpp \
	texcompress_astc_luts_wrap.cpp \
	texcompress_astc.cpp \
	format_srgb.c \
	u_format.c \
	u_format_bptc.c \
	u_format_etc.c \
	u_format_fxt1.c \
	u_format_latc.c \
	u_format_other.c \
	u_format_rgtc.c \
	u_format_s3tc.c \
	u_format_tests.c \
	u_format_unpack_neon.c \
	u_format_yuv.c \
	u_format_zs.c \
	u_format_table.c

# The vendored BLAKE3 implementation.  Use its portable path here; the
# architecture-specific assembly is an optimization rather than an ABI need.
SRCS.mesa_util+= \
	blake3.c \
	blake3_dispatch.c \
	blake3_portable.c \
	c11_time.c \
	c11_threads_posix.c

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS.mesa_util+=	cache_ops_x86.c
COPTS.cache_ops_x86.c+=	-msse2
.else
SRCS.mesa_util+=	cache_ops_null.c
.endif

BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/c11/impl/time.c c11_time.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/c11/impl/threads_posix.c c11_threads_posix.c

CPPFLAGS.blake3_dispatch.c+=	-DBLAKE3_NO_SSE2 -DBLAKE3_NO_SSE41 \
				-DBLAKE3_NO_AVX2 -DBLAKE3_NO_AVX512
.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
COPTS.streaming-load-memcpy.c+=	-msse4.1
.endif

CPPFLAGS.hash_table.c+=		-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.format_srgb.c+=	-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.u_hash_table.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary
CPPFLAGS.strtod.c+=		-D_GNU_SOURCE -DHAVE_STRTOD_L
CPPFLAGS.u_process.c+=		-DHAVE_NOATEXIT

CPUFLAGS.u_format_unpack_neon.c+= \
	${${MACHINE_CPU} == "arm" && ${ACTIVE_CC} == "clang":?-mfpu=neon -march=armv7-a:}

.for _f in ${SRCS.mesa_util:M*u_format*.c} format_srgb.c
CPPFLAGS.${_f}+=	-I${X11SRCDIR.Mesa}/src/util/format \
			-I${X11SRCDIR.Mesa}/../src/util/format \
			-I${X11SRCDIR.Mesa}/../src
.endfor
