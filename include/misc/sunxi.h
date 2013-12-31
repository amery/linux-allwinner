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
#define SUNXI_SSE_IO_BASE	(SUNXI_IO_BASE + 0x15000)
#define SUNXI_TIMERC_IO_BASE	(SUNXI_IO_BASE + 0x20c00)
#define SUNXI_SID_IO_BASE	(SUNXI_IO_BASE + 0x23800)

void __iomem *sunxi_phy_to_virt(u32 phy_base, u32 offset) __pure;

/* soc-detect */
enum sunxi_chip_id {
	SUNXI_UNKNOWN_MACH = 0xffffffff,

	SUNXI_MACH_SUN4I = 1623,
	SUNXI_MACH_SUN5I = 1625,
	SUNXI_MACH_SUN6I = 1633,
	SUNXI_MACH_SUN7I = 1651,
};

enum {
	SUNXI_BIT_SUN4I = BIT(30),
	SUNXI_BIT_SUN5I = BIT(29),
	SUNXI_BIT_SUN6I = BIT(28),
	SUNXI_BIT_SUN7I = BIT(27),

	/* SUNXI_BIT_UNKNOWN can't OR anything known */
	SUNXI_BIT_UNKNOWN = BIT(20),

	/* sun4i */
	SUNXI_SOC_A10  = SUNXI_BIT_SUN4I | BIT(4),

	/* sun5i */
	SUNXI_SOC_A13  = SUNXI_BIT_SUN5I | BIT(4),
	SUNXI_SOC_A12  = SUNXI_BIT_SUN5I | BIT(5),
	SUNXI_SOC_A10S = SUNXI_BIT_SUN5I | BIT(6),

	/* sun6i */
	SUNXI_SOC_A31 = SUNXI_BIT_SUN6I | BIT(4),

	/* sun7i */
	SUNXI_SOC_A20 = SUNXI_BIT_SUN7I | BIT(4),

	SUNXI_REV_UNKNOWN = 0,
	SUNXI_REV_A,
	SUNXI_REV_B,
	SUNXI_REV_C,
};

enum sunxi_chip_ver {
	SUNXI_VER_UNKNOWN = SUNXI_BIT_UNKNOWN,

	/* sun4i */
	SUNXI_VER_A10A = SUNXI_SOC_A10 + SUNXI_REV_A,
	SUNXI_VER_A10B,
	SUNXI_VER_A10C,

	/* sun5i */
	SUNXI_VER_A13 = SUNXI_SOC_A13,
	SUNXI_VER_A13A,
	SUNXI_VER_A13B,
	SUNXI_VER_A12 = SUNXI_SOC_A12,
	SUNXI_VER_A12A,
	SUNXI_VER_A12B,
	SUNXI_VER_A10S = SUNXI_SOC_A10S,
	SUNXI_VER_A10SA,
	SUNXI_VER_A10SB,

	/* sun6i */
	SUNXI_VER_A31 = SUNXI_SOC_A31,

	/* sun7i */
	SUNXI_VER_A20 = SUNXI_SOC_A20,
};

u32 sunxi_sc_chip_id(void) __pure;
u32 sunxi_chip_id(void) __pure;
enum sunxi_chip_ver sunxi_chip_ver(void) __pure;

#define _sunxi_is(M)		((sunxi_chip_ver()&M) == M)

#define sunxi_is_sun4i()	(sunxi_chip_id() == SUNXI_MACH_SUN4I)
#define sunxi_is_sun5i()	(sunxi_chip_id() == SUNXI_MACH_SUN5I)
#define sunxi_is_sun6i()	(sunxi_chip_id() == SUNXI_MACH_SUN6I)
#define sunxi_is_sun7i()	(sunxi_chip_id() == SUNXI_MACH_SUN7I)

#define sunxi_is_a10()		_sunxi_is(SUNXI_SOC_A10)
#define sunxi_is_a13()		_sunxi_is(SUNXI_SOC_A13)
#define sunxi_is_a12()		_sunxi_is(SUNXI_SOC_A12)
#define sunxi_is_a10s()		_sunxi_is(SUNXI_SOC_A10S)
#define sunxi_is_a31()		_sunxi_is(SUNXI_SOC_A31)
#define sunxi_is_a20()		_sunxi_is(SUNXI_SOC_A20)

#define sunxi_chip_rev()	(sunxi_chip_ver() & 0xf)

void sunxi_setup_soc_detect(void);

const char *sunxi_chip_id_name(void) __pure;

#endif /* CONFIG_ARCH_SUNXI */
#endif /* !_SUNXI_H_ */
