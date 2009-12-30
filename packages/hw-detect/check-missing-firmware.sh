#!/bin/sh
set -e
. /usr/share/debconf/confmodule

MISSING=/dev/.udev/firmware-missing
DENIED=/tmp/missing-firmware-denied

log () {
	logger -t check-missing-firmware "$@"
}

# Not all drivers register themselves if firmware is missing; in that
# case determine the module via the device's modalias.
get_module () {
	local devpath=$1

	if [ -d $devpath/driver ]; then
		# The realpath of the destination of the driver/module
		# symlink should be something like "/sys/module/e100"
		basename $(realpath $devpath/driver/module) || true
	elif [ -e $devpath/modalias ]; then
		modalias="$(cat $devpath/modalias)"
		# Take the last module returned by modprobe
		modprobe --show-depends "$modalias" 2>/dev/null | \
			sed -n -e '$s#^.*/\([^.]*\)\.ko.*$#\1#p'
	fi
}

check_missing () {
	# Give modules some time to request firmware.
	sleep 1
	
	modules=""
	files=""
	if [ -d "$MISSING" ]; then
		for file in $(find $MISSING -type l); do
			# decode firmware filename as encoded by
			# udev firmware.agent
			fwfile="$(basename $file | sed -e 's#\\x2f#/#g')"
			
			# strip probably nonexistant firmware subdirectory
			devpath="$(readlink $file | sed 's/\/firmware\/.*//')"
			# the symlink is supposed to point to the device in /sys
			if ! echo "$devpath" | grep -q '^/sys/'; then
				devpath="/sys$devpath"
			fi

			module=$(get_module "$devpath")
			if [ -z "$module" ]; then
				log "failed to determine module from $devpath"
				continue
			fi

			rm -f "$file"

			if grep -q "^$fwfile$" $DENIED 2>/dev/null; then
				continue
			fi

			modules="$module${modules:+ $modules}"
			files="$fwfile${files:+ $files}"
		done
	fi

	if [ -n "$modules" ]; then
		log "missing firmware files ($files) for $modules"
		return 0
	else
		log "no missing firmware in $MISSING"
		return 1
	fi
}

first_try=1
first_ask=1
ask_load_firmware () {
	if [ "$first_try" ]; then
		first_try=""
		return 0
	fi

	db_subst hw-detect/load_firmware FILES "$files"
	if ! db_input high hw-detect/load_firmware; then
		if [ ! "$first_ask" ]; then
			exit 1;
		else
			first_ask=""
		fi
	fi
	if ! db_go; then
		exit 10 # back up
	fi
	db_get hw-detect/load_firmware
	if [ "$RET" = true ]; then
		return 0
	else
		echo "$files" | tr ' ' '\n' >> $DENIED
		return 1
	fi
}

list_deb_firmware () {
	ar p "$1" data.tar.gz | tar zt \
		| grep '^\./lib/firmware/' \
		| sed -e 's!^\./lib/firmware/!!' \
		| grep -v '^$'
}

check_deb_arch () {
	arch=$(ar p "$1" control.tar.gz | tar zxO ./control | grep '^Architecture:' | sed -e 's/Architecture: *//')
	[ "$arch" = all ] || [ "$arch" = "$(udpkg --print-architecture)" ]
}

install_firmware_pkg () {
	if echo "$1" | grep -q '\.deb$'; then
		# cache deb for installation into /target later
		mkdir -p /var/cache/firmware/
		cp -a "$1" /var/cache/firmware/ || true
		udpkg --unpack "/var/cache/firmware/$(basename "$1")"
	else
		udpkg --unpack "$1"
	fi
}

while check_missing && ask_load_firmware; do
	# first, look for loose firmware files on the media.
	if mountmedia; then
		for file in $files; do
			for f in "/media/$file" "/media/firmware/$file"; do
				if [ -e "$f" ]; then
					log "copying loose file $file"
					mkdir -p /lib/firmware
					rm -f "/lib/firmware/$file"
					cp -a "$f" /lib/firmware/ || true
					break
				fi
			done
		done
		umount /media || true
	fi

	# Try to load udebs (or debs) that contain the missing firmware.
	# This does not use anna because debs can have arbitrary
	# dependencies, which anna might try to install.
	if mountmedia driver; then
		echo "$files" | sed -e 's/ /\n/g' >/tmp/grepfor
		for filename in /media/*.deb /media/*.udeb /media/*.ude /media/firmware/*.deb /media/firmware/*.udeb /media/firmware/*.ude; do
			if [ -f "$filename" ]; then
				if check_deb_arch "$filename" && list_deb_firmware "$filename" | grep -qf /tmp/grepfor; then
					log "installing firmware package $filename"
					install_firmware_pkg "$filename" || true
				fi
			fi
		done
		rm -f /tmp/grepfor
		umount /media || true
	fi

	# remove and reload modules so they see the new firmware
	for module in $modules; do
		modprobe -r $module || true
		modprobe $module || true
	done
done
