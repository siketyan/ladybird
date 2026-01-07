/*
 * Copyright (c) 2026, Naoki Ikeguchi <me@s6n.jp>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibJS/Runtime/ArrayBuffer.h>
#include <LibWeb/Bindings/Transferable.h>
#include <LibWeb/WebCodecs/AudioData.h>
#include <LibWeb/WebIDL/ExceptionOr.h>

namespace Web::WebCodecs {

GC_DEFINE_ALLOCATOR(AudioData);

WebIDL::ExceptionOr<GC::Ref<AudioData>> AudioData::construct_impl(JS::Realm& realm, AudioDataInit const& init)
{
    // 1. If init is not a valid AudioDataInit, throw a TypeError;
    if (!is_valid_audio_data_init(init)) {
        return realm.vm().throw_completion<JS::TypeError>("Invalid AudioDataInit"sv);
    }

    // 4. Let frame be a new AudioData object, initialized as follows:
    auto frame = realm.create<AudioData>(realm);
    {
        // 1. Assign false to [[Detached]].
        frame->set_detached(false);
        // 2. Assign init.format to [[format]].
        frame->m_format = init.format;
        // 3. Assign init.sampleRate to [[sample rate]].
        frame->m_sample_rate = init.sample_rate;
        // 4. Assign init.numberOfFrames to [[number of frames]].
        frame->m_number_of_frames = init.number_of_frames;
        // 5. Assign init.numberOfChannels to [[number of channels]].
        frame->m_number_of_channels = init.number_of_channels;
        // 6. Assign init.timestamp to [[timestamp]].
        frame->m_timestamp = init.timestamp;

        // TODO: 7. If init.transfer contains an ArrayBuffer referenced by init.data the User Agent MAY choose to:
        // TODO: 8. Otherwise:

        // 9. Let resourceReference be a reference to resource.
        auto resource_reference = init.data;
        // TODO: 10. Assign resourceReference to [[resource reference]].
    }

    // 6. Return frame.
    return frame;
}

AudioData::AudioData(JS::Realm& realm) : PlatformObject(realm) {};

AudioData::~AudioData() = default;

WebIDL::ExceptionOr<void> AudioData::serialization_steps(HTML::TransferDataEncoder&, bool, HTML::SerializationMemory&)
{
    // FIXME: Implement this
    dbgln("(STUBBED) AudioData::serialization_steps(HTML::TransferDataEncoder&)");
    return {};
}

WebIDL::ExceptionOr<void> AudioData::deserialization_steps(HTML::TransferDataDecoder&, HTML::DeserializationMemory&)
{
    // FIXME: Implement this
    dbgln("(STUBBED) AudioData::deserialization_steps(HTML::TransferDataEncoder&)");
    return {};
}

WebIDL::ExceptionOr<void> AudioData::transfer_steps(HTML::TransferDataEncoder&)
{
    // FIXME: Implement this
    dbgln("(STUBBED) AudioData::transfer_steps(HTML::TransferDataEncoder&)");
    return {};
}

WebIDL::ExceptionOr<void> AudioData::transfer_receiving_steps(HTML::TransferDataDecoder&)
{
    // FIXME: Implement this
    dbgln("(STUBBED) AudioData::transfer_receiving_steps(HTML::TransferDataDecoder&)");
    return {};
}

HTML::TransferType AudioData::primary_interface() const
{
    // FIXME: Implement this
    dbgln("(STUBBED) AudioData::primary_interface()");
    return {};
}

// The duration, in microseconds, for this AudioData.
WebIDL::UnsignedLongLong AudioData::duration() const
{
    // 1. Let microsecondsPerSecond be 1,000,000.
    constexpr WebIDL::UnsignedLongLong microseconds_per_second = 1000000;

    // 2. Let durationInSeconds be the result of dividing [[number of frames]] by [[sample rate]].
    const double duration_in_seconds = this->m_number_of_frames / this->m_sample_rate;

    // 3. Return the product of durationInSeconds and microsecondsPerSecond.
    return duration_in_seconds * microseconds_per_second;
}

// Returns the number of bytes required to hold the samples as described by options.
WebIDL::ExceptionOr<WebIDL::UnsignedLong> AudioData::allocation_size(AudioDataCopyToOptions const& options)
{
    // 1. If [[Detached]] is true, throw an InvalidStateError DOMException.
    if (this->is_detached()) {
        return WebIDL::InvalidStateError::create(this->realm(), "AudioData is detached"_utf16);
    }

    // 2. Let copyElementCount be the result of running the Compute Copy Element Count algorithm with options.
    auto copy_element_count = compute_copy_element_count(options);
    if (copy_element_count.is_exception()) {
        return copy_element_count.exception();
    }

    // 3. Let destFormat be the value of [[format]].
    auto dest_format = this->m_format.value(); // SAFETY: Non-nullable when [[Detached]] is false

    // 4. If options.format exists, assign options.format to destFormat.
    if (options.format.has_value()) {
        dest_format = options.format.value();
    }

    // 5. Let bytesPerSample be the number of bytes per sample, as defined by the destFormat.
    auto bytes_per_sample = get_bytes_per_sample(dest_format);

    // 6. Return the product of multiplying bytesPerSample by copyElementCount.
    return bytes_per_sample * copy_element_count.value();
}

// Copies the samples from the specified plane of the AudioData to the destination buffer.
WebIDL::ExceptionOr<void> AudioData::copy_to(GC::Root<WebIDL::BufferSource>& destination, AudioDataCopyToOptions const& options)
{
    // 1. If [[Detached]] is true, throw an InvalidStateError DOMException.
    if (this->is_detached()) {
        return WebIDL::InvalidStateError::create(this->realm(), "AudioData is detached"_utf16);
    }

    // 2. Let copyElementCount be the result of running the Compute Copy Element Count algorithm with options.
    auto copy_element_count = compute_copy_element_count(options);
    if (copy_element_count.is_exception()) {
        return copy_element_count.exception();
    }

    // 3. Let destFormat be the value of [[format]].
    auto dest_format = this->m_format.value(); // SAFETY: Non-nullable when [[Detached]] is false

    // 4. If options.format exists, assign options.format to destFormat.
    if (options.format.has_value()) {
        dest_format = options.format.value();
    }

    // 5. Let bytesPerSample be the number of bytes per sample, as defined by the destFormat.
    auto bytes_per_sample = get_bytes_per_sample(dest_format);

    // 6. If the product of multiplying bytesPerSample by copyElementCount is greater than destination.byteLength, throw a RangeError.
    if (bytes_per_sample * copy_element_count.value() > destination->byte_length()) {
        return this->realm().vm().throw_completion<JS::RangeError>(
            "bytesPerSample * copyElementCount must be less than destination.byteLength"sv
        );
    }

    // 7. Let resource be the media resource referenced by [[resource reference]].
    auto& resource = this->m_data;

    // 8. Let planeFrames be the region of resource corresponding to options.planeIndex.
    auto plane_frames = resource->bytes().slice(
        m_number_of_frames * bytes_per_sample * options.plane_index,
        m_number_of_frames * bytes_per_sample
    );

    // 9. Copy elements of planeFrames into destination, starting with the frame positioned at options.frameOffset and
    //    stopping after copyElementCount samples have been copied. If destFormat does not equal [[format]], convert
    //    elements to the destFormat AudioSampleFormat while making the copy.
    const auto source = plane_frames.slice(
        options.frame_offset.value_or(0) * bytes_per_sample,
        copy_element_count.value() * bytes_per_sample
    );
    const auto buffer = destination->viewed_array_buffer()->buffer().bytes().slice(destination->byte_offset(), destination->byte_length());
    const auto result = source.copy_to(buffer);
    VERIFY(result == source.size());

    return {};
}

// https://w3c.github.io/webcodecs/#clone-audiodata
GC::Ref<AudioData> AudioData::clone()
{
    auto& realm = this->realm();

    // 1. Let clone be a new AudioData initialized as follows:
    auto clone = realm.create<AudioData>(realm);
    {
        // 1. Let resource be the media resource referenced by data’s [[resource reference]].
        const auto resource = this->m_data;

        // 2. Let reference be a new reference to resource.
        const auto reference = resource; // TODO

        // 3. Assign reference to [[resource reference]].
        clone->m_data = reference;

        // 4. Assign the values of data’s [[Detached]], [[format]], [[sample rate]], [[number of frames]], [[number of channels]],
        //    and [[timestamp]] slots to the corresponding slots in clone.
        clone->set_detached(this->is_detached());
        clone->m_format = this->m_format;
        clone->m_sample_rate = this->m_sample_rate;
        clone->m_number_of_frames = this->m_number_of_frames;
        clone->m_number_of_channels = this->m_number_of_channels;
        clone->m_timestamp = this->m_timestamp;
    }

    // 2. Return clone.
    return clone;
}

// https://w3c.github.io/webcodecs/#close-audiodata
void AudioData::close()
{
    this->set_detached(true);
    this->m_data = {};
    this->m_sample_rate = 0;
    this->m_number_of_frames = 0;
    this->m_number_of_channels = 0;
    this->m_format = {};
}

WebIDL::ExceptionOr<WebIDL::UnsignedLong> AudioData::compute_copy_element_count(AudioDataCopyToOptions const& options) const
{
    auto& realm = this->realm();
    auto& vm = realm.vm();

    // 1. Let destFormat be the value of [[format]].
    auto dest_format = this->m_format.value(); // SAFETY: Non-nullable when [[Detached]] is false

    // 2. If options.format exists, assign options.format to destFormat.
    if (options.format.has_value()) {
        dest_format = options.format.value();
    }

    const auto is_interleaved_format =
        dest_format == Bindings::AudioSampleFormat::U8 ||
        dest_format == Bindings::AudioSampleFormat::S16 ||
        dest_format == Bindings::AudioSampleFormat::S32 ||
        dest_format == Bindings::AudioSampleFormat::F32;

    const auto is_planar_format = !is_interleaved_format;

    // 3. If destFormat describes an interleaved AudioSampleFormat and options.planeIndex is greater than 0, throw a RangeError.
    if (is_interleaved_format && options.plane_index > 0) {
        return vm.throw_completion<JS::RangeError>("planeIndex must be 0 for interleaved audio sample formats"sv);
    }

    // 4. Otherwise, if destFormat describes a planar AudioSampleFormat and if options.planeIndex is greater or equal to
    //    [[number of channels]], throw a RangeError.
    if (is_planar_format && options.plane_index >= m_number_of_channels) {
        return vm.throw_completion<JS::RangeError>("planeIndex must be less than the number of channels for planar formats"sv);
    }

    // 5. If [[format]] does not equal destFormat and the User Agent does not support the requested AudioSampleFormat conversion,
    //    throw a NotSupportedError DOMException. Conversion to f32-planar MUST always be supported.
    if (m_format != dest_format) { // TODO: Support implicit conversions
        return WebIDL::NotSupportedError::create(realm, "format conversion is not supported"_utf16);
    }

    // 6. Let frameCount be the number of frames in the plane identified by options.planeIndex.
    const auto frame_count = m_number_of_frames;

    // 7. If options.frameOffset is greater than or equal to frameCount, throw a RangeError.
    if (options.frame_offset.value_or(0) >= frame_count) {
        return vm.throw_completion<JS::RangeError>("frameOffset must be less than frameCount"sv);
    }

    // 8. Let copyFrameCount be the difference of subtracting options.frameOffset from frameCount.
    auto copy_frame_count = frame_count - options.frame_offset.value_or(0);

    // 9. If options.frameCount exists:
    if (options.frame_count.has_value()) {
        const auto options_frame_count = options.frame_count.value();

        // 1. If options.frameCount is greater than copyFrameCount, throw a RangeError.
        if (options_frame_count > copy_frame_count) {
            return vm.throw_completion<JS::RangeError>("frameCount must be less than or equal to copyFrameCount"sv);
        }

        // 2. Otherwise, assign options.frameCount to copyFrameCount.
        copy_frame_count = options_frame_count;
    }

    // 10. Let elementCount be copyFrameCount.
    auto element_count = copy_frame_count;

    // 11. If destFormat describes an interleaved AudioSampleFormat, multiply elementCount by [[number of channels]]
    if (is_interleaved_format) {
        element_count *= m_number_of_channels;
    }

    // 12. return elementCount.
    return element_count;
}

bool is_valid_audio_data_init(AudioDataInit const& init)
{
    // 1. If sampleRate less than or equal to 0, return false.
    if (init.sample_rate <= 0) return false;

    // 2. If numberOfFrames = 0, return false.
    if (init.number_of_frames == 0) return false;

    // 3. If numberOfChannels = 0, return false.
    if (init.number_of_channels == 0) return false;

    // 4. Verify data has enough data by running the following steps:
    {
        // 1. Let totalSamples be the product of multiplying numberOfFrames by numberOfChannels.
        const auto total_samples = init.number_of_frames * init.number_of_channels;

        // 2. Let bytesPerSample be the number of bytes per sample, as defined by the format.
        const auto bytes_per_sample = get_bytes_per_sample(init.format);

        // 3. Let totalSize be the product of multiplying bytesPerSample with totalSamples.
        const auto total_size = bytes_per_sample * total_samples;

        // 4. Let dataSize be the size in bytes of data.
        const auto data_size = init.data->byte_length();

        // 5. If dataSize is less than totalSize, return false.
        if (data_size < total_size) return false;
    }

    // 5. Return true.
    return true;
}

WebIDL::UnsignedLong get_bytes_per_sample(Bindings::AudioSampleFormat format)
{
    switch (format) {
    case Bindings::AudioSampleFormat::U8:
    case Bindings::AudioSampleFormat::U8Planar:
        return 1;

    case Bindings::AudioSampleFormat::S16:
    case Bindings::AudioSampleFormat::S16Planar:
        return 2;

    case Bindings::AudioSampleFormat::S32:
    case Bindings::AudioSampleFormat::F32:
    case Bindings::AudioSampleFormat::S32Planar:
    case Bindings::AudioSampleFormat::F32Planar:
        return 4;

    default:
        VERIFY_NOT_REACHED();
    }
}

}
