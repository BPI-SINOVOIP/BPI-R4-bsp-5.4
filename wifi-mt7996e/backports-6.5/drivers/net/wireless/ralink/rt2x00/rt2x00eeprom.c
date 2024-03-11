// SPDX-License-Identifier: GPL-2.0-or-later
/*	Copyright (C) 2004 - 2009 Ivo van Doorn <IvDoorn@gmail.com>
 *	Copyright (C) 2004 - 2009 Gertjan van Wingerde <gwingerde@gmail.com>
 *	<http://rt2x00.serialmonkey.com>
 */

/*	Module: rt2x00lib
 *	Abstract: rt2x00 eeprom file loading routines.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#if IS_ENABLED(CONFIG_MTD)
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#endif
#include <linux/nvmem-consumer.h>
#include <linux/of.h>

#include "rt2x00.h"
#include "rt2x00soc.h"

static void rt2800lib_eeprom_swap(struct rt2x00_dev *rt2x00dev)
{
	struct device_node *np = rt2x00dev->dev->of_node;
	size_t len = rt2x00dev->ops->eeprom_size;
	int i;

	if (!of_find_property(np, "ralink,eeprom-swap", NULL))
		return;

	for (i = 0; i < len / sizeof(u16); i++)
		rt2x00dev->eeprom[i] = swab16(rt2x00dev->eeprom[i]);
}

#if IS_ENABLED(CONFIG_MTD)
static int rt2800lib_read_eeprom_mtd(struct rt2x00_dev *rt2x00dev)
{
	int ret = -EINVAL;
#ifdef CONFIG_OF
	struct device_node *np = rt2x00dev->dev->of_node, *mtd_np = NULL;
	int size, offset = 0;
	struct mtd_info *mtd;
	const char *part;
	const __be32 *list;
	phandle phandle;
	size_t retlen;

	list = of_get_property(np, "ralink,mtd-eeprom", &size);
	if (!list)
		return -ENOENT;

	phandle = be32_to_cpup(list++);
	if (phandle)
		mtd_np = of_find_node_by_phandle(phandle);
	if (!mtd_np) {
		dev_err(rt2x00dev->dev, "failed to load mtd phandle\n");
		return -EINVAL;
	}

	part = of_get_property(mtd_np, "label", NULL);
	if (!part)
		part = mtd_np->name;

	mtd = get_mtd_device_nm(part);
	if (IS_ERR(mtd)) {
		dev_err(rt2x00dev->dev, "failed to get mtd device \"%s\"\n", part);
		return PTR_ERR(mtd);
	}

	if (size > sizeof(*list))
		offset = be32_to_cpup(list);

	ret = mtd_read(mtd, offset, rt2x00dev->ops->eeprom_size,
		       &retlen, (u_char *)rt2x00dev->eeprom);
	put_mtd_device(mtd);

	if (retlen != rt2x00dev->ops->eeprom_size || ret) {
		dev_err(rt2x00dev->dev, "failed to load eeprom from device \"%s\"\n", part);
		return ret;
	}

	rt2800lib_eeprom_swap(rt2x00dev);

	dev_info(rt2x00dev->dev, "loaded eeprom from mtd device \"%s\"\n", part);
#endif

	return ret;
}
#endif

static int rt2800lib_read_eeprom_nvmem(struct rt2x00_dev *rt2x00dev)
{
	struct device_node *np = rt2x00dev->dev->of_node;
	unsigned int len = rt2x00dev->ops->eeprom_size;
	struct nvmem_cell *cell;
	const void *data;
	size_t retlen;
	int ret = 0;

	cell = of_nvmem_cell_get(np, "eeprom");
	if (IS_ERR(cell))
		return PTR_ERR(cell);

	data = nvmem_cell_read(cell, &retlen);
	nvmem_cell_put(cell);

	if (IS_ERR(data))
		return PTR_ERR(data);

	if (retlen != len) {
		dev_err(rt2x00dev->dev, "invalid eeprom size, required: 0x%04x\n", len);
		ret = -EINVAL;
		goto exit;
	}

	memcpy(rt2x00dev->eeprom, data, len);

	rt2800lib_eeprom_swap(rt2x00dev);

exit:
	kfree(data);
	return ret;
}

static const char *
rt2x00lib_get_eeprom_file_name(struct rt2x00_dev *rt2x00dev)
{
	struct rt2x00_platform_data *pdata = rt2x00dev->dev->platform_data;
#ifdef CONFIG_OF
	struct device_node *np;
	const char *eep;
#endif

	if (pdata && pdata->eeprom_file_name)
		return pdata->eeprom_file_name;

#ifdef CONFIG_OF
	np = rt2x00dev->dev->of_node;
	if (np && !of_property_read_string(np, "ralink,eeprom", &eep))
		return eep;
#endif

	return NULL;
}

static int rt2x00lib_read_eeprom_file(struct rt2x00_dev *rt2x00dev)
{
	const struct firmware *ee;
	const char *ee_name;
	int retval;

	ee_name = rt2x00lib_get_eeprom_file_name(rt2x00dev);
	if (!ee_name && test_bit(REQUIRE_EEPROM_FILE, &rt2x00dev->cap_flags)) {
		rt2x00_err(rt2x00dev, "Required EEPROM name is missing.");
		return -EINVAL;
	}

	if (!ee_name)
		return 0;

	rt2x00_info(rt2x00dev, "Loading EEPROM data from '%s'.\n", ee_name);

	retval = request_firmware(&ee, ee_name, rt2x00dev->dev);
	if (retval) {
		rt2x00_err(rt2x00dev, "Failed to request EEPROM.\n");
		return retval;
	}

	if (!ee || !ee->size || !ee->data) {
		rt2x00_err(rt2x00dev, "Failed to read EEPROM file.\n");
		retval = -ENOENT;
		goto err_exit;
	}

	if (ee->size != rt2x00dev->ops->eeprom_size) {
		rt2x00_err(rt2x00dev,
			   "EEPROM file size is invalid, it should be %d bytes\n",
			   rt2x00dev->ops->eeprom_size);
		retval = -EINVAL;
		goto err_release_ee;
	}

	memcpy(rt2x00dev->eeprom, ee->data, rt2x00dev->ops->eeprom_size);

err_release_ee:
	release_firmware(ee);
err_exit:
	return retval;
}

int rt2x00lib_read_eeprom(struct rt2x00_dev *rt2x00dev)
{
	int ret;

#if IS_ENABLED(CONFIG_MTD)
	ret = rt2800lib_read_eeprom_mtd(rt2x00dev);
	if (!ret)
		return 0;
#endif

	ret = rt2800lib_read_eeprom_nvmem(rt2x00dev);
	if (!ret)
		return 0;

	return rt2x00lib_read_eeprom_file(rt2x00dev);
}
EXPORT_SYMBOL_GPL(rt2x00lib_read_eeprom);
