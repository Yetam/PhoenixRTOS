/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * ehci/phy.c
 *
 * Copyright 2018 Phoenix Systems
 * Copyright 2007 Pawel Pisarczyk
 * Author: Jan Sikorski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/mman.h>
#include <sys/platform.h>
#include <phoenix/arch/imx6ull.h>
#include <stdio.h>
#include "phy.h"

#define PHY_ADDR 0x020c9000


enum { usbphy_pwd, usbphy_pwd_set, usbphy_pwd_clr, usbphy_pwd_tog, usbphy_tx, usbphy_tx_set,
	usbphy_tx_clr, usbphy_tx_tog, usbphy_rx, usbphy_rx_set, usbphy_rx_clr, usbphy_rx_tog,
	usbphy_ctrl, usbphy_ctrl_set, usbphy_ctrl_clr, usbphy_ctrl_tog, usbphy_status,
	usbphy_debug = usbphy_status + 4, usbphy_debug_set, usbphy_debug_clr, usbphy_debug_tog,
	usbphy_debug0_status, usbphy_debug1 = usbphy_debug0_status + 4, usbphy_debug1_set,
	usbphy_debug1_clr, usbphy_debug1_tog, usbphy_version };


struct {
	volatile unsigned *base;
} phy_common;


void phy_dumpRegisters(FILE *stream)
{
	fprintf(stream, "%18s: %08x", "usbphy_pwd", *(phy_common.base + usbphy_pwd));
	fprintf(stream, "%18s: %08x\n", "usbphy_tx", *(phy_common.base + usbphy_tx));
	fprintf(stream, "%18s: %08x", "usbphy_rx", *(phy_common.base + usbphy_rx));
	fprintf(stream, "%18s: %08x\n", "usbphy_ctrl", *(phy_common.base + usbphy_ctrl));
	fprintf(stream, "%18s: %08x", "usbphy_status", *(phy_common.base + usbphy_status));
	fprintf(stream, "%18s: %08x\n", "usbphy_debug", *(phy_common.base + usbphy_debug));
	fprintf(stream, "%18s: %08x", "usbphy_debug0_status", *(phy_common.base + usbphy_debug0_status));
	fprintf(stream, "%18s: %08x\n", "usbphy_debug1", *(phy_common.base + usbphy_debug1));
	fprintf(stream, "%18s: %08x\n\n", "usbphy_version", *(phy_common.base + usbphy_version));
}


void phy_config(void)
{
	*(phy_common.base + usbphy_ctrl) |= 3 << 14;
	*(phy_common.base + usbphy_ctrl) |= 2;
	*(phy_common.base + usbphy_ctrl) |= 1 << 11;
}

void phy_initPad(void)
{
	platformctl_t set_mux = {
		.action = pctl_set,
		.type = pctl_iomux,
		.iomux = { .mux = pctl_mux_sd1_d1, .sion = 0, .mode = 8 },
	};

	platformctl_t set_pad = {
		.action = pctl_set,
		.type = pctl_iopad,
		.iopad = { .pad = pctl_pad_sd1_d1, .hys = 0, .pus = 0, .pue = 0,
			.pke = 0, .ode = 0, .speed = 2, .dse = 4, .sre = 0 },
	};

	platformctl(&set_mux);
	platformctl(&set_pad);
}


void phy_reset(void)
{
	*(phy_common.base + usbphy_ctrl) |= (1 << 31);
	*(phy_common.base + usbphy_ctrl) &= ~(1 << 30);
	*(phy_common.base + usbphy_ctrl) &= ~(1 << 31);
	*(phy_common.base + usbphy_pwd) = 0;
}


void phy_initClock(void)
{
	platformctl_t ctl = (platformctl_t) {
		.action = pctl_set,
		.type = pctl_devclock,
		.devclock = {
			.dev = pctl_clk_usboh3,
			.state = 3,
		}
	};

	platformctl(&ctl);
}


void phy_disableClock(void)
{
	platformctl_t ctl = (platformctl_t) {
		.action = pctl_set,
		.type = pctl_devclock,
		.devclock = {
			.dev = pctl_clk_usboh3,
			.state = 0,
		}
	};

	platformctl(&ctl);
}


void phy_init(void)
{
	phy_common.base = mmap(NULL, 2 * _PAGE_SIZE, PROT_WRITE | PROT_READ, MAP_DEVICE, OID_PHYSMEM, PHY_ADDR);

	/* Offset into PHY2 */
	phy_common.base += 1024;

	phy_initClock();
	phy_initPad();
	phy_reset();
	phy_config();
}
