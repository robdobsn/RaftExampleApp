////////////////////////////////////////////////////////////////////////////////
//
// AppSysMod.h
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RaftArduino.h"
#include "RaftSysMod.h"

class AppSysMod : public RaftSysMod
{
public:
    AppSysMod(const char *pModuleName, RaftJsonIF& sysConfig);
    virtual ~AppSysMod();

    // Create function (for use by SysManager factory)
    static RaftSysMod* create(const char* pModuleName, RaftJsonIF& sysConfig)
    {
        return new AppSysMod(pModuleName, sysConfig);
    }

protected:

    // Setup
    virtual void setup() override final;

    // Loop (called frequently)
    virtual void loop() override final;

private:
    // Example of how to control loop rate
    uint32_t _lastLoopMs = 0;
};
