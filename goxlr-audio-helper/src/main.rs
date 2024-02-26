use std::ffi::{c_char, c_void};
use std::sync::{Arc, Mutex};
use std::thread::sleep;
use std::time::Duration;
use std::{mem, process, ptr};

use anyhow::{bail, Result};
use bounded_vec_deque::BoundedVecDeque;
use core_foundation::base::{kCFAllocatorDefault, CFType, TCFType};
use core_foundation::dictionary::{CFDictionary, CFMutableDictionary, CFMutableDictionaryRef};
use core_foundation::number::CFNumber;
use core_foundation::string::CFString;
use coreaudio::audio_unit::audio_format::LinearPcmFlags;
use coreaudio::audio_unit::macos_helpers::{
    audio_unit_from_device_id, get_hogging_pid, toggle_hog_mode,
};
use coreaudio::audio_unit::render_callback::data;
use coreaudio::audio_unit::{
    render_callback, AudioUnit, Element, SampleFormat, Scope, StreamFormat,
};
use coreaudio_sys::{
    kAudioHardwareNoError, kAudioHardwarePropertyDeviceForUID, kAudioObjectPropertyElementMaster,
    kAudioObjectPropertyScopeGlobal, kAudioObjectSystemObject, kAudioObjectUnknown,
    kAudioUnitProperty_StreamFormat, AudioDeviceID, AudioObjectGetPropertyData,
    AudioObjectGetPropertyDataSize, AudioObjectID, AudioObjectPropertyAddress,
    AudioValueTranslation, KERN_SUCCESS,
};
use enum_map::{Enum, EnumMap};
use io_kit_sys::types::io_iterator_t;
use io_kit_sys::{
    kIOMasterPortDefault, IOIteratorNext, IORegistryEntryCreateCFProperties,
    IOServiceGetMatchingServices, IOServiceMatching,
};
use strum::{EnumIter, IntoEnumIterator};

type Queue = Option<Arc<Mutex<BoundedVecDeque<f32>>>>;

const VID_GOXLR: u16 = 0x1220;
const PID_GOXLR_MINI: u16 = 0x8fe4;
const PID_GOXLR_FULL: u16 = 0x8fe0;

#[derive(Debug, Enum, EnumIter, Clone, Copy)]
enum GoXLROutputChannels {
    System,
    Game,
    Chat,
    Music,
    Sample,
}

impl GoXLROutputChannels {
    fn get_left(&self) -> usize {
        match self {
            GoXLROutputChannels::System => 0,
            GoXLROutputChannels::Game => 2,
            GoXLROutputChannels::Chat => 4,
            GoXLROutputChannels::Music => 6,
            GoXLROutputChannels::Sample => 8,
        }
    }

    fn get_channel_string(&self) -> &str {
        match self {
            GoXLROutputChannels::System => "System",
            GoXLROutputChannels::Game => "Game",
            GoXLROutputChannels::Chat => "Chat",
            GoXLROutputChannels::Music => "Music",
            GoXLROutputChannels::Sample => "Sample",
        }
    }
}

#[derive(Debug, Enum, EnumIter, Clone, Copy)]
enum GoXLRInputChannels {
    ChatMic,
    Sampler,
    StreamMix,
}

impl GoXLRInputChannels {
    fn get_left(&self) -> usize {
        match self {
            GoXLRInputChannels::ChatMic => 2,
            GoXLRInputChannels::Sampler => 4,
            GoXLRInputChannels::StreamMix => 0,
        }
    }

    fn get_channel_string(&self) -> &str {
        match self {
            GoXLRInputChannels::ChatMic => "ChatMic",
            GoXLRInputChannels::Sampler => "Sampler",
            GoXLRInputChannels::StreamMix => "StreamMix",
        }
    }
}

