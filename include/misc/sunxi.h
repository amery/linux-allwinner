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

u32 sunxi_sc_chip_id(void) __pure;
u32 sunxi_chip_id(void) __pure;

void sunxi_setup_soc_detect(void);

#endif /* CONFIG_ARCH_SUNXI */
#endif /* !_SUNXI_H_ */
