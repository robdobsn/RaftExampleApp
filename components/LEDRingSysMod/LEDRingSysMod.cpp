////////////////////////////////////////////////////////////////////////////////
//
// LEDRingSysMod.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include "LEDRingSysMod.h"
#include "RaftUtils.h"
#include "RaftJson.h"
#include "RestAPIEndpointManager.h"
#include "LEDPatternRainbowSnake.h"

#define DEBUG_LED_GRID_SETUP

static const char *MODULE_PREFIX = "LEDRingSysMod";

LEDRingSysMod::LEDRingSysMod(const char *pModuleName, RaftJsonIF& sysConfig)
    : RaftSysMod(pModuleName, sysConfig)
{
}

LEDRingSysMod::~LEDRingSysMod()
{
}

void LEDRingSysMod::setup()
{
    // Add patterns
    _ledPixels.addPattern("RainbowSnake", &LEDPatternRainbowSnake::create);

    // Setup LED Pixels
    bool rslt = _ledPixels.setup(config);

    // Log
#ifdef DEBUG_LED_GRID_SETUP
    LOG_I(MODULE_PREFIX, "setup %s numPixels %d", 
                rslt ? "OK" : "FAILED", _ledPixels.getNumPixels());
#endif
}

void LEDRingSysMod::loop()
{
    _ledPixels.loop();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Endpoints
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LEDRingSysMod::addRestAPIEndpoints(RestAPIEndpointManager &endpointManager)
{
    // Control shade
    endpointManager.addEndpoint("ledring", RestAPIEndpoint::ENDPOINT_CALLBACK, RestAPIEndpoint::ENDPOINT_GET,
                            std::bind(&LEDRingSysMod::apiControl, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                            "ledring?pattern=<pattern>");
    LOG_I(MODULE_PREFIX, "addRestAPIEndpoints ledring");
}

RaftRetCode LEDRingSysMod::apiControl(const String &reqStr, String &respStr, const APISourceInfo& sourceInfo)
{
    // Extract parameters
    std::vector<String> params;
    std::vector<RaftJson::NameValuePair> nameValues;
    RestAPIEndpointManager::getParamsAndNameValues(reqStr.c_str(), params, nameValues);
    RaftJson nameValueParamsJson = RaftJson::getJSONFromNVPairs(nameValues, true);

    // Get pattern
    String pattern = nameValueParamsJson.getString("pattern", "");

    // Set pattern
    _ledPixels.setPattern(pattern, nameValueParamsJson.c_str());

    // Debug
    LOG_I(MODULE_PREFIX, "apiControl %s JSON %s", reqStr.c_str(), nameValueParamsJson.c_str());

    // Return result
    bool rslt = true;
    return Raft::setJsonBoolResult(reqStr.c_str(), respStr, rslt);
}
