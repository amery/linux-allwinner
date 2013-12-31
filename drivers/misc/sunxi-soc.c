/*
 * SoC detect code for Allwinner chips
 *
 * Copyright (C) 2013 Alejandro Mery
 *
 * Alejandro Mery
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(fmt) "sunxi: " fmt

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/io.h>

#include <misc/sunxi.h>

#define SRAMC_IO_BASE		0x01c00000

#define SC_CHIP_ID_EN_MASK	0x1
#define SC_CHIP_ID_EN_OFF	15
#define SC_CHIP_ID_EN		(SC_CHIP_ID_EN_MASK<<SC_CHIP_ID_EN_OFF)

#define SC_CHIP_ID_MASK		0xffff
#define SC_CHIP_ID_OFF		16
#define SC_CHIP_ID		(SC_CHIP_ID_MASK<<SC_CHIP_ID_OFF)

static void __iomem *sramc_base, *sc_base;

void sunxi_setup_soc_detect(void)
{
	u32 chip_id;

	if (!sramc_base) {
		sramc_base = ioremap(SRAMC_IO_BASE, SZ_4K);
		if (!sramc_base) {
			pr_err("Failed to iomap the SRAMC register\n");
			return;
		}
		sc_base = (void*)((char*)sramc_base + 0x24);
	}

	chip_id = sunxi_chip_id();
	if (chip_id != SUNXI_UNKNOWN_MACH)
		pr_info("Allwinner AW%u/%s detected\n", chip_id,
			sunxi_chip_id_name());
}
EXPORT_SYMBOL(sunxi_setup_soc_detect);

u32 sunxi_sc_chip_id(void)
{
	u32 chip_id, reg_val;

	if (unlikely(!sc_base))
		return SUNXI_UNKNOWN_MACH;

	/* enable chip_id reading */
	reg_val = readl(sc_base);
	writel(reg_val | SC_CHIP_ID_EN, sc_base);

	reg_val = readl(sc_base);
	chip_id = ((reg_val&SC_CHIP_ID)>>SC_CHIP_ID_OFF) & SC_CHIP_ID_MASK;

	switch (chip_id) {
	case 0x1623:
		return SUNXI_MACH_SUN4I;
	case 0x1625:
		return SUNXI_MACH_SUN5I;
	case 0x1633:
		return SUNXI_MACH_SUN6I;
	case 0x1651:
		return SUNXI_MACH_SUN7I;
	default:
		pr_err("SC: failed to identify chip-id 0x%04x (*%p == 0x%08x)\n",
		       chip_id, sc_base, reg_val);
		return SUNXI_UNKNOWN_MACH;
	}
}
EXPORT_SYMBOL(sunxi_sc_chip_id);

u32 sunxi_chip_id(void)
{
	static u32 chip_id;

	if (unlikely(chip_id == 0))
		chip_id = sunxi_sc_chip_id();

	return chip_id;
}
EXPORT_SYMBOL(sunxi_chip_id);

const char *sunxi_chip_id_name(void)
{
	switch (sunxi_chip_id()) {
	case SUNXI_MACH_SUN4I:
		return "sun4i";
	case SUNXI_MACH_SUN5I:
		return "sun5i";
	case SUNXI_MACH_SUN6I:
		return "sun6i";
	case SUNXI_MACH_SUN7I:
		return "sun7i";
	default:
		return "sunNi";
	}
}
EXPORT_SYMBOL(sunxi_chip_id_name);
