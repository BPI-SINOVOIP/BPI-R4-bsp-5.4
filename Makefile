.PHONY: all clean help
.PHONY: u-boot kernel kernel-config
.PHONY: linux pack

include chosen_board.mk

SUDO=sudo
CROSS_COMPILE=$(COMPILE_TOOL)/aarch64-linux-gnu-
AARCH64_CROSS_COMPILE?=aarch64-linux-gnu-
U_CROSS_COMPILE=$(CROSS_COMPILE)
K_CROSS_COMPILE=$(AARCH64_CROSS_COMPILE)

OUTPUT_DIR=$(CURDIR)/output

U_O_PATH=u-boot-mt
K_O_PATH=linux-mt
U_CONFIG_H_SDMMC=$(U_O_PATH)/build_sdmmc/include/config.h
U_CONFIG_H_EMMC=$(U_O_PATH)/build_emmc/include/config.h
K_DOT_CONFIG=$(K_O_PATH)/.config

ROOTFS=$(CURDIR)/rootfs/linux/default_linux_rootfs.tar.gz

Q=
J=$(shell expr `grep ^processor /proc/cpuinfo  | wc -l` \* 2)

all: bsp

## DK, if u-boot and kernel KBUILD_OUT issue fix, u-boot-clean and kernel-clean
## are no more needed
clean: u-boot-clean wifi-mt7996e-clean kernel-clean
	rm -rf chosen_board.mk env.sh out SD

## pack
pack: mt-pack
	$(Q)scripts/mk_pack.sh

# atf 
atf_sdmmc:
	rm -rf atf-mt/build_sdmmc
	$(Q)$(MAKE) --jobserver-fds=3,4 -j  -C atf-mt BUILD_BASE=./build_sdmmc CROSS_COMPILE=$(U_CROSS_COMPILE) PLAT=mt7988 BUILD_STRING="BPI-R4 v2023-10-13-0ea67d76-1 (mt7988-sdmmc-ddr4)" MKIMAGE=${PWD}/atf-mt/tools/mkimage BOOT_DEVICE=sdmmc USE_MKIMAGE=1  BOARD_BGA=1 HAVE_DRAM_OBJ_FILE=yes  DRAM_USE_COMB=1  all

atf_emmc:
	rm -rf atf-mt/build_emmc
	$(Q)$(MAKE) --jobserver-fds=3,4 -j  -C atf-mt BUILD_BASE=./build_emmc CROSS_COMPILE=$(U_CROSS_COMPILE) PLAT=mt7988 BUILD_STRING="BPI-R4 v2023-10-13-0ea67d76-1 (mt7988-emmc-ddr4)" MKIMAGE=${PWD}/atf-mt/tools/mkimage BOOT_DEVICE=emmc USE_MKIMAGE=1  BOARD_BGA=1 HAVE_DRAM_OBJ_FILE=yes  DRAM_USE_COMB=1  all

# u-boot
$(U_CONFIG_H_SDMMC): u-boot-mt
	rm -rf u-boot-mt/build_sdmmc
	$(Q)$(MAKE) -C u-boot-mt O=build_sdmmc $(UBOOT_SD_CONFIG) CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

$(U_CONFIG_H_EMMC): u-boot-mt
	rm -rf u-boot-mt/build_emmc
	$(Q)$(MAKE) -C u-boot-mt O=build_emmc $(UBOOT_EMMC_CONFIG) CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot_sdmmc: $(U_CONFIG_H_SDMMC)	
	$(Q)$(MAKE) -C u-boot-mt O=build_sdmmc CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J LOCALVERSION="-BPI-R4-r24894-4d30d371e7" PKG_CONFIG_EXTRAARGS="--static" V='' u-boot.bin
	./u-boot-mt/fiptool create --soc-fw atf-mt/build_sdmmc/mt7988/release/bl31.bin --nt-fw u-boot-mt/build_sdmmc/u-boot.bin u-boot-mt/build_sdmmc/u-boot_sdmmc.fip

u-boot_emmc: $(U_CONFIG_H_EMMC)
	$(Q)$(MAKE) -C u-boot-mt O=build_emmc CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J LOCALVERSION="-BPI-R4-r24894-4d30d371e7" PKG_CONFIG_EXTRAARGS="--static" V='' u-boot.bin
	./u-boot-mt/fiptool create --soc-fw atf-mt/build_emmc/mt7988/release/bl31.bin --nt-fw u-boot-mt/build_emmc/u-boot.bin u-boot-mt/build_emmc/u-boot_emmc.fip

u-boot: atf_sdmmc atf_emmc u-boot_sdmmc u-boot_emmc

u-boot-clean:
	rm -rf atf-mt/build_sdmmc
	rm -rf atf-mt/build_emmc
	rm -rf u-boot-mt/build_sdmmc
	rm -rf u-boot-mt/build_emmc

## linux
$(K_DOT_CONFIG): linux-mt
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 $(KERNEL_CONFIG)

kernel: $(K_DOT_CONFIG)
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output UIMAGE_LOADADDR=0x40008000 Image dtbs
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules_install

kernel-clean:
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J distclean
	rm -rf linux-mt/output/

kernel-config: $(K_DOT_CONFIG)
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J menuconfig
	cp linux-mt/.config linux-mt/arch/arm64/configs/$(KERNEL_CONFIG)

wifi-mt7996e: $(K_DOT_CONFIG)
	$(Q)$(MAKE) --jobserver-fds=3,4 -j -C wifi-mt7996e/backports-6.5 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=${PWD}/linux-mt/output EXTRA_CFLAGS="-I${PWD}/wifi-mt7996e/backports-6.5/include" KBUILD_HAVE_NLS=no KBUILD_BUILD_USER="" KBUILD_BUILD_HOST="" KBUILD_BUILD_TIMESTAMP="Sat Jan 27 16:05:14 2024" KBUILD_BUILD_VERSION="0" CONFIG_SHELL="bash" V=''  cmd_syscalls= KLIB_BUILD="${PWD}/linux-mt" KERNELRELEASE=5.4.260 MODPROBE=true  KERNEL_SUBLEVEL=1 KBUILD_LDFLAGS_MODULE_PREREQ= modules
	$(Q)$(MAKE) --jobserver-fds=3,4 -j -C wifi-mt7996e/backports-6.5 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=${PWD}/linux-mt/output EXTRA_CFLAGS="-I${PWD}/wifi-mt7996e/backports-6.5/include" KBUILD_HAVE_NLS=no KBUILD_BUILD_USER="" KBUILD_BUILD_HOST="" KBUILD_BUILD_TIMESTAMP="Sat Jan 27 16:05:14 2024" KBUILD_BUILD_VERSION="0" CONFIG_SHELL="bash" V=''  cmd_syscalls= KLIB_BUILD="${PWD}/linux-mt" KERNELRELEASE=5.4.260 MODPROBE=true  KERNEL_SUBLEVEL=1 KBUILD_LDFLAGS_MODULE_PREREQ= modules_install
	$(Q)$(MAKE) --jobserver-fds=3,4 -j -C "${PWD}/linux-mt" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=${PWD}/linux-mt/output KBUILD_HAVE_NLS=no KBUILD_BUILD_USER="" KBUILD_BUILD_HOST="" KBUILD_BUILD_TIMESTAMP="Sat Jan 20 16:05:14 2024" KBUILD_BUILD_VERSION="0" CONFIG_SHELL="bash" V=''  cmd_syscalls= KBUILD_EXTRA_SYMBOLS="${PWD}/linux-mt/Module.symvers ${PWD}/wifi-mt7996e/backports-6.5/Module.symvers ${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff/Module.symvers" KERNELRELEASE=5.4.260 CONFIG_MAC80211_DEBUGFS=y CONFIG_NL80211_TESTMODE=y CONFIG_MT76_CONNAC_LIB=m CONFIG_MT7996E=m M="${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff" NOSTDINC_FLAGS="-nostdinc -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/7/include -I${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff -I${PWD}/wifi-mt7996e/backports-6.5/backport-include/uapi -I${PWD}/wifi-mt7996e/backports-6.5/backport-include -I${PWD}/wifi-mt7996e/backports-6.5/include/uapi -I${PWD}/wifi-mt7996e/backports-6.5/include -include backport/autoconf.h -include backport/backport.h -DCONFIG_MAC80211_MESH -DCONFIG_MAC80211_DEBUGFS -DCONFIG_NL80211_TESTMODE" modules
	$(Q)$(MAKE) --jobserver-fds=3,4 -j -C "${PWD}/linux-mt" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=${PWD}/linux-mt/output KBUILD_HAVE_NLS=no KBUILD_BUILD_USER="" KBUILD_BUILD_HOST="" KBUILD_BUILD_TIMESTAMP="Sat Jan 20 16:05:14 2024" KBUILD_BUILD_VERSION="0" CONFIG_SHELL="bash" V=''  cmd_syscalls= KBUILD_EXTRA_SYMBOLS="${PWD}/linux-mt/Module.symvers ${PWD}/wifi-mt7996e/backports-6.5/Module.symvers ${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff/Module.symvers" KERNELRELEASE=5.4.260 CONFIG_MAC80211_DEBUGFS=y CONFIG_NL80211_TESTMODE=y CONFIG_MT76_CONNAC_LIB=m CONFIG_MT7996E=m M="${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff" NOSTDINC_FLAGS="-nostdinc -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/7/include -I${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff -I${PWD}/wifi-mt7996e/backports-6.5/backport-include/uapi -I${PWD}/wifi-mt7996e/backports-6.5/backport-include -I${PWD}/wifi-mt7996e/backports-6.5/include/uapi -I${PWD}/wifi-mt7996e/backports-6.5/include -include backport/autoconf.h -include backport/backport.h -DCONFIG_MAC80211_MESH -DCONFIG_MAC80211_DEBUGFS -DCONFIG_NL80211_TESTMODE" modules_install

wifi-mt7996e-clean:
	$(Q)$(MAKE) --jobserver-fds=3,4 -j -C wifi-mt7996e/backports-6.5 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=${PWD}/linux-mt/output EXTRA_CFLAGS="-I${PWD}/wifi-mt7996e/backports-6.5/include" KBUILD_HAVE_NLS=no KBUILD_BUILD_USER="" KBUILD_BUILD_HOST="" KBUILD_BUILD_TIMESTAMP="Sat Jan 27 16:05:14 2024" KBUILD_BUILD_VERSION="0" CONFIG_SHELL="bash" V=''  cmd_syscalls= KLIB_BUILD="${PWD}/linux-mt" KERNELRELEASE=5.4.260 MODPROBE=true  KERNEL_SUBLEVEL=1 KBUILD_LDFLAGS_MODULE_PREREQ= clean
	$(Q)$(MAKE) --jobserver-fds=3,4 -j -C "${PWD}/linux-mt" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=${PWD}/linux-mt/output KBUILD_HAVE_NLS=no KBUILD_BUILD_USER="" KBUILD_BUILD_HOST="" KBUILD_BUILD_TIMESTAMP="Sat Jan 20 16:05:14 2024" KBUILD_BUILD_VERSION="0" CONFIG_SHELL="bash" V=''  cmd_syscalls= KBUILD_EXTRA_SYMBOLS="${PWD}/linux-mt/Module.symvers ${PWD}/wifi-mt7996e/backports-6.5/Module.symvers ${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff/Module.symvers" KERNELRELEASE=5.4.260 CONFIG_MAC80211_DEBUGFS=y CONFIG_NL80211_TESTMODE=y CONFIG_MT76_CONNAC_LIB=m CONFIG_MT7996E=m M="${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff" NOSTDINC_FLAGS="-nostdinc -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/7/include -I${PWD}/wifi-mt7996e/mt76-2023-12-18-bebd9cff -I${PWD}/wifi-mt7996e/backports-6.5/backport-include/uapi -I${PWD}/wifi-mt7996e/backports-6.5/backport-include -I${PWD}/wifi-mt7996e/backports-6.5/include/uapi -I${PWD}/wifi-mt7996e/backports-6.5/include -include backport/autoconf.h -include backport/backport.h -DCONFIG_MAC80211_MESH -DCONFIG_MAC80211_DEBUGFS -DCONFIG_NL80211_TESTMODE" clean

## bsp
bsp: u-boot kernel wifi-mt7996e

help:
	@echo ""
	@echo "Usage:"
	@echo "  make bsp             - Default 'make'"
	@echo "  make pack            - pack the images and rootfs to a PhenixCard download image."
	@echo "  make clean"
	@echo ""
	@echo "Optional targets:"
	@echo "  make kernel          - Builds linux kernel"
	@echo "  make kernel-config   - Menuconfig"
	@echo "  make u-boot          - Builds u-boot"
	@echo ""

