// The 'Main' driver object, everything will live in here as the root..
#include <aspl/Plugin.hpp>
#include <aspl/Driver.hpp>

#include "device.hpp"

namespace GoXLR {
    class Driver : private aspl::DriverRequestHandler {
        public:
            Driver();
            ~Driver();

            Driver(const Driver&) = delete;
            Driver& operator=(const Driver&) = delete;

            // Reference Fetcher
            AudioServerPlugInDriverRef GetReference();

        private:
            // Invoked during async driver initialiseation, we setup here.
            OSStatus OnInitialize() override;

            // Objects Registered in coreaudiod
            std::shared_ptr<aspl::Driver> driver;
            std::shared_ptr<aspl::Plugin> plugin;

            // A thing we create..
            std::shared_ptr<Device> device;
    };
}