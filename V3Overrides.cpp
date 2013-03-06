//
//  V3Overrides.cpp
//  GenericUSBXHCI
//
//  Created by Zenith432 on December 26th, 2012.
//  Copyright (c) 2012-2013 Zenith432. All rights reserved.
//

#include "GenericUSBXHCI.h"
#include "XHCITypes.h"

#define CLASS GenericUSBXHCI
#define super IOUSBControllerV3

#pragma mark -
#pragma mark IOUSBControllerV3 Overrides
#pragma mark -

IOReturn CLASS::GetRootHubBOSDescriptor(OSData* desc)
{
	static
	struct {
		IOUSBBOSDescriptor osDesc;
		IOUSBDeviceCapabilityUSB2Extension dcu2eDesc;
		IOUSBDeviceCapabilitySuperSpeedUSB dcssuDesc;
		IOUSBDeviceCapabilityContainerID dcciDesc;
	} __attribute__((packed)) const allDesc = {
		{
			sizeof(IOUSBBOSDescriptor),
			kUSBBOSDescriptor,
			HostToUSBWord(sizeof(IOUSBBOSDescriptor) +
						  sizeof(IOUSBDeviceCapabilityUSB2Extension) +
						  sizeof(IOUSBDeviceCapabilitySuperSpeedUSB) +
						  sizeof(IOUSBDeviceCapabilityContainerID)),
			3U,
		},
		{
			sizeof(IOUSBDeviceCapabilityUSB2Extension),
			kUSBDeviceCapability,
			kUSBDeviceCapabilityUSB20Extension,
			HostToUSBLong(2U),
		},
		{
			sizeof(IOUSBDeviceCapabilitySuperSpeedUSB),
			kUSBDeviceCapability,
			kUSBDeviceCapabilitySuperSpeedUSB,
			2U,
			HostToUSBWord(12U),
			8U,
			10U,
			HostToUSBWord(100U),
		},
		{
			sizeof(IOUSBDeviceCapabilityContainerID),
			kUSBDeviceCapability,
			kUSBDeviceCapabilityContainerID,
			0U,
			{ 0xDF, 0xFB, 0x1E, 0x9E, 0x1B, 0x1D, 0x10, 0x46, 0xAE, 0x91, 0x3B, 0x66, 0xF5, 0x56, 0x1D, 0x0A },
		},
	};
	if (!desc || desc->appendBytes(&allDesc, sizeof allDesc))
		return(kIOReturnNoMemory);
	return kIOReturnSuccess;
}

IOReturn CLASS::GetRootHub3Descriptor(IOUSB3HubDescriptor* desc)
{
	IOUSB3HubDescriptor hubDesc;
	uint32_t appleCaptive, i, numBytes;
	uint8_t* dstPtr;
	OSNumber* appleCaptiveProperty;

	hubDesc.length = sizeof(hubDesc);
	hubDesc.hubType = kUSB3HubDescriptorType;
	hubDesc.numPorts = _v3ExpansionData->_rootHubNumPortsSS;
	hubDesc.characteristics = XHCI_HCC_PPC(_HCCLow) ? kPerPortSwitchingBit : 0U;
	hubDesc.powerOnToGood = 50U;
	hubDesc.hubCurrent = 0U;
	hubDesc.hubHdrDecLat = 0U;
	hubDesc.hubDelay = 10U;
	numBytes = ((hubDesc.numPorts + 1U) / 8U) + 1U;
	appleCaptiveProperty = OSDynamicCast(OSNumber, _device->getProperty(kAppleInternalUSBDevice));
	appleCaptive = appleCaptiveProperty ? appleCaptiveProperty->unsigned32BitValue() : 0U;
	dstPtr = &hubDesc.removablePortFlags[0];
	for (i = 0U; i < numBytes; i++) {
		*dstPtr++ = (uint8_t) (appleCaptive & 0xFFU);
        appleCaptive >>= 8;
	}
    for (i = 0U; i < numBytes; i++) {
        *dstPtr++ = 0xFFU;
    }
	if (!desc)
		return kIOReturnNoMemory;
	bcopy(&hubDesc, desc, hubDesc.length);
	return kIOReturnSuccess;
}

IOReturn CLASS::UIMDeviceToBeReset(short functionAddress)
{
	if (functionAddress == _hub3Address || functionAddress == _hub2Address) {
		if (gux_log_level >= 2)
			IOLog("%s: got request to reset root hub %d\n", __FUNCTION__, functionAddress);
		return kIOReturnSuccess;
	}
	uint8_t slot = GetSlotID(functionAddress);
	if (!slot)
		return kIOReturnBadArgument;
	if (_errataBits & kErrataIntelPantherPoint)
		SetIntelFlag(slot, false);
	return kIOReturnSuccess;
}

IOReturn CLASS::UIMAbortStream(UInt32 streamID, short functionNumber, short endpointNumber, short direction)
{
	IOReturn rc1, rc2;
	uint16_t lastStream;
	uint8_t slot, endpoint;

	if (functionNumber == _hub3Address || functionNumber == _hub2Address) {
		if (streamID != UINT32_MAX)
			return kIOReturnInternalError;
		if (endpointNumber) {
			if (endpointNumber != 1)
				return kIOReturnBadArgument;
			if (direction != kUSBIn)
				return kIOReturnInternalError;
			RootHubAbortInterruptRead();
		}
		return kIOReturnSuccess;
	}
	if (gux_log_level >= 2)
		IOLog("%s(%#x, %d, %d, %d)\n", __FUNCTION__, streamID, functionNumber, endpointNumber, direction);
	slot = GetSlotID(functionNumber);
	if (!slot)
		return kIOReturnInternalError;
	endpoint = TranslateEndpoint(endpointNumber, direction);
	if (!endpoint || endpoint >= kUSBMaxPipes)
		return kIOReturnBadArgument;
	if (streamID == UINT32_MAX) {
		QuiesceEndpoint(slot, endpoint);
	} else if (!streamID) {
		if (IsStreamsEndpoint(slot, endpoint))
			return kIOReturnBadArgument;
		QuiesceEndpoint(slot, endpoint);
		return ReturnAllTransfersAndReinitRing(slot, endpoint, streamID);
	} else {
		if (streamID > GetLastStreamForEndpoint(slot, endpoint))
			return kIOReturnBadArgument;
		uint32_t ret = QuiesceEndpoint(slot, endpoint);
		rc1 = ReturnAllTransfersAndReinitRing(slot, endpoint, streamID);
		if (ret == 1U)
			RestartStreams(slot, endpoint, streamID);
		return rc1;
	}
	if (!IsStreamsEndpoint(slot, endpoint))
		return ReturnAllTransfersAndReinitRing(slot, endpoint, 0U);
	lastStream = GetLastStreamForEndpoint(slot, endpoint);
	rc1 = kIOReturnSuccess;
	for (uint16_t streamId = 1U; streamId <= lastStream; ++streamId) {
		rc2 = ReturnAllTransfersAndReinitRing(slot, endpoint, streamId);
		if (rc2 != kIOReturnSuccess)
			rc1 = rc2;
	}
	return rc1;
}

UInt32 CLASS::UIMMaxSupportedStream(void)
{
	if (!_maxPSASize)
		return 0U;
	return _maxPSASize < kMaxStreamsAllowed ? (_maxPSASize - 1U) : (kMaxStreamsAllowed - 1U);
}

USBDeviceAddress CLASS::UIMGetActualDeviceAddress(USBDeviceAddress current)
{
	USBDeviceAddress addr;
	ContextStruct* pContext;
	uint8_t slot = GetSlotID(current);
	if (!slot)
		return 0U;
	pContext = GetSlotContext(slot);
	if (!pContext)
		return 0U;
	addr = static_cast<uint16_t>(XHCI_SCTX_3_DEV_ADDR_GET(pContext->_s.dwSctx3));
	if (addr == current || _addressMapper.Active[addr])
		return current;
	_addressMapper.HubAddress[addr] = _addressMapper.HubAddress[current];
	_addressMapper.PortOnHub[addr] = _addressMapper.PortOnHub[current];
	_addressMapper.Slot[addr] = _addressMapper.Slot[current];
	_addressMapper.Active[addr] = _addressMapper.Active[current];
	_addressMapper.HubAddress[current] = 0U;
	_addressMapper.PortOnHub[current] = 0U;
	_addressMapper.Slot[current] = 0U;
	_addressMapper.Active[current] = false;
	return addr;
}

IOReturn CLASS::UIMCreateSSBulkEndpoint(UInt8 functionNumber, UInt8 endpointNumber, UInt8 direction, UInt8 speed,
										UInt16 maxPacketSize, UInt32 maxStream, UInt32 maxBurst)
{
	if (maxStream) {
		if (!_maxPSASize)
			return kIOUSBStreamsNotSupported;
		if (maxStream >= kMaxStreamsAllowed)
			return kIOReturnBadArgument;
	}
	if (maxStream & (maxStream + 1U))	// Note: checks if (maxStream + 1U) is a power of 2
		return kIOReturnBadArgument;
	return CreateBulkEndpoint(functionNumber, endpointNumber, direction, 0U, maxPacketSize, 0U, 0, maxStream, maxBurst);
}

IOReturn CLASS::UIMCreateSSInterruptEndpoint(short functionAddress, short endpointNumber, UInt8 direction, short speed,
											 UInt16 maxPacketSize, short pollingRate, UInt32 maxBurst)
{
	return CreateInterruptEndpoint(functionAddress, endpointNumber, direction, speed, maxPacketSize,
								   pollingRate, 0U, 0, maxBurst);
}

IOReturn CLASS::UIMCreateSSIsochEndpoint(short functionAddress, short endpointNumber, UInt32 maxPacketSize, UInt8 direction,
										 UInt8 interval, UInt32 maxBurst)
{
	return CreateIsochEndpoint(functionAddress, endpointNumber, maxPacketSize, direction, interval, maxBurst);
}

IOReturn CLASS::UIMCreateStreams(UInt8 functionNumber, UInt8 endpointNumber, UInt8 direction, UInt32 maxStream)
{
	if (!_maxPSASize)
		return kIOUSBStreamsNotSupported;
	uint8_t slot = GetSlotID(functionNumber);
	if (!slot)
		return kIOReturnInternalError;
	SlotStruct* pSlot = SlotPtr(slot);
	uint8_t endpoint = TranslateEndpoint(endpointNumber, direction);
	if (endpoint < 2U || endpoint >= kUSBMaxPipes)
		return kIOReturnBadArgument;
	if (pSlot->lastStreamForEndpoint[endpoint])
		return maxStream ? kIOReturnBadArgument : kIOReturnInternalError;
	if (!IsStreamsEndpoint(slot, endpoint))
		return kIOReturnBadArgument;
	if (maxStream < 2U)
		return kIOReturnBadArgument;
	pSlot->lastStreamForEndpoint[endpoint] = static_cast<uint16_t>(maxStream);
	for (uint16_t streamId = 1U; streamId <= maxStream; ++streamId) {
		IOReturn rc = CreateStream(slot, endpoint, streamId);
		if (rc != kIOReturnSuccess) {
			pSlot->lastStreamForEndpoint[endpoint] = 0U;
			return rc;
		}
	}
	return kIOReturnSuccess;
}

IOReturn CLASS::GetRootHubPortErrorCount(UInt16 port, UInt16* count)
{
	if (!count)
		return kIOReturnBadArgument;
	uint16_t _port = PortNumberProtocolToCanonical(port, kUSBDeviceSpeedSuper);
	if (_port >= _rootHubNumPorts)
		return kIOReturnBadArgument;
	uint32_t portLi = Read32Reg(&_pXHCIOperationalRegisters->prs[_port].PortLi);
	if (m_invalid_regspace)
		return kIOReturnNoDevice;
	*count = static_cast<uint16_t>(portLi);
	return kIOReturnSuccess;
}

IOReturn CLASS::GetBandwidthAvailableForDevice(IOUSBDevice* forDevice, UInt32* pBandwidthAvailable)
{
	ContextStruct* pContext;
	size_t siz;
	IOReturn rc;
	uint32_t ret;
	USBDeviceAddress addr, hubAddress;
	uint8_t slot, hubSlot, portOnHub, speed;
	uint8_t buffer[256];

	if (!forDevice || !pBandwidthAvailable)
		return kIOReturnBadArgument;
	addr = forDevice->GetAddress();
	slot = GetSlotID(addr);
	if (!slot)
		return kIOReturnNoDevice;
	hubAddress = _addressMapper.HubAddress[addr];
	portOnHub = _addressMapper.PortOnHub[addr];
	if (hubAddress == _hub3Address || hubAddress == _hub2Address)
		hubSlot = 0U;
	else {
		hubSlot = GetSlotID(hubAddress);
		if (!hubSlot)
			return kIOReturnInternalError;
	}
	pContext = GetSlotContext(slot);
	if (!pContext)
		return kIOReturnInternalError;
	speed = GetSlCtxSpeed(pContext);
	siz = sizeof buffer;
	rc = GetPortBandwidth(hubSlot, speed, &buffer[0], &siz);
	if (rc != kIOReturnSuccess) {
		IOLog("%s: GetPortBandwidth(%u, %u, ...) failed, returning %#x\n", __FUNCTION__, hubSlot, speed, rc);
		return rc;
	}
	ret = buffer[static_cast<uint8_t>(portOnHub - 1U)];
	/*
	 * Note: These are just rough estimates.  The exact
	 *   number of bytes per microframe depend on packetization
	 *   overhead, symbol encoding (e.g. 8b/10b for SS) and
	 *   scheduling.
	 */
	switch (speed) {
		case kUSBDeviceSpeedLow:
			*pBandwidthAvailable = 234375U * ret / 1000000U;	// based on 23.4375 bytes/microframe
			break;
		case kUSBDeviceSpeedFull:
			*pBandwidthAvailable = 1875U * ret / 1000U;	// based on 187.5 bytes/microframe
			break;
		case kUSBDeviceSpeedHigh:
			*pBandwidthAvailable = 75U * ret;	// based on 7500 bytes/microframe
			break;
		case kUSBDeviceSpeedSuper:
			*pBandwidthAvailable = 78125U * ret / 100U;	// based on 78125 bytes/microframe
			break;
		default:
			*pBandwidthAvailable = 0U;
			break;
	}
	return kIOReturnSuccess;
}
