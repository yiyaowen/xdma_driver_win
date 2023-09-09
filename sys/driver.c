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
-- Revision       : $Revision: #11 $
-- Date           : $DateTime: 2019/06/30 21:59:08 $
-- Last Author    : $Author: arayajig $
--
-------------------------------------------------------------------------------
-- Description :
-- This file is part of the Xilinx DMA IP Core driver for Windows.
--
-------------------------------------------------------------------------------
*/

// ========================= include dependencies =================================================

#include "driver.h"
#include "file_io.h"
#include "trace.h"

#ifdef DBG
// The trace message header (.tmh) file must be included in a source file before any WPP macro 
// calls and after defining a WPP_CONTROL_GUIDS macro (defined in trace.h). see trace.h
#include "driver.tmh"
#endif

// ========================= declarations ================================================= 

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD           EvtDeviceAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      EvtDeviceCleanup;
EVT_WDF_DEVICE_PREPARE_HARDWARE     EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     EvtDeviceReleaseHardware;

static NTSTATUS EngineCreateQueue(WDFDEVICE device, XDMA_ENGINE* engine, WDFQUEUE* queue);

// Mark these functions as pageable code
#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, DriverUnload)
#pragma alloc_text (PAGE, EvtDeviceAdd)
#pragma alloc_text (PAGE, EvtDevicePrepareHardware)
#pragma alloc_text (PAGE, EvtDeviceReleaseHardware)
#pragma alloc_text (PAGE, EngineCreateQueue)
#endif

// ========================= definitions =================================================

const char * const dateTimeStr = "Built " __DATE__ ", " __TIME__ ".";

// Get the driver parameter for POLL_MODE from the Windows registry 
static NTSTATUS GetPollModeParameter(IN PULONG pollMode) {
    WDFDRIVER driver = WdfGetDriver();
    WDFKEY key;
    NTSTATUS status = WdfDriverOpenParametersRegistryKey(driver, STANDARD_RIGHTS_ALL,
                                                         WDF_NO_OBJECT_ATTRIBUTES, &key);
    ULONG tracepollmode;
    
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "WdfDriverOpenParametersRegistryKey failed: %!STATUS!", status);
        WdfRegistryClose(key);
        return status;
    }

    DECLARE_CONST_UNICODE_STRING(valueName, L"POLL_MODE");

    status = WdfRegistryQueryULong(key, &valueName, pollMode);
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "WdfRegistryQueryULong failed: %!STATUS!", status);
        WdfRegistryClose(key);
        return status;
    }

    tracepollmode = *pollMode;
    TraceVerbose(DBG_INIT, "pollMode=%u", tracepollmode);

    WdfRegistryClose(key);
    return status;
}

// main entry point - Called when driver is installed
NTSTATUS DriverEntry(IN PDRIVER_OBJECT driverObject, IN PUNICODE_STRING registryPath) {
    NTSTATUS			status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG	DriverConfig;
    WDFDRIVER			Driver;

    // Initialize WPP Tracing
    WPP_INIT_TRACING(driverObject, registryPath);
    TraceInfo(DBG_INIT, "XDMA Driver - %s", dateTimeStr);

    // Initialize the Driver Config; register the device add event callback
    // EvtDeviceAdd() will be called when a device is found
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, EvtDeviceAdd);

	/*isWheaEnabled = PshedIsSystemWheaEnabled();
	if (isWheaEnabled == FALSE)
	{
		TraceInfo(DBG_INIT, "PSHED Enabled: %!STATUS!", status);
		status = STATUS_SUCCESS;
	}
	else {
		TraceInfo(DBG_INIT, "PSHED failed: %!STATUS!", status);
	}*/


    // Creates a WDFDRIVER object, the top of our device's tree of objects
    status = WdfDriverCreate(driverObject, registryPath, WDF_NO_OBJECT_ATTRIBUTES, &DriverConfig,
                             &Driver);
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "WdfDriverCreate failed: %!STATUS!", status);
        WPP_CLEANUP(driverObject);
        return status;
    }

	driverObject->DriverUnload = DriverUnload;
    return status;
}

// Called before the driver is removed
VOID DriverUnload(IN PDRIVER_OBJECT driverObject) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(driverObject);
    TraceVerbose(DBG_INIT, "%!FUNC!");

    WPP_CLEANUP(driverObject); // cleanup tracing

    return;
}

NTSTATUS EvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit) {
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    TraceVerbose(DBG_INIT, "(Driver=0x%p)", Driver);

    //  We prefer Direct I/O
    //  Direct I/O only works with deferred buffer retrieval No guarantee that Direct I/O is
    //  actually used Direct I/O is only used for buffers that are full pages Buffered I/O is used
    //  for other parts of the transfer
	    
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    // Set call-backs for any of the functions we are interested in. If no call-back is set, the 
    // framework will take the default action by itself.
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
    PnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    PnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPowerCallbacks);

    WDF_POWER_POLICY_EVENT_CALLBACKS powerPolicyCallbacks;
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&powerPolicyCallbacks);
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &powerPolicyCallbacks);

    // Register file object call-backs
    WDF_OBJECT_ATTRIBUTES fileAttributes;
    WDF_FILEOBJECT_CONFIG fileConfig;
    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig, EvtDeviceFileCreate, EvtFileClose, EvtFileCleanup);
    WDF_OBJECT_ATTRIBUTES_INIT(&fileAttributes);
    fileAttributes.SynchronizationScope = WdfSynchronizationScopeNone;
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&fileAttributes, FILE_CONTEXT);
    WdfDeviceInitSetFileObjectConfig(DeviceInit, &fileConfig, &fileAttributes);
    WdfDeviceInitSetIoInCallerContextCallback(DeviceInit, EvtDeviceIoInCallerContext);

    // Specify the context type and size for the device we are about to create.
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DeviceContext);
    // ContextCleanup will be called by the framework when it deletes the device. So you can defer
    // freeing any resources allocated to Cleanup callback in the event EvtDeviceAdd returns any 
    // error after the device is created.
    deviceAttributes.EvtCleanupCallback = EvtDeviceCleanup;
    WDFDEVICE device;
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
		TraceError(DBG_INIT, "WdfDeviceCreate failed: %!STATUS!", status);
        return status;
    }

    // Create a user-space device interface
    status = WdfDeviceCreateDeviceInterface(device, (LPGUID)&GUID_DEVINTERFACE_XDMA, NULL);
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
        return status;
    }

	WDF_OBJECT_ATTRIBUTES attribs;
	WDF_OBJECT_ATTRIBUTES_INIT(&attribs);
	attribs.SynchronizationScope = WdfSynchronizationScopeNone;
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attribs, QUEUE_CONTEXT);

    // create the default queue upon all I/O requests arrive
    // accept multiple I/O request to run in parallel, they are sequentialized later
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl; // callback handler for control requests
    queueConfig.EvtIoRead = EvtIoRead; // callback handler for read requests
    queueConfig.EvtIoWrite = EvtIoWrite; // callback handler for write requests
    WDFQUEUE entryQueue;
    status = WdfIoQueueCreate(device, &queueConfig, &attribs, &entryQueue);
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "WdfIoQueueCreate failed: %!STATUS!", status);
        return status;
    }

    TraceVerbose(DBG_INIT, "returns %!STATUS!", status);
    return status;
}

// Any device specific cleanup  - TODO device reset?
VOID EvtDeviceCleanup(IN WDFOBJECT device) {
    UNREFERENCED_PARAMETER(device);
    TraceInfo(DBG_INIT, "%!FUNC!");
}

// Initialize device hardware and host buffers.
// Called by plug and play manager
NTSTATUS EvtDevicePrepareHardware(IN WDFDEVICE device, IN WDFCMRESLIST Resources,
                                  IN WDFCMRESLIST ResourcesTranslated) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Resources);
    TraceVerbose(DBG_INIT, "-->Entry");

    DeviceContext* ctx = GetDeviceContext(device);
    PXDMA_DEVICE xdma = &(ctx->xdma);
    ULONG userMax = 0;
    ULONG h2cChannelMax = 0;
    ULONG c2hChannelMax = 0;

    NTSTATUS status = XDMA_DeviceOpen(device, xdma, &userMax, &h2cChannelMax, &c2hChannelMax,
                                      Resources, ResourcesTranslated);
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "XDMA_DeviceOpen failed: %!STATUS!", status);
        return status;
    }

    // get poll mode parameter and configure engines as poll mode if needed
    ULONG pollMode = 0;
    status = GetPollModeParameter(&pollMode);
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "GetPollModeParameter failed: %!STATUS!", status);
        return status;
    }
    for (UINT dir = H2C; dir < 2; dir++) { // 0=H2C, 1=C2H
        for (ULONG ch = 0; ch < XDMA_MAX_NUM_CHANNELS; ch++) {
            XDMA_ENGINE* engine = &(xdma->engines[ch][dir]);
            XDMA_EngineSetPollMode(engine, (BOOLEAN)pollMode);
        }
    }

    // create a queue for each engine
    for (UINT dir = H2C; dir < 2; dir++) { // 0=H2C, 1=C2H
        for (ULONG ch = 0; ch < XDMA_MAX_NUM_CHANNELS; ch++) {
            XDMA_ENGINE* engine = &(xdma->engines[ch][dir]);
            if (engine->enabled == TRUE) {
                status = EngineCreateQueue(device, engine, &(ctx->engineQueue[dir][ch]));
                if (!NT_SUCCESS(status)) {
                    TraceError(DBG_INIT, "EngineCreateQueue() failed: %!STATUS!", status);
                    return status;
                }
            }
        }
    }

    for (UINT i = 0; i < XDMA_MAX_USER_IRQ; ++i) {
        KeInitializeEvent(&ctx->eventSignals[i], NotificationEvent, FALSE);
        XDMA_UserIsrRegister(xdma, i, HandleUserEvent, &ctx->eventSignals[i]);
    }

    TraceVerbose(DBG_INIT, "<--Exit returning %!STATUS!", status);
    return status;
}

// Unmap PCIe resources
NTSTATUS EvtDeviceReleaseHardware(IN WDFDEVICE Device, IN WDFCMRESLIST ResourcesTranslated) {

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ResourcesTranslated);
    TraceVerbose(DBG_INIT, "DeviceReleaseHw");
    
    DeviceContext* ctx = GetDeviceContext(Device);
    if (ctx != NULL) {
        XDMA_DeviceClose(&ctx->xdma);
    }

    TraceVerbose(DBG_INIT, "exit");
    return STATUS_SUCCESS;
}

NTSTATUS EngineCreateQueue(WDFDEVICE device, XDMA_ENGINE* engine, WDFQUEUE* queue)
// Create a WDF IO queue for a DMA engine
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG config;
    WDF_OBJECT_ATTRIBUTES attribs;
    PQUEUE_CONTEXT context;

    PAGED_CODE();

    // engine queue is sequential
    WDF_IO_QUEUE_CONFIG_INIT(&config, WdfIoQueueDispatchSequential);

    ASSERTMSG("direction is neither H2C nor C2H!", (engine->dir == C2H) || (engine->dir == H2C));
    if (engine->dir == H2C) { // callback handler for write requests
        config.EvtIoWrite = EvtIoWriteDma;
        TraceInfo(DBG_INIT, "EvtIoWrite=EvtIoWriteDma");
    } else if (engine->dir == C2H) { // callback handler for read requests

        if (engine->type == EngineType_ST) {
            config.EvtIoRead = EvtIoReadEngineRing;
            TraceInfo(DBG_INIT, "EvtIoRead=EvtIoReadEngineRing");
        } else {
            config.EvtIoRead = EvtIoReadDma;
            TraceInfo(DBG_INIT, "EvtIoRead=EvtIoReadDma");
        }
    }

    // serialize all callbacks related to this queue. see ref [2]
    WDF_OBJECT_ATTRIBUTES_INIT(&attribs);
    attribs.SynchronizationScope = WdfSynchronizationScopeQueue;
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attribs, QUEUE_CONTEXT);
    status = WdfIoQueueCreate(device, &config, &attribs, queue);
    if (!NT_SUCCESS(status)) {
        TraceError(DBG_INIT, "WdfIoQueueCreate failed %d", status);
        return status;
    }

    // store arguments into queue context
    context = GetQueueContext(*queue);
    context->engine = engine;

    return status;
}
