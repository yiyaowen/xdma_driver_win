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
-- Revision       : $Revision: #6 $
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

#include <ntddk.h>
#include <wdf.h>

#include "device.h"

#define XDMA_MAKE_VERSION(major, minor, patch) (((major) << 24) | ((minor) << 26) | (patch))
#define XDMA_VERSION_MAJOR(version) (((uint32_t)(version) >> 24) & 0xff)
#define XDMA_VERSION_MINOR(version) (((uint32_t)(version) >> 16) & 0xff)
#define XDMA_VERSION_PATCH(version) ((uint32_t)(version) & 0xffff)

// TODO Bump this number when making a new release
#define XDMA_LIB_VERSION XDMA_MAKE_VERSION(2017, 4, 1)

/**
 * \brief Open and initialize an XDMA device given by the WDFDEVICE handle.
 * \param wdfDevice     [IN]        The OS device handle
 * \param xdma          [IN]        The XDMA device context
 * \param userMax       [IN OUT]    Max # of user/event (interrupts) to be configured
 * \param h2cChannelMax [IN OUT]    Max # of h2c channels to be enabled
 * \param c2hChannelMax [IN OUT]    Max # of c2h channels to be enabled
 * \param ResourcesRaw  [IN]        List of PCIe resources assigned to this device
 * \param ResourcesTranslated [IN]  List of PCIe resources assigned to this device
 * \return STATUS_SUCCESS on successful completion. All other return values indicate error conditions. 
 * \NOTE: If the user provisioned is more than the max specified or zero,
 * \libxdma will update the userMax, h2cChannelMax, c2hChannelMax.
 */
NTSTATUS XDMA_DeviceOpen(WDFDEVICE wdfDevice,
                         PXDMA_DEVICE xdma,
                         ULONG *userMax,
                         ULONG *h2cChannelMax,
                         ULONG *c2hChannelMax,
                         WDFCMRESLIST ResourcesRaw,
                         WDFCMRESLIST ResourcesTranslated);

/**
 * \brief Close and cleanup the XDMA device.
 * \param xdma          [IN]        The XDMA device context
 */
void XDMA_DeviceClose(PXDMA_DEVICE xdma);

/**
 * \brief Register a callback function to execute when user events occur. 
 * \param xdma          [IN]        The XDMA device context
 * \param index         [IN]        The Event ID of the user event (0-15)
 * \param handler       [IN]        The callback function to execute on event detection
 * \param userData      [IN]        Custom user data/handle which will be passed to the callback 
 *                                  function. 
 * \return STATUS_SUCCESS on successful completion. All other return values indicate error conditions. 
 */
NTSTATUS XDMA_UserIsrRegister(PXDMA_DEVICE xdma,
                              ULONG index,
                              PFN_XDMA_USER_WORK handler,
                              void* userData);

/**
 * \brief Enable interrupt handling for a specific user event. 
 * \param xdma          [IN]        The XDMA device context
 * \param eventId       [IN]        The Event ID of the user event (0-15)
 * \return STATUS_SUCCESS on successful completion. All other return values indicate error conditions. 
 */
NTSTATUS XDMA_UserIsrEnable(PXDMA_DEVICE xdma, ULONG eventId);

/**
 * \brief Disable interrupt handling for a specific user event. 
 * \param xdma          [IN]        The XDMA device context
 * \param eventId       [IN]        The Event ID of the user event (0-15)
 * \return STATUS_SUCCESS on successful completion. All other return values indicate error conditions. 
 */
NTSTATUS XDMA_UserIsrDisable(PXDMA_DEVICE xdma, ULONG eventId);

/**
 * \brief OS callback function for programming the XDMA engine
 * \param Transaction    [IN]        The WDFDMATRANSACTION handle
 * \param Device         [IN]        The WDFDEVICE handle
 * \param Context        [IN]        The XDMA engine context
 * \param Direction      [IN]        Data transaction direction. H2C=WdfDmaDirectionToDevice. C2H=WdfDmaDirectionFromDevice
 * \param SgList         [IN]        The Scatter-Gather list describing the Host-side memory.
 * \return TRUE on success, else FALSE
 */
EVT_WDF_PROGRAM_DMA XDMA_EngineProgramDma;

/**
 * \brief Select between polling and interrupts as a mechanism for determining dma transfer 
 *        completion on a per DMA engine basis.
 * \param engine        [IN]        The DMA engine context
 * \param pollMode      [IN]        true = use polling, false = use interrupts
 */
void XDMA_EngineSetPollMode(XDMA_ENGINE* engine, BOOLEAN pollMode);