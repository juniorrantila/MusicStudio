#pragma once
#include <Basic/Types.h>

C_API inline const i64 dsp_ticks_per_quarter_note = 960;
C_API inline const i64 dsp_ticks_per_sixteenth_note = 240;
C_API inline const i64 dsp_ticks_per_ninetysixth_note = 40;
C_API inline const i64 dsp_max_bar_position = 160000;

C_API i64 dsp_frames_from_seconds(f64 seconds, f64 sample_rate);
C_API i64 dsp_frames_from_milliseconds(f64 ms, f64 sample_rate);
C_API i64 dsp_ticks_from_milliseconds(f64 ms, f64 sample_rate, f64 ticks_per_frame);
C_API i64 dsp_frames_from_ticks(f64 ticks, f64 frames_per_tick);
C_API f64 dsp_ticks_from_frames(i64 frames, f64 ticks_per_frame);

typedef struct DSPPosition {
  f64 ticks;
  i64 frames; // Use this for most calculations.
} DSPPosition;
C_API inline const DSPPosition dsp_position_zero = { .ticks = 0.0, .frames = 0 };

C_API DSPPosition dsp_position_from_ticks(f64 ticks, f64 frames_per_tick);
C_API DSPPosition dsp_position_from_frames(i64 frames, f64 ticks_per_frame);
C_API DSPPosition dsp_position_from_bars(i64 bars, i64 ticks_per_bar, f64 frames_per_tick);
C_API DSPPosition dsp_position_from_seconds(f64 seconds, f64 sample_rate, f64 ticks_per_frame);
C_API DSPPosition dsp_position_from_milliseconds(f64 ms, f64 sample_rate, f64 ticks_per_frame);
C_API DSPPosition dsp_position_between(DSPPosition, DSPPosition);
C_API DSPPosition dsp_position_closest(DSPPosition, DSPPosition a, DSPPosition b); // NOTE: Assumes positive position.

C_API i64 dsp_position_frame_compare(DSPPosition a, DSPPosition b);
C_API bool dsp_position_ticks_almost_equal(DSPPosition a, DSPPosition b);

C_API DSPPosition dsp_position_min(DSPPosition a, DSPPosition b);
C_API DSPPosition dsp_position_max(DSPPosition a, DSPPosition b);
C_API bool dsp_position_is_positive(DSPPosition a);

C_API bool dsp_position_is_between_left_inclusive(DSPPosition, DSPPosition a_inclusive, DSPPosition b_exclusive);
C_API bool dsp_position_is_between_right_inclusive(DSPPosition, DSPPosition a_exclusive, DSPPosition b_inclusive);
C_API bool dsp_position_is_between_inclusive(DSPPosition, DSPPosition, DSPPosition);
C_API bool dsp_position_is_between_exclusive(DSPPosition, DSPPosition, DSPPosition);

C_API DSPPosition dsp_position_add_frames(DSPPosition, i64 frames, f64 ticks_per_frame);
C_API DSPPosition dsp_position_add_bars(DSPPosition, i64 bars, i64 ticks_per_bar, f64 frames_per_tick);
C_API DSPPosition dsp_position_add_beats(DSPPosition, i64 beats, i64 ticks_per_beat, f64 frames_per_tick);
C_API DSPPosition dsp_position_add_sixteenths(DSPPosition, i64 sixteenths, f64 frames_per_tick);
C_API DSPPosition dsp_position_add_ticks(DSPPosition, f64 ticks, f64 frames_per_tick);
C_API DSPPosition dsp_position_add_milliseconds(DSPPosition, f64 ms, f64 sample_rate, f64 ticks_per_frame);
C_API DSPPosition dsp_position_add_minutes(DSPPosition, f64 mins, f64 sample_rate, f64 ticks_per_frame);
C_API DSPPosition dsp_position_add_seconds(DSPPosition, f64 seconds, f64 sample_rate, f64 ticks_per_frame);
C_API DSPPosition dsp_position_flip_sign(DSPPosition);

C_API f64 dsp_position_as_milliseconds(DSPPosition, f64 sample_rate);
C_API f64 dsp_position_as_seconds(DSPPosition, f64 sample_rate);

C_API i64 dsp_position_milliseconds_part(DSPPosition, f64 sample_rate);
C_API i64 dsp_position_seconds_part(DSPPosition, f64 sample_rate);
C_API i64 dsp_position_minutes_part(DSPPosition, f64 sample_rate);
C_API f64 dsp_position_ticks_part(DSPPosition, f64 frames_per_tick);

C_API i64 dsp_position_total_bars(DSPPosition, i64 ticks_per_bar, f64 frames_per_tick);
C_API i64 dsp_position_total_beats(DSPPosition, i64 beats_per_bar, f64 ticks_per_beat, f64 frames_per_tick);
C_API i64 dsp_position_total_sixteenths(DSPPosition, f64 frames_per_tick);

C_API i64 dsp_position_bars(DSPPosition, i64 ticks_per_bar);
C_API i64 dsp_position_beats(DSPPosition, i64 beats_per_bar, i64 ticks_per_beat);
C_API i64 dsp_position_sixteenths(DSPPosition, i64 beats_per_bar, i64 sixteenths_per_beat, f64 frames_per_tick);
