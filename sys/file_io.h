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
-- Revision       : $Revision: #9 $
-- Date           : $DateTime: 2019/05/31 03:43:11 $
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

#include "xdma.h"

// ========================= declarations =========================================================

// The type of the device node - this determines behaviour of EvtIoRead and EvtIoWrite functions
typedef enum {
    DEVNODE_TYPE_USER,
    DEVNODE_TYPE_CONTROL,
    DEVNODE_TYPE_EVENTS,
    DEVNODE_TYPE_BYPASS,
    DEVNODE_TYPE_H2C,
    DEVNODE_TYPE_C2H,
    ID_DEVNODE_UNKNOWN = 255,
} DEVNODE_TYPE;

// File Context Data
typedef struct _FILE_CONTEXT {
    DEVNODE_TYPE devType;
    union {
        void* bar;              // USER / CONTROL / BYPASS
        XDMA_EVENT* event;      // EVENTS
        XDMA_ENGINE* engine;    // H2C / C2H
    } u;
    WDFQUEUE queue;
	PMDL mdl;
    PVOID virtAddress;          // For mapped BAR
} FILE_CONTEXT, *PFILE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILE_CONTEXT, GetFileContext)

// Queue Context Data
typedef struct _QUEUE_CONTEXT {
    XDMA_ENGINE* engine;
} QUEUE_CONTEXT, *PQUEUE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, GetQueueContext)

EVT_WDF_IO_IN_CALLER_CONTEXT        EvtDeviceIoInCallerContext;
EVT_WDF_DEVICE_FILE_CREATE          EvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE                  EvtFileClose;
EVT_WDF_FILE_CLEANUP                EvtFileCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  EvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_READ			EvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE			EvtIoWrite;

EVT_WDF_IO_QUEUE_IO_READ    EvtIoReadDma;
EVT_WDF_IO_QUEUE_IO_WRITE   EvtIoWriteDma;
EVT_WDF_IO_QUEUE_IO_READ    EvtIoReadEngineRing;

NTSTATUS EvtReadUserEvent(WDFREQUEST request, size_t length);
VOID HandleUserEvent(ULONG eventId, void* userData);