
#include "driver.hpp"

namespace GoXLR {
    Driver::Driver() {
        // Create the Driver..
        auto context = std::make_shared<aspl::Context>();

        plugin = std::make_shared<aspl::Plugin>(context);
        driver = std::make_shared<aspl::Driver>(context, plugin);

        // Will invoke OnInitialize
        driver->SetDriverHandler(this);
    }

    Driver::~Driver() {

    }

    AudioServerPlugInDriverRef Driver::reference() {
        return driver->GetReference();
    }

    OSStatus Driver::OnInitialize() {
        // We need to start the GoXLR Manager here..
        
        // The Manager creates the device, we don't do anything
        return kAudioHardwareNoError;
    }
}