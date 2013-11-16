/*
 * Copyright 2012-2013 Alejandro Mery
 *
 * Alejandro Mery <amery@geeks.cl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define pr_fmt(fmt)	"sunxi: script: " fmt

#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#include <plat/pio.h>
#include <plat/script.h>

const struct sunxi_script *sunxi_script_base = NULL;
EXPORT_SYMBOL(sunxi_script_base);

void sunxi_script_init(const struct sunxi_script *base)
{
	sunxi_script_base = base;
	pr_debug("base: 0x%p\n", base);
	pr_debug("version: %u.%u.%u count: %u\n",
		base->version[0], base->version[1], base->version[2],
		base->count);
}
EXPORT_SYMBOL(sunxi_script_init);

const struct sunxi_property *sunxi_find_property_fmt(
		const struct sunxi_section *sp,
		const char *fmt, ...)
{
	const struct sunxi_property *prop;
	char name[sizeof(prop->name)];
	va_list args;

	va_start(args, fmt);
	vsprintf(name, fmt, args);
	va_end(args);

	prop = sunxi_find_property(sp, name);
	return prop;
}
EXPORT_SYMBOL_GPL(sunxi_find_property_fmt);

ssize_t sunxi_request_section_gpio(u32 **out,
				   const struct sunxi_section *sp)
{
	const struct sunxi_property *pp;
	unsigned count = 0, i;

	sunxi_for_each_property(sp, pp, i) {
		if (SUNXI_PROP_TYPE_GPIO == sunxi_property_type(pp))
			count++;
	}

	if (count > 0) {
		u32 *p = kzalloc((count + 1)*sizeof(u32), GFP_KERNEL);
		*out = p;

		if (!p)
			return -ENOMEM;

		sunxi_for_each_property(sp, pp, i) {
			s32 pio = sunxi_property_request_gpio(pp);
			if (pio >= 0)
				*p++ = pio;
			else if (pio < -1)
				*p++ = 0;
			else
				;
		}
		*p = -1;
	} else
		*out = NULL;

	return count;
}
EXPORT_SYMBOL_GPL(sunxi_request_section_gpio);
