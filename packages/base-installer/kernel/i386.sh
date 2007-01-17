arch_get_kernel_flavour () {
	VENDOR=`grep '^vendor_id' "$CPUINFO" | cut -d: -f2 | head -n1`
	FAMILY=`grep '^cpu family' "$CPUINFO" | cut -d: -f2 | head -n1`
	case "$VENDOR" in
		" AuthenticAMD"*)
			case "$FAMILY" in
				" 6"|" 15")	echo k7 ;;
				" 5")		echo k6 ;;
				*)		echo 386 ;;
			esac
		;;
		" GenuineIntel"|" GenuineTMx86"*)
			case "$FAMILY" in
				" 6"|" 15")	echo 686 ;;
				" 5")		echo 586tsc ;;
				*)		echo 386 ;;
			esac
		;;
		*) echo 386 ;;
	esac
	return 0
}

arch_check_usable_kernel () {
	if expr "$1" : '.*-[34]86.*' >/dev/null; then return 0; fi
	if [ "$2" = 386 ]; then return 1; fi
	if expr "$1" : '.*-586.*' >/dev/null; then return 0; fi
	if [ "$2" = 586tsc ]; then return 1; fi
	if [ "$2" = 686 ]; then
		if expr "$1" : '.*-686.*' >/dev/null; then return 0; fi
		return 1
	fi
	if expr "$1" : '.*-k6.*' >/dev/null; then return 0; fi
	if [ "$2" = k6 ]; then return 1; fi
	if expr "$1" : '.*-k7.*' >/dev/null; then return 0; fi

	# default to usable in case of strangeness
	warning "Unknown kernel usability: $1 / $2"
	return 0
}

arch_get_kernel_etch () {
	if [ "$KERNEL_MAJOR" = 2.4 ]; then
		# Kernel images are identical with Sarge
		return
	else
		imgbase=linux-image
	fi

	if [ "$1" = k7 ]; then
		echo "$imgbase-$KERNEL_MAJOR-k7"
		set k6
	fi
	if [ "$1" = k6 ]; then
		if [ "$KERNEL_MAJOR" = 2.4 ]; then
			echo "$imgbase-$KERNEL_MAJOR-k6"
		fi
		set 586tsc
	fi

	if [ "$1" = 686 ]; then
		echo "$imgbase-$KERNEL_MAJOR-686"
		set 586tsc
	fi
	if [ "$1" = 586tsc ]; then
		if [ "$KERNEL_MAJOR" = 2.4 ]; then
			echo "$imgbase-$KERNEL_MAJOR-586tsc"
		fi
		set 386
	fi
	echo "$imgbase-$KERNEL_MAJOR-486"
}

arch_get_kernel_sarge () {
	imgbase=kernel-image

	if [ "$1" = k7 ]; then
		echo "$imgbase-$KERNEL_MAJOR-k7"
		set k6
	fi
	if [ "$1" = k6 ]; then
		if [ "$KERNEL_MAJOR" = 2.4 ]; then
			echo "$imgbase-$KERNEL_MAJOR-k6"
		fi
		set 586tsc
	fi

	if [ "$1" = 686 ]; then
		echo "$imgbase-$KERNEL_MAJOR-686"
		set 586tsc
	fi
	if [ "$1" = 586tsc ]; then
		if [ "$KERNEL_MAJOR" = 2.4 ]; then
			echo "$imgbase-$KERNEL_MAJOR-586tsc"
		fi
		set 386
	fi
	echo "$imgbase-$KERNEL_MAJOR-386"
}

arch_get_kernel () {
	arch_get_kernel_etch "$1"
	arch_get_kernel_sarge "$1"
}