fn main() -> Result<()> {
    let goxlr_devices = get_goxlr_devices()?;
    if goxlr_devices.is_empty() {
        bail!("No GoXLRs found, Bailing!");
    }
    let goxlr = get_audio_id_for_uid(&goxlr_devices[0])?;
    let mut goxlr_output_unit = audio_unit_from_device_id(goxlr, false)?;
    let mut goxlr_input_unit = audio_unit_from_device_id(goxlr, true)?;
    println!("Found Device: {:?}", goxlr);

    let mut output_map: EnumMap<GoXLROutputChannels, AudioDeviceID> = EnumMap::default();
    let mut output_units: EnumMap<GoXLROutputChannels, Option<AudioUnit>> = EnumMap::default();
    for channel in GoXLROutputChannels::iter() {
        // Try and locate the channels..
        let uid = format!("GoXLR::{}::Input", channel.get_channel_string());
        let device_id = get_audio_id_for_uid(&uid)?;
        let audio_unit = audio_unit_from_device_id(device_id, true)?;

        output_map[channel] = device_id;
        output_units[channel] = Some(audio_unit);
    }

    let mut input_map: EnumMap<GoXLRInputChannels, AudioDeviceID> = EnumMap::default();
    let mut input_units: EnumMap<GoXLRInputChannels, Option<AudioUnit>> = EnumMap::default();
    for channel in GoXLRInputChannels::iter() {
        let uid = format!("GoXLR::{}::Output", channel.get_channel_string());
        let device_id = get_audio_id_for_uid(&uid)?;
        let audio_unit = audio_unit_from_device_id(device_id, false)?;

        input_map[channel] = device_id;
        input_units[channel] = Some(audio_unit);
    }

    // Ok, Step 1, we want to claim exclusive access to the GoXLR..
    let pid = get_hogging_pid(goxlr)?;
    if pid != -1 {
        bail!("Something already has Exclusive access to the GoXLR");
    } else {
        println!("Device free, try to grab it..");
        if toggle_hog_mode(goxlr)? == process::id() as i32 {
            println!("Claimed exclusive access..");
        } else {
            // TODO: We can probably proceed without it, but for now, bail.
            bail!("Unable to Claim Exclusive access");
        }
    }

    // GoXLR Format (CoreAudio will do most the work here)..
    let format_flags = LinearPcmFlags::IS_FLOAT | LinearPcmFlags::IS_PACKED;
    let goxlr_output = StreamFormat {
        sample_rate: 48000.,
        sample_format: SampleFormat::F32,
        flags: format_flags,
        channels: 10,
    };

    let goxlr_input = StreamFormat {
        sample_rate: 48000.,
        sample_format: SampleFormat::F32,
        flags: format_flags,
        channels: 21,
    };

    let coreaudio_virtual_format = StreamFormat {
        sample_rate: 48000.,
        sample_format: SampleFormat::F32,
        flags: format_flags,
        channels: 2,
    };

    // Ok, configure the stream mixes..
    let id = kAudioUnitProperty_StreamFormat;
    let coreaudio_format = coreaudio_virtual_format.to_asbd();
    for channel in GoXLROutputChannels::iter() {
        if let Some(ref mut unit) = &mut output_units[channel] {
            unit.set_property(id, Scope::Output, Element::Input, Some(&coreaudio_format))?;
        }
    }
    for channel in GoXLRInputChannels::iter() {
        if let Some(ref mut unit) = &mut input_units[channel] {
            unit.set_property(id, Scope::Input, Element::Output, Some(&coreaudio_format))?;
        }
    }

    // Configure the GoXLR Formats..
    let output_format = goxlr_output.to_asbd();
    goxlr_output_unit.set_property(id, Scope::Input, Element::Output, Some(&output_format))?;

    let input_format = goxlr_input.to_asbd();
    goxlr_input_unit.set_property(id, Scope::Output, Element::Input, Some(&input_format))?;

    let mut out_consumers: EnumMap<GoXLROutputChannels, Queue> = Default::default();

    type Args = render_callback::Args<data::Interleaved<f32>>;
    // We need to set up a *LOT* of buffers here, one for each channel..
    for channel in GoXLROutputChannels::iter() {
        // Create the Base..
        let buffer = Arc::new(Mutex::new(BoundedVecDeque::<f32>::new(1024)));

        // Create the producers..
        let producer = buffer.clone();
        let consumer = buffer.clone();

        out_consumers[channel] = Some(consumer);

        println!("Registering for {:?}", channel);

        // Register the Callback..
        output_units[channel]
            .as_mut()
            .unwrap()
            .set_input_callback(move |args| {
                let Args {
                    num_frames, data, ..
                } = args;

                // Lock the buffer for writing..
                let mut buffer_interlaced = producer.as_ref().lock().unwrap();
                for i in 0..num_frames * 2 {
                    buffer_interlaced.push_back(data.buffer[i]);
                }
                Ok(())
            })?;
        output_units[channel].as_mut().unwrap().start()?;
    }

    let mut in_producers: EnumMap<GoXLRInputChannels, Queue> = Default::default();
    for channel in GoXLRInputChannels::iter() {
        let buffer = Arc::new(Mutex::new(BoundedVecDeque::<f32>::new(1024)));
        let producer = buffer.clone();
        let consumer = buffer.clone();

        in_producers[channel] = Some(producer);
        println!("Registering for {:?}", channel);

        input_units[channel]
            .as_mut()
            .unwrap()
            .set_render_callback(move |args| {
                let Args {
                    num_frames, data, ..
                } = args;
                // Simply fill the buffer with the data we have..
                let consumer = consumer.as_ref().lock().unwrap();
                let samples_needed = num_frames * 2;
                let start = if consumer.len() < samples_needed {
                    samples_needed - consumer.len()
                } else {
                    0
                };

                // If we're under-running here, put the samples at the end of the buffer..
                for i in start..samples_needed {
                    data.buffer[i] = consumer[i];
                }
                Ok(())
            })?;

        input_units[channel].as_mut().unwrap().start()?
    }

    // On the GoXLR side, it's slightly different :)
    goxlr_output_unit.set_render_callback(move |args: Args| {
        let Args {
            num_frames, data, ..
        } = args;

        for channel in GoXLROutputChannels::iter() {
            let consumer = out_consumers[channel].as_ref().unwrap();
            let buffer_interlaced = consumer.lock().unwrap();

            for i in 0..num_frames {
                // Ok, the 'position' in the data structure should be simple..
                let left = i * 10 + channel.get_left();

                data.buffer[left] = buffer_interlaced[i * 2];
                data.buffer[left + 1] = buffer_interlaced[(i * 2) + 1];
            }
        }
        Ok(())
    })?;

    goxlr_input_unit.set_input_callback(move |args: Args| {
        let Args {
            num_frames, data, ..
        } = args;
        //println!("Received: {} frames", num_frames);
        for i in 0..num_frames {
            let start = i * 21;

            for channel in GoXLRInputChannels::iter() {
                let producer = in_producers[channel].as_ref().unwrap();
                let mut interlaced = producer.lock().unwrap();

                interlaced.push_back(data.buffer[start + channel.get_left()]);
                interlaced.push_back(data.buffer[start + channel.get_left() + 1]);
            }
        }
        Ok(())
    })?;

    goxlr_output_unit.start()?;
    goxlr_input_unit.start()?;

    loop {
        // For now, perpetually sleep..
        sleep(Duration::from_secs(100000));
    }
    Ok(())
}

