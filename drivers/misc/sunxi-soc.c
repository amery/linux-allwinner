#define pr_fmt(fmt) "sunxi: " fmt

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/io.h>

#include <misc/sunxi.h>

#define SRAMC_IO_BASE		0x01c00000

#define SC_CHIP_ID_EN_MASK	0x1
#define SC_CHIP_ID_EN_OFF	15
#define SC_CHIP_ID_EN		(SC_CHIP_ID_EN_MASK<<SC_CHIP_ID_EN_OFF)

#define SC_CHIP_ID_MASK		0xffff
#define SC_CHIP_ID_OFF		16
#define SC_CHIP_ID		(SC_CHIP_ID_MASK<<SC_CHIP_ID_OFF)

u32 sunxi_sc_chip_id(void)
{
	static void __iomem *sramc_base, *sc_base;
	u32 chip_id, reg_val;

	if (unlikely(!sc_base)) {
		sramc_base = ioremap(SRAMC_IO_BASE, SZ_4K);
		if (!sramc_base) {
			pr_err("Failed to iomap the SRAMC register\n");
			return 0;
		}
		sc_base = (void*)((char*)sramc_base + 0x24);
	}

	/* enable chip_id reading */
	reg_val = readl(sc_base);
	writel(reg_val | SC_CHIP_ID_EN, sc_base);

	reg_val = readl(sc_base);
	chip_id = ((reg_val&SC_CHIP_ID)>>SC_CHIP_ID_OFF) & SC_CHIP_ID_MASK;

	return chip_id;
}
EXPORT_SYMBOL(sunxi_sc_chip_id);

u32 sunxi_chip_id(void)
{
	static u32 chip_id;

	if (unlikely(chip_id == 0))
		chip_id = sunxi_sc_chip_id();

	return chip_id;
}
EXPORT_SYMBOL(sunxi_chip_id);
