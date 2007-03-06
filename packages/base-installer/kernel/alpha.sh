arch_get_kernel_flavour () {
	echo $MACHINE
	return 0
}

arch_check_usable_kernel () {
	return 0
}

arch_get_kernel () {
	imgbase=linux-image
	version=$KERNEL_MAJOR-alpha
	
	if [ -n "$NUMCPUS" ] && [ "$NUMCPUS" -gt 1 ]; then
		echo "$imgbase-$version-smp"
	fi
	
	echo "$imgbase-$version-generic"
}
