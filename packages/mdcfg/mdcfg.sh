#!/bin/sh

. /usr/share/debconf/confmodule

md_get_devices() {
	DEVICES=""
	for i in $(grep ^md /proc/mdstat | \
		   sed -e 's/^\(md.*\) : active \([[:alnum:]]*\).*/\1_\2/'); do
		DEVICES="${DEVICES:+$DEVICES, }$i"
	done
}

md_delete_verify() {
	DEVICE=$(echo "$1" | sed -e "s/^\(md.*\)_.*/\1/")
	INFO=$(grep "^$DEVICE[ :]" /proc/mdstat | \
		sed -e "s/^md.* : active \([[:alnum:]]*\) \(.*\)/\1:\2/")
	TYPE=$(echo $INFO | sed -e "s/\(.*\):.*/\1/")
	DEVICES=$(echo $INFO | sed -e "s/.*:\(.*\)/\1/")
	NUMBER=$(echo $DEVICE | sed -e "s/^md//")

	# Also handle the case where the first does not exist
	if [ -b /dev/md/$NUMBER ]; then
		MDDEV=/dev/md/$NUMBER
	elif [ -b /dev/md$NUMBER ]; then
		MDDEV=/dev/md$NUMBER
	else
		return 1
	fi

	db_set mdcfg/deleteverify false
	db_subst mdcfg/deleteverify DEVICE "/dev/$DEVICE"
	db_subst mdcfg/deleteverify TYPE "$TYPE"
	db_subst mdcfg/deleteverify DEVICES "$DEVICES"
	db_input critical mdcfg/deleteverify
	db_go
	db_get mdcfg/deleteverify

	case $RET in
	    true)
		# Stop the MD device and zero the superblock
		# of all the component devices
		DEVICES=$(mdadm -Q --detail $MDDEV | \
			  grep -E "(active|spare)" | sed -e 's/.* //')
		logger -t mdcfg "Removing $MDDEV ($DEVICES)"
		log-output -t mdcfg mdadm --stop $MDDEV || return 1
		for DEV in "$DEVICES"; do
			log-output -t mdcfg \
				mdadm --zero-superblock --force $DEV || return 1
		done
		;;
	esac
	return 0
}

md_delete() {
	md_get_devices
	if [ -z "$DEVICES" ]; then
		db_set mdcfg/delete_no_md false
		db_input high mdcfg/delete_no_md
		db_go
		return
	fi
	db_set mdcfg/deletemenu false
	db_subst mdcfg/deletemenu DEVICES "$DEVICES"
	db_input critical mdcfg/deletemenu
	db_go
	db_get mdcfg/deletemenu

	case $RET in
	    md*)
		if ! md_delete_verify $RET; then
			db_input critical mdcfg/deletefailed
			db_go
		fi
		;;
	esac
}

md_createmain() {
	db_set mdcfg/createmain false
	db_input critical mdcfg/createmain
	db_go
	if [ $? -ne 30 ]; then
		db_get mdcfg/createmain
		if [ "$RET" = Cancel ]; then
			return
		fi
		RAID_SEL="$RET"

		if ! get_partitions; then
			return
		fi

		case "$RAID_SEL" in
		    RAID5)
			md_create_raid5 ;;
		    RAID1)
			md_create_raid1 ;;
		    RAID0)
			md_create_raid0 ;;
		esac
	fi
}

# This will set PARTITIONS and NUM_PART global variables
get_partitions() {
	PARTITIONS=""

	# Get a list of RAID partitions. This only works if there is no
	# filesystem on the partitions, which is fine by us.
	RAW_PARTITIONS=$(/usr/lib/partconf/find-partitions --ignore-fstype 2>/dev/null | \
		grep "[[:space:]]RAID[[:space:]]" | cut -f1)

	# Convert it into a proper list form for a select question
	# (comma seperated)
	NUM_PART=0
	for i in $RAW_PARTITIONS; do
		DEV=$(echo $i | sed -e "s/\/dev\///")
		REALDEV=$(mapdevfs "$i")
		MAPPEDDEV=$(echo "$REALDEV" | sed -e "s/\/dev\///")

		if grep -Eq "($DEV|$MAPPEDDEV)" /proc/mdstat; then
			continue
		fi

		PARTITIONS="${PARTITIONS:+$PARTITIONS, }$REALDEV"
		NUM_PART=$(($NUM_PART + 1))
	done

	if [ -z "$PARTITIONS" ] ; then
		db_input critical mdcfg/noparts
		db_go
		return 1
	fi
	return 0
}

