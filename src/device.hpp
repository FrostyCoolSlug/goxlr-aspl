#include <aspl/Driver.hpp>
#include <aspl/Plugin.hpp>

namespace GoXLR {
    class Device : private aspl::ControlRequestHandler, private aspl::IORequestHandler {
        public: 
            Device(std::shared_ptr<aspl::Plugin> plugin);
            ~Device();

            Device(const Device&) = delete;
            Device& operator=(const Device&) = delete;

        private:
            OSStatus OnStartIO() override;
            void OnStopIO() override;
            virtual void OnReadClientInput(const std::shared_ptr<aspl::Client>& client, const std::shared_ptr<aspl::Stream>& stream, Float64 zeroTimestamp, Float64 timestamp, void* bytes, UInt32 bytesCount) override;
            void OnWriteMixedOutput(const std::shared_ptr<aspl::Stream>& stream, Float64 zeroTimestamp, Float64 timestamp, const void* buff, UInt32 buffByteSize) override;

            std::shared_ptr<aspl::Plugin> plugin;
            std::shared_ptr<aspl::Device> device;
        
    };
}
