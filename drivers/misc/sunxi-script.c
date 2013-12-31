/*
 * Copyright (C) 2012, Alejandro Mery <amery@geeks.cl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define pr_fmt(fmt)	"sunxi: script: " fmt

#include <linux/export.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>

#include <misc/sunxi-script.h>

/*
 * script.bin parsing
 */
const struct sunxi_script *sunxi_script_base = NULL;
EXPORT_SYMBOL(sunxi_script_base);

static int __init sunxi_script_probe(u32 addr)
{
	const struct sunxi_script *base = __va((void*)addr);

	pr_debug("base: 0x%08x (0x%p)\n", addr, base);
	pr_debug("version: %u.%u.%u count: %u\n",
		base->version[0], base->version[1], base->version[2],
		base->count);

	if (base->version[0] < 1 &&
	    base->version[1] < 2 &&
	    base->version[2] < 10 &&
	    base->count <= 256) {
		/* reasonable */
		sunxi_script_base = base;

		return 1;
	}

	return 0;
}

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

/*
 * script.bin based device creation
 */
static int __init init_sunxi_script_devices(void)
{
	/* should use memory from dts, and command line argument */
	if (!sunxi_script_probe(0x43000000))
		pr_warn("script.bin not found\n");

	return 0;
}
pure_initcall(init_sunxi_script_devices);
