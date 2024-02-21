#include "device.hpp"

namespace GoXLR {
    namespace {
        aspl::DeviceParameters make_device_params() {
            aspl::DeviceParameters deviceParams;
            deviceParams.Name = "GoXLR";
            deviceParams.SampleRate = 44100;
            deviceParams.ChannelCount = 2;
            deviceParams.EnableMixing = true;

            return deviceParams;
        }
    }

    Device::Device(std::shared_ptr<aspl::Plugin> plugin) : plugin(plugin) {
        assert(plugin);

        plugin->GetContext()->Tracer->Message("Creating Device..");
        auto params = make_device_params();

        device = std::make_shared<aspl::Device>(plugin->GetContext(), params);
        device->AddStreamWithControlsAsync(aspl::Direction::Input);
        device->AddStreamWithControlsAsync(aspl::Direction::Output);
        device->SetControlHandler(this);
        device->SetIOHandler(this);
        plugin->AddDevice(device);
    }

    Device::~Device() {
        plugin->GetContext()->Tracer->Message("Destroying Device");
    }

    OSStatus Device::OnStartIO() { 
        plugin->GetContext()->Tracer->Message("Starting Handler..");
        return kAudioHardwareNoError; 
    }

    void Device::OnStopIO() {
        plugin->GetContext()->Tracer->Message("Stopping Handler..");
    }

    void Device::OnReadClientInput(const std::shared_ptr<aspl::Client>& client, const std::shared_ptr<aspl::Stream>& stream, Float64 zeroTimestamp, Float64 timestamp, void* bytes, UInt32 bytesCount) {
        plugin->GetContext()->Tracer->Message("Reading Message..");
    }

    void Device::OnWriteMixedOutput(const std::shared_ptr<aspl::Stream>& stream, Float64 zeroTimestamp, Float64 timestamp, const void* buff, UInt32 buffByteSize) {
        plugin->GetContext()->Tracer->Message("Writing MEssage?");
    }
}