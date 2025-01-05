#include <CLI/ArgumentParser.h>
#include <Main/Main.h>
#include <Core/MappedFile.h>
#include <AU/Audio.h>
#include <Core/File.h>
#include <Ty/System.h>
#include <Ty/ArenaAllocator.h>
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
    auto size = TRY(AU::Audio::samples_byte_size(AU::AudioFormat::WAV, wav_file.bytes()));
    auto arena = TRY(ArenaAllocator::create(size));
    auto audio = TRY(AU::Audio::decode(&arena, AU::AudioFormat::WAV, wav_file.bytes()));

    u32 channel_count = audio.channel_count();
    u32 sample_rate = audio.sample_rate();
    View<f64 const> samples = audio.samples();
    TRY(System::write(1, &channel_count, sizeof(channel_count)));
    TRY(System::write(1, &sample_rate, sizeof(sample_rate)));
    TRY(System::write(1, samples.data(), samples.byte_size()));
    return 0;
}
