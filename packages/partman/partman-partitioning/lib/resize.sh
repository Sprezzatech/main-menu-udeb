. /lib/partman/lib/base.sh
. /lib/partman/lib/commit.sh

# Sets $virtual; used by other functions here.
check_virtual () {
	open_dialog VIRTUAL $oldid
	read_line virtual
	close_dialog
}

get_real_device () {
	local backupdev num
	# A weird way to get the real device path. The partition numbers
	# in parted_server may be changed and the partition table is still
	# not commited to the disk.
	backupdev=/var/lib/partman/backup/${dev#/var/lib/partman/devices/}
	if [ -f $backupdev/$oldid/view ] && [ -f $backupdev/device ]; then
		num=$(sed 's/^[^0-9]*\([0-9]*\)[^0-9].*/\1/' $backupdev/$oldid/view)
		bdev=$(cat $backupdev/device)
		case $bdev in
		    */disc)
			bdev=${bdev%/disc}/part$num
			;;
			/dev/[hs]d[a-z])
			bdev=$bdev$num
			;;
		    *)
			log "get_real_device: strange device name $bdev"
			return
			;;
		esac
		if [ ! -b $bdev ]; then
			bdev=
		fi
	fi
}

do_ntfsresize () {
	local RET
	ntfsresize="$(ntfsresize $@ 2>&1)"
	RET=$?
	echo "$ntfsresize" | grep -v "percent completed" | \
		logger -t ntfsresize
	return $RET
}

get_ntfs_resize_range () {
	local bdev size
	open_dialog GET_VIRTUAL_RESIZE_RANGE $oldid
	read_line minsize cursize maxsize
	close_dialog
	get_real_device
	if [ "$bdev" ]; then
		if ! do_ntfsresize -f -i $bdev; then
			logger -t partman "Error running 'ntfsresize --info'"
			return 1
		fi
		size=$(echo "$ntfsresize" | \
			grep '^You might resize at' | \
			sed 's/^You might resize at \([0-9]*\) bytes.*/\1/' | \
			grep '^[0-9]*$')
		if [ "$size" ]; then
			minsize=$size
		fi
	fi
}

get_ext2_resize_range () {
	local bdev tune2fs block_size block_count free_blocks
	open_dialog GET_VIRTUAL_RESIZE_RANGE $oldid
	read_line minsize cursize maxsize
	close_dialog
	get_real_device
	if [ "$bdev" ]; then
		if ! tune2fs="$(tune2fs -l $bdev)"; then
			logger -t partman "Error running 'tune2fs -l $bdev'"
			return 1
		fi
		block_size="$(echo "$tune2fs" | grep '^Block size:' | \
			head -n1 | sed 's/.*:[[:space:]]*//')"
		block_count="$(echo "$tune2fs" | grep '^Block count:' | \
			head -n1 | sed 's/.*:[[:space:]]*//')"
		free_blocks="$(echo "$tune2fs" | grep '^Free blocks:' | \
			head -n1 | sed 's/.*:[[:space:]]*//')"
		if expr "$block_size" : '[0-9][0-9]*$' >/dev/null && \
		   expr "$block_count" : '[0-9][0-9]*$' >/dev/null && \
		   expr "$free_blocks" : '[0-9][0-9]*$' >/dev/null; then
			minsize="$(expr \( "$block_count" - "$free_blocks" \) \* "$block_size")"
		fi
	fi
	return 0
}

get_resize_range () {
	open_dialog GET_RESIZE_RANGE $oldid
	read_line minsize cursize maxsize
	close_dialog
}

human_resize_range () {
	hminsize=$(longint2human $minsize)
	hcursize=$(longint2human $cursize)
	hmaxsize=$(longint2human $maxsize)
	minpercent="$(expr 100 \* "$minsize" / "$maxsize")"
}

