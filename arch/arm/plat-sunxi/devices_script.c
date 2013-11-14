/*
 * Copyright (C) 2012  Alejandro Mery <amery@geeks.cl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt)	"sunxi:sys_config: " fmt

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <linux/ioport.h>

#include <mach/irqs.h>

#include <plat/system.h>
#include <plat/script.h>

#define SUNXI_DEVICE_NAME_LEN	32
#define RES(d, i, s)	(d)->resource[i] = (struct resource) s

struct sunxi_device {
	char name[SUNXI_DEVICE_NAME_LEN];
	const struct sunxi_section *config;
	u32 *pio;

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
	unsigned int manual_pio;
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

/* sunxi-uart */
#define UART_BASE       (0x01C28000)
#define UART_BASE_OS    (0x400)
#define UARTx_BASE(x)   (UART_BASE + (x) * UART_BASE_OS)

static int uart_irqs[] = {
	SW_INT_IRQNO_UART0,
	SW_INT_IRQNO_UART1,
	SW_INT_IRQNO_UART2,
	SW_INT_IRQNO_UART3,
	SW_INT_IRQNO_UART4,
	SW_INT_IRQNO_UART5,
	SW_INT_IRQNO_UART6,
	SW_INT_IRQNO_UART7,
};

static struct sunxi_device *uart_new(const struct sunxi_section *sp,
				     const char *feature, int id)
{
	int max = sunxi_is_sun5i() ? 4 : 8;
	struct sunxi_device *dev = NULL;

	if (id >= 0 && id < max)
		dev = sunxi_device_alloc(2, "sunxi-%s", feature);

	if (dev) {
		dev->pdev.id = id;
		dev->config = sp;
		RES(dev, 0, DEFINE_RES_MEM(UARTx_BASE(id), UART_BASE_OS));
		RES(dev, 1, DEFINE_RES_IRQ(uart_irqs[id]));
		dev->pdev.num_resources = dev->num_res;
		dev->pdev.resource = dev->resource;
	}

	return dev;
}

static struct pdev_constructor constructors[] = {
	{ .name = "uart", .f = uart_new, },
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

	if (!pc->f)
		return; /* skip */

	dev = pc->f(sp, name, index);

	if (dev) {
		int ret;

		if (pc->manual_pio)
			; /* driver will do it */
		else if (sunxi_request_section_gpio(&dev->pio, sp) < 0) {
			if (index < 0)
				pr_err("%s: failed to request GPIOs", dev->pdev.name);
			else
				pr_err("%s.%d: failed to request GPIOs",
				       dev->pdev.name, dev->pdev.id);
			goto free_dev;
		}

		ret = platform_device_register(&dev->pdev);
		if (ret) {
			if (index < 0)
				pr_err("%s: registration failed: %d\n",
				       dev->pdev.name, ret);
			else
				pr_err("%s.%d: registration failed: %d\n",
				       dev->pdev.name, dev->pdev.id, ret);

			goto free_gpio;
		} else if (index < 0)
			pr_info("%s: registered\n", dev->pdev.name);
		else
			pr_info("%s.%d: registered\n", dev->pdev.name, dev->pdev.id);

	} else if (index < 0)
		pr_err("%s failed to allocate pdev\n", name);
	else
		pr_err("%s.%d failed to allocate pdev\n", name, index);

	goto done;
free_gpio:
	sunxi_pio_release_array(dev->pio);
	kfree(dev->pio);
free_dev:
	kfree(dev);
done:
	return;
}

/*
 * scan script.bin
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

void __init sunxi_pdev_script_init(void)
{
	int i;
	const struct sunxi_section *section;

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
}
