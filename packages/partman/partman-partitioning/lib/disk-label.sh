# Calling scripts should also source base.sh if create_new_label is called

default_disk_label () {
	if [ -x /bin/archdetect ]; then
		archdetect=$(archdetect)
	else
		archdetect=unknown/generic
	fi
	arch=${archdetect%/*}
	sub=${archdetect#*/}
	case "$arch" in
	    alpha)
		# Load srm_env.o if we can; this should fail on ARC-based systems.
		(modprobe srm_env || true) 2>/dev/null
		if [ -f /proc/srm_environment/named_variables/booted_dev ]; then
			# Have SRM, so need BSD disklabels
			echo bsd
		else
			echo msdos
		fi;;	    
	    arm|armel)
		case "$sub" in
		    iop32x)
			echo msdos;;
		    iop33x)
			echo msdos;;
		    ixp4xx)
			echo msdos;;
		    orion)
			echo msdos;;
		    riscstation)
			echo msdos;;
		    netwinder)
			echo msdos;;
		    ads)
			echo msdos;;
		    versatile)
			echo msdos;;
		    *)
			echo UNKNOWN;;
		esac;;
	    armeb)
		case "$sub" in
		    ixp4xx)
			echo msdos;;
		    *)
			echo UNKNOWN;;
		esac;;
	    amd64)
		case "$sub" in
		    mac)
			echo gpt;;
		    *)
			echo msdos;;
		esac;;
	    hppa)
		echo msdos;;
	    ia64)
		echo gpt;;
	    i386)
		case "$sub" in
		    mac)
			echo gpt;;
		    *)
			echo msdos;;
		esac;;
	    m68k)
		case "$sub" in
		    amiga)
			echo amiga;;
		    atari|q40)
			# unsupported by parted
			echo UNSUPPORTED;;
		    mac)
			echo mac;;
		    *vme*)
			echo msdos;;
		    sun*)
	    		echo sun;;
		    *)
			echo UNKNOWN;;
		esac;;
	    mips)
		case "$sub" in
		    r4k-ip22 | r5k-ip22 | r8k-ip26 | r10k-ip28)
			# Indy
			echo dvh;;
		    r10k-ip27 | r12k-ip27)
			# Origin
			echo dvh;;
		    r5k-ip32 | r10k-ip32 | r12k-ip32)
			# O2
			echo dvh;;
		    sb1-bcm91250a | sb1a-bcm91480b)
			# Broadcom SB1 evaluation boards
			echo msdos;;
		    qemu-mips32)
			echo msdos;;
		    *)
			echo UNKNOWN;;
		esac;;
	    mipsel)
		case "$sub" in
		    # DECstation
		    r3k-kn02)
			echo msdos;;
		    r4k-kn04)
			echo msdos;;
		    sb1-bcm91250a | sb1a-bcm91480b)
			# Broadcom SB1 evaluation boards
			echo msdos;;
		    cobalt)
			echo msdos;;
		    bcm947xx)
			echo msdos;;
		    qemu-mips32)
			echo msdos;;
		    *)
			echo UNKNOWN;;
		esac;;
	    powerpc)
		case "$sub" in
		    apus)
			echo amiga;;
		    amiga)
			echo amiga;;
		    chrp)
			echo msdos;;
		    chrp_rs6k|chrp_ibm)
			echo msdos;;
		    chrp_pegasos)
			echo amiga;;
		    prep)
			echo msdos;;
		    powermac_newworld)
			echo mac;;
		    powermac_oldworld)
			echo mac;;
		    ps3)
			echo msdos;;
		    cell)
			echo msdos;;
		    *)
			echo UNKNOWN;;
		esac;;
	    s390)
		echo msdos;;
	    sparc)
		echo sun;;
	    *)
		echo UNKNOWN;;
	esac
}

create_new_label() {
	local dev default_type chosen_type types
	dev="$1"
	prompt_for_label="$2"

	[ -d "$dev" ] || return 1
	cd $dev

	open_dialog LABEL_TYPES
	types=$(read_list)
	close_dialog

	db_subst partman-partitioning/choose_label CHOICES "$types"
	PRIORITY=critical

	default_label=$(default_disk_label)

	# Use gpt instead of msdos disklabel for disks larger than 2TB
	if expr "$types" : ".*gpt.*" >/dev/null; then
		if [ "$default_label" = msdos ]; then
			disksize=$(cat size)
			if $(longint_le $(human2longint 2TB) $disksize); then
				default_label=gpt
			fi
		fi
	fi

	if [ "$prompt_for_label" = no ] && \
	   expr "$types" : ".*${default_label}.*" >/dev/null; then
		chosen_type="$default_label"
	else
		if expr "$types" : ".*${default_label}.*" >/dev/null; then
			db_set partman-partitioning/choose_label "$default_label"
			PRIORITY=low
		fi
		db_input $PRIORITY partman-partitioning/choose_label || true
		db_go || exit 1
		db_get partman-partitioning/choose_label

		chosen_type="$RET"
	fi

	if [ "$chosen_type" = sun ]; then
		db_input critical partman-partitioning/confirm_write_new_label
		db_go || exit 0
		db_get partman-partitioning/confirm_write_new_label
		if [ "$RET" = false ]; then
			db_reset partman-partitioning/confirm_write_new_label
			return 1
		fi
		db_reset partman-partitioning/confirm_write_new_label
	fi

	open_dialog NEW_LABEL "$chosen_type"
	close_dialog

	if [ "$chosen_type" = sun ]; then
		# write the partition table to the disk
		disable_swap
		open_dialog COMMIT
		close_dialog
		sync
		# reread it from there
		open_dialog UNDO
		close_dialog
		enable_swap
	fi

	# Different types partition tables support different visuals.
	# Some have partition names others don't have, some have extended
	# and logical partitions, others don't. Hence we have to regenerate
	# the list of the visuals.
	rm -f visuals
}
