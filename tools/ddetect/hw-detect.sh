#! /bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

NEWLINE="
"

MISSING_MODULES_LIST=""

# This is a gross and stupid hack, but we don't have a better idea right
# now. See Debian bug #136743
if [ -x /sbin/depmod ]; then
	depmod -a > /dev/null 2>&1 || true
fi

log () {
    logger -t hw-detect "$@"
}

is_not_loaded() {
    local module="$1"
    if cut -d" " -f1 /proc/modules | grep -q "^${module}\$" ; then
	false
    else
	true
    fi
}

load_module() {
    local module="$1"
    db_fset hw-detect/module_params seen false
    db_subst hw-detect/module_params MODULE "$module"
    db_input low hw-detect/module_params || [ $? -eq 30 ]
    db_go
    db_get hw-detect/module_params
    if modprobe -v "$module" "$RET" >> /var/log/messages 2>&1 ; then
    	if [ "$RET" != "" ]; then
		register-module "$module" "$RET"
	fi
    else   
	db_fset hw-detect/modprobe_error seen false
	db_subst hw-detect/modprobe_error CMD_LINE_PARAM "modprobe -v $module"
	db_input medium hw-detect/modprobe_error || [ $? -eq 30 ]
	db_go
    fi
}

load_modules()
{
    old=`cat /proc/sys/kernel/printk`
    echo 0 > /proc/sys/kernel/printk
    for module in $* ; do
	load_module $module
    done
    echo $old > /proc/sys/kernel/printk
}

# HACK ALERT! (pere: do not use as an example ;-) )
# join hack for discover 2 (ask Eric Gillespie why this
# is needed ;-) )
dumb_join_discover (){
    IFS_SAVE="$IFS"
    IFS="
"
    for i in $MODEL_INFOS; do
        echo $1:$i;
        shift
    done
    IFS="$IFS_SAVE"
}

# wrapper for discover command that can distinguish Discover 1.x and 2.x
discover_hw () {
    DISCOVER=/sbin/discover
    if [ -f /usr/bin/discover ] ; then
        log "Testing experimental discover2 package."

        DISCOVER=/usr/bin/discover
    fi
    # Ugh, Discover 1.x didn't exit with nonzero status if given an
    # unrecongized option!
    DISCOVER_TEST=$($DISCOVER --version 2> /dev/null)
    if expr "$DISCOVER_TEST" : 'discover 2.*' > /dev/null 2>&1; then
        # Discover 2.x, see <URL:http://hackers.progeny.com/discover/> for doc
        # This worked with jeff's didiscover utility, which progeny removed from
        # the discover subversion repository :-(

        dpath=linux/module/name
        dver=`uname -r|cut -d. -f1,2` # Kernel version (e.g. 2.4)
        dflags="-d all -e ata -e pci -e pcmcia -e \
                scsi bridge broadband fixeddisk humaninput modem \
                network optical removabledisk"

        MODEL_INFOS=$($DISCOVER -t $dflags)
        MODULES=$($DISCOVER --data-path=$dpath --data-version=$dver $dflags)
        dumb_join_discover $MODULES

    else
        # must be Discover 1.x
        $DISCOVER --format="%m:%V %M\n" \
            --disable-all --enable=pci,ide,scsi,pcmcia scsi cdrom ethernet |
	  sed 's/ $//'
    fi
}

