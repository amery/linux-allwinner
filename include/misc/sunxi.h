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

enum sunxi_chip_id {
	SUNXI_UNKNOWN_MACH = 0xffffffff,

	SUNXI_MACH_SUN4I = 1623,
	SUNXI_MACH_SUN5I = 1625,
	SUNXI_MACH_SUN6I = 1633,
	SUNXI_MACH_SUN7I = 1651,
};

u32 sunxi_sc_chip_id(void) __pure;
u32 sunxi_chip_id(void) __pure;

void sunxi_setup_soc_detect(void);

const char *sunxi_chip_id_name(void) __pure;

#endif /* CONFIG_ARCH_SUNXI */
#endif /* !_SUNXI_H_ */
