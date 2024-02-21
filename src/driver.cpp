#include "driver.hpp"

namespace GoXLR {
    Driver::Driver() {
        auto context = std::make_shared<aspl::Context>();
        plugin = std::make_shared<aspl::Plugin>(context);
        driver = std::make_shared<aspl::Driver>(context, plugin);
        
        driver->SetDriverHandler(this);
    }

    Driver::~Driver() {
        plugin->GetContext()->Tracer->Message("Destroying Driver");
    }

    AudioServerPlugInDriverRef Driver::GetReference() {
        return driver->GetReference();
    }

    OSStatus Driver::OnInitialize() {
        plugin->GetContext()->Tracer->Message("Attempting to Create Device..");
        device = std::make_shared<Device>(plugin);

        // The Manager creates the device, we don't do anything
        return kAudioHardwareNoError;
    }
}
