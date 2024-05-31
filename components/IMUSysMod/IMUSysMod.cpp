////////////////////////////////////////////////////////////////////////////////
//
// IMUSysMod.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include "IMUSysMod.h"
#include "RaftUtils.h"
#include "BusI2C.h"
#include "DeviceTypeRecords_generated.h"

static const char *MODULE_PREFIX = "IMUSysMod";

// Comment this line to decode on-device
// #define DECODE_OFF_DEVICE

IMUSysMod::IMUSysMod(const char *pModuleName, RaftJsonIF& sysConfig)
    : RaftSysMod(pModuleName, sysConfig)
{
}

IMUSysMod::~IMUSysMod()
{
}

void IMUSysMod::setup()
{
    // Register BusI2C
    _raftBusSystem.registerBus("I2C", BusI2C::createFn);

    // Setup buses
    _raftBusSystem.setup("Buses", modConfig(),
            std::bind(&IMUSysMod::busElemStatusCB, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&IMUSysMod::busOperationStatusCB, this, std::placeholders::_1, std::placeholders::_2)
    );    
}

#ifdef DECODE_OFF_DEVICE

void IMUSysMod::loop()
{
    // Service the buses
    _raftBusSystem.loop();

    // Check if we should report device data (100ms intervals)
    if (Raft::isTimeout(millis(), _debugLastReportTimeMs, 100))
    {
        String jsonStr;
        for (RaftBus* pBus : _raftBusSystem.getBusList())
        {
            if (!pBus)
                continue;

            // Get device interface
            RaftBusDevicesIF* pDevicesIF = pBus->getBusDevicesIF();
            if (!pDevicesIF)
                continue; 
            String jsonRespStr = pDevicesIF->getPollResponsesJson();
            if (jsonRespStr.length() > 0)
            {
                jsonStr += (jsonStr.length() == 0 ? "{\"" : ",\"") + pBus->getBusName() + "\":" + jsonRespStr;
            }
        }

        LOG_I(MODULE_PREFIX, "loop %s", (jsonStr.length() == 0 ? "{}" : (jsonStr + "}").c_str()));
        _debugLastReportTimeMs = millis();
    }
}

#else

void IMUSysMod::loop()
{
    // Service the buses
    _raftBusSystem.loop();

    // Check if we should report device data (100ms intervals)
    if (Raft::isTimeout(millis(), _debugLastReportTimeMs, 100))
    {
        for (RaftBus* pBus : _raftBusSystem.getBusList())
        {
            if (!pBus)
                continue;

            // Get device interface
            RaftBusDevicesIF* pDevicesIF = pBus->getBusDevicesIF();
            if (!pDevicesIF)
                continue; 

            // Get list of devices that have data
            std::vector<uint32_t> addresses;
            pDevicesIF->getDeviceAddresses(addresses, true);

            // Loop through devices
            for (uint32_t address : addresses)
            {
                // Since there is only one device we're just going to get the data
                // in the format for the LSM6DS3
                std::vector<poll_LSM6DS3> pollResponses;
                pollResponses.resize(2);

                // Get decoded poll responses
                uint32_t numDecoded = pDevicesIF->getDecodedPollResponses(address, pollResponses.data(), 
                            sizeof(poll_LSM6DS3)*pollResponses.size(),
                            pollResponses.size(), _decodeState);

                // Log the decoded data
                for (uint32_t i = 0; i < numDecoded; i++)
                {
                    auto& pollResponse = pollResponses[i];
                    LOG_I(MODULE_PREFIX, "loop time %lu addr %s ax %.2fg ay %.2fg az %.2fg gx %.2fdps gy %.2fdps gz %.2fdps",
                        pollResponse.timeMs, pBus->addrToString(address).c_str(),
                        pollResponse.ax, pollResponse.ay, pollResponse.az,
                        pollResponse.gx, pollResponse.gy, pollResponse.gz
                    );
                }
            }
        }
        _debugLastReportTimeMs = millis();
    }
}

#endif

/// @brief Bus operation status callback
/// @param bus
/// @param busOperationStatus - indicates bus ok/failing
void IMUSysMod::busOperationStatusCB(RaftBus& bus, BusOperationStatus busOperationStatus)
{
    // Debug
    LOG_I(MODULE_PREFIX, "busOperationStatusInfo %s %s", bus.getBusName().c_str(), 
        RaftBus::busOperationStatusToString(busOperationStatus));
}

/// @brief Bus element status callback
/// @param bus 
/// @param statusChanges - an array of status changes (online/offline) for bus elements
void IMUSysMod::busElemStatusCB(RaftBus& bus, const std::vector<BusElemAddrAndStatus>& statusChanges)
{
    // Debug
    for (const auto& el : statusChanges)
    {
        LOG_I(MODULE_PREFIX, "busElemStatusInfo %s %s %s", bus.getBusName().c_str(), 
            bus.addrToString(el.address).c_str(), el.isChangeToOnline ? "Online" : ("Offline" + String(el.isChangeToOffline ? " (was online)" : "")).c_str());
    }
}
