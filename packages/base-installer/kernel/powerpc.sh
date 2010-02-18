arch_get_kernel_flavour () {
	CPU=`grep '^cpu[[:space:]]*:' "$CPUINFO" | head -n1 | cut -d: -f2 | sed 's/^ *//; s/[, ].*//' | tr A-Z a-z`
	case "$CPU" in
	    power3|power4|power4+|ppc970*|power5|power5+|power6|power6x)
		family=powerpc64 ;;
	    cell|i-star|s-star|pa6t|rs64-*)
		family=powerpc64 ;;
    	    *)
		family=powerpc ;;
	esac
	case "$SUBARCH" in
	    powermac*|chrp*|pasemi|ps3)
		echo "$family" ;;
	    prep)
		echo prep ;;
	    amiga)
		echo apus ;;
	    *)
		warning "Unknown $ARCH subarchitecture '$SUBARCH'."
		return 1 ;;
	esac
	return 0
}

arch_check_usable_kernel () {
	# CPU family must match exactly
	if echo "$1" | grep -Eq -- "-$2(-.*)?$"; then return 0; fi
	return 1
}

arch_get_kernel () {
	CPUS="$(grep -ci ^processor "$CPUINFO")" || CPUS=1
	if [ "$CPUS" ] && [ "$CPUS" -gt 1 ] && \
	   [ "$1" != "powerpc64" ] && [ "$1" != "prep" ] ; then
		SMP=-smp
	else
		SMP=
	fi

	case "$KERNEL_MAJOR" in
	    2.6)
		if [ "$SMP" ]; then
			echo "linux-image-$KERNEL_MAJOR-$1$SMP"
		fi
		echo "linux-image-$KERNEL_MAJOR-$1"
		;;
	    *)
		warning "Unsupported kernel major '$KERNEL_MAJOR'."
		;;
	esac
}
