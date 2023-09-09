/*
-- (c) Copyright 2019 Xilinx, Inc. All rights reserved.
--
-- This file contains confidential and proprietary information
-- of Xilinx, Inc. and is protected under U.S. and
-- international copyright and other intellectual property
-- laws.
--
-- DISCLAIMER
-- This disclaimer is not a license and does not grant any
-- rights to the materials distributed herewith. Except as
-- otherwise provided in a Valid license issued to you by
-- Xilinx, and to the maximum extent permitted by applicable
-- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
-- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
-- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
-- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
-- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
-- (2) Xilinx shall not be liable (whether in contract or tort,
-- including negligence, or under any other theory of
-- liability) for any loss or damage of any kind or nature
-- related to, arising under or in connection with these
-- materials, including for any direct, or any indirect,
-- special, incidental, or consequential loss or damage
-- (including loss of Data, profits, goodwill, or any type of
-- loss or damage suffered as a result of any action brought
-- by a third party) even if such damage or loss was
-- reasonably foreseeable or Xilinx had been advised of the
-- possibility of the same.
--
-- CRITICAL APPLICATIONS
-- Xilinx products are not designed or intended to be fail-
-- safe, or for use in any application requiring fail-safe
-- performance, such as life-support or safety devices or
-- systems, Class III medical devices, nuclear facilities,
-- applications related to the deployment of airbags, or any
-- other applications that could lead to death, personal
-- injury, or severe property or environmental damage
-- (individually and collectively, "Critical
-- Applications"). Customer assumes the sole risk and
-- liability of any use of Xilinx products in Critical
-- Applications, subject only to applicable laws and
-- regulations governing limitations on product liability.
--
-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
-- PART OF THIS FILE AT ALL TIMES.
-------------------------------------------------------------------------------
--
-- Vendor         : Xilinx
-- Revision       : $Revision: #8 $
-- Date           : $DateTime: 2019/05/06 02:06:58 $
-- Last Author    : $Author: arayajig $
--
-------------------------------------------------------------------------------
-- Description :
-- This file is part of the Xilinx DMA IP Core driver for Windows.
--
-------------------------------------------------------------------------------
*/

#pragma once

// ========================= include dependencies =================================================

#include <ntddk.h>
#include "reg.h"
#include "dma_engine.h"
#include "interrupt.h"

// ========================= constants ============================================================

#define XDMA_MAX_NUM_BARS (3)

// ========================= type declarations ====================================================


/// Callback function type for user-defined work to be executed on receiving a user event.
typedef void(*PFN_XDMA_USER_WORK)(ULONG eventId, void* userData);

/// user event interrupt context
typedef struct XDMA_EVENT_T {
    PFN_XDMA_USER_WORK work; // user callback 
    void* userData; // custom user data. will be passed into work callback function
    WDFINTERRUPT irq; //wdf interrupt handle
} XDMA_EVENT;

/// The XDMA device context
typedef struct XDMA_DEVICE_T {

    // WDF 
    WDFDEVICE wdfDevice;

    // PCIe BAR access
    UINT numBars;
    PVOID bar[XDMA_MAX_NUM_BARS]; // kernel virtual address of BAR
    ULONG barLength[XDMA_MAX_NUM_BARS];
    ULONG configBarIdx;
    LONG userBarIdx;
    LONG bypassBarIdx;
    volatile XDMA_CONFIG_REGS *configRegs;
    volatile XDMA_IRQ_REGS *interruptRegs;
    volatile XDMA_SGDMA_COMMON_REGS * sgdmaRegs;

    // DMA Engine management
    XDMA_ENGINE engines[XDMA_MAX_NUM_CHANNELS][XDMA_NUM_DIRECTIONS];
    WDFDMAENABLER dmaEnabler;   // WDF DMA Enabler for the engine queues

    // Interrupt Resources
    WDFINTERRUPT lineInterrupt;
    WDFINTERRUPT channelInterrupts[XDMA_MAX_CHAN_IRQ];

    // user events
    XDMA_EVENT userEvents[XDMA_MAX_USER_IRQ];

    // available user and channel interrupts
    ULONG userMax;
    ULONG h2cChannelMax;
    ULONG c2hChannelMax;

} XDMA_DEVICE, *PXDMA_DEVICE;

// ========================= function declarations ================================================


