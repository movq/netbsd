/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Native syscall tests on an already mounted, disposable filesystem.
 * Deliberately independent of rump and of the ZFS userspace libraries.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/filio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CHECK(x) do { if (!(x)) errx(1, "%s:%d: %s (errno %d: %s)", \
    __func__, __LINE__, #x, errno, strerror(errno)); } while (0)
#define DENIED(x, e) do { errno = 0; CHECK((x) == -1); \
    CHECK(errno == (e)); } while (0)

static size_t pagesize;
static unsigned iterations = 1000;

static pid_t
spawn(void)
{
	pid_t pid = fork();
	CHECK(pid >= 0);
	if (pid == 0)
		alarm(600);
	return pid;
}

static int
file(const char *name, mode_t mode)
{
	int fd = open(name, O_CREAT | O_EXCL | O_RDWR, mode);
	CHECK(fd >= 0);
	return fd;
}

static void
reap(pid_t pid)
{
	int status;
	CHECK(waitpid(pid, &status, 0) == pid);
	CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

static void
identity(uid_t uid, gid_t gid, gid_t supplementary)
{
	CHECK(setgroups(1, &supplementary) == 0);
	CHECK(setgid(gid) == 0);
	CHECK(setuid(uid) == 0);
}

static void
permissions(void)
{
	struct stat st;
	pid_t pid;
	int fd;

	CHECK(geteuid() == 0);
	umask(0027);
	fd = file("mask", 0666);
	CHECK(fstat(fd, &st) == 0 && (st.st_mode & 0777) == 0640);
	CHECK(mkdir("maskdir", 0777) == 0);
	CHECK(stat("maskdir", &st) == 0 && (st.st_mode & 0777) == 0750);
	umask(0);
	CHECK(fchown(fd, 60001, 60002) == 0);
	CHECK(fchmod(fd, 0640) == 0);
	CHECK(fstat(fd, &st) == 0 && st.st_uid == 60001 &&
	    st.st_gid == 60002);
	CHECK(close(fd) == 0);
	CHECK(chmod(".", 0777) == 0);
	CHECK(mkdir("sticky", 01777) == 0);
	CHECK(chmod("sticky", 01777) == 0);
	CHECK(stat("sticky", &st) == 0 && (st.st_mode & S_ISVTX));
	fd = file("sticky/owned", 0600);
	CHECK(fchown(fd, 60001, 60002) == 0);
	CHECK(close(fd) == 0);
	fd = file("sticky/replacement", 0600);
	CHECK(fchown(fd, 60003, 60003) == 0);
	CHECK(close(fd) == 0);
	CHECK(mkdir("setgid", 02777) == 0);
	CHECK(chown("setgid", 0, 60002) == 0);
	CHECK(chmod("setgid", 02777) == 0);

	pid = spawn();
	CHECK(pid >= 0);
	if (pid == 0) {
		identity(60003, 60003, 60002);
		fd = open("mask", O_RDONLY);
		CHECK(fd >= 0);	/* Access through supplementary group. */
		CHECK(close(fd) == 0);
		DENIED(open("mask", O_WRONLY), EACCES);
		DENIED(chmod("mask", 0666), EPERM);
		DENIED(chown("mask", 60003, 60003), EPERM);
		DENIED(unlink("sticky/owned"), EPERM);
		DENIED(rename("sticky/replacement", "sticky/owned"), EPERM);
		fd = file("setgid/child", 0666);
		CHECK(fstat(fd, &st) == 0 && st.st_gid == 60002);
		CHECK(close(fd) == 0);
		CHECK(mkdir("setgid/subdir", 0777) == 0);
		CHECK(stat("setgid/subdir", &st) == 0 &&
		    st.st_gid == 60002);
		_exit(0);
	}
	reap(pid);
	pid = spawn();
	CHECK(pid >= 0);
	if (pid == 0) {
		identity(60004, 60004, 60004);
		DENIED(open("mask", O_RDONLY), EACCES);
		DENIED(open("maskdir/absent", O_CREAT | O_RDWR, 0600),
		    EACCES);
		_exit(0);
	}
	reap(pid);
	pid = spawn();
	CHECK(pid >= 0);
	if (pid == 0) {
		identity(60001, 60001, 60002);
		fd = open("mask", O_WRONLY);
		CHECK(fd >= 0);
		CHECK(fchmod(fd, 06755) == 0);
		CHECK(write(fd, "x", 1) == 1);
		CHECK(fstat(fd, &st) == 0 &&
		    (st.st_mode & (S_ISUID | S_ISGID)) == 0);
		DENIED(fchown(fd, 60004, (gid_t)-1), EPERM);
		CHECK(close(fd) == 0);
		CHECK(unlink("sticky/owned") == 0);
		_exit(0);
	}
	reap(pid);
}

static void
metadata(void)
{
	struct stat st, old;
	struct statvfs sv;
	struct timespec times[2] = {{1234567890, 123456789},
	    {1234567891, 987654321}};
	struct sockaddr_un sun;
	char name[NAME_MAX + 2], buf[32];
	int fd, sock, reader;

	fd = file("file", 0600);
	CHECK(write(fd, "hello", 5) == 5);
	CHECK(futimens(fd, times) == 0);
	CHECK(fstat(fd, &st) == 0);
	CHECK(st.st_atim.tv_sec == times[0].tv_sec &&
	    st.st_atim.tv_nsec == times[0].tv_nsec);
	CHECK(st.st_mtim.tv_sec == times[1].tv_sec &&
	    st.st_mtim.tv_nsec == times[1].tv_nsec);
	old = st;
	CHECK(pread(fd, buf, 5, 0) == 5);
	CHECK(fstat(fd, &st) == 0 && st.st_atim.tv_sec > old.st_atim.tv_sec);
	CHECK(pwrite(fd, "H", 1, 0) == 1);
	CHECK(fstat(fd, &st) == 0 && st.st_mtim.tv_sec > old.st_mtim.tv_sec);
	CHECK(st.st_ctim.tv_sec > times[1].tv_sec);
	CHECK(link("file", "hard") == 0);
	CHECK(stat("hard", &old) == 0 && old.st_ino == st.st_ino &&
	    old.st_nlink == 2);
	CHECK(symlink("file", "sym") == 0);
	CHECK(lstat("sym", &st) == 0 && S_ISLNK(st.st_mode));
	CHECK(readlink("sym", buf, sizeof(buf)) == 4 &&
	    memcmp(buf, "file", 4) == 0);
	CHECK(stat("sym", &st) == 0 && st.st_ino == old.st_ino);
	DENIED(open("sym", O_RDONLY | O_NOFOLLOW), EFTYPE);
	CHECK(fstatvfs(fd, &sv) == 0 && sv.f_bsize > 0);
	long namemax = fpathconf(fd, _PC_NAME_MAX);
	CHECK(namemax > 0 && namemax <= NAME_MAX);
	memset(name, 'n', namemax);
	name[namemax] = 0;
	reader = file(name, 0600);
	CHECK(close(reader) == 0);
	name[namemax] = 'n';
	name[namemax + 1] = 0;
	DENIED(open(name, O_CREAT | O_RDWR, 0600), ENAMETOOLONG);
	CHECK(mkfifo("fifo", 0600) == 0);
	reader = open("fifo", O_RDONLY | O_NONBLOCK);
	CHECK(reader >= 0);
	int writer = open("fifo", O_WRONLY | O_NONBLOCK);
	CHECK(writer >= 0 && write(writer, "x", 1) == 1);
	CHECK(read(reader, buf, 1) == 1 && buf[0] == 'x');
	CHECK(close(reader) == 0 && close(writer) == 0);
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	CHECK(sock >= 0);
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	strlcpy(sun.sun_path, "socket", sizeof(sun.sun_path));
	CHECK(bind(sock, (struct sockaddr *)&sun, sizeof(sun)) == 0);
	CHECK(lstat("socket", &st) == 0 && S_ISSOCK(st.st_mode));
	CHECK(close(sock) == 0);
	CHECK(mkdir("dir", 0700) == 0);
	DENIED(link("dir", "dirlink"), EPERM);
	DENIED(rename("file", "dir"), EISDIR);
	DENIED(rename("dir", "hard"), ENOTDIR);
	CHECK(rename("dir", "newdir") == 0);
	CHECK(mkdir("newdir/child", 0700) == 0);
	CHECK(mkdir("other", 0700) == 0);
	CHECK(rename("newdir/child", "other/child") == 0);
	CHECK(stat("other", &old) == 0);
	CHECK(stat("other/child/..", &st) == 0 && st.st_ino == old.st_ino);
	DENIED(rename("other", "other/child/loop"), EINVAL);
	DENIED(rmdir("other"), ENOTEMPTY);
	CHECK(rmdir("other/child") == 0 && rmdir("other") == 0);
	CHECK(rmdir("newdir") == 0);
	CHECK(fsync(fd) == 0 && close(fd) == 0);
}

static void
sparse(void)
{
	const off_t end = 64 * 1024 * 1024 + 123;
	struct stat st;
	char buf[8192];
	int fd = file("sparse", 0600);

	CHECK(pwrite(fd, "end", 3, end) == 3);
	CHECK(fstat(fd, &st) == 0 && st.st_size == end + 3);
	CHECK(st.st_blocks * 512 < end / 2);
	for (off_t off = 0; off < end; off += sizeof(buf)) {
		size_t n = end - off < (off_t)sizeof(buf) ?
		    (size_t)(end - off) : sizeof(buf);
		memset(buf, 0xff, n);
		CHECK(pread(fd, buf, n, off) == (ssize_t)n);
		for (size_t j = 0; j < n; j++)
			CHECK(buf[j] == 0);
	}
	CHECK(ftruncate(fd, 123) == 0);
	CHECK(ftruncate(fd, end + 3) == 0);
	CHECK(pread(fd, buf, 3, end) == 3);
	CHECK(buf[0] == 0 && buf[1] == 0 && buf[2] == 0);
	CHECK(fsync(fd) == 0 && close(fd) == 0);
}

static void
flags(void)
{
	struct stat st;
	int fd = file("flags", 0600);
	char c;

	CHECK(write(fd, "A", 1) == 1);
	/* ZFS maps immutable/append-only to NetBSD's system flags. */
	CHECK(fchflags(fd, UF_NODUMP) == 0);
	CHECK(fstat(fd, &st) == 0 && (st.st_flags & UF_NODUMP));
	CHECK(fchflags(fd, 0) == 0);
	CHECK(fchflags(fd, SF_IMMUTABLE) == 0);
	CHECK(fstat(fd, &st) == 0 && (st.st_flags & SF_IMMUTABLE));
	CHECK(close(fd) == 0);
	DENIED(open("flags", O_WRONLY), EPERM);
	DENIED(truncate("flags", 0), EPERM);
	DENIED(unlink("flags"), EPERM);
	DENIED(rename("flags", "renamed"), EPERM);
	CHECK(chflags("flags", 0) == 0);
	CHECK(chflags("flags", SF_APPEND) == 0);
	DENIED(open("flags", O_WRONLY), EPERM);
	fd = open("flags", O_WRONLY | O_APPEND);
	CHECK(fd >= 0 && write(fd, "B", 1) == 1);
	CHECK(fchflags(fd, 0) == 0 && close(fd) == 0);
	fd = open("flags", O_RDONLY);
	CHECK(fd >= 0 && pread(fd, &c, 1, 1) == 1 && c == 'B');
	CHECK(close(fd) == 0);
}

static void
holes(void)
{
	const off_t end = 16 * 1024 * 1024;
	off_t off;
	int fd = file("holes", 0600);
	CHECK(pwrite(fd, "A", 1, end) == 1 && fsync(fd) == 0);
	off = end;
	CHECK(ioctl(fd, FIOSEEKDATA, &off) == 0 && off == end);
	off = end;
	CHECK(ioctl(fd, FIOSEEKHOLE, &off) == 0 && off == end + 1);
	off = 0;
	CHECK(ioctl(fd, FIOSEEKDATA, &off) == 0 && off >= 0 && off <= end);
	off = end + 1;
	DENIED(ioctl(fd, FIOSEEKDATA, &off), ENXIO);
	off = end + 1;
	DENIED(ioctl(fd, FIOSEEKHOLE, &off), ENXIO);
	CHECK(ftruncate(fd, 0) == 0);
	off = 0;
	DENIED(ioctl(fd, FIOSEEKDATA, &off), ENXIO);
	CHECK(close(fd) == 0);
}

static void
append(void)
{
	enum { WORKERS = 4 };
	struct record { unsigned worker, sequence; char pad[248]; } rec;
	pid_t children[WORKERS];
	unsigned next[WORKERS] = {0};
	int fd = file("append", 0600);

	CHECK(close(fd) == 0);
	for (unsigned w = 0; w < WORKERS; w++) {
		children[w] = spawn();
		CHECK(children[w] >= 0);
		if (children[w] != 0)
			continue;
		fd = open("append", O_WRONLY | O_APPEND);
		CHECK(fd >= 0);
		for (unsigned i = 0; i < iterations; i++) {
			memset(&rec, (int)('a' + w), sizeof(rec));
			rec.worker = w;
			rec.sequence = i;
			CHECK(write(fd, &rec, sizeof(rec)) == sizeof(rec));
		}
		CHECK(fsync(fd) == 0 && close(fd) == 0);
		_exit(0);
	}
	for (unsigned w = 0; w < WORKERS; w++)
		reap(children[w]);
	fd = open("append", O_RDONLY);
	CHECK(fd >= 0);
	for (unsigned i = 0; i < iterations * WORKERS; i++) {
		CHECK(read(fd, &rec, sizeof(rec)) == sizeof(rec));
		CHECK(rec.worker < WORKERS);
		CHECK(rec.sequence == next[rec.worker]++);
		for (size_t j = 0; j < sizeof(rec.pad); j++)
			CHECK(rec.pad[j] == (char)('a' + rec.worker));
	}
	CHECK(read(fd, &rec, sizeof(rec)) == 0);
	for (unsigned w = 0; w < WORKERS; w++)
		CHECK(next[w] == iterations);
	CHECK(close(fd) == 0);
}

static void
locking(void)
{
	int fd = file("lock", 0600);
	struct flock lock = {.l_type = F_WRLCK, .l_whence = SEEK_SET,
	    .l_start = 0, .l_len = 100};
	pid_t pid;

	CHECK(fcntl(fd, F_SETLK, &lock) == 0);
	pid = spawn();
	CHECK(pid >= 0);
	if (pid == 0) {
		CHECK(fcntl(fd, F_GETLK, &lock) == 0);
		CHECK(lock.l_type == F_WRLCK && lock.l_pid == getppid());
		lock.l_type = F_WRLCK;
		CHECK(fcntl(fd, F_SETLK, &lock) == -1);
		CHECK(errno == EAGAIN || errno == EACCES);
		lock.l_start = 100;
		CHECK(fcntl(fd, F_SETLK, &lock) == 0);
		_exit(0);
	}
	reap(pid);
	lock.l_type = F_UNLCK;
	CHECK(fcntl(fd, F_SETLK, &lock) == 0);
	CHECK(flock(fd, LOCK_EX | LOCK_NB) == 0);
	pid = spawn();
	CHECK(pid >= 0);
	if (pid == 0) {
		CHECK(close(fd) == 0);
		fd = open("lock", O_RDWR);
		CHECK(fd >= 0);
		DENIED(flock(fd, LOCK_EX | LOCK_NB), EWOULDBLOCK);
		_exit(0);
	}
	reap(pid);
	CHECK(flock(fd, LOCK_UN) == 0 && close(fd) == 0);
}

static void
readonly(void)
{
	int fd = open("held", O_RDONLY);
	char c, *p;
	CHECK(fd >= 0 && read(fd, &c, 1) == 1 && c == 'A');
	DENIED(open("held", O_RDWR), EROFS);
	DENIED(open("new", O_CREAT | O_RDWR, 0600), EROFS);
	DENIED(unlink("held"), EROFS);
	DENIED(rename("held", "new"), EROFS);
	DENIED(link("held", "new"), EROFS);
	DENIED(mkdir("dir", 0700), EROFS);
	DENIED(chmod("held", 0666), EROFS);
	p = mmap(NULL, pagesize, PROT_READ, MAP_SHARED, fd, 0);
	CHECK(p != MAP_FAILED && p[0] == 'A');
	CHECK(munmap(p, pagesize) == 0 && close(fd) == 0);
}

static void
token(int fd, bool send)
{
	char c = 0;
	CHECK((send ? write(fd, &c, 1) : read(fd, &c, 1)) == 1);
}

static void
coherence(void)
{
	int fd = file("mapped", 0600), request[2], reply[2];
	char *p, *buf = malloc(pagesize);
	pid_t pid;

	CHECK(buf != NULL);
	CHECK(ftruncate(fd, pagesize * 3) == 0);
	p = mmap(NULL, pagesize * 3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	CHECK(p != MAP_FAILED);
	CHECK(pipe(request) == 0 && pipe(reply) == 0);
	pid = spawn();
	CHECK(pid >= 0);
	if (pid == 0) {
		close(request[1]);
		close(reply[0]);
		for (unsigned i = 0; i < iterations; i++) {
			char value = (char)(1 + i % 126);
			token(request[0], false);
			CHECK(pread(fd, buf, pagesize, 0) == (ssize_t)pagesize);
			for (size_t j = 0; j < pagesize; j++)
				CHECK(buf[j] == value);
			memset(buf, -value, pagesize);
			CHECK(pwrite(fd, buf, pagesize, 0) == (ssize_t)pagesize);
			CHECK(fsync(fd) == 0);
			token(reply[1], true);
		}
		_exit(0);
	}
	close(request[0]);
	close(reply[1]);
	for (unsigned i = 0; i < iterations; i++) {
		char value = (char)(1 + i % 126);
		memset(p, value, pagesize);
		/* Half the rounds test dirty UVM visibility before msync. */
		if (i & 1)
			CHECK(msync(p, pagesize, MS_SYNC) == 0);
		token(request[1], true);
		token(reply[0], false);
		for (size_t j = 0; j < pagesize; j++)
			CHECK(p[j] == (char)-value);
		memset(p + pagesize, 0x5a, pagesize * 2);
		CHECK(msync(p, pagesize * 3, MS_SYNC) == 0);
		CHECK(ftruncate(fd, pagesize + 37) == 0);
		CHECK(ftruncate(fd, pagesize * 3) == 0);
		for (size_t j = pagesize + 37; j < pagesize * 3; j++)
			CHECK(p[j] == 0);
	}
	reap(pid);
	CHECK(close(request[1]) == 0 && close(reply[0]) == 0);
	CHECK(munmap(p, pagesize * 3) == 0);
	/* Copy-on-write mappings must never change the file. */
	p = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	CHECK(p != MAP_FAILED);
	CHECK(pread(fd, buf, pagesize, 0) == (ssize_t)pagesize);
	p[0] = (char)(buf[0] ^ 0x7f);
	CHECK(msync(p, pagesize, MS_SYNC) == 0);
	char c;
	CHECK(pread(fd, &c, 1, 0) == 1 && c == buf[0]);
	CHECK(munmap(p, pagesize) == 0);
	CHECK(fsync(fd) == 0 && close(fd) == 0);
	free(buf);
}

static void
lifetime(void)
{
	struct stat st;
	int a = file("a", 0600), b = file("b", 0600), status;
	char *p, c;
	pid_t pid;

	CHECK(ftruncate(a, pagesize) == 0);
	CHECK(pwrite(a, "A", 1, 0) == 1);
	CHECK(pwrite(b, "B", 1, 0) == 1);
	p = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, a, 0);
	CHECK(p != MAP_FAILED && p[0] == 'A');
	CHECK(link("a", "alias") == 0);
	CHECK(rename("b", "a") == 0);
	CHECK(pread(a, &c, 1, 0) == 1 && c == 'A');
	CHECK(pread(b, &c, 1, 0) == 1 && c == 'B');
	CHECK(unlink("alias") == 0);
	CHECK(fstat(a, &st) == 0 && st.st_nlink == 0);
	p[0] = 'C';
	CHECK(msync(p, pagesize, MS_SYNC) == 0);
	CHECK(pread(a, &c, 1, 0) == 1 && c == 'C');
	CHECK(close(a) == 0);	/* Mapping alone retains the removed vnode. */
	CHECK(p[0] == 'C');
	CHECK(munmap(p, pagesize) == 0);
	CHECK(unlink("a") == 0);
	CHECK(fstat(b, &st) == 0 && st.st_nlink == 0);
	CHECK(pwrite(b, "D", 1, 0) == 1 && fsync(b) == 0);
	CHECK(close(b) == 0);
	a = file("bus", 0600);
	CHECK(ftruncate(a, pagesize * 2) == 0);
	p = mmap(NULL, pagesize * 2, PROT_READ, MAP_SHARED, a, 0);
	CHECK(p != MAP_FAILED);
	CHECK(p[pagesize] == 0);
	CHECK(ftruncate(a, 0) == 0);
	pid = spawn();
	CHECK(pid >= 0);
	if (pid == 0) {
		struct rlimit lim = {0, 0};
		setrlimit(RLIMIT_CORE, &lim);
		c = *(volatile char *)(p + pagesize);
		_exit(c == 0 ? 2 : 3);
	}
	CHECK(waitpid(pid, &status, 0) == pid);
	CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGBUS);
	CHECK(munmap(p, pagesize * 2) == 0 && close(a) == 0);
}

static void
contention(void)
{
	enum { WORKERS = 6 };
	pid_t children[WORKERS];
	int fd = file("shared", 0600);
	char *p;

	CHECK(ftruncate(fd, (WORKERS + 2) * pagesize) == 0);
	p = mmap(NULL, (WORKERS + 2) * pagesize, PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0);
	CHECK(p != MAP_FAILED);
	for (unsigned worker = 0; worker < WORKERS; worker++) {
		children[worker] = spawn();
		CHECK(children[worker] >= 0);
		if (children[worker] != 0)
			continue;
		char *buf = malloc(pagesize);
		CHECK(buf != NULL);
		for (unsigned i = 0; i < iterations; i++) {
			off_t off = worker * pagesize;
			char value = (char)(1 + (i + worker) % 126);
			memset(buf, value, pagesize);
			if (worker & 1) {
				memcpy(p + off, buf, pagesize);
				CHECK(msync(p + off, pagesize, MS_SYNC) == 0);
			} else {
				CHECK(pwrite(fd, buf, pagesize, off) ==
				    (ssize_t)pagesize);
				CHECK(fsync(fd) == 0);
			}
			CHECK(pread(fd, buf, pagesize, off) == (ssize_t)pagesize);
			for (size_t j = 0; j < pagesize; j++)
				CHECK(buf[j] == value && p[off + j] == value);
		}
		_exit(0);
	}
	/* Truncate only the tail: workers' byte ranges always remain valid. */
	for (unsigned i = 0; i < iterations; i++) {
		CHECK(ftruncate(fd, WORKERS * pagesize + 17) == 0);
		CHECK(ftruncate(fd, (WORKERS + 2) * pagesize) == 0);
	}
	for (unsigned worker = 0; worker < WORKERS; worker++)
		reap(children[worker]);
	CHECK(msync(p, (WORKERS + 2) * pagesize, MS_SYNC) == 0);
	CHECK(munmap(p, (WORKERS + 2) * pagesize) == 0);
	CHECK(close(fd) == 0);
}

static void
directories(void)
{
	enum { WORKERS = 4 };
	pid_t children[WORKERS];
	char a[64], b[64], c[64];
	struct dirent *de;
	DIR *dir;

	CHECK(mkdir("entries", 0700) == 0);
	for (unsigned w = 0; w < WORKERS; w++) {
		children[w] = spawn();
		CHECK(children[w] >= 0);
		if (children[w] != 0)
			continue;
		for (unsigned i = 0; i < iterations; i++) {
			snprintf(a, sizeof(a), "entries/w%u-a%u", w, i % 32);
			snprintf(b, sizeof(b), "entries/w%u-b%u", w, i % 32);
			snprintf(c, sizeof(c), "entries/w%u-c%u", w, i % 32);
			int fd = file(a, 0600), victim = file(b, 0600);
			size_t len = strlen(a) + 1;
			CHECK(write(fd, a, len) == (ssize_t)len);
			CHECK(link(a, c) == 0);
			CHECK(rename(a, b) == 0);
			CHECK(unlink(b) == 0 && unlink(c) == 0);
			CHECK(close(victim) == 0 && close(fd) == 0);
		}
		_exit(0);
	}
	for (unsigned i = 0; i < iterations; i++) {
		unsigned count = 0;
		dir = opendir("entries");
		CHECK(dir != NULL);
		errno = 0;
		while ((de = readdir(dir)) != NULL) {
			CHECK(de->d_namlen < sizeof(de->d_name));
			CHECK(de->d_name[de->d_namlen] == '\0');
			CHECK(strcmp(de->d_name, ".") == 0 ||
			    strcmp(de->d_name, "..") == 0 || de->d_name[0] == 'w');
			CHECK(++count < 100000); /* Detect cookie loops. */
			errno = 0;
		}
		CHECK(errno == 0 && closedir(dir) == 0);
	}
	for (unsigned w = 0; w < WORKERS; w++)
		reap(children[w]);
	dir = opendir("entries");
	CHECK(dir != NULL);
	while ((de = readdir(dir)) != NULL)
		CHECK(strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0);
	CHECK(closedir(dir) == 0 && rmdir("entries") == 0);
}

static void
namespace(void)
{
	enum { WORKERS = 4 };
	pid_t children[WORKERS];
	char *buf = malloc(pagesize);
	int fd = file("target", 0600);
	CHECK(buf != NULL);
	memset(buf, 'A', pagesize);
	CHECK(write(fd, buf, pagesize) == (ssize_t)pagesize);
	CHECK(close(fd) == 0);
	for (unsigned w = 0; w < WORKERS; w++) {
		children[w] = spawn();
		if (children[w] != 0)
			continue;
		char name[32];
		unsigned linked = 0, vanished = 0;
		snprintf(name, sizeof(name), "worker-%u", w);
		for (unsigned i = 0; i < iterations; i++) {
			if (w < 2) {
				fd = file(name, 0600);
				memset(buf, 'A' + w, pagesize);
				CHECK(write(fd, buf, pagesize) == (ssize_t)pagesize);
				CHECK(rename(name, "target") == 0);
				CHECK(close(fd) == 0);
			} else {
				/* Rename replacement must never leave a pathname gap. */
				fd = open("target", O_RDONLY);
				CHECK(fd >= 0 && close(fd) == 0);
				/*
				 * link(2) drops the source namei lock before taking
				 * the destination lock.  ZFS rejects resurrection
				 * if that source vnode was unlinked in between.
				 */
				if (link("target", name) == -1) {
					CHECK(errno == ENOENT);
					vanished++;
					continue;
				}
				linked++;
				fd = open(name, O_RDONLY);
				CHECK(fd >= 0);
				char *p = mmap(NULL, pagesize, PROT_READ,
				    MAP_SHARED, fd, 0);
				CHECK(p != MAP_FAILED);
				CHECK(unlink(name) == 0);
				CHECK(pread(fd, buf, pagesize, 0) ==
				    (ssize_t)pagesize);
				CHECK(close(fd) == 0);
				CHECK(buf[0] == 'A' || buf[0] == 'B');
				for (size_t j = 0; j < pagesize; j++)
					CHECK(buf[j] == buf[0] && p[j] == buf[0]);
				CHECK(munmap(p, pagesize) == 0);
			}
		}
		if (w >= 2) {
			CHECK(linked > 0);
			printf("namespace reader %u: %u links, %u source-unlink races\n",
			    w, linked, vanished);
			fflush(stdout);
		}
		_exit(0);
	}
	for (unsigned w = 0; w < WORKERS; w++)
		reap(children[w]);
	free(buf);
}

static unsigned char
pattern(size_t offset)
{
	return (unsigned char)(offset ^ (offset >> 9) ^ (offset >> 17));
}

static void
paging(void)
{
	/* Keep enough data to cross many records and exercise page disposal. */
	const size_t length = 256 * 1024 * 1024;
	int fd = file("paging", 0600);
	unsigned char *p, buf[65536];
	CHECK(ftruncate(fd, length) == 0);
	p = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	CHECK(p != MAP_FAILED);
	for (size_t j = 0; j < length; j++)
		p[j] = pattern(j);
	CHECK(msync(p, length, MS_SYNC | MS_INVALIDATE) == 0);
	CHECK(munmap(p, length) == 0 && fsync(fd) == 0);
	for (size_t off = 0; off < length; off += sizeof(buf)) {
		CHECK(pread(fd, buf, sizeof(buf), off) == sizeof(buf));
		for (size_t j = 0; j < sizeof(buf); j++) {
			CHECK(buf[j] == pattern(off + j));
			buf[j] ^= 0xff;
		}
		CHECK(pwrite(fd, buf, sizeof(buf), off) == sizeof(buf));
	}
	CHECK(fsync(fd) == 0);
	p = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0);
	CHECK(p != MAP_FAILED);
	for (size_t j = 0; j < length; j++)
		CHECK(p[j] == (unsigned char)(pattern(j) ^ 0xff));
	CHECK(munmap(p, length) == 0 && close(fd) == 0);
}

