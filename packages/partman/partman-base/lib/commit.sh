# General functions related to committing changes to devices
# Calling scripts must ensure /lib/partman/lib/base.sh is sourced as well!

# List the changes that are about to be committed and let the user confirm first
confirm_changes () {
	local dev part partitions num id size type fs path name filesystem
	local x template partdesc partitems items formatted_previously
	local device dmtype
	template="$1"

	# Compute the changes we are going to do
	partitems=''
	items=''
	formatted_previously=no
	for dev in $DEVICES/*; do
		[ -d "$dev" ] || continue
		cd $dev

		open_dialog IS_CHANGED
		read_line x
		close_dialog
		if [ "$x" = yes ]; then
			partitems="${partitems}   $(humandev $(cat device))
"
		fi

		partitions=
		open_dialog PARTITIONS
		while { read_line num id size type fs path name; [ "$id" ]; }; do
			[ "$fs" != free ] || continue
			partitions="$partitions $id,$num"
		done
		close_dialog

		for part in $partitions; do
			id=${part%,*}
			num=${part#*,}
			[ -f $id/method -a -f $id/format \
			  -a -f $id/visual_filesystem ] || continue
			# if no filesystem (e.g. swap) should either be not
			# formatted or formatted before the method is specified
			[ -f $id/filesystem -o ! -f $id/formatted \
			  -o $id/formatted -ot $id/method ] || continue
			# if it is already formatted filesystem it must be formatted
			# before the method or filesystem is specified
			[ ! -f $id/filesystem -o ! -f $id/formatted \
			  -o $id/formatted -ot $id/method \
			  -o $id/formatted -ot $id/filesystem ] ||
			{
				formatted_previously=yes
				continue
			}
			filesystem=$(cat $id/visual_filesystem)

			partdesc=""
			# Special case d-m devices to use a different description
			if cat device | grep -q "/dev/mapper" ; then
				device=$(cat device)
				# dmraid and multipath devices are partitioned
				if [ ! -f sataraid ] && \
				   ! is_multipath_dev $device && \
				   ! is_multipath_part $device); then
					partdesc="partman/text/confirm_unpartitioned_item"
				fi
			fi
			if [ -z "$partdesc" ]; then
				partdesc="partman/text/confirm_item"
				db_subst $partdesc PARTITION "$num"
			fi
			db_subst $partdesc TYPE "$filesystem"
			db_subst $partdesc DEVICE $(humandev $(cat device))
			db_metaget $partdesc description

			items="${items}   ${RET}
"
		done
	done

	if [ "$items" ]; then
		db_metaget partman/text/confirm_item_header description
		items="$RET
$items"
	fi

	if [ "$partitems" ]; then
		db_metaget partman/text/confirm_partitem_header description
		partitems="$RET
$partitems"
	fi

	if [ "$partitems$items" ]; then
		if [ -z "$items" ]; then
			x="$partitems"
		elif [ -z "$partitems" ]; then
			x="$items"
		else
			x="$partitems
$items"
		fi
		maybe_escape "$x" db_subst $template/confirm ITEMS
		db_input critical $template/confirm
		db_go || true
		db_get $template/confirm
		if [ "$RET" = false ]; then
			db_reset $template/confirm
			return 1
		else
			db_reset $template/confirm
			return 0
		fi
	else
		if [ "$formatted_previously" = no ]; then
			db_input critical $template/confirm_nochanges
			db_go || true
			if [ $template = partman-dmraid ]; then
				# for dmraid, only a note is displayed
				return 1
			fi
			db_get $template/confirm_nochanges
			if [ "$RET" = false ]; then
				db_reset $template/confirm_nochanges
				return 1
			else
				db_reset $template/confirm_nochanges
				return 0
			fi
		else
			return 0
		fi
	fi
}

# Remove directories for partitions that no longer exist
# Device directory must be current
device_cleanup_partitions () {
	local partitions pdirs pdir

	partitions=
	open_dialog PARTITIONS
	while { read_line x1 id x; [ "$id" ]; }; do
		partitions="${partitions:+$partitions$NL}$id"
	done
	close_dialog

	pdirs="$(find . -type d | cut -d/ -f2 | sort -u |
		grep "^[0-9]\+-[0-9]\+$")"
	for pdir in $pdirs; do
		if ! echo "$partitions" | grep -q "^$pdir$"; then
			rm -rf $pdir
		fi
	done
}

commit_changes () {
	local template
	template=$1

	for s in /lib/partman/commit.d/*; do
		if [ -x $s ]; then
			$s || {
				db_input critical $template || true
				db_go || true
				for s in /lib/partman/init.d/*; do
					if [ -x $s ]; then
						$s || return 255
					fi
				done
				return 1
			}
		fi
	done

	return 0
}
