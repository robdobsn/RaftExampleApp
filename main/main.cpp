/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Main entry point
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "RaftCoreApp.h"
#include "RegisterSysMods.h"
#include "RegisterWebServer.h"
#include "LEDRingSysMod.h"
#include "AppSysMod.h"
#include "IMUSysMod.h"

// Entry point
extern "C" void app_main(void)
{
    RaftCoreApp raftCoreApp;
    
    // Register SysMods from RaftSysMods library
    RegisterSysMods::registerSysMods(raftCoreApp.getSysManager());

    // Register WebServer from RaftWebServer library
    RegisterSysMods::registerWebServer(raftCoreApp.getSysManager());

    // Register the LEDRing system module
    raftCoreApp.registerSysMod("LEDRingSysMod", LEDRingSysMod::create, true);

    // Register the IMU system module
    raftCoreApp.registerSysMod("IMUSysMod", IMUSysMod::create, true);

    // Register app
    raftCoreApp.registerSysMod("AppSysMod", AppSysMod::create, true);

    // Loop forever
    while (1)
    {
        // Yield for 1 tick
        vTaskDelay(1);

        // Loop the app
        raftCoreApp.loop();
    }
}
