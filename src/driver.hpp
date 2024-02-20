// The 'Main' driver object, everything will live in here as the root..

#include <aspl/Driver.hpp>
#include <aspl/Plugin.hpp>

namespace GoXLR {
    class Driver : private aspl::DriverRequestHandler {
        public:
            Driver();
            ~Driver();

            Driver(const Driver&) = delete;
            Driver& operator=(const Driver&) = delete;

            AudioServerPlugInDriverRef reference();

        private:
            // Invoked during async driver initialisation, we generally setup here.
            OSStatus OnInitialize() override;

            // Objects Registered in coreaudiod
            std::shared_ptr<aspl::Driver> driver;
            std::shared_ptr<aspl::Plugin> plugin;
    };
}