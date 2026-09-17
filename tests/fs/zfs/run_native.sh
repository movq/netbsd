#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
#
# Run as root against an existing disposable pool.  Only the child dataset
# named below is destroyed.  No disks are partitioned or pools created.
set -eu
LC_ALL=C
export LC_ALL

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
	echo "usage: $0 pool [iterations]" >&2
	exit 2
fi
pool=$1
iterations=${2:-1000}
case "$pool" in
	""|*[!a-zA-Z0-9_.-]*) echo "expected a pool name" >&2; exit 2 ;;
esac
[ "$(id -u)" -eq 0 ] || { echo "root required" >&2; exit 2; }
helper=$(realpath "$(dirname "$0")/h_zfs_stress")
zpool list "$pool" >/dev/null
work=$(mktemp -d /var/tmp/zfs-native.XXXXXX)
dataset=$pool/native-$$
holder=
pressure=
created=no
cleanup()
{
	if [ -n "$pressure" ]; then
		kill "$pressure" 2>/dev/null || :
		wait "$pressure" 2>/dev/null || :
		cat "$work/pressure.log" >&2 || :
	fi
	if [ -n "$holder" ]; then
		cat "$work/holder.log" >&2 || :
		kill "$holder" 2>/dev/null || :
		wait "$holder" 2>/dev/null || :
	fi
	if [ "$created" = yes ]; then
		zfs destroy -rf "$dataset" || :
	fi
	rm -rf "$work"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
chmod 0755 "$work"
zfs create -o mountpoint="$work/fs" -o atime=on -o relatime=off \
    -o compression=off "$dataset"
created=yes
echo "Testing $dataset: $(uname -v)"
zfs version

if [ "${PRESSURE_MIB:-0}" != 0 ]; then
	mkfifo "$work/pressure-control"
	"$helper" pressure "$work" "$PRESSURE_MIB" <"$work/pressure-control" \
	    >"$work/pressure.log" 2>&1 &
	pressure=$!
	exec 4>"$work/pressure-control"
	n=0
	until grep -q '^ready$' "$work/pressure.log"; do
		kill -0 "$pressure"
		n=$((n + 1))
		[ "$n" -lt 60 ] || exit 1
		sleep 1
	done
	echo "Holding $PRESSURE_MIB MiB of touched anonymous memory"
fi

for test in ${CASES:-permissions metadata sparse holes flags append locking coherence lifetime contention directories namespace paging readonly lifecycle}; do
	echo "BEGIN $test"
	if [ "$test" = readonly ]; then
		zfs create "$dataset/ro"
		dd if=/dev/zero of="$work/fs/ro/held" bs=4096 count=1 2>/dev/null
		printf A | dd of="$work/fs/ro/held" conv=notrunc 2>/dev/null
		zfs snapshot "$dataset/ro@before"
		"$helper" readonly "$work/fs/ro/.zfs/snapshot/before"
		# The native mount adapter does not support mount updates yet.
		zfs unmount "$dataset/ro"
		zfs set readonly=on "$dataset/ro"
		zfs mount "$dataset/ro"
		"$helper" readonly "$work/fs/ro"
		zfs destroy -r "$dataset/ro"
		continue
	fi
	if [ "$test" != lifecycle ]; then
		mkdir "$work/fs/$test"
		"$helper" "$test" "$work/fs/$test" "$iterations"
		if [ "$test" = paging ]; then
			sum=$(sha256 -q "$work/fs/paging/paging")
			zfs unmount "$dataset"
			zfs mount "$dataset"
			[ "$(sha256 -q "$work/fs/paging/paging")" = "$sum" ]
			echo "PASS paging checksum after unmount/remount"
		fi
		continue
	fi
	for kind in fd map cwd; do
		for operation in unmount destroy rollback unmount-force destroy-force; do
			child=$dataset/held
			mnt=$work/fs/held
			zfs create "$child"
			dd if=/dev/zero of="$mnt/held" bs=4096 count=1 2>/dev/null
			printf A | dd of="$mnt/held" conv=notrunc 2>/dev/null
			zfs snapshot "$child@before"
			mkfifo "$work/control"
			"$helper" hold "$mnt" "$kind" <"$work/control" \
			    >"$work/holder.log" 2>&1 &
			holder=$!
			exec 3>"$work/control"
			n=0
			until grep -q '^ready$' "$work/holder.log"; do
				kill -0 "$holder"
				n=$((n + 1))
				[ "$n" -lt 30 ] || exit 1
				sleep 1
			done
			release=v
			case "$operation" in
			unmount|destroy)
				# Drop the snapshot so destruction reaches busy-vnode checks.
				zfs destroy "$child@before"
				if zfs "$operation" "$child"; then
					echo "FAIL $operation succeeded with $kind held" >&2
					exit 1
				fi
				zfs list "$child" >/dev/null
				[ -f "$mnt/held" ]
				;;
			rollback)
				printf B | dd of="$mnt/held" conv=notrunc 2>/dev/null
				zfs rollback "$child@before"
				[ "$(dd if="$mnt/held" bs=1 count=1 2>/dev/null)" = A ]
				;;
			unmount-force)
				zfs unmount -f "$child"
				release=x
				;;
			destroy-force)
				zfs destroy -rf "$child"
				release=x
				;;
			esac
			printf %s "$release" >&3
			exec 3>&-
			wait "$holder"
			holder=
			cat "$work/holder.log"
			rm "$work/control"
			if [ "$operation" != destroy-force ]; then
				zfs destroy -r "$child"
			fi
			echo "PASS lifecycle $kind $operation"
		done
	done
done
if [ -n "$pressure" ]; then
	printf x >&4
	exec 4>&-
	wait "$pressure"
	pressure=
	cat "$work/pressure.log"
fi
zpool sync "$pool"
zpool scrub "$pool"
zpool wait -t scrub "$pool"
zpool status "$pool"
zpool status -x "$pool" | grep -qx "pool '$pool' is healthy"
zfs destroy -r "$dataset"
created=no
echo "PASS native suite"