ask_for_size () {
	local noninteractive digits minmb
	noninteractive=true
	while true; do
		newsize=''
		while [ ! "$newsize" ]; do
			db_set partman-partitioning/new_size "$hcursize"
			db_subst partman-partitioning/new_size MINSIZE "$hminsize"
			db_subst partman-partitioning/new_size MAXSIZE "$hmaxsize"
			db_subst partman-partitioning/new_size PERCENT "$minpercent%"
			db_input critical partman-partitioning/new_size || $noninteractive
			noninteractive="return 1"
			db_go || return 1
			db_get partman-partitioning/new_size
			case "$RET" in
			    max)
				newsize=$maxsize
				;;
			    *%)
				digits=$(expr "$RET" : '\([1-9][0-9]*\) *%$')
				if [ "$digits" ]; then
					maxmb=$(expr 0000000"$maxsize" : '0*\(..*\)......$')
					newsize=$(($digits * $maxmb / 100))000000
				fi
				;;
			    *)
				if valid_human "$RET"; then
					newsize=$(human2longint "$RET")
				fi
				;;
			esac
			if [ -z "$newsize" ]; then
				db_input high partman-partitioning/bad_new_size || true
				db_go || true
			elif ! longint_le "$newsize" "$maxsize"; then
				db_input high partman-partitioning/big_new_size || true
				db_go || true
				newsize=''
			elif ! longint_le "$minsize" "$newsize"; then
				db_input high partman-partitioning/small_new_size || true
				db_go || true
				newsize=''
			fi
		done

		if perform_resizing; then break; fi
	done
	return 0
}

