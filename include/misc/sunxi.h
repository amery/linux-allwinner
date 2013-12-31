/*
 * Copyright (C) 2013 Alejandro Mery
 *
 * Alejandro Mery <amery@geeks.cl>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _SUNXI_H_
#define _SUNXI_H_
#ifdef CONFIG_ARCH_SUNXI

/* I/O Addresses */
#define SUNXI_IO_BASE	0x01c00000
#define SUNXI_IO_SIZE	(0x02000000-SUNXI_IO_BASE)

#define SUNXI_SRAMC_IO_BASE	(SUNXI_IO_BASE + 0x00000)
#define SUNXI_SC_IO_BASE	(SUNXI_IO_BASE + 0x00024)

void __iomem *sunxi_phy_to_virt(u32 phy_base, u32 offset) __pure;

/* soc-detect */
enum sunxi_chip_id {
	SUNXI_UNKNOWN_MACH = 0xffffffff,

	SUNXI_MACH_SUN4I = 1623,
	SUNXI_MACH_SUN5I = 1625,
	SUNXI_MACH_SUN6I = 1633,
	SUNXI_MACH_SUN7I = 1651,
};

u32 sunxi_sc_chip_id(void) __pure;
u32 sunxi_chip_id(void) __pure;

#define sunxi_is_sun4i()	(sunxi_chip_id() == SUNXI_MACH_SUN4I)
#define sunxi_is_sun5i()	(sunxi_chip_id() == SUNXI_MACH_SUN5I)
#define sunxi_is_sun6i()	(sunxi_chip_id() == SUNXI_MACH_SUN6I)
#define sunxi_is_sun7i()	(sunxi_chip_id() == SUNXI_MACH_SUN7I)

void sunxi_setup_soc_detect(void);

const char *sunxi_chip_id_name(void) __pure;

#endif /* CONFIG_ARCH_SUNXI */
#endif /* !_SUNXI_H_ */
