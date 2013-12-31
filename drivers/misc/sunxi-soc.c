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

/*
 * IO Addresses
 */

void __iomem *sunxi_phy_to_virt(u32 phy_base, u32 offset)
{
	static void __iomem *virt_base;

	if (unlikely(!virt_base)) {
		virt_base = ioremap(SUNXI_IO_BASE, SUNXI_IO_SIZE);
		BUG_ON(!virt_base);

		pr_info("iomap: 0x%08x - 0x%08x @ 0x%p\n",
			SUNXI_IO_BASE,
			SUNXI_IO_BASE + SUNXI_IO_SIZE - 1,
			virt_base);
	}

	BUG_ON(phy_base < SUNXI_IO_BASE || phy_base >= SUNXI_IO_BASE + SUNXI_IO_SIZE);
	return ((char*)virt_base) + (phy_base - SUNXI_IO_BASE + offset);
}
EXPORT_SYMBOL(sunxi_phy_to_virt);

/*
 * soc-detect
 */
static u32 chip_id, chip_ver;

#define SC_CHIP_ID_EN_MASK	0x1
#define SC_CHIP_ID_EN_OFF	15
#define SC_CHIP_ID_EN		(SC_CHIP_ID_EN_MASK<<SC_CHIP_ID_EN_OFF)

#define SC_CHIP_ID_MASK		0xffff
#define SC_CHIP_ID_OFF		16
#define SC_CHIP_ID		(SC_CHIP_ID_MASK<<SC_CHIP_ID_OFF)

void sunxi_setup_soc_detect(void)
{
	chip_id = sunxi_sc_chip_id();
	if (chip_id != SUNXI_UNKNOWN_MACH) {
		pr_info("Allwinner AW%u/%s detected\n", chip_id,
			sunxi_chip_id_name());
	}
}
EXPORT_SYMBOL(sunxi_setup_soc_detect);

u32 sunxi_sc_chip_id(void)
{
	void __iomem *sc_base = sunxi_phy_to_virt(SUNXI_SC_IO_BASE, 0x0);
	u32 ret = SUNXI_UNKNOWN_MACH;
	u32 val, reg_val;

	/* enable chip_id reading */
	reg_val = readl(sc_base);
	writel(reg_val | SC_CHIP_ID_EN, sc_base);

	reg_val = readl(sc_base);
	val = ((reg_val&SC_CHIP_ID)>>SC_CHIP_ID_OFF) & SC_CHIP_ID_MASK;

	switch (val) {
	case 0x1623:
		ret = SUNXI_MACH_SUN4I;
		break;
	case 0x1625:
		ret = SUNXI_MACH_SUN5I;
		break;
	case 0x1633:
		ret = SUNXI_MACH_SUN6I;
		break;
	case 0x1651:
		ret = SUNXI_MACH_SUN7I;
		break;
	default:
		pr_err("SC: failed to identify chip-id 0x%04x (*%p == 0x%08x)\n",
		       val, sc_base, reg_val);
	}

	return ret;
}
EXPORT_SYMBOL(sunxi_sc_chip_id);

u32 sunxi_chip_id(void)
{
	return chip_id;
}
EXPORT_SYMBOL(sunxi_chip_id);

static inline void reg_dump(const char *name, u32 base, unsigned len)
{
	unsigned i,j;
	void __iomem *reg = sunxi_phy_to_virt(base, 0x0);

	for (i=0; i<len; ) {
		pr_info("soc-detect: %s (0x%p):", name, reg);

		for (j=0; i<len && j<4; i++, j++) {
			u32 val = readl(reg++);
			printk(" %08x", val);
		}

		printk("\n");
	}
}

enum sunxi_chip_ver sunxi_chip_ver(void)
{
	if (likely(chip_ver))
		return chip_ver;

	if (sunxi_is_sun4i()) {
		u32 val = readl(sunxi_phy_to_virt(SUNXI_TIMERC_IO_BASE, 0x13c));
		val = (val >> 6) & 0x3;

		if (val == 0)
			chip_ver = SUNXI_VER_A10A;
		else if (val == 3)
			chip_ver = SUNXI_VER_A10B;
		else
			chip_ver = SUNXI_VER_A10C;
	} else if (sunxi_is_sun5i()) {
		u32 val = readl(sunxi_phy_to_virt(SUNXI_SID_IO_BASE, 0x08));
		val = (val >> 12) & 0xf;
		switch (val) {
		case 0: chip_ver = SUNXI_VER_A12; break;
		case 3: chip_ver = SUNXI_VER_A13; break;
		case 7: chip_ver = SUNXI_VER_A10S; break;
		default:
			goto unknown_chip;
		}

		val = readl(sunxi_phy_to_virt(SUNXI_SID_IO_BASE, 0x00));
		val = (val >> 8) & 0xffffff;

		if (val == 0 || val == 0x162541)
			chip_ver += SUNXI_REV_A;
		else if (val == 0x162542)
			chip_ver += SUNXI_REV_B;
		else {
			const char *name;
			if (chip_ver == SUNXI_VER_A13)
				name = "A13";
			else if (chip_ver == SUNXI_VER_A12)
				name = "A12";
			else
				name = "A10S";

			pr_err("unrecongnized %s revision (%x)\n",
			       name, val);

			reg_dump("SID", SUNXI_SID_IO_BASE, 4);
		}
	} else if (sunxi_is_sun6i())
		chip_ver = SUNXI_VER_A31;
	else if (sunxi_is_sun7i())
		chip_ver = SUNXI_VER_A20;

	goto done;

unknown_chip:
	pr_err("unrecognized IC (chip-id=%u)\n", sunxi_chip_id());
	chip_ver = SUNXI_VER_UNKNOWN;

	if (sunxi_is_sun5i())
		reg_dump("SSE", SUNXI_SSE_IO_BASE, 1);
	reg_dump("SID", SUNXI_SID_IO_BASE, 4);
done:
	return chip_ver;
}
EXPORT_SYMBOL(sunxi_chip_ver);

const char *sunxi_chip_id_name(void)
{
	switch (chip_id) {
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
