#!/bin/sh -e
# Setup for using apt to install packages in /target.

mountpoints () {
	cut -d" " -f2 /proc/mounts | sort | uniq
}

chroot_setup () {
	# Create a policy-rc.d to stop maintainer scripts using invoke-rc.d 
	# from running init scripts. In case of maintainer scripts that don't
	# use invoke-rc.d, add a dummy start-stop-daemon.
	cat > /target/usr/sbin/policy-rc.d <<EOF
#!/bin/sh
exit 101
EOF
	chmod a+rx /target/usr/sbin/policy-rc.d
	
	if [ -e /target/sbin/start-stop-daemon ]; then
		mv /target/sbin/start-stop-daemon /target/sbin/start-stop-daemon.REAL
	fi
	cat > /target/sbin/start-stop-daemon <<EOF
#!/bin/sh
echo 1>&2
echo 'Warning: Fake start-stop-daemon called, doing nothing.' 1>&2
exit 0
EOF
	chmod a+rx /target/sbin/start-stop-daemon
	
	# Record the current mounts
	mountpoints > /tmp/mount.pre

	# Some packages (eg. the kernel-image package) require a mounted
	# /proc/. Only mount it if not mounted already
	if [ ! -f /target/proc/cmdline ] ; then
		mount -t proc proc /target/proc
	fi

	# For installing >=2.6.14 kernels we also need sysfs mounted
	# Only mount it if not mounted already and we're running 2.6
	case $(uname -r | cut -d . -f 1,2) in
	2.6)
		if [ ! -d /target/sys/devices ] ; then
			mount -t sysfs none /target/sys
		fi
	;;
	esac

	# Try to enable proxy when using HTTP.
	# What about using ftp_proxy for FTP sources?
	RET=$(debconf-get mirror/protocol || true)
	if [ "$RET" = "http" ]; then
		RET=$(debconf-get mirror/http/proxy || true)
		if [ "$RET" ] ; then
			http_proxy="$RET"
			export http_proxy
		fi
	fi

	# Pass debconf priority through.
	DEBIAN_PRIORITY=$(debconf-get debconf/priority || true)
	export DEBIAN_PRIORITY

	LANG=$(debconf-get debian-installer/locale || true)
	export LANG

	# Unset variables that would make scripts in the target think
	# that debconf is already running there.
	unset DEBIAN_HAS_FRONTEND
	unset DEBIAN_FRONTEND
	unset DEBCONF_FRONTEND
	unset DEBCONF_REDIR
	# Avoid debconf mailing notes.
	DEBCONF_ADMIN_EMAIL=""
	export DEBCONF_ADMIN_EMAIL
}

chroot_cleanup () {
	rm -f /target/usr/sbin/policy-rc.d
	rm /target/sbin/start-stop-daemon
	mv /target/sbin/start-stop-daemon.REAL /target/sbin/start-stop-daemon

	# Undo the mounts done by the packages during installation.
	# Reverse sorting to umount the deepest mount points first.
	# Items with count of 1 are new.
	for dir in $( (cat /tmp/mount.pre /tmp/mount.pre; mountpoints ) | \
		     sort -r | uniq -c | grep "^[[:space:]]*1[[:space:]]" | \
		     sed "s/^[[:space:]]*[0-9][[:space:]]//"); do
		if ! umount $dir ; then
			logger -t $0 "warning: Unable to umount '$dir'"
		fi
	done
	rm -f /tmp/mount.pre
}
