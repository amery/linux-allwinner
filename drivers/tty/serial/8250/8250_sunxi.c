/*
 *  8250_sunxi.c
 *
 *  Copyright (C) 1996-2003 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * HdG: The sunxi uarts seem to be similar to the Synopsys DesignWare 8250
 * uarts, they also have a busy-detect interrupt signalled by a value of 7
 * in the IIR register. The code for handling this is copied from 8250_dw.c :
 *
 * Synopsys DesignWare 8250 driver.
 *
 * Copyright 2011 Picochip, Jamie Iles.
 *
 * The Synopsys DesignWare 8250 has an extra feature whereby it detects if the
 * LCR is written whilst busy.  If it is, then a busy detect interrupt is
 * raised, the LCR needs to be rewritten and the uart status register read.
 */
#define DRIVER_NAME	"sunxi-uart"
#define pr_fmt(fmt)	DRIVER_NAME ": " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/tty.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/ecard.h>
#include <asm/string.h>
#include <linux/clk.h>

#include "8250.h"

/* Register base define */
#define UART_BASE       (0x01C28000)
#define UART_BASE_OS    (0x400)
#define UARTx_BASE(x)   (UART_BASE + (x) * UART_BASE_OS)
#define RESSIZE(res)    (((res)->end - (res)->start)+1)

#define UART_HALT           0x29 /* Halt TX register */
#define UART_FORCE_CFG      (1 << 1)
#define UART_FORCE_UPDATE   (1 << 2)

struct sw_serial_port {
	int port_no;
	int line;
	int last_lcr;
	struct clk *clk;
	u32 sclk;
	struct resource *mmres;
	u32 irq;
	struct platform_device *pdev;
};

static int sw_serial_get_resource(struct sw_serial_port *sport)
{
	char name[16];
	struct clk *pclk = NULL;
	int ret;

	/* get register base */
	sport->mmres = platform_get_resource(sport->pdev, IORESOURCE_MEM, 0);
	if (!sport->mmres) {
		ret = -ENODEV;
		goto err_out;
	}

	/* get clock */
	pclk = clk_get(&sport->pdev->dev, "apb1");
	if (IS_ERR(pclk)) {
		ret = PTR_ERR(pclk);
		goto iounmap;
	}
	sport->sclk = clk_get_rate(pclk);
	clk_put(pclk);

	sprintf(name, "apb_uart%d", sport->port_no);
	sport->clk = clk_get(&sport->pdev->dev, name);
	if (IS_ERR(sport->clk)) {
		ret = PTR_ERR(sport->clk);
		goto iounmap;
	}
	clk_enable(sport->clk);

	/* get irq */
	sport->irq = platform_get_irq(sport->pdev, 0);
	if (sport->irq == 0) {
		ret = -EINVAL;
		goto free_pclk;
	}

	return 0;

 free_pclk:
	clk_put(sport->clk);
 iounmap:
 err_out:
	return ret;
}

static int sw_serial_put_resource(struct sw_serial_port *sport)
{
	clk_disable(sport->clk);
	clk_put(sport->clk);
	return 0;
}

static void sw_serial_out32(struct uart_port *p, int offset, int value)
{
	struct sw_serial_port *d = p->private_data;

	if (offset == UART_LCR)
		d->last_lcr = value;

	offset <<= p->regshift;
	writel(value, p->membase + offset);
}

static unsigned int sw_serial_in32(struct uart_port *p, int offset)
{
	offset <<= p->regshift;

	return readl(p->membase + offset);
}

static int sw_serial_handle_irq(struct uart_port *p)
{
	struct sw_serial_port *d = p->private_data;
	unsigned int iir = p->serial_in(p, UART_IIR);

	if (serial8250_handle_irq(p, iir)) {
		return 1;
	} else if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
		if (p->serial_in(p, UART_USR) & 1) {
			p->serial_out(p, UART_HALT, UART_FORCE_CFG);
			p->serial_out(p, UART_LCR, d->last_lcr);
			p->serial_out(p, UART_HALT, UART_FORCE_CFG | UART_FORCE_UPDATE);
			while(p->serial_in(p, UART_HALT) & UART_FORCE_UPDATE);
			p->serial_out(p, UART_HALT, 0x00);
			p->serial_in(p, UART_USR);
		} else
			p->serial_out(p, UART_LCR, d->last_lcr);
		return 1;
	}

	return 0;
}

static void sw_serial_pm(struct uart_port *port, unsigned int state,
			 unsigned int oldstate)
{
	struct sw_serial_port *up = port->private_data;

	if (!state)
		clk_enable(up->clk);
	else
		clk_disable(up->clk);
}

static int __devinit sw_serial_probe(struct platform_device *dev)
{
	struct sw_serial_port *sport;
	struct uart_port port = {};
	int ret;

	sport = kzalloc(sizeof(struct sw_serial_port), GFP_KERNEL);
	if (!sport)
		return -ENOMEM;
	sport->port_no = dev->id;
	sport->pdev = dev;

	ret = sw_serial_get_resource(sport);
	if (ret) {
		printk(KERN_ERR "Failed to get resource\n");
		goto free_dev;
	}

	port.private_data = sport;
	port.irq = sport->irq;
	port.mapbase = sport->mmres->start;
	port.fifosize = 64;
	port.regshift = 2;
	port.iotype  = UPIO_MEM32;
	port.flags = UPF_IOREMAP | UPF_BOOT_AUTOCONF;
	port.uartclk = sport->sclk;
	port.pm = sw_serial_pm;
	port.dev = &dev->dev;
	port.serial_in = sw_serial_in32;
	port.serial_out = sw_serial_out32;
	port.handle_irq = sw_serial_handle_irq;

	pr_info("serial probe %d irq %d mapbase 0x%08x\n", dev->id,
		sport->irq, sport->mmres->start);
	ret = serial8250_register_port(&port);
	if (ret < 0)
		goto free_dev;

	sport->line = ret;
	platform_set_drvdata(dev, sport);
	return 0;

 free_dev:
	kfree(sport);
	sport = NULL;
	return ret;
}

static int __devexit sw_serial_remove(struct platform_device *dev)
{
	struct sw_serial_port *sport = platform_get_drvdata(dev);

	pr_info("serial remove\n");
	serial8250_unregister_port(sport->line);
	sw_serial_put_resource(sport);

	platform_set_drvdata(dev, NULL);
	kfree(sport);
	sport = NULL;
	return 0;
}

static struct platform_driver sw_serial_driver = {
	.probe = sw_serial_probe,
	.remove = sw_serial_remove,
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init sw_serial_init(void)
{
	return platform_driver_register(&sw_serial_driver);
}

static void __exit sw_serial_exit(void)
{
	platform_driver_unregister(&sw_serial_driver);
}

MODULE_AUTHOR("Aaron.myeh<leafy.myeh@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI 8250-compatible serial port expansion card driver");
MODULE_LICENSE("GPL");

module_init(sw_serial_init);
module_exit(sw_serial_exit);
