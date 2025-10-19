#include <LibAudio/AudioDecoder.h>
#include <LibCLI/ArgumentParser.h>
#include <LibCore/File.h>
#include <LibCore/MappedFile.h>
#include <LibMain/Main.h>
#include <LibTy/System.h>

#include <Basic/FixedArena.h>
#include <Basic/PageAllocator.h>

#include <unistd.h>

ErrorOr<int> Main::main(int argc, c_string argv[]) {
    auto argument_parser = CLI::ArgumentParser();
    
    auto wav_path = StringView();
    TRY(argument_parser.add_positional_argument("wav-path", [&](c_string arg) {
        wav_path = StringView::from_c_string(arg);
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto wav_file = TRY(Core::MappedFile::open(wav_path));
    AUAudio audio;
    auto err = au_audio_decode_wav(wav_file.bytes(), &audio);
    if (err != e_au_decode_none)
        return Error::from_string_literal(au_decode_strerror(err));
    f64* samples = (f64*)page_alloc(audio.channel_count * audio.frame_count * sizeof(f64));
    if (!samples) return Error::from_string_literal("could not allocate samples");

    u64 sample_index = 0;
    for (u64 frame = 0; frame < audio.frame_count; frame += 1) {
        for (u64 channel = 0; channel < audio.channel_count; channel += 1) {
            samples[sample_index++] = au_audio_sample_f64(&audio, channel, frame);
        }
    }

    u32 channel_count = audio.channel_count;
    u32 sample_rate = audio.sample_rate;
    TRY(System::write(1, &channel_count, sizeof(channel_count)));
    TRY(System::write(1, &sample_rate, sizeof(sample_rate)));
    TRY(System::write(1, samples, sample_index * sizeof(f64)));
    return 0;
}
