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

#define pr_fmt(fmt)	"sunxi:sys_config: " fmt

#include <linux/export.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/platform_device.h>

#include <misc/sunxi-script.h>

#define SUNXI_DEVICE_NAME_LEN	32

struct sunxi_device {
	char name[SUNXI_DEVICE_NAME_LEN];
	const struct sunxi_section *config;

	struct platform_device pdev;

	unsigned int num_res;
	struct resource resource[];
};

struct sunxi_device *sunxi_device_alloc(size_t num_res, const char *fmt, ...)
{
	va_list ap;
	struct sunxi_device *dev = kzalloc(sizeof(*dev) +
					   num_res*sizeof(struct resource),
					   GFP_KERNEL);
	if (dev) {
		va_start(ap, fmt);
		vsnprintf(dev->name, sizeof(dev->name), fmt, ap);
		va_end(ap);

		dev->pdev.name = dev->name;
		dev->num_res = num_res;
	}
	return dev;
}

/*
 * pdev constructors
 */
struct pdev_constructor {
	const char *name;
	struct sunxi_device *(*f)(const struct sunxi_section *,
				  const char *, int);
};

static struct sunxi_device *generic_new(const struct sunxi_section *sp,
					   const char *feature, int id)
{
	struct sunxi_device *dev = sunxi_device_alloc(0, "sunxi-%s", feature);
	if (dev) {
		dev->pdev.id = id;
		dev->config = sp;
	}
	return dev;
}

static struct pdev_constructor constructors[] = {
	{ .f = generic_new, }, /* fallback */
};

static void create_pdev(const struct sunxi_section *sp,
			const char *name, int index)
{
	struct sunxi_device *dev;
	struct pdev_constructor *pc;

	/* search constructor */
	for (pc = constructors; pc->name && strcmp(pc->name, name) != 0; pc++)
		;

	dev = pc->f(sp, name, index);

	if (dev) {
		int ret = platform_device_register(&dev->pdev);

		if (ret) {
			if (index < 0)
				pr_info("%s: registration failed: %d\n", dev->pdev.name, ret);
			else
				pr_info("%s.%d: registration failed: %d\n", dev->pdev.name, dev->pdev.id,
					ret);

			kfree(dev);
			dev = NULL;
		} else if (index < 0)
			pr_info("%s: registered\n", dev->pdev.name);
		else
			pr_info("%s.%d: registered\n", dev->pdev.name, dev->pdev.id);
	} else if (index < 0)
		pr_warning("%s failed to allocate pdev\n", name);
	else
		pr_warning("%s.%d failed to allocate pdev\n", name, index);
}

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
static int feature_name(const char *name, char *feature, int *id)
{
	char c;
	int ret = 0;
	size_t l = strlen(name);

	if (l == 0)
		goto done;

	/* UARTs and USBs have the %d at the end */
	c = name[l-1];
	if (c >= '0' && c <= '9') {
		*id = c - '0';
		--l;
	} else {
		*id = -1;
	}

	if (l==4 && strncmp(name, "usbc", 4) == 0) {
		/* USB controllers don't end in _para */
		strcpy(feature, "usb");
		ret = 1;
	} else if (l<6) {
		/* needs room for _para */
		;
	} else if (strncmp(name+l-5, "_para", 5) == 0) {
		l -= 5;
		if (*id < 0) {
			c = name[l-1];
			if (c >= '0' && c <= '9') {
				*id = c - '0';
				--l;
				/* ps2_%d_para */
				if (l>1 && name[l-1] == '_')
					--l;
			}
		}
		if (l > 0) {
			memcpy(feature, name, l);
			feature[l] = '\0';
			ret = 1;
		}
	}
done:
	return ret;
}

static void config_feature(const struct sunxi_section *section,
			  const char *name, int index)
{
	const struct sunxi_property *prop;
	const u32 *used;

	prop = sunxi_find_property_fmt(section, "%s_used", name);
	if (prop && (sunxi_property_type(prop) == SUNXI_PROP_TYPE_U32)) {
		used = sunxi_property_value(prop);
		if (index < 0)
			pr_debug("[%s] -> %s used:%u\n",
				 section->name, name, *used);
		else
			pr_debug("[%s] -> %s:%d used:%u\n",
				 section->name, name, index, *used);

		if (*used == 1)
			create_pdev(section, name, index);
	} else if (index < 0) {
		pr_debug("[%s] -> %s assumed unused\n",
			 section->name, name);
	} else {
		pr_debug("[%s] -> %s:%u assumed unused\n",
			 section->name, name, index);
	}
}

static int __init init_sunxi_script_devices(void)
{
	int i;
	const struct sunxi_section *section;

	/* should use memory from dts, and command line argument */
	if (!sunxi_script_probe(0x43000000)) {
		pr_warn("script.bin not found\n");
		return 0;
	}

	pr_debug("scanning %d sections at 0x%p\n",
		 sunxi_get_section_count(), sunxi_script_base);

	sunxi_for_each_section(section, i) {
		char feature[32] = "";
		int index = -1;

		if (feature_name(section->name, feature, &index))
			config_feature(section, feature, index);
		else
			pr_debug("[%s] SKIP\n", section->name);
	}
	return 1;
}
pure_initcall(init_sunxi_script_devices);