static void
pressure(void)
{
	/* Explicit opt-in; the caller chooses MiB to suit available RAM/swap. */
	CHECK(iterations <= 65536);
	size_t length = (size_t)iterations * 1024 * 1024;
	unsigned char *p = mmap(NULL, length, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANON, -1, 0);
	char c;
	CHECK(p != MAP_FAILED);
	for (size_t j = 0; j < length; j += pagesize)
		p[j] = pattern(j);
	puts("ready");
	fflush(stdout);
	CHECK(read(STDIN_FILENO, &c, 1) == 1);
	for (size_t j = 0; j < length; j += pagesize)
		CHECK(p[j] == pattern(j));
	CHECK(munmap(p, length) == 0);
}

/*
 * stdin/stdout provide barriers for administrative operations.  The caller
 * keeps its own working directory outside the tested dataset.
 */
static void
hold(const char *kind)
{
	char *p = NULL, c;
	int fd = -1;
	if (strcmp(kind, "cwd") != 0) {
		fd = open("held", O_RDWR);
		CHECK(fd >= 0);
		if (strcmp(kind, "map") == 0) {
			p = mmap(NULL, pagesize, PROT_READ | PROT_WRITE,
			    MAP_SHARED, fd, 0);
			CHECK(p != MAP_FAILED && p[0] == 'A');
			CHECK(close(fd) == 0);
			fd = -1;
		}
		CHECK(chdir("/") == 0);
	}
	puts("ready");
	fflush(stdout);
	CHECK(read(STDIN_FILENO, &c, 1) == 1);
	if (c == 'v') {
		if (p != NULL)
			CHECK(p[0] == 'A');
		if (fd >= 0)
			CHECK(pread(fd, &c, 1, 0) == 1 && c == 'A');
		if (strcmp(kind, "cwd") == 0) {
			struct stat st;
			CHECK(stat(".", &st) == 0 && S_ISDIR(st.st_mode));
		}
	}
	if (p != NULL)
		CHECK(munmap(p, pagesize) == 0);
	if (fd >= 0)
		CHECK(close(fd) == 0);
}

