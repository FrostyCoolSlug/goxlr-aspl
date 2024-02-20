
#include "driver.hpp"

namespace GoXLR {
    Driver::Driver() {
        // Create the Driver..
        auto context = std::make_shared<aspl::Context>();

        this.plugin = std::make_shared<aspl::Plugin>(context);
        this.driver = std::make_shared<aspl::Driver>(context, this.plugin);

        // Will invoke OnInitialize
        this.driver.SetDriverHandler(this);
    }

    Driver::~Driver() {

    }

    AudioServerPlugInDriverRef Driver::reference() {
        return this.driver.getReference();
    }

    OSStatus Driver::OnInitialize() {
        // We need to start the GoXLR Manager here..
        
        // The Manager creates the device, we don't do anything
        return kAudioHardwareNoError;
    }
}