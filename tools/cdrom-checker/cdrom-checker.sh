#!/bin/sh

. /usr/share/debconf/confmodule

TARGET="/cdrom"
DEVICE=`mount | grep ' /cdrom ' | cut -d " " -f 1`
TMPFILE="/tmp/md5tmp"

ask_mount() {
	db_fset cdrom-checker/askmount seen false
	db_input high cdrom-checker/askmount
	db_go

	mount -t auto $DEVICE $TARGET -o ro
	if [ $? -ne 0 ]; then
		db_fset cdrom-checker/mntfailed seen false
		db_input high cdrom-checker/mntfailed
		db_go
		break
	fi
}

cd_check() {
	if [ ! -d ".disk" -o ! -f "md5sum.txt" -o ! -d "debian" ]; then
		db_fset cdrom-checker/wrongcd seen false
		db_input high cdrom-checker/wrongcd
		db_go
		break
	fi
}

cleanup() {
	[ -f "$TMPFILE" ] && rm -f $TMPFILE
}



# ask the user if we should check
db_set cdrom-checker/start "false"
db_fset cdrom-checker/start seen false
db_input high cdrom-checker/start
db_go
db_get cdrom-checker/start
[ "$RET" != "true" ] && exit 0

# goeing here into main-loop
while [ 1 ]; do
	# make sure, cdrom is already mounted under $TARGET
	grep -q " $TARGET " /proc/mounts
	[ $? -ne 0 ] && ask_mount

	# cdrom is mounted, start checking
	cd $TARGET
	cd_check
	#db_progress_start 0 $mcount cdrom-checker/progress_title	# FIXME

	# calculate the max percent
	set -- `wc -l md5sum.txt`
	mcount="$1"

	cleanup
	count=0
	cat md5sum.txt 2>/dev/null | \
	while read LINE; do
		count=$(($count+1))
		set -- $LINE
		file=`echo $2 | sed -e 's/^\.\///'`
		#db_subst cdrom-checker/progress_step FILE "$file"
		#db_progress_step 1 cdrom-checker/progress_step
		echo "$1  $2" >$TMPFILE
		md5sum -c $TMPFILE 1>/dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo -n "$2" >$TMPFILE
			break
		fi
		cleanup
	done

	cd /	# make sure, cdrom is not bussy
	umount /cdrom
	#db_progress_stop			# FIXME

	# some file is faulty, stop here
	if [ -f "$TMPFILE" ]; then
		file=`cat $TMPFILE | sed -e 's/^\.\///'`
		db_subst cdrom-checker/missmatch FILE "$file"
		db_fset cdrom-checker/missmatch seen false
		db_input critical cdrom-checker/missmatch
		db_go
	else	# notice the user about success
		db_fset cdrom-checker/passed seen false
		db_input high cdrom-checker/passed
		db_go
	fi
	cleanup

	# ask for next cdrom
	db_set cdrom-checker/nextcd "false"
	db_fset cdrom-checker/nextcd seen false
	db_input high cdrom-checker/nextcd
	db_go
	db_get cdrom-checker/nextcd
	[ "$RET" != "true" ] && break
done

db_fset cdrom-checker/firstcd seen false
db_input high cdrom-checker/firstcd
db_go
udpkg --force-configure --configure cdrom-detect
exit 0

