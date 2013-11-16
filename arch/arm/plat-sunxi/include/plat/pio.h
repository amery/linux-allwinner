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

#ifndef _SUNXI_PLAT_PIO_H
#define _SUNXI_PLAT_PIO_H

/*
 * register
 */
struct sunxi_pio_bank {
	u32 mux[4];
	u32 dat;
	u32 drv[2];
	u32 pull[2];
};

struct sunxi_pio_intr {
	u32 cfg[3];
	u32 ctl;
	u32 sta;
	u32 deb;	/* interrupt debounce */
};

struct sunxi_pio_reg {
	struct sunxi_pio_bank bank[9];
	u8 reserved[0xbc];
	struct sunxi_pio_intr intr;
};

/*
 * low level API
 */
static inline unsigned sunxi_pio_get_mux(struct sunxi_pio_reg *reg,
					 unsigned bank, unsigned num)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	unsigned index = num >> 3;
	unsigned offset = (num & 0x7) << 2;

	return (pio->mux[index] >> offset) & 0x7;
}

static inline void sunxi_pio_set_mux(struct sunxi_pio_reg *reg,
				     unsigned bank, unsigned num,
				     unsigned mux)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	unsigned index = num >> 3;
	unsigned offset = (num & 0x7) << 2;
	u32 val = pio->mux[index] & ~(0x7 << offset);

	pio->mux[index] = val | ((mux & 0x7) << offset);
}

static inline unsigned sunxi_pio_get_drv(struct sunxi_pio_reg *reg,
					 unsigned bank, unsigned num)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	unsigned index = num >> 4;
	unsigned offset = (num & 0xf) << 1;

	return (pio->drv[index] >> offset) & 0x3;
}

static inline void sunxi_pio_set_drv(struct sunxi_pio_reg *reg,
				     unsigned bank, unsigned num,
				     unsigned drv)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	unsigned index = num >> 4;
	unsigned offset = (num & 0xf) << 1;
	u32 val = pio->drv[index] & ~(0x3 << offset);

	pio->drv[index] = val | ((drv & 0x3) << offset);
}

static inline unsigned sunxi_pio_get_pull(struct sunxi_pio_reg *reg,
					  unsigned bank, unsigned num)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	unsigned index = num >> 4;
	unsigned offset = (num & 0xf) << 1;

	return (pio->pull[index] >> offset) & 0x3;
}

static inline void sunxi_pio_set_pull(struct sunxi_pio_reg *reg,
				      unsigned bank, unsigned num,
				      unsigned pull)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	unsigned index = num >> 4;
	unsigned offset = (num & 0xf) << 1;
	u32 val = pio->pull[index] & ~(0x3 << offset);

	pio->pull[index] = val | ((pull & 0x3) << offset);
}

static inline unsigned sunxi_pio_get_val(struct sunxi_pio_reg *reg,
					 unsigned bank, unsigned num)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	return (pio->dat >> num) & 0x01;
}

static inline void sunxi_pio_set_val(struct sunxi_pio_reg *reg,
				     unsigned bank, unsigned num,
				     unsigned v)
{
	struct sunxi_pio_bank *pio = &reg->bank[bank];
	u32 val = pio->dat & ~(0x01 << num);

	pio->dat = val | ((v & 0x01) << num);
}

/*
 * request/release
 */
int sunxi_pio_request(const char *name, unsigned bank, unsigned num,
		      int mux, int pull, int drv, int val);

int sunxi_pio_release(u32 pin);
int sunxi_pio_release_array(u32 *pin);

#endif
