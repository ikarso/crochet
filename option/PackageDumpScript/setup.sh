# Build application to extract pre/post/install script from pkg
#
# Usage:
#  option PackageDumpScript

package_dump_script ( ) {
	# Stage 0 - BUILD
        echo "Build package dump script"
	cc -Wall -I/usr/local/include/ -L/usr/local/lib/ -lpkg \
		${TOPDIR}/option/PackageDumpScript/packagedumpscript.c \
		-o ${TOPDIR}/option/PackageDumpScript/PackageDumpScript

	# Stage 1 - Extract scripts
	# Create scripts dir
	mkdir ${BOARD_FREEBSD_MOUNTPOINT}/root/scripts

	# Find installed packages
	FILENAMES=`find ${BOARD_FREEBSD_MOUNTPOINT}/var/cache/pkg/ -type f`
	for FNAME in ${FILENAMES}
	do
		# Extract pre/post/install script from package.
		${TOPDIR}/option/PackageDumpScript/PackageDumpScript \
			${FNAME} ${BOARD_FREEBSD_MOUNTPOINT}/root/scripts

	done

	# Stage 2 - Populate firstboot
	# find scripts extracted and populate firstboot 
	# Create /usr/local/etc/rc.d
	mkdir -p ${BOARD_FREEBSD_MOUNTPOINT}/usr/local/etc/rc.d

	# Copy template to /usr/local/etc/rc.d
	cp ${TOPDIR}/option/PackageDumpScript/firstboot.template \
		${BOARD_FREEBSD_MOUNTPOINT}/usr/local/etc/rc.d/firstboot

	SCRIPTFILENAMES=`find ${BOARD_FREEBSD_MOUNTPOINT}/root/scripts/ -type f`
	for FNAME in ${SCRIPTFILENAMES}
	do
		# Add script to firstboot
		NAME=`basename ${FNAME}`
		echo "sh -x /root/scripts/${NAME}" \
		    >> ${BOARD_FREEBSD_MOUNTPOINT}/usr/local/etc/rc.d/firstboot
	done

	# Somehow the firstboot script cant find indexinfo in /usr/local/bin
	sed -i '' -e 's/indexinfo/\/usr\/local\/bin\/indexinfo/g'\
		${BOARD_FREEBSD_MOUNTPOINT}/root/scripts/*.sh

	# Remove /root/scripts
	echo "rm -rf /root/scripts" \
		>> ${BOARD_FREEBSD_MOUNTPOINT}/usr/local/etc/rc.d/firstboot

	# set execute bit
	chmod +x ${BOARD_FREEBSD_MOUNTPOINT}/usr/local/etc/rc.d/firstboot
}

package_dump_init () {
	PACKAGE_NO_INSTALL_SCRIPTS=--no-install-scripts
}

# Ensure to extract scripts after all packages are installed
PRIORITY=40 strategy_add $PHASE_FREEBSD_OPTION_INSTALL package_dump_init
PRIORITY=160 strategy_add $PHASE_FREEBSD_OPTION_INSTALL package_dump_script

