// This is the Driver's EntryPoint, simply creates the base driver and registers it.

#include "driver.hpp"
#include <CoreAudio/AudioServerPlugIn.h>

using namespace GoXLR;

extern "C" void* GoXLREntryPoint(CFAllocatorRef allocator, CFUUIDRef typeUUID)
{
    // The UUID of the plug-in type (443ABAB8-E7B3-491A-B985-BEB9187030DB).
    if (!CFEqual(typeUUID, kAudioServerPlugInTypeUUID)) {
        return nullptr;
    }

    // Return a reference to the Driver..
    static Driver driver;
    return driver->GetReference();
}