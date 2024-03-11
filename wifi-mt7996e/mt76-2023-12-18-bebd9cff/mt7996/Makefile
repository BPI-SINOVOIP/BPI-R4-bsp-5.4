# SPDX-License-Identifier: ISC
EXTRA_CFLAGS += -DCONFIG_MT76_LEDS
EXTRA_CFLAGS += -DCONFIG_MTK_DEBUG
EXTRA_CFLAGS += -DCONFIG_MTK_VENDOR

obj-$(CONFIG_MT7996E) += mt7996e.o

mt7996e-y := pci.o init.o dma.o eeprom.o main.o mcu.o mac.o \
	     debugfs.o mmio.o vendor.o

mt7996e-$(CONFIG_DEV_COREDUMP) += coredump.o
mt7996e-$(CONFIG_NL80211_TESTMODE) += testmode.o

mt7996e-y += mtk_debugfs.o mtk_mcu.o
