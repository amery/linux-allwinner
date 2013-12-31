#ifndef _SUNXI_H_
#define _SUNXI_H_
#ifdef CONFIG_ARCH_SUNXI

u32 sunxi_sc_chip_id(void) __pure;
u32 sunxi_chip_id(void) __pure;

#endif /* CONFIG_ARCH_SUNXI */
#endif /* !_SUNXI_H_ */
