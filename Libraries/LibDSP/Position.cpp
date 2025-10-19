#include "./Position.h"

#include <Basic/Verify.h>

#include <float.h>
#include <math.h>

C_API i64 dsp_frames_from_seconds(f64 seconds, f64 sample_rate)
{
    return (i64)(sample_rate * seconds);
}

C_API i64 dsp_frames_from_milliseconds(f64 ms, f64 sample_rate)
{
    return dsp_frames_from_seconds(ms * 0.001, sample_rate);
}

C_API i64 dsp_ticks_from_milliseconds(f64 ms, f64 sample_rate, f64 ticks_per_frame)
{
    return (i64)(ms * 0.001 * sample_rate * ticks_per_frame);
}

C_API i64 dsp_frames_from_ticks(f64 ticks, f64 frames_per_tick)
{
    return (i64)(frames_per_tick * ticks);
}

C_API f64 dsp_ticks_from_frames(i64 frames, f64 ticks_per_frame)
{
    return ticks_per_frame * (f64)frames;
}

C_API DSPPosition dsp_position_from_ticks(f64 ticks, f64 frames_per_tick)
{
    return (DSPPosition){
        .ticks = ticks,
        .frames = dsp_frames_from_ticks(ticks, frames_per_tick),
    };
}

C_API DSPPosition dsp_position_from_frames(i64 frames, f64 ticks_per_frame)
{
    return (DSPPosition){
        .ticks = dsp_ticks_from_frames(frames, ticks_per_frame),
        .frames = frames,
    };
}

C_API DSPPosition dsp_position_from_bars(i64 bars, i64 ticks_per_bar, f64 frames_per_tick)
{
    f64 ticks = (f64)(ticks_per_bar * bars);
    i64 frames = (i64)(frames_per_tick * ticks);
    return (DSPPosition){
        .ticks = ticks,
        .frames = frames,
    };
}

C_API DSPPosition dsp_position_from_seconds(f64 seconds, f64 sample_rate, f64 ticks_per_frame)
{
    f64 frames_per_second = sample_rate;
    f64 frames = frames_per_second * seconds;
    f64 ticks = ticks_per_frame * frames;
    return (DSPPosition){
        .ticks = ticks,
        .frames = (i64)frames,
    };
}

C_API DSPPosition dsp_position_from_milliseconds(f64 ms, f64 sample_rate, f64 ticks_per_frame)
{
    return dsp_position_from_seconds(ms * 0.001, sample_rate, ticks_per_frame);
}

C_API DSPPosition dsp_position_between(DSPPosition a, DSPPosition b)
{
    return (DSPPosition){
        .ticks = (a.ticks + b.ticks) * 0.5,
        .frames = (a.frames + b.frames) / 2,
    };
}

C_API DSPPosition dsp_position_closest(DSPPosition p, DSPPosition a, DSPPosition b)
{
    if (p.ticks - a.ticks <= b.ticks - p.ticks) {
        return a;
    }
    return b;
}

C_API i64 dsp_position_frame_compare(DSPPosition a, DSPPosition b)
{
    return a.frames - b.frames;
}

C_API bool dsp_position_ticks_almost_equal(DSPPosition a, DSPPosition b)
{
    return abs(a.ticks - b.ticks) < DBL_EPSILON;
}

C_API DSPPosition dsp_position_min(DSPPosition a, DSPPosition b)
{
    if (a.frames < b.frames)
        return a;
    return b;
}

C_API DSPPosition dsp_position_max(DSPPosition a, DSPPosition b)
{
    if (a.frames > b.frames)
        return a;
    return b;
}

C_API bool dsp_position_is_positive(DSPPosition a)
{
    return a.frames >= 0;
}

C_API bool dsp_position_is_between_left_inclusive(DSPPosition p, DSPPosition a_inclusive, DSPPosition b_exclusive)
{
    if (p.frames < a_inclusive.frames)
        return false;
    if (p.frames >= b_exclusive.frames)
        return false;
    return true;
}

C_API bool dsp_position_is_between_right_inclusive(DSPPosition p, DSPPosition a_exclusive, DSPPosition b_inclusive)
{
    if (p.frames <= a_exclusive.frames)
        return false;
    if (p.frames > b_inclusive.frames)
        return false;
    return true;
}

C_API bool dsp_position_is_between_inclusive(DSPPosition p, DSPPosition a, DSPPosition b)
{
    if (p.frames < a.frames)
        return false;
    if (p.frames > b.frames)
        return false;
    return true;
}

C_API bool dsp_position_is_between_exclusive(DSPPosition p, DSPPosition a, DSPPosition b)
{
    if (p.frames <= a.frames)
        return false;
    if (p.frames >= b.frames)
        return false;
    return true;
}

C_API DSPPosition dsp_position_add_frames(DSPPosition p, i64 frames, f64 ticks_per_frame)
{
    p.frames += frames;
    p.ticks = dsp_ticks_from_frames(p.frames, ticks_per_frame);
    return p;
}

C_API DSPPosition dsp_position_add_bars(DSPPosition p, i64 bars, i64 ticks_per_bar, f64 frames_per_tick)
{
    i64 ticks = ticks_per_bar * bars;
    f64 frames = frames_per_tick * (f64)ticks;
    p.frames += (i64)frames;
    p.ticks += (f64)ticks;
    return p;
}

