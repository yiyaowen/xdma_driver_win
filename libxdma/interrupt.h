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
-- Revision       : $Revision: #7 $
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
#include "dma_engine.h"

// ========================= declarations =================================================

#define XDMA_MAX_USER_IRQ (16)
#define XMDA_MAX_NUM_IRQ (XDMA_MAX_USER_IRQ + XDMA_MAX_CHAN_IRQ)

typedef struct _IRQ_CONTEXT {
    ULONG eventId;
    UINT32 channelIrqPending; // channel irq that have fired
    UINT32 userIrqPending; // user event irq that have fired
    XDMA_ENGINE* engine;
    volatile XDMA_IRQ_REGS* regs;
    PXDMA_DEVICE xdma;
} IRQ_CONTEXT, *PIRQ_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(IRQ_CONTEXT, GetIrqContext)


/// Initialize the interrupt resources given by the OS for use by DMA engines and user events 
 NTSTATUS SetupInterrupts(IN PXDMA_DEVICE xdma,
                          IN OUT ULONG *userMax,
                          IN OUT ULONG *h2cChannelMax,
                          IN OUT ULONG *c2hChannelMax,
                          IN WDFCMRESLIST ResourcesRaw,
                          IN WDFCMRESLIST ResourcesTranslated);