perform_resizing () {
	if [ "$virtual" = no ]; then
		commit_changes partman-partitioning/new_size_commit_failed || exit 100
	fi

	disable_swap

	if [ "$virtual" = no ] && \
	   [ -f $oldid/detected_filesystem ] && \
	   [ "$(cat $oldid/detected_filesystem)" = ntfs ]; then

		# Resize NTFS
		db_progress START 0 1000 partman/text/please_wait
		db_progress INFO partman-partitioning/progress_resizing
		if longint_le "$cursize" "$newsize"; then
			open_dialog VIRTUAL_RESIZE_PARTITION $oldid $newsize
			read_line newid
			close_dialog
			open_dialog COMMIT
			close_dialog
			open_dialog PARTITION_INFO $newid
			read_line x1 x2 x3 x4 x5 path x7
			close_dialog
			# Wait for the device file to be created again
			update-dev

			if ! echo y | do_ntfsresize -f $path; then
				logger -t partman "Error resizing the NTFS file system to the partition size"
				db_input high partman-partitioning/new_size_commit_failed || true
				db_go || true
				db_progress STOP
				exit 100
			fi
		else
			open_dialog COMMIT
			close_dialog
			open_dialog PARTITION_INFO $oldid
			read_line x1 x2 x3 x4 x5 path x7
			close_dialog
			# Wait for the device file to be created
			update-dev

			if echo y | do_ntfsresize -f --size "$newsize" $path; then
				open_dialog VIRTUAL_RESIZE_PARTITION $oldid $newsize
				read_line newid
				close_dialog
				# Wait for the device file to be created
				update-dev

				if ! echo y | do_ntfsresize -f $path; then
					logger -t partman "Error resizing the NTFS file system to the partition size"
					db_input high partman-partitioning/new_size_commit_failed || true
					db_go || true
					db_progress STOP
					exit 100
				fi
			else
				logger -t partman "Error resizing the NTFS file system"
				db_input high partman-partitioning/new_size_commit_failed || true
				db_go || true
				db_progress STOP
				exit 100
			fi
		fi
		db_progress SET 1000
		db_progress STOP

	elif [ "$virtual" = no ] && \
	     [ -f $oldid/detected_filesystem ] && \
	     ([ "$(cat $oldid/detected_filesystem)" = ext2 ] || \
	      [ "$(cat $oldid/detected_filesystem)" = ext3 ]); then

		# Resize ext2/ext3; parted can handle simple cases but can't deal
		# with certain common features such as resize_inode
		fs="$(cat $oldid/detected_filesystem)"
		db_progress START 0 1000 partman/text/please_wait
		open_dialog PARTITION_INFO $oldid
		read_line num x2 x3 x4 x5 x6 x7
		close_dialog

		db_metaget "partman/filesystem_short/$fs" description || RET=
		[ "$RET" ] || RET="$fs"
		db_subst partman-basicfilesystems/progress_checking TYPE "$RET"
		db_subst partman-basicfilesystems/progress_checking PARTITION "$num"
		db_subst partman-basicfilesystems/progress_checking DEVICE "$(humandev $(cat device))"
		db_progress INFO partman-basicfilesystems/progress_checking

		if longint_le "$cursize" "$newsize"; then
			open_dialog VIRTUAL_RESIZE_PARTITION $oldid $newsize
			read_line newid
			close_dialog
			open_dialog COMMIT
			close_dialog
			open_dialog PARTITION_INFO $newid
			read_line x1 x2 x3 x4 x5 path x7
			close_dialog
		else
			open_dialog COMMIT
			close_dialog
			open_dialog PARTITION_INFO $oldid
			read_line x1 x2 x3 x4 x5 path x7
			close_dialog
		fi
		# Wait for the device file to be created
		update-dev

		e2fsck_code=0
		e2fsck -f -p $path || e2fsck_code=$?
		if [ $e2fsck_code -gt 1 ]; then
			db_subst partman-basicfilesystems/check_failed TYPE "$fs"
			db_subst partman-basicfilesystems/check_failed PARTITION "$num"
			db_subst partman-basicfilesystems/check_failed DEVICE "$(humandev $(cat device))"
			db_set partman-basicfilesystems/check_failed 'true'
			db_input critical partman-basicfilesystems/check_failed || true
			db_go || true
			db_get partman-basicfilesystems/check_failed
			if [ "$RET" = 'true' ]; then
				exit 100
			fi
		fi

		db_progress INFO partman-partitioning/progress_resizing
		db_progress SET 500
		if longint_le "$cursize" "$newsize"; then
			if ! resize2fs $path; then
				logger -t partman "Error resizing the ext2/ext3 file system to the partition size"
				db_input high partman-partitioning/new_size_commit_failed || true
				db_go || true
				db_progress STOP
				exit 100
			fi
		else
			if resize2fs $path "$(expr "$newsize" / 1024)K"; then
				open_dialog VIRTUAL_RESIZE_PARTITION $oldid $newsize
				read_line newid
				close_dialog
				# Wait for the device file to be created
				update-dev

				if ! resize2fs $path; then
					logger -t partman "Error resizing the ext2/ext3 file system to the partition size"
					db_input high partman-partitioning/new_size_commit_failed || true
					db_go || true
					db_progress STOP
					exit 100
				fi
			else
				logger -t partman "Error resizing the ext2/ext3 file system"
				db_input high partman-partitioning/new_size_commit_failed || true
				db_go || true
				db_progress STOP
				exit 100
			fi
		fi
		db_progress SET 1000
		db_progress STOP

	else

		# Resize virtual partitions, ext2, ext3, swap, fat16, fat32
		# and probably reiserfs
		name_progress_bar partman-partitioning/progress_resizing
		open_dialog RESIZE_PARTITION $oldid $newsize
		read_line newid
		close_dialog

	fi

	if [ "$newid" ] && [ "$newid" != "$oldid" ]; then
		rm -rf $newid
		mkdir $newid
		cp -r $oldid/* $newid/
	fi
	if [ "$virtual" = no ]; then
		for s in /lib/partman/init.d/*; do
			if [ -x $s ]; then
				$s || exit 100
			fi
		done
	else
		partitions=''
		open_dialog PARTITIONS
		while { read_line num part size type fs path name; [ "$part" ]; }; do
			partitions="$partitions $part"
		done
		close_dialog
		for part in $partitions; do
			update_partition $dev $part
		done
	fi
}