# Some pci chipsets are needed or there can be DMA or other problems.
get_ide_chipset_info() {
	for ide_module in $(find /lib/modules/*/kernel/drivers/ide/pci/ -type f 2>/dev/null); do
    	if [ -e $ide_module ]; then
		baseidemod=$(echo $ide_module | sed 's/\.o$//' | sed 's/\.ko$//' | sed 's/.*\///')
		echo "$baseidemod:IDE chipset support"
    	fi
    done
}

# Return list of lines with "Kernel module<tab>Vendor<tab>Model"
get_all_hw_info() {
    discover_hw
    if [ -d /proc/bus/usb ]; then
    	echo "usb-storage:USB storage"
    fi
    get_manual_hw_info
}
   
# Manually load modules to enable things we can't detect.
# XXX: This isn't the best way to do this; we should autodetect.
# The order of these modules are important. [pere 2003-03-16]
get_manual_hw_info() {
    echo "floppy:Linux Floppy"
    # ide-mod and ide-probe-mod are needed for older (2.4.20) kernels
    echo "ide-mod:Linux IDE driver"
    echo "ide-probe-mod:Linux IDE probe driver"
    get_ide_chipset_info
    echo "ide-detect:Linux IDE detection"
    echo "ide-floppy:Linux IDE floppy"
    echo "ide-disk:Linux ATA DISK"
    echo "ide-cd:Linux ATAPI CD-ROM"
    echo "isofs:Linux ISO 9660 filesystem"
}

db_settitle hw-detect/title

# Should be greater than the number of kernel modules we can reasonably
# expect it will ever need to load.
MAX_STEPS=1000
OTHER_STEPS=4
# Use 1/10th of the progress bar for the non-module-load steps.
OTHER_STEPSIZE=$(expr $MAX_STEPS / 10 / $OTHER_STEPS)
db_progress START 0 $MAX_STEPS hw-detect/detect_progress_title

log "Detecting hardware..."
db_progress INFO hw-detect/detect_progress_step
MANUAL_HW_INFO=$(get_manual_hw_info)
ALL_HW_INFO=$(get_all_hw_info)
db_progress STEP $OTHER_STEPSIZE

# Remove modules that are already loaded, and count how many are left.
LOADED_MODULES=$(cat /proc/modules | cut -f 1 -d ' ')
count=0
# Setting IFS to adjust how the for loop splits the values
IFS_SAVE="$IFS"
IFS="$NEWLINE"
for device in $ALL_HW_INFO; do
    	module="`echo $device | cut -d: -f1`"
	loaded=0
	for m in $LOADED_MODULES; do
		if [ "$m" = "$module" ]; then
			loaded=1
			break
		fi
	done
	if [ "$loaded" = 0 ]; then
		count=$(expr $count + 1)
		HW_INFO="$HW_INFO
$device"
	fi
done
IFS="$IFS_SAVE"
db_progress STEP $OTHER_STEPSIZE

# Work out amount to step per module load. expr rounds down, so 
# it may not get quite to 100%, but will at least never exceed it.
MODULE_STEPSIZE=$(expr \( $MAX_STEPS - \( $OTHER_STEPS \* $OTHER_STEPSIZE \) \) / $count)

log "Loading modules..."
IFS_SAVE="$IFS"
IFS="$NEWLINE"
for device in $HW_INFO; do
    module="`echo $device | cut -d: -f1`"
    cardname="`echo $device | cut -d: -f2`"
    # Restore IFS after extracting the fields.
    IFS="$IFS_SAVE"

    if [ -z "$module" ] ; then module="[Unknown]" ; fi
    if [ -z "$cardname" ] ; then cardname="[Unknown]" ; fi

    log "Detected module '$module' for '$cardname'"

    if [ "$module" != "ignore" -a "$module" != "[Unknown]" ]; then
    	db_subst hw-detect/load_progress_step CARDNAME "$cardname"
        db_subst hw-detect/load_progress_step MODULE "$module"
        db_progress INFO hw-detect/load_progress_step
        log "Trying to load module '$module'"

        if find /lib/modules/`uname -r`/ | grep -q /${module}\\. ; then
            if load_modules "$module"; then
                :
            else
                log "Error loading driver '$module' for '$cardname'!"
	    fi
        else
       	    db_subst hw-detect/load_progress_skip_step CARDNAME "$cardname"
            db_subst hw-detect/load_progress_skip_step MODULE "$module"
            db_progress INFO hw-detect/load_progress_skip_step
            log "Could not load driver '$module' for '$cardname'."
	    # Only add the module to the missing list if it was not
	    # manually added to the list of modules to load.
	    if ! echo "$MANUAL_HW_INFO" | grep -q "$module:"; then
		    if [ -n "$MISSING_MODULES_LIST" ]; then
			    MISSING_MODULES_LIST="$MISSING_MODULES_LIST, "
		    fi
		    MISSING_MODULES_LIST="$MISSING_MODULES_LIST$module ($cardname)"
	    fi
        fi
    fi

    db_progress STEP $MODULE_STEPSIZE
    IFS="$NEWLINE"
done
IFS="$IFS_SAVE"

# always load sd_mod and sr_mod if a scsi controller module was loaded.
# sd_mod to find the disks, and sr_mod to find the CD-ROMs
if [ -e /proc/scsi/scsi ] ; then
    if grep -q "Attached devices: none" /proc/scsi/scsi ; then
        :
    else
    	if grep -q "Type:.*Direct-Access" /proc/scsi/scsi ; then
	    if is_not_loaded "sd_mod" ; then
		if ! [ "$(cat /proc/devices | sed 's/[^[:alpha:]]*//' |grep sd|head -n 1)" = "sd" ]; then
                    db_subst hw-detect/load_progress_step CARDNAME "SCSI disk support"
                    db_subst hw-detect/load_progress_step MODULE "sd_mod"
                    db_progress INFO hw-detect/load_progress_step
		    load_modules sd_mod
		    register-module sd_mod
		fi
	    fi
	fi
	db_progress STEP $OTHER_STEPSIZE
    	if grep -q "Type:.*CD-ROM" /proc/scsi/scsi ; then
	    if is_not_loaded "sr_mod" ; then
		if ! [ "$(cat /proc/devices | sed 's/[^[:alpha:]]*//' |grep sr|head -n 1)" = "sr" ]; then
                    db_subst hw-detect/load_progress_step CARDNAME "SCSI CDROM support"
                    db_subst hw-detect/load_progress_step MODULE "sr_mod"
                    db_progress INFO hw-detect/load_progress_step
		    load_modules sr_mod
		    register-module sr_mod
		fi
	    fi
	fi
	db_progress STEP $OTHER_STEPSIZE
    fi
fi

# if there is an ide bus, then register the ide CD modules so they'll be
# available on the target system for base-config
if [ -e /proc/ide/ -a "`find /proc/ide/* -type d 2>/dev/null`" != "" ]; then
	register-module ide-cd
	register-module ide-detect
fi

# get pcmcia running if possible
if [ -x /etc/init.d/pcmcia ]; then
	/etc/init.d/pcmcia start </dev/null 2>&1 | logger -t hw-detect
fi

if [ -d /proc/bus/pccard ]; then
	# Ask for pcmcia-cs to be installed into target
	apt-install pcmcia-cs || true
fi

# Ask for discover to be installed into /target/, to make sure the
# required drivers are loaded.
apt-install discover || true

# Install hotplug as well (for USB, IEEE1394, CardBus, and some SCSI)
apt-install hotplug || true

db_progress SET $MAX_STEPS
db_progress STOP

if [ -n "$MISSING_MODULES_LIST" ]; then
	log "Missing modules '$MISSING_MODULES_LIST"
	# Tell the user to try to load more modules from floppy
	template=hw-detect/missing_modules
	db_fset "$template" seen false
	db_subst "$template" MISSING_MODULES_LIST "$MISSING_MODULES_LIST" || true
	db_input low "$template" || [ $? -eq 30 ]
	db_go || true
fi

exit 0