int
main(int argc, char **argv)
{
	if (argc < 3 || argc > 4)
		errx(1, "usage: %s case directory [iterations|MiB|fd|map|cwd]",
		    argv[0]);
	pagesize = (size_t)sysconf(_SC_PAGESIZE);
	if (argc == 4 && strcmp(argv[1], "hold") != 0) {
		char *end;
		unsigned long n = strtoul(argv[3], &end, 10);
		CHECK(*end == 0 && n > 0 && n <= 1000000);
		iterations = (unsigned)n;
	}
	CHECK(chdir(argv[2]) == 0);
	alarm(600);
	if (strcmp(argv[1], "permissions") == 0)
		permissions();
	else if (strcmp(argv[1], "metadata") == 0)
		metadata();
	else if (strcmp(argv[1], "sparse") == 0)
		sparse();
	else if (strcmp(argv[1], "flags") == 0)
		flags();
	else if (strcmp(argv[1], "holes") == 0)
		holes();
	else if (strcmp(argv[1], "append") == 0)
		append();
	else if (strcmp(argv[1], "locking") == 0)
		locking();
	else if (strcmp(argv[1], "readonly") == 0)
		readonly();
	else if (strcmp(argv[1], "coherence") == 0)
		coherence();
	else if (strcmp(argv[1], "lifetime") == 0)
		lifetime();
	else if (strcmp(argv[1], "contention") == 0)
		contention();
	else if (strcmp(argv[1], "directories") == 0)
		directories();
	else if (strcmp(argv[1], "namespace") == 0)
		namespace();
	else if (strcmp(argv[1], "paging") == 0)
		paging();
	else if (strcmp(argv[1], "pressure") == 0)
		pressure();
	else if (strcmp(argv[1], "hold") == 0 && argc == 4)
		hold(argv[3]);
	else
		errx(1, "unknown case: %s", argv[1]);
	if (strcmp(argv[1], "pressure") == 0)
		printf("PASS pressure (%u MiB)\n", iterations);
	else
		printf("PASS %s (%u iterations for looped cases)\n",
		    argv[1], iterations);
	return 0;
}
