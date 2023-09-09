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
-- Revision       : $Revision: #5 $
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

#include <wdf.h>
#include <ntddk.h>
#include <initguid.h> // required for GUID definitions
#include <wdmguid.h> // required for WMILIB_CONTEXT

static UCHAR FindCapability(IN PBUS_INTERFACE_STANDARD pciBus, IN PPCI_COMMON_HEADER pciHeader,
                            IN ULONG capabilityID) {

    PCI_EXPRESS_CAPABILITY pcieCapabilities = { 0 };

    // check if capabilities list supported
    if ((pciHeader->Status & PCI_STATUS_CAPABILITIES_LIST) == 0) {
        return 0;
    }

    // get offset to capabilites list
    UCHAR capabilityOffset;
    if ((pciHeader->HeaderType &	(~PCI_MULTIFUNCTION)) == PCI_BRIDGE_TYPE) {
        capabilityOffset = pciHeader->u.type1.CapabilitiesPtr;
    } else if ((pciHeader->HeaderType & (~PCI_MULTIFUNCTION)) == PCI_CARDBUS_BRIDGE_TYPE) {
        capabilityOffset = pciHeader->u.type2.CapabilitiesPtr;
    } else {
        capabilityOffset = pciHeader->u.type0.CapabilitiesPtr; // PCI_DEVICE_TYPE
    }

    // Loop through the capabilities in search of the pcie capability. 
    while (capabilityOffset != 0) {

        // Read the header of the capability at this offset.
        pciBus->GetBusData(pciBus->Context, PCI_WHICHSPACE_CONFIG, &pcieCapabilities,
                           capabilityOffset, sizeof(PCI_CAPABILITIES_HEADER));

        // If the retrieved capability is not the pcie capability that we are looking 
        // for, follow the link to the next capability and continue looping.
        if (pcieCapabilities.Header.CapabilityID == capabilityID) {
            break; // Found the capability
        } else {
            // This is some other capability. Keep looking.
            capabilityOffset = pcieCapabilities.Header.Next;
        }
    }

    return capabilityOffset;
}

/// Determine how many MSI message IDs are assigned by the OS.
static NTSTATUS GetNumMsiVectors(IN WDFDEVICE device, OUT PUSHORT numMsiVectors) {

    BUS_INTERFACE_STANDARD pciBus;
    PCI_COMMON_HEADER pciHeader;

    // get bus interface
    NTSTATUS status = WdfFdoQueryForInterface(device, &GUID_BUS_INTERFACE_STANDARD,
        (PINTERFACE)&pciBus, sizeof(BUS_INTERFACE_STANDARD), 1 /* Version */, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONG bytesRead = pciBus.GetBusData(pciBus.Context, PCI_WHICHSPACE_CONFIG, &pciHeader,
                                        0, PCI_COMMON_HDR_LENGTH);
    if (bytesRead != (ULONG)PCI_COMMON_HDR_LENGTH) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    UCHAR msiCapabilityOffset = FindCapability(&pciBus, &pciHeader, PCI_CAPABILITY_ID_MSI);
    if (msiCapabilityOffset == 0) {
        return STATUS_NOINTERFACE;

    }

    USHORT messageControl = 0;
    bytesRead = pciBus.GetBusData(pciBus.Context, PCI_WHICHSPACE_CONFIG, &messageControl,
                                        msiCapabilityOffset + sizeof(PCI_CAPABILITIES_HEADER), sizeof(USHORT));
    if (bytesRead != sizeof(USHORT)) {
        return STATUS_INTERNAL_ERROR;
    }

    *numMsiVectors = 1 << ((messageControl & 0x70) >> 4);
    return STATUS_SUCCESS;

}

/// Determine how many MSI-X message IDs are assigned by the OS.
static NTSTATUS GetNumMsixVectors(IN WDFDEVICE device, OUT PUSHORT numMsixVectors) {

    BUS_INTERFACE_STANDARD pciBus;
    PCI_COMMON_HEADER pciHeader;

    // get bus interface
    NTSTATUS status = WdfFdoQueryForInterface(device, &GUID_BUS_INTERFACE_STANDARD,
        (PINTERFACE)&pciBus, sizeof(BUS_INTERFACE_STANDARD), 1 /* Version */, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONG bytesRead = pciBus.GetBusData(pciBus.Context, PCI_WHICHSPACE_CONFIG, &pciHeader,
        0, PCI_COMMON_HDR_LENGTH);
    if (bytesRead != (ULONG)PCI_COMMON_HDR_LENGTH) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    UCHAR msixCapabilityOffset = FindCapability(&pciBus, &pciHeader, PCI_CAPABILITY_ID_MSIX);
    if (msixCapabilityOffset == 0) {
        return STATUS_NOINTERFACE;
    }

    USHORT messageControl = 0;
    bytesRead = pciBus.GetBusData(pciBus.Context, PCI_WHICHSPACE_CONFIG, &messageControl,
        msixCapabilityOffset + sizeof(PCI_CAPABILITIES_HEADER), sizeof(USHORT));
    if (bytesRead != sizeof(USHORT)) {
        return STATUS_INTERNAL_ERROR;
    }

    /* First 10 bits represents (n-1) vector table size */
    *numMsixVectors = (messageControl & 0x07FF) + 1;

    return STATUS_SUCCESS;

}

/// Determine which Legacy/Line Interrupt bin is used.
/// 1=A, 2=B, 3=C, 4=D
static NTSTATUS GetLineInterruptPin(IN WDFDEVICE device, OUT PUINT32 interruptPin) {
    BUS_INTERFACE_STANDARD pciBus;
    PCI_COMMON_HEADER pciHeader;

    // get bus interface
    NTSTATUS status = WdfFdoQueryForInterface(device, &GUID_BUS_INTERFACE_STANDARD,
        (PINTERFACE)&pciBus, sizeof(BUS_INTERFACE_STANDARD), 1 /* Version */, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONG bytesRead = pciBus.GetBusData(pciBus.Context, PCI_WHICHSPACE_CONFIG, &pciHeader,
                                        0, PCI_COMMON_HDR_LENGTH);
    if (bytesRead != (ULONG)PCI_COMMON_HDR_LENGTH) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    *interruptPin = pciHeader.u.type0.InterruptPin;
    
    return STATUS_SUCCESS;
}