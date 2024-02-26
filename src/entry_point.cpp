// This is the Driver's EntryPoint, simply creates the base driver and registers it.

// #include "driver.hpp"
#include "ringbuffer.cpp"
#include <aspl/Driver.hpp>
#include <aspl/Plugin.hpp>

#include <CoreAudio/AudioServerPlugIn.h>

const UInt32 CHANNEL_COUNT = 2;
const UInt32 SAMPLE_RATE = 48000;

// The following is what's hit when audio is sent from multiple apps and mixed together, we don't need to care
// about specific app behaviour, and the primary goal of this class is to push the IO across to the Monitor for
// output back to userspace..
class GoXLRChannelHandler : public aspl::ControlRequestHandler, public aspl::IORequestHandler {
    public: 
        GoXLRChannelHandler(std::shared_ptr<SampleRingBuffer> buffer, std::shared_ptr<aspl::Plugin> plugin) : buffer(buffer), plugin(plugin) {
            assert(plugin);
        }

        OSStatus OnStartIO() override {
            return kAudioHardwareNoError;
        }

        // Something's attempting to write to us, push it into the buffer and consider it dealt with..
        void OnWriteMixedOutput(const std::shared_ptr<aspl::Stream>& stream, Float64 zeroTimestamp, Float64 timestamp, const void* buff, UInt32 buffBytesSize) override {
            // Ok, firstly, we need to grab the samples..
            Float32* samples = (Float32*) buff;
            UInt32 sampleCount = buffBytesSize / sizeof(Float32);

            if (sampleCount > BufferSize) {
                plugin->GetContext()->Tracer->Message("More Samples than Buffer! (%d)", sampleCount);
            }

            // Shove all the samples into the buffer for later processing..
            buffer->push_all(samples, sampleCount);
        }

        // A read is being attempted, pull from the buffer and handle any underruns / overruns that may occur..
        virtual void OnReadClientInput(const std::shared_ptr<aspl::Client>& client, const std::shared_ptr<aspl::Stream>& stream, Float64 zeroTimestamp, Float64 timestamp, void* bytes, UInt32 bytesCount) override {
            Float32* samples = (Float32*) bytes;
            UInt32 neededSamples = bytesCount / sizeof(Float32);
            UInt32 neededFrames = neededSamples / CHANNEL_COUNT;

            UInt32 startFrame = 0;
            UInt32 tailDistance = buffer->getTailDistance();

            // The buffer runneth over, pull ourselves back to a useful spot..
            if (buffer->is_overrun) {
                buffer->setTailDistance(neededSamples);
            }

            // This handles underruns (generally caused by unpausing), and either Zeros the entire request, or puts the sample
            // pointer in the correct position to 'continue' with the available data..
            if (tailDistance < neededSamples) {
                // Possibly correcting an underrun here, fill with 0s up until the point we have useful data..
                for (UInt32 i = 0; i < neededSamples - tailDistance; i++) {
                    samples[i] = 0;
                }
                startFrame = (neededSamples - tailDistance) / CHANNEL_COUNT;
            }

            // Grab the available samples, and dump them into the output
            for (UInt32 i = startFrame; i < neededFrames; i++) {
                // Set it into the Samples list..
                samples[i * CHANNEL_COUNT] = buffer->pop();
                samples[i * CHANNEL_COUNT + 1] = buffer->pop();
            }
        }
    private:
        std::shared_ptr<SampleRingBuffer> buffer;
        std::shared_ptr<aspl::Plugin> plugin;
};


aspl::StreamParameters GetStreamParameters(aspl::Direction direction) {
    auto parameters = aspl::StreamParameters {};
    parameters.Direction = direction;
    parameters.Format = {
        .mSampleRate = 48000,
        .mFormatID = kAudioFormatLinearPCM,
        .mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked,
        .mBitsPerChannel = 32,
        .mChannelsPerFrame = 2,
        .mBytesPerFrame = 8,
        .mBytesPerPacket = 8,
        .mFramesPerPacket = 1
    };
    return parameters;
}