prune_partitions() {
	CHOSEN="$1"
	OLDIFS="$IFS"
	IFS=,
	NEW_PARTITIONS=""
	for i in $PARTITIONS; do
		found=0
		for j in $CHOSEN; do
			if [ "$i" = "$j" ]; then
				found=1
			fi
		done
		if [ $found -eq 0 ]; then
			NEW_PARTITIONS="${NEW_PARTITIONS:+$NEW_PARTITIONS,}$i"
		fi
	done
	IFS=$OLDIFS
	PARTITIONS=$NEW_PARTITIONS
}

md_create_raid0() {
	db_subst mdcfg/raid0devs PARTITIONS "$PARTITIONS"
	db_set mdcfg/raid0devs ""
	db_input critical mdcfg/raid0devs
	db_go

	if [ $? -eq 30 ]; then return; fi

	db_get mdcfg/raid0devs
	SELECTED=0
	for i in $RET; do
		let SELECTED++
	done

	prune_partitions "$RET"

	MD_NUM=$(grep ^md /proc/mdstat | \
		 sed -e 's/^md\(.*\) : active .*/\1/' | sort | tail -n1)

	if [ -z "$MD_NUM" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	logger -t mdcfg "Number of devices in the RAID0 array md$MD_NUM: $SELECTED"

	RAID_DEVICES="$(echo $RET | sed -e 's/,//g')"
	log-output -t mdcfg \
		mdadm --create /dev/md/$MD_NUM --auto=yes --force -R -l raid0 \
		      -n $SELECTED $RAID_DEVICES
}

md_create_raid1() {
	OK=0

	db_set mdcfg/raid1devcount 2

	# Get the count of active devices
	while [ $OK -eq 0 ]; do
		db_input critical mdcfg/raid1devcount
		db_go
		if [ $? -eq 30 ]; then
			return
		fi

		# Figure out, if the user entered a number
		db_get mdcfg/raid1devcount
		RET=$(echo $RET | sed -e "s/[[:space:]]//g")
		if [ "$RET" ]; then
			let "OK=${RET}>0 && ${RET}<99"
		fi
	done

	db_set mdcfg/raid1sparecount "0"
	OK=0

	# Same procedure as above, but get the number of spare partitions
	# this time.
	# TODO: Make a general function for this kind of stuff
	while [ $OK -eq 0 ]; do
		db_input critical mdcfg/raid1sparecount
		db_go
		if [ $? -eq 30 ]; then
			return
		fi
		db_get mdcfg/raid1sparecount
		RET=$(echo $RET | sed -e "s/[[:space:]]//g")
		if [ "$RET" ]; then
			let "OK=${RET}>=0 && ${RET}<99"
		fi
	done

	db_get mdcfg/raid1devcount
	DEV_COUNT="$RET"
	db_get mdcfg/raid1sparecount
	SPARE_COUNT="$RET"
	REQUIRED=$(($DEV_COUNT + $SPARE_COUNT))

	db_set mdcfg/raid1devs ""
	SELECTED=0

	# Loop until at least one device has been selected
	until [ $SELECTED -gt 0 ] && [ $SELECTED -le $DEV_COUNT ]; do
		db_subst mdcfg/raid1devs COUNT "$DEV_COUNT"
		db_subst mdcfg/raid1devs PARTITIONS "$PARTITIONS"
		db_input critical mdcfg/raid1devs
		db_go
		if [ $? -eq 30 ]; then
			return
		fi

		db_get mdcfg/raid1devs
		SELECTED=0
		for i in $RET; do
			DEVICE=$(echo $i | sed -e "s/,//")
			let SELECTED++
		done
	done

	# Add "missing" for as many devices as weren't selected
	MISSING_DEVICES=""
	while [ $SELECTED -lt $DEV_COUNT ]; do
		MISSING_DEVICES="$MISSING_DEVICES missing"
		let SELECTED++
	done

	# Remove partitions selected in raid1devs from the PARTITION list
	db_get mdcfg/raid1devs

	prune_partitions "$RET"

	db_set mdcfg/raid1sparedevs ""
	SELECTED=0
	if [ $SPARE_COUNT -gt 0 ]; then
		FIRST=1
		# Loop until the correct number of devices has been selected.
		# That means any number less than or equal to the spare count.
		while [ $SELECTED -gt $SPARE_COUNT ] || [ $FIRST -eq 1 ]; do
			FIRST=0
			db_subst mdcfg/raid1sparedevs COUNT "$SPARE_COUNT"
			db_subst mdcfg/raid1sparedevs PARTITIONS "$PARTITIONS"
			db_input critical mdcfg/raid1sparedevs
			db_go
			if [ $? -eq 30 ]; then
				return
			fi

			db_get mdcfg/raid1sparedevs
			SELECTED=0
			for i in $RET; do
				DEVICE=$(echo $i | sed -e "s/,//")
				let SELECTED++
			done
		done
	fi

	# The number of spares the user has selected
	NAMED_SPARES=$SELECTED

	db_get mdcfg/raid1devs
	RAID_DEVICES=$(echo $RET | sed -e "s/,//g")

	db_get mdcfg/raid1sparedevs
	SPARE_DEVICES=$(echo $RET | sed -e "s/,//g")

	MISSING_SPARES=""

	COUNT=$NAMED_SPARES
	while [ $COUNT -lt $SPARE_COUNT ]; do
		MISSING_SPARES="$MISSING_SPARES missing"
		let COUNT++
	done

	# Find the next available md-number
	MD_NUM=$(grep ^md /proc/mdstat | \
		 sed -e 's/^md\(.*\) : active .*/\1/' | sort | tail -n1)
	if [ -z "$MD_NUM" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	logger -t mdcfg "Selected spare count: $NAMED_SPARES"
	logger -t mdcfg "Raid devices count: $DEV_COUNT"
	logger -t mdcfg "Spare devices count: $SPARE_COUNT"
	log-output -t mdcfg \
		mdadm --create /dev/md/$MD_NUM --auto=yes --force -R -l raid1 \
		      -n $DEV_COUNT -x $SPARE_COUNT $RAID_DEVICES $MISSING_DEVICES \
		      $SPARE_DEVICES $MISSING_SPARES
}

md_create_raid5() {
	OK=0

	db_set mdcfg/raid5devcount "3"

	# Get the count of active devices
	while [ $OK -eq 0 ]; do
		db_input critical mdcfg/raid5devcount
		db_go
		if [ $? -eq 30 ]; then
			return
		fi

		# Figure out, if the user entered a number
		db_get mdcfg/raid5devcount
		RET=$(echo $RET | sed -e "s/[[:space:]]//g")
		if [ "$RET" ]; then
			let "OK=${RET}>0 && ${RET}<99"
		fi
	done

	db_set mdcfg/raid5sparecount "0"
	OK=0

	# Same procedure as above, but get the number of spare partitions
	# this time.
	# TODO: Make a general function for this kind of stuff
	while [ $OK -eq 0 ]; do
		db_input critical mdcfg/raid5sparecount
		db_go
		if [ $? -eq 30 ]; then
			return
		fi
		db_get mdcfg/raid5sparecount
		RET=$(echo $RET | sed -e "s/[[:space:]]//g")
		if [ "$RET" ]; then
			let "OK=${RET}>=0 && ${RET}<99"
		fi
	done

	db_get mdcfg/raid5devcount
	DEV_COUNT="$RET"
	if [ $DEV_COUNT -lt 3 ] ; then
		DEV_COUNT=3 # Minimum number for RAID5
	fi
	db_get mdcfg/raid5sparecount
	SPARE_COUNT="$RET"
	REQUIRED=$(($DEV_COUNT + $SPARE_COUNT))
	if [ $REQUIRED -gt $NUM_PART ] ; then
		db_subst mdcfg/notenoughparts NUM_PART "$NUM_PART"
		db_subst mdcfg/notenoughparts REQUIRED "$REQUIRED"
		db_input critical mdcfg/notenoughparts
		db_go mdcfg/notenoughparts
		return
	fi

	db_set mdcfg/raid5devs ""
	SELECTED=0

	# Loop until the correct number of active devices has been selected
	while [ $SELECTED -ne $DEV_COUNT ]; do
		db_subst mdcfg/raid5devs COUNT "$DEV_COUNT"
		db_subst mdcfg/raid5devs PARTITIONS "$PARTITIONS"
		db_input critical mdcfg/raid5devs
		db_go
		if [ $? -eq 30 ]; then
			return
		fi

		db_get mdcfg/raid5devs
		SELECTED=0
		for i in $RET; do
			DEVICE=$(echo $i | sed -e "s/,//")
			let SELECTED++
		done
	done

	# Remove partitions selected in raid5devs from the PARTITION list
	db_get mdcfg/raid5devs

	prune_partitions "$RET"

	db_set mdcfg/raid5sparedevs ""
	SELECTED=0
	if [ $SPARE_COUNT -gt 0 ]; then
		FIRST=1
		# Loop until the correct number of devices has been selected.
		# That means any number less than or equal to the spare count.
		while [ $SELECTED -gt $SPARE_COUNT ] || [ $FIRST -eq 1 ]; do
			FIRST=0
			db_subst mdcfg/raid5sparedevs COUNT "$SPARE_COUNT"
			db_subst mdcfg/raid5sparedevs PARTITIONS "$PARTITIONS"
			db_input critical mdcfg/raid5sparedevs
			db_go
			if [ $? -eq 30 ]; then
				return
			fi

			db_get mdcfg/raid5sparedevs
			SELECTED=0
			for i in $RET; do
				DEVICE=$(echo $i | sed -e "s/,//")
				let SELECTED++
			done
		done
	fi

	# The number of spares the user has selected
	NAMED_SPARES=$SELECTED

	db_get mdcfg/raid5devs
	RAID_DEVICES=$(echo $RET | sed -e "s/,//g")

	db_get mdcfg/raid5sparedevs
	SPARE_DEVICES=$(echo $RET | sed -e "s/,//g")

	MISSING_SPARES=""

	COUNT=$NAMED_SPARES
	while [ $COUNT -lt $SPARE_COUNT ]; do
		MISSING_SPARES="$MISSING_SPARES missing"
		let COUNT++
	done

	# Find the next available md-number
	MD_NUM=$(grep ^md /proc/mdstat | \
		 sed -e 's/^md\(.*\) : active .*/\1/' | sort | tail -n1)
	if [ -z "$MD_NUM" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	logger -t mdcfg "Selected spare count: $NAMED_SPARES"
	logger -t mdcfg "Raid devices count: $DEV_COUNT"
	logger -t mdcfg "Spare devices count: $SPARE_COUNT"
	log-output -t mdcfg \
		mdadm --create /dev/md/$MD_NUM --auto=yes --force -R -l raid5 \
		      -n $DEV_COUNT -x $SPARE_COUNT $RAID_DEVICES \
		      $SPARE_DEVICES $MISSING_SPARES
}

md_mainmenu() {
	while true; do
		db_set mdcfg/mainmenu false
		db_input critical mdcfg/mainmenu
		db_go
		if [ $? -eq 30 ]; then
			exit 30
		fi
		db_get mdcfg/mainmenu
		case $RET in
		    "Create MD device")
			md_createmain ;;
		    "Delete MD device")
			md_delete ;;
		    "Finish")
			break ;;
		esac
	done
}

### Main of script ###

# Try to load the necesarry modules.
# Supported schemes: RAID 0, RAID 1, RAID 5
depmod -a >/dev/null 2>&1
modprobe md >/dev/null 2>&1 || modprobe md-mod >/dev/null 2>&1
modprobe raid0 >/dev/null 2>&1
modprobe raid1 >/dev/null 2>&1
# kernels >=2.6.18 have raid456
modprobe raid456 >/dev/null 2>&1 || modprobe raid5 >/dev/null 2>&1

# Try to detect MD devices, and start them
# mdadm will fail if /dev/md does not already exist
mkdir -p /dev/md

log-output -t mdcfg --pass-stdout \
	mdadm --examine --scan --config=partitions >/tmp/mdadm.conf
# Convert to /dev/md/X notation as then both /dev/mdX and /dev/md/X
# are created when the array is assembled
sed -i "s:/dev/md\([0-9]\):/dev/md/\1:" /tmp/mdadm.conf

log-output -t mdcfg \
	mdadm --assemble --scan --run --config=/tmp/mdadm.conf --auto=yes

# Make sure that we have md-support
if [ ! -e /proc/mdstat ]; then
	db_set mdcfg/nomd false
	db_input high mdcfg/nomd
	db_go
	exit 0
fi

# Force mdadm to be installed on the target system
apt-install mdadm

# We want the "go back" button
#db_capb backup

md_mainmenu

exit 0
