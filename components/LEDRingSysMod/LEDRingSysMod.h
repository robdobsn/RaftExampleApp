////////////////////////////////////////////////////////////////////////////////
//
// LEDRingSysMod.h
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RaftArduino.h"
#include "RaftSysMod.h"
#include "LEDPixels.h"

class APISourceInfo;

class LEDRingSysMod : public RaftSysMod
{
public:
    LEDRingSysMod(const char *pModuleName, RaftJsonIF& sysConfig);
    virtual ~LEDRingSysMod();

    // Create function (for use by SysManager factory)
    static RaftSysMod* create(const char* pModuleName, RaftJsonIF& sysConfig)
    {
        return new LEDRingSysMod(pModuleName, sysConfig);
    }

protected:

    // Setup
    virtual void setup() override final;

    // Loop (called frequently)
    virtual void loop() override final;

    // Add endpoints
    virtual void addRestAPIEndpoints(RestAPIEndpointManager& pEndpoints) override final;

private:

    // LED pixels
    LEDPixels _ledPixels;
    
    // API
    RaftRetCode apiControl(const String &reqStr, String &respStr, const APISourceInfo& sourceInfo);
};
