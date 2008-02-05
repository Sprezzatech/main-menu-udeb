arch_get_kernel_flavour () {
	case "$SUBARCH" in
	    iop32x|iop33x|ixp4xx|orion)
		echo "$SUBARCH"
		return 0 ;;
	    ads)
		# NOTE: this kernel is not in Debian (sarge), but makes it
		# easier to offer unofficial support from a private apt-archive
		echo "ads"
		return 0 ;;
	    *)
		warning "Unknown $ARCH subarchitecture '$SUBARCH'."
		return 1 ;;
	esac
}

arch_check_usable_kernel () {
	# Subarchitecture must match exactly
	if echo "$1" | grep -Eq -- "-$2(-.*)?$"; then return 0; fi
	return 1
}

arch_get_kernel () {
	case "$KERNEL_MAJOR" in
	    2.6)
		case "$1" in
		    bast)
			echo "linux-image-$KERNEL_MAJOR-s3c2410"
			;;
		    *)
			echo "linux-image-$KERNEL_MAJOR-$1"
			;;
		esac
		;;
	    *)	warning "Unsupported kernel major '$KERNEL_MAJOR'."
		;;
	esac
}
