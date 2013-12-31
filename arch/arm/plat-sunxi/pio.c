/*
 * Copyright 2013 Alejandro Mery
 *
 * Alejandro Mery <amery@geeks.cl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define pr_fmt(fmt)	"sunxi:pio: " fmt

#include <linux/bug.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kernel.h>

#include <plat/platform.h>
#include <plat/pio.h>

#if defined(CONFIG_AW_AXP) || defined(CONFIG_AW_AXP_MODULE)
#include "../../drivers/power/axp_power/axp-gpio.h"
#define pio_is_axp(bank, num) ((bank) == 0xffff && (num) < 32)
static u32 power;
#endif

static u32 requested[9]; /* bitmask */
#define pio_is_valid(bank, num)	((bank) < ARRAY_SIZE(requested) && (num) < 32)

int sunxi_pio_request(const char *name, unsigned bank, unsigned num,
		      int mux, int pull, int drv, int val)
{
	int ret;
	struct sunxi_pio_reg *reg = (void*)SW_VA_PORTC_IO_BASE;

	if (pio_is_valid(bank, num)) {
		if (requested[bank] & BIT(num)) {
			pr_err("%s: P%c%u already requested.\n", name, 'A'+bank, num);
			ret = -EBUSY;
		} else {
			pr_debug("%s: P%c%u<%d><%d><%d><%d> requested.\n",
				 name, 'A'+bank, num,
				 mux, pull, drv, val);

			requested[bank] |= BIT(num); /* flag as requested */

			ret = (bank << 5) | num;

			if (mux >= 0)
				sunxi_pio_set_mux(reg, bank, num, mux);
			if (pull >= 0)
				sunxi_pio_set_pull(reg, bank, num, pull);
			if (drv >= 0)
				sunxi_pio_set_drv(reg, bank, num, drv);
			if (val >= 0)
				sunxi_pio_set_val(reg, bank, num, val);
		}
#if defined(CONFIG_AW_AXP) || defined(CONFIG_AW_AXP_MODULE)
	} else if (pio_is_axp(bank, num)) {
		if (power & BIT(num)) {
			pr_err("%s: power%u already requested.\n", name, num);
			ret = -EBUSY;
		} else {
			pr_debug("%s: power%u<%d><-><-><%d> requested.\n",
				 name, num, mux, val);

			power |= BIT(num); /* flag as requested */

			if (mux >= 0) {
				ret = axp_gpio_set_io(num, mux);
				if (ret < 0)
					goto axp_ret_set;
			}

			if (val >= 0) {
				ret = axp_gpio_set_value(num, val);
				if (ret < 0)
					goto axp_ret_set;
			}

			ret = (bank << 5) | num;
axp_ret_set:
			;
		}
#endif
	} else {
		pr_err("%s: invalid PIO (%u,%u)\n", name, bank, num);
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_pio_request);

int sunxi_pio_release(u32 pin)
{
	int ret = 0;
	unsigned bank = pin >> 5;
	unsigned num = pin & 0x1f;

#if defined(CONFIG_AW_AXP) || defined(CONFIG_AW_AXP_MODULE)
	if (pio_is_axp(bank, num)) {
		if (power & BIT(num)) {
			pr_debug("release: power%u\n", num);
			power &= ~BIT(num);
		} else {
			pr_warn("release: power%u\n wasn't requested.", num);
			ret = 1;
		}
	} else
#endif
	if (!pio_is_valid(bank, num)) {
		pr_err("release: invalid PIO (%u,%u)\n", bank, num);
		ret = -EINVAL;
	} else if (requested[bank] & BIT(num)) {
		pr_debug("release: P%c%u\n", 'A'+bank, num);
		requested[bank] &= ~BIT(num);
	} else {
		pr_warn("release: P%c%u\n wasn't requested.", 'A'+bank, num);
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_pio_release);

int sunxi_pio_release_array(u32 *pin)
{
	int ret;
	if (pin) {
		for (ret = 0; *pin != -1; pin++) {
			if (*pin > 0) {
				int r = sunxi_pio_release(*pin);
				if (r < 0 ||
				    (ret == 0 && r > 0))
					ret = r;
			}
		}
	} else
		ret = 1;
	return ret;
}

EXPORT_SYMBOL_GPL(sunxi_pio_release_array);
