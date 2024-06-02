////////////////////////////////////////////////////////////////////////////////
//
// IMUSysMod.h
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RaftArduino.h"
#include "RaftSysMod.h"
#include "RaftBusSystem.h"

class IMUSysMod : public RaftSysMod
{
public:
    IMUSysMod(const char *pModuleName, RaftJsonIF& sysConfig);
    virtual ~IMUSysMod();

    // Create function (for use by SysManager factory)
    static RaftSysMod* create(const char* pModuleName, RaftJsonIF& sysConfig)
    {
        return new IMUSysMod(pModuleName, sysConfig);
    }

protected:

    // Setup
    virtual void setup() override final;

    // Loop (called frequently)
    virtual void loop() override final;

    // Status
    virtual String getStatusJSON() const override final;

private:
    // Bus manager
    RaftBusSystem _raftBusSystem;

    // Bus operation and status functions
    void busElemStatusCB(RaftBus& bus, const std::vector<BusElemAddrAndStatus>& statusChanges);
    void busOperationStatusCB(RaftBus& bus, BusOperationStatus busOperationStatus);

    // Hash of status
    void getStatusHash(std::vector<uint8_t>& stateHash) const;

    // Last report time
    uint32_t _debugLastReportTimeMs = 0;

    // Decode state
    RaftBusDeviceDecodeState _decodeState;
};