C_API DSPPosition dsp_position_add_beats(DSPPosition p, i64 beats, i64 ticks_per_beat, f64 frames_per_tick)
{
    i64 ticks = ticks_per_beat * beats;
    f64 frames = frames_per_tick * (f64)ticks;
    p.frames += (i64)frames;
    p.ticks += (f64)ticks;
    return p;
}

C_API DSPPosition dsp_position_add_sixteenths(DSPPosition p, i64 sixteenths, f64 frames_per_tick)
{
    f64 ticks = (f64)(sixteenths * dsp_ticks_per_sixteenth_note);
    i64 frames = (i64)(frames_per_tick * ticks);
    p.ticks += ticks;
    p.frames += frames;
    return p;
}

C_API DSPPosition dsp_position_add_ticks(DSPPosition p, f64 ticks, f64 frames_per_tick)
{
    i64 frames = (i64)(frames_per_tick * ticks);
    p.ticks += ticks;
    p.frames += frames;
    return p;
}

C_API DSPPosition dsp_position_add_milliseconds(DSPPosition p, f64 ms, f64 sample_rate, f64 ticks_per_frame)
{
    auto pp = dsp_position_from_milliseconds(ms, sample_rate, ticks_per_frame);
    p.frames += pp.frames;
    p.ticks += pp.ticks;
    return p;
}

C_API DSPPosition dsp_position_add_seconds(DSPPosition p, f64 seconds, f64 sample_rate, f64 ticks_per_frame)
{
    auto pp = dsp_position_from_seconds(seconds, sample_rate, ticks_per_frame);
    p.frames += pp.frames;
    p.ticks += pp.ticks;
    return p;
}

C_API DSPPosition dsp_position_add_minutes(DSPPosition p, f64 mins, f64 sample_rate, f64 ticks_per_frame)
{
    return dsp_position_add_seconds(p, mins * 60.0, sample_rate, ticks_per_frame);
}

C_API DSPPosition dsp_position_flip_sign(DSPPosition p)
{
    p.frames = -p.frames;
    p.ticks = -p.ticks;
    return p;
}

C_API f64 dsp_position_as_seconds(DSPPosition p, f64 sample_rate)
{
    f64 seconds_per_frame = (1.0 / (f64)sample_rate);
    return seconds_per_frame * (f64)p.frames;
}

C_API f64 dsp_position_as_milliseconds(DSPPosition p, f64 sample_rate)
{
    return dsp_position_as_seconds(p, sample_rate) * 1000.0;
}

C_API i64 dsp_position_milliseconds_part(DSPPosition p, f64 sample_rate)
{
    f64 milliseconds_per_frame = 1000.0 / sample_rate;
    f64 milliseconds = milliseconds_per_frame * (f64)p.frames;
    return ((i64)milliseconds) % 1000;
}

C_API i64 dsp_position_seconds_part(DSPPosition p, f64 sample_rate)
{
    f64 seconds_per_frame = 1.0 / sample_rate;
    f64 seconds = seconds_per_frame * (f64)p.frames;
    return ((i64)seconds) % 60;
}

C_API i64 dsp_position_minutes_part(DSPPosition p, f64 sample_rate)
{
    f64 minutes_per_frame = 1.0 / sample_rate / 60.0;
    f64 minutes = minutes_per_frame * (f64)p.frames;
    return (i64)minutes;
}

C_API f64 dsp_position_ticks_part(DSPPosition p, f64 frames_per_tick)
{
    u64 sixteenths = dsp_position_total_sixteenths(p, frames_per_tick);
    return p.ticks - (f64)(sixteenths * dsp_ticks_per_sixteenth_note);
}

C_API i64 dsp_position_total_bars(DSPPosition p, i64 ticks_per_bar, f64 frames_per_tick)
{
    (void)frames_per_tick;
    return (i64)(p.ticks / (f64)ticks_per_bar);
}

C_API i64 dsp_position_total_beats(DSPPosition p, i64 beats_per_bar, f64 ticks_per_beat, f64 frames_per_tick)
{
    (void)beats_per_bar;
    f64 ticks_per_frame = 1.0 / frames_per_tick;
    f64 ticks = ticks_per_frame * (f64)p.frames;
    f64 beats_per_tick = 1.0 / ticks_per_beat;
    return (i64)(beats_per_tick * ticks);
}

C_API i64 dsp_position_total_sixteenths(DSPPosition p, f64 frames_per_tick)
{
    (void)frames_per_tick;
    return (i64)(p.ticks / (f64)dsp_ticks_per_sixteenth_note);
}

C_API i64 dsp_position_bars(DSPPosition p, i64 ticks_per_bar)
{
    return (i64)(p.ticks / (f64)ticks_per_bar);
}

C_API i64 dsp_position_beats(DSPPosition p, i64 beats_per_bar, i64 ticks_per_beat)
{
    f64 total_bars = (f64)dsp_position_bars(p, ticks_per_beat * beats_per_bar);
    return (i64)(total_bars * (f64)beats_per_bar);
}

C_API i64 dsp_position_sixteenths(DSPPosition p, i64 beats_per_bar, i64 sixteenths_per_beat, f64 frames_per_tick)
{
    (void)beats_per_bar;
    (void)sixteenths_per_beat;
    (void)frames_per_tick;
    return (i64)(p.ticks / (f64)dsp_ticks_per_sixteenth_note);

}
