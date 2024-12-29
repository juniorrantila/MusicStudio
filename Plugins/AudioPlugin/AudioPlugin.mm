#include "./AudioPlugin.h"

#include <FS/Bundle.h>
#include <Fonts/Fonts.h>
#include <Rexim/LA.h>
#include <Ty/Defer.h>
#include <UI/FreeGlyph.h>
#include <UI/Shaders/Shaders.h>
#include <UI/SimpleRenderer.h>
#include <UI/UI.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#import <AppKit/AppKit.h>
#include <OpenGL/OpenGL.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define LOG_IF(cond, fmt, ...) do { if (cond) { fprintf(stderr, "HelloWorld: " fmt "\n", ## __VA_ARGS__); } } while(0)
#define LOG(fmt, ...) fprintf(stderr, "HelloWorld: " fmt "\n", ## __VA_ARGS__)

@interface PluginView : NSOpenGLView {
    UI::UI* m_ui;
    NSTimer* m_timer;
}

- (id)initWithFrame:(NSRect)frameRect pixelFormat:(NSOpenGLPixelFormat *)format;
- (void)update:(NSTimer*)timer;

@end

@implementation PluginView

- (id)initWithFrame:(NSRect)frameRect pixelFormat:(NSOpenGLPixelFormat *)format {
    self = [super initWithFrame:frameRect pixelFormat:format];
    if (!self)
        return nil;
    [[self openGLContext] makeCurrentContext];
    self.wantsBestResolutionOpenGLSurface = YES;
    [self prepareOpenGL];

    auto bundle = FS::Bundle()
        .add_pack(Fonts())
        .add_pack(UI::Shaders());

    auto sr = MUST(UI::SimpleRenderer::create(bundle));
    sr.set_resolution(vec2f(frameRect.size.width, frameRect.size.height));
    sr.set_camera_pos(vec2f(frameRect.size.width, frameRect.size.height) / 2.0f);

    m_ui = new UI::UI(move(sr));

    auto font_file_path = "./Fonts/OxaniumLight/Oxanium-Light.ttf"sv;
    auto font = MUST(bundle.open(font_file_path).or_throw([]{
        return Error::from_string_literal("Could not load font");
    }));
    MUST(m_ui->load_font(font.bytes(), vec2f(0.0f, 20.0f)));

    m_timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 120.0
                                               target:self
                                             selector:@selector(update:)
                                             userInfo:nil
                                              repeats:YES];
    return self;
}

-(void)dealloc {
    [m_timer invalidate];
    delete m_ui;
}

- (void)update:(NSTimer*)timer
{
    if (!m_ui)
        return;
    [[self openGLContext] makeCurrentContext];
    m_ui->begin_frame();

    Vec4f background_color = hex_to_vec4f(0x363C40FF);
    Vec4f text_color = hex_to_vec4f(0xD3D9DDFF);

    m_ui->clear(background_color);

    auto text = "Do you hear the audio?"sv;
    auto text_size = m_ui->measure_text(text);
    auto text_pos = m_ui->resolution() / 2.0f - text_size / 2.0f;
    m_ui->text(text_pos, text, text_color);

    m_ui->end_frame();
    [self update];
    [[self openGLContext] flushBuffer];
}

-(void)mouseMoved:(NSEvent *)event
{
    m_ui->set_mouse_pos(event.locationInWindow.x, event.locationInWindow.y);
}

-(void)mouseDown:(NSEvent *)event
{
    m_ui->set_mouse_down(true);
}

-(void)mouseUp:(NSEvent *)event
{
    m_ui->set_mouse_down(false);
}

@end

bool AudioPlugin::open_editor(Vst::NativeHandle window_handle)
{
    if (m_view) {
        NSView* view = window_handle;
        [view addSubview:m_view];
        return true;
    }
    LOG("open editor %p", window_handle);
    m_window_handle = window_handle;
    NSView* view = window_handle;

    // clang-format off
    NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFANoRecovery,
        0
    };
    // clang-format on

    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
    m_view = [[PluginView alloc] initWithFrame:view.bounds pixelFormat:format];
    [view addSubview:m_view];

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void AudioPlugin::close_editor()
{

}

static f32 square_oscillator(f32 time, f32 frequency, f32 amplitude, f32 k_value, int num_harmonics);
static f32 sine_oscillator(f32 time, f32 frequency, f32 amplitude);

static f32 voice(f32 time, f32 main_frequency)
{
    f32 inner_time_warp_frequency = 0.05;
    f32 inner_time_warp_amplitude = 1.0f;
    f32 outer_time_warp_frequency = 0.05;
    f32 outer_time_warp_amplitude = 1.0f;
    f32 square_amplitude = 0.3f;
    f32 square_k_value = 2.0f;
    f32 square_harmonics = 8.0f;
    return square_oscillator(
        (
            time + sine_oscillator(
                time + sine_oscillator(
                    time,
                    inner_time_warp_frequency,
                    inner_time_warp_amplitude
                ),
                outer_time_warp_frequency,
                outer_time_warp_amplitude
            )
        ),
        main_frequency,
        square_amplitude,
        square_k_value,
        square_harmonics
    );
}

void AudioPlugin::process_f32(f32 const* const*,
    f32* const* outputs,
    i32 frames_in_block)
{
    f32 seconds_per_frame = 1.0f / m_sample_rate;

    auto out_left = outputs[0];
    auto out_right = outputs[1];

    i32 frames_left = frames_in_block;
    for (;;) {
        i32 frame_count = frames_left;
        if (!frame_count)
            break;

        for (i32 frame = 0; frame < frame_count; frame += 1) {
            f32 sample = voice((m_seconds_offset + frame * seconds_per_frame), m_freq);
            out_left[frame] = out_right[frame] = sample;
        }
        m_seconds_offset = fmod(m_seconds_offset + seconds_per_frame * frame_count, 1.0);

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }
}

bool AudioPlugin::set_sample_rate(f32 value)
{
    LOG("new sample rate: %f", value);
    m_sample_rate = value;
    return true;
}

bool AudioPlugin::set_block_size(i32 value)
{
    LOG("new block size: %d", value);
    m_block_size = value;
    return true;
}

static f32 sine_oscillator(f32 time, f32 frequency, f32 amplitude)
{
    return __builtin_sinf(frequency * time * 2.0f * M_PI) * amplitude;
}

static f32 square_oscillator(
    f32 time,
    f32 frequency,
    f32 amplitude,
    f32 k_value,
    int num_harmonics
) {
    f32 square = 0.0f;
    f32 k = 1.0f;
    for (int i = 0; i < num_harmonics; i++, k += k_value) {
        square += 1.0f / k * sine_oscillator(time, k * frequency, amplitude);
    }

    return square;
}