fn get_audio_id_for_uid(uid: &str) -> Result<AudioDeviceID> {
    let mut device_id = kAudioObjectUnknown;
    let properties = AudioObjectPropertyAddress {
        mSelector: kAudioHardwarePropertyDeviceForUID,
        mScope: kAudioObjectPropertyScopeGlobal,
        mElement: kAudioObjectPropertyElementMaster,
    };

    let size = 0u32;
    let status = unsafe {
        AudioObjectGetPropertyDataSize(
            kAudioObjectSystemObject,
            &properties,
            0,
            ptr::null(),
            &size as *const _ as *mut _,
        )
    };
    assert_eq!(status, kAudioHardwareNoError as i32);

    let translation = AudioValueTranslation {
        mInputData: &CFString::new(uid) as *const CFString as *mut c_void,
        mInputDataSize: mem::size_of::<CFString>() as u32,
        mOutputData: &mut device_id as *mut AudioObjectID as *mut c_void,
        mOutputDataSize: mem::size_of::<AudioDeviceID>() as u32,
    };

    let status = unsafe {
        AudioObjectGetPropertyData(
            kAudioObjectSystemObject,
            &properties,
            0,
            ptr::null(),
            &size as *const _ as *mut _,
            &translation as *const _ as *mut _,
        )
    };

    assert_eq!(status, kAudioHardwareNoError as i32);
    Ok(device_id)
}

fn get_goxlr_devices() -> Result<Vec<String>> {
    let mut devices = Vec::new();

    let mut iterator = mem::MaybeUninit::<io_iterator_t>::uninit();
    let matcher = unsafe { IOServiceMatching(b"IOAudioEngine\0".as_ptr() as *const c_char) };
    let status = unsafe {
        IOServiceGetMatchingServices(kIOMasterPortDefault, matcher, iterator.as_mut_ptr())
    };

    if status != KERN_SUCCESS as i32 {
        bail!("Failed to Get Matching Service: {}", status);
    }

    let vid = CFString::new("idVendor");
    let pid = CFString::new("idProduct");
    let uid = CFString::new("IOAudioEngineGlobalUniqueID");
    //let dsc = CFString::new("IOAudioEngineDescription");

    loop {
        let service = unsafe { IOIteratorNext(iterator.assume_init()) };
        if service == 0 {
            break;
        }

        // Pull the properties for this device..
        let mut dictionary = mem::MaybeUninit::<CFMutableDictionaryRef>::uninit();
        unsafe {
            IORegistryEntryCreateCFProperties(
                service,
                dictionary.as_mut_ptr(),
                kCFAllocatorDefault,
                0,
            );
        }
        let properties: CFDictionary<CFString, CFType> = unsafe {
            CFMutableDictionary::wrap_under_get_rule(dictionary.assume_init()).to_immutable()
        };

        // Check to see if this result includes 'idVendor' and 'idProduct'..
        if properties.contains_key(&pid) && properties.contains_key(&vid) {
            // Pull out the values..
            let vid = properties.get(&vid).downcast::<CFNumber>().unwrap();
            let pid = properties.get(&pid).downcast::<CFNumber>().unwrap();

            // Check whether the Vendor is TC-Helicon..
            if vid.to_i32().unwrap() != VID_GOXLR.into() {
                continue;
            }

            let pid = pid.to_i32().unwrap();
            // Check whether we're a GoXLR
            if pid == PID_GOXLR_FULL.into() || pid == PID_GOXLR_MINI.into() {
                // Get the UID of this device..
                if properties.contains_key(&uid) {
                    let uid = properties.get(&uid).downcast::<CFString>().unwrap();
                    devices.push(uid.to_string());
                }
            }
        }
    }

    Ok(devices)
}
