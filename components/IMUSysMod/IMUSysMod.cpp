////////////////////////////////////////////////////////////////////////////////
//
// IMUSysMod.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include "IMUSysMod.h"
#include "RaftUtils.h"
#include "BusI2C.h"
#include "SysManager.h"
#include "DevicePollRecords_generated.h"

static const char *MODULE_PREFIX = "IMUSysMod";

// Uncomment one of the following line to decode off-device or on-device
// or leave them both commented to not decode at all
// #define DECODE_OFF_DEVICE
// #define DECODE_ON_DEVICE

IMUSysMod::IMUSysMod(const char *pModuleName, RaftJsonIF& sysConfig)
    : RaftSysMod(pModuleName, sysConfig)
{
}

IMUSysMod::~IMUSysMod()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief setup function
void IMUSysMod::setup()
{
    // Register BusI2C
    _raftBusSystem.registerBus("I2C", BusI2C::createFn);

    // Setup buses
    _raftBusSystem.setup("Buses", modConfig(),
            std::bind(&IMUSysMod::busElemStatusCB, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&IMUSysMod::busOperationStatusCB, this, std::placeholders::_1, std::placeholders::_2)
    );

    // Register data source (message generator and state detector functions)
    getSysManager()->registerDataSource("Publish", "IMU", 
        [this](const char* messageName, CommsChannelMsg& msg) {
            String statusStr = getStatusJSON();
            msg.setFromBuffer((uint8_t*)statusStr.c_str(), statusStr.length());
            return true;
        },
        [this](const char* messageName, std::vector<uint8_t>& stateHash) {
            return getStatusHash(stateHash);
        }
    );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief loop function
void IMUSysMod::loop()
{
    // Service the buses
    _raftBusSystem.loop();

#ifdef DECODE_OFF_DEVICE

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

#endif

#ifdef DECODE_ON_DEVICE

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
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Get status as JSON
/// @return JSON string
String IMUSysMod::getStatusJSON() const
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
    // LOG_I(MODULE_PREFIX, "getStatusJSON %s", jsonStr.c_str());

    return jsonStr.length() == 0 ? "{}" : jsonStr + "}";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Check for change of state
/// @param stateHash hash of the current state
void IMUSysMod::getStatusHash(std::vector<uint8_t>& stateHash) const
{
    stateHash.clear();

    // Check all buses for data
    for (RaftBus* pBus : _raftBusSystem.getBusList())
    {
        // Check bus
        if (pBus)
        {
            // Check bus status
            uint32_t identPollLastMs = pBus->getLastStatusUpdateMs(true, true);
            stateHash.push_back(identPollLastMs & 0xff);
            stateHash.push_back((identPollLastMs >> 8) & 0xff);
        }
    }

    // Debug
    // LOG_I(MODULE_PREFIX, "getStatusHash %s", stateHash.size() == 0 ? "No data" : ("Data " + String(stateHash[0])).c_str());
}

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
        LOG_I(MODULE_PREFIX, "busElemStatusInfo %s %s %s %s", bus.getBusName().c_str(), 
            bus.addrToString(el.address).c_str(), el.isChangeToOnline ? "Online" : ("Offline" + String(el.isChangeToOffline ? " (was online)" : "")).c_str(),
            el.isNewlyIdentified ? ("DevTypeIdx " + String(el.deviceTypeIndex)).c_str() : "");
    }
}
