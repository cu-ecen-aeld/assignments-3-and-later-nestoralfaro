#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

WRITER_DIR=$(realpath $(dirname $0))

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
	# clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
	cd linux-stable
	echo "Checking out version ${KERNEL_VERSION}"
	git checkout ${KERNEL_VERSION}

	# TODO: Add your kernel build steps here
	# more about setting `ARCH` and `CROSS_COMPILE`: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/Makefile?h=linux-5.1.y#n334
	# the commands below are taken from "Building the Linux Kernel" lecture video
	echo "Building kernel"

	# "deep clean" kernel build tree
	echo "mrproper..."
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

	# configure for "virt" arm dev board to simulate in QEMU
	# got stick here due to outdated dependencies.
	# resolved with: https://github.com/cu-ecen-aeld/aesd-autotest-docker/blob/master/docker/Dockerfile#L30
	echo "defconfig..."
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

	# build kernel image for booting with QEMU
	echo "all..."
	make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

	# build kernel modules
	echo "modules..."
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

	# build devicetree
	echo "dtbs..."
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
	sudo rm -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories (rootfs) - taken from "Linux Root Filesystems" lecture video
echo "Creating the necessary base directories (rootfs)"
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
	git clone git://busybox.net/busybox.git
	cd busybox
	git checkout ${BUSYBOX_VERSION}

	# TODO:  Configure busybox - taken from "Linux Root Filesystem" lecture video
	make distclean
	make defconfig
else
	cd busybox
fi

# TODO: Make and install busybox - install also creates the symlinks
echo "Installing busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

# TODO: Add library dependencies to rootfs
CC_SYSROOT=$(aarch64-none-linux-gnu-gcc -print-sysroot)
cd ${OUTDIR}/rootfs

echo "Adding program interpreter to rootfs/lib"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
cp ${CC_SYSROOT}/lib/ld-linux-aarch64.so.1 lib/

echo "Adding library dependencies to rootfs/lib64"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"
cp ${CC_SYSROOT}/lib64/libm.so.6 lib64/
cp ${CC_SYSROOT}/lib64/libresolv.so.2 lib64/
cp ${CC_SYSROOT}/lib64/libc.so.6 lib64/

# TODO: Make device nodes
echo "Making null device (major 1 minor 3)..."
sudo mknod -m 666 dev/null c 1 3
echo "Making console device (major 5 minor 1)..."
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
echo "Cleaning and building ./writer utility for cross-compilation..."
cd ${WRITER_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying finder related items from previous assignments into ${OUTDIR}/rootfs/home/ directory"
cp -RL . ${OUTDIR}/rootfs/home/

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
echo "Creating RAMDISK..."
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

echo "manual-linux finished with exit status: $?"
