/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * Hardware Abstraction Layer (IA32 PC)
 *
 * Copyright 2012, 2016 Phoenix Systems
 * Copyright 2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "spinlock.h"
#include "console.h"
#include "exceptions.h"
#include "interrupts.h"
#include "cpu.h"
#include "pmap.h"
#include "timer.h"
#include "pci.h"


struct {
	int started;
} hal_common;


int hal_started(void)
{
	return hal_common.started;
}


void _hal_start(void)
{
	hal_common.started = 1;
}


__attribute__ ((section (".init"))) void _hal_init(void)
{
	_hal_spinlockInit();
	_hal_consoleInit();

	_hal_exceptionsInit();
	_hal_interruptsInit();

	_timer_init(SYSTICK_INTERVAL);
	_hal_cpuInit();
	_hal_pciInit();

	hal_common.started = 0;
	return;
}