void CreateDevicePair(std::string displayName, std::string uidName, std::shared_ptr<aspl::Context> context, std::shared_ptr<aspl::Plugin> plugin, Boolean is_output) {
    aspl::PluginParameters pluginParams;

    // The two Param Setups for our Devices..
    aspl::DeviceParameters outputDeviceParams;
    outputDeviceParams.Name = displayName + ((!is_output) ? " Hidden" : "");
    outputDeviceParams.SampleRate = SAMPLE_RATE;
    outputDeviceParams.ChannelCount = CHANNEL_COUNT;
    outputDeviceParams.EnableMixing = true;
    outputDeviceParams.DeviceUID = uidName + "::Output";
    outputDeviceParams.IconURL = "icon.icns";

    aspl::DeviceParameters inputDeviceParams;
    inputDeviceParams.Name = displayName + ((is_output) ? " Hidden" : "");
    inputDeviceParams.SampleRate = SAMPLE_RATE;
    inputDeviceParams.ChannelCount = CHANNEL_COUNT;
    inputDeviceParams.DeviceUID = uidName + "::Input";
    inputDeviceParams.IconURL = "icon.icns";

    // Create the RingBuffer..
    auto buffer = std::make_shared<SampleRingBuffer>();

    // Create our handlers..
    auto outputHandler = std::make_shared<GoXLRChannelHandler>(buffer, plugin);
    auto inputHandler = std::make_shared<GoXLRChannelHandler>(buffer, plugin);

    // Create our output device..
    auto outputDevice = std::make_shared<aspl::Device>(context, outputDeviceParams);
    outputDevice->AddStreamWithControlsAsync(GetStreamParameters(aspl::Direction::Output));
    outputDevice->SetControlHandler(outputHandler);
    outputDevice->SetIOHandler(outputHandler);
    if (!is_output) {
        outputDevice->SetIsHidden(true);
    }

    // Create our input Device..
    auto inputDevice = std::make_shared<aspl::Device>(context, inputDeviceParams);
    auto inputStream = inputDevice->AddStreamWithControlsAsync(GetStreamParameters(aspl::Direction::Input));
    inputDevice->SetControlHandler(inputHandler);
    inputDevice->SetIOHandler(inputHandler);
    if (is_output) {
        inputDevice->SetIsHidden(true);
    }

    // Add the devices to the plugin..
    plugin->AddDevice(outputDevice);
    plugin->AddDevice(inputDevice);
}

std::shared_ptr<aspl::Driver> CreateDevices() {
    // Create the Context..
    auto context = std::make_shared<aspl::Context>();

    // Create the Plugin
    auto plugin = std::make_shared<aspl::Plugin>(context);

    CreateDevicePair("System", "GoXLR::System", context, plugin, true);
    CreateDevicePair("Game", "GoXLR::Game", context, plugin, true);
    CreateDevicePair("Chat", "GoXLR::Chat", context, plugin, true);
    CreateDevicePair("Music", "GoXLR::Music", context, plugin, true);
    CreateDevicePair("Sample", "GoXLR::Sample", context, plugin, true);

    CreateDevicePair("Chat Mic", "GoXLR::ChatMic", context, plugin, false);
    CreateDevicePair("Broadcast Stream Mix", "GoXLR::StreamMix", context, plugin, false);
    CreateDevicePair("Sampler", "GoXLR::Sampler", context, plugin, false);

    // Create and return the driver.
    auto driver = std::make_shared<aspl::Driver>(context, plugin);
    return driver;
}


extern "C" void* GoXLREntryPoint(CFAllocatorRef allocator, CFUUIDRef typeUUID)
{
    // The UUID of the plug-in type (443ABAB8-E7B3-491A-B985-BEB9187030DB).
    if (!CFEqual(typeUUID, kAudioServerPlugInTypeUUID)) {
        return nullptr;
    }

    static std::shared_ptr<aspl::Driver> driver = CreateDevices();
    return driver->GetReference();
}