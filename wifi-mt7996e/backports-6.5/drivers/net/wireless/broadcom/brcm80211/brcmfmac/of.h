// SPDX-License-Identifier: ISC
/*
 * Copyright (c) 2014 Broadcom Corporation
 */
#ifdef CONFIG_OF
void brcmf_of_probe(struct device *dev, enum brcmf_bus_type bus_type,
		    struct brcmf_mp_device *settings);
#ifdef CPTCFG_BRCMFMAC_SDIO
struct brcmf_firmware_mapping *
brcmf_of_fwnames(struct device *dev, u32 *map_count);
#endif
#else
static void brcmf_of_probe(struct device *dev, enum brcmf_bus_type bus_type,
			   struct brcmf_mp_device *settings)
{
}
#ifdef CPTCFG_BRCMFMAC_SDIO
static struct brcmf_firmware_mapping *
brcmf_of_fwnames(struct device *dev, u32 *map_count)
{
	return NULL;
}
#endif
#endif /* CONFIG_OF */
