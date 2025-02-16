/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2005 Stephen Street / StreetFire Sound Labs
 */
#ifndef __LINUX_SPI_PXA2XX_SPI_H
#define __LINUX_SPI_PXA2XX_SPI_H

#include <linux/dmaengine.h>
#include <linux/types.h>

#include <linux/pxa2xx_ssp.h>

struct dma_chan;

/*
 * The platform data for SSP controller devices
 * (resides in device.platform_data).
 */
struct pxa2xx_spi_controller {
	u8 num_chipselect;
	u8 enable_dma;
	u8 dma_burst_size;
	bool is_target;

	/* DMA engine specific config */
	dma_filter_fn dma_filter;
	void *tx_param;
	void *rx_param;

	/* For non-PXA arches */
	struct ssp_device ssp;
};

/*
 * The controller specific data for SPI target devices
 * (resides in spi_board_info.controller_data),
 * copied to spi_device.platform_data ... mostly for
 * DMA tuning.
 */
struct pxa2xx_spi_chip {
	u8 tx_threshold;
	u8 tx_hi_threshold;
	u8 rx_threshold;
	u8 dma_burst_size;
	u32 timeout;
};

#endif	/* __LINUX_SPI_PXA2XX_SPI_H */
