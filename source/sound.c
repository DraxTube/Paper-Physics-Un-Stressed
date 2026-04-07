#include "sound.h"
#include <string.h>
#include <stdlib.h>

/*=============================================================================
 * PCM SAMPLE BUFFERS
 * We generate short waveforms in RAM and play them via the PCM channels.
 * All samples are 8-bit signed, mono.
 *===========================================================================*/
#define SAMPLE_RATE     22050
#define NOISE_LEN       2048
#define TONE_LEN        512
#define IMPACT_LEN      1024
#define RUSTLE_LEN      1536
#define PENCIL_LEN      1024
#define WHOOSH_LEN      3072
#define JINGLE_LEN      4096

static s8 smp_noise[NOISE_LEN]    ALIGN(4);  /* White noise burst */
static s8 smp_tone_hi[TONE_LEN]   ALIGN(4);  /* High click/ping */
static s8 smp_tone_lo[TONE_LEN]   ALIGN(4);  /* Low thud */
static s8 smp_impact[IMPACT_LEN]  ALIGN(4);  /* Impact crunch */
static s8 smp_rustle[RUSTLE_LEN]  ALIGN(4);  /* Paper rustle */
static s8 smp_pencil[PENCIL_LEN]  ALIGN(4);  /* Pencil scratch */
static s8 smp_whoosh[WHOOSH_LEN]  ALIGN(4);  /* Whoosh (gravity flip) */
static s8 smp_snap[TONE_LEN]      ALIGN(4);  /* Elastic snap */
static s8 smp_jingle[JINGLE_LEN]  ALIGN(4);  /* Win jingle */

/*=============================================================================
 * SIMPLE PSEUDO-RANDOM (for noise generation)
 *===========================================================================*/
static u32 s_rng = 0xDEADBEEF;

static int noise_next(void) {
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return (int)(s_rng & 0xFF) - 128;
}

/*=============================================================================
 * GENERATE SAMPLES
 * All procedurally synthesized — no external files needed.
 *===========================================================================*/
static void generate_samples(void) {
    int i;

    /*--- White noise burst (paper sounds base) ---*/
    for (i = 0; i < NOISE_LEN; i++) {
        int env = 127 - (i * 127 / NOISE_LEN); /* Decay envelope */
        smp_noise[i] = (s8)((noise_next() * env) >> 7);
    }

    /*--- High click/ping (grab, menu select) ---*/
    for (i = 0; i < TONE_LEN; i++) {
        int env = 127 - (i * 127 / TONE_LEN);
        /* ~1kHz square wave with decay */
        int phase = (i * 1000 / SAMPLE_RATE); /* period index */
        int wave = (phase & 1) ? 80 : -80;
        smp_tone_hi[i] = (s8)((wave * env) >> 7);
    }

    /*--- Low thud (impact base) ---*/
    for (i = 0; i < TONE_LEN; i++) {
        int env = 127 - (i * 127 / TONE_LEN);
        /* ~200Hz sine-ish approximation + noise */
        int phase = (i * 200 * 256 / SAMPLE_RATE) & 0xFF;
        int wave = (phase < 128) ? (phase - 64) : (192 - phase);
        int n = noise_next() >> 3;
        smp_tone_lo[i] = (s8)(((wave + n) * env) >> 7);
    }

    /*--- Impact crunch (wall hit — noise + low freq) ---*/
    for (i = 0; i < IMPACT_LEN; i++) {
        int env;
        if (i < 30) {
            env = i * 127 / 30; /* Quick attack */
        } else {
            env = 127 - ((i - 30) * 127 / (IMPACT_LEN - 30)); /* Decay */
        }
        int n = noise_next();
        /* Mix noise with low rumble */
        int phase = (i * 150 * 256 / SAMPLE_RATE) & 0xFF;
        int rumble = (phase < 128) ? 60 : -60;
        int mix = (n * 3 + rumble * 2) / 5;
        smp_impact[i] = (s8)((mix * env) >> 7);
    }

    /*--- Paper rustle (filtered noise with flutter) ---*/
    for (i = 0; i < RUSTLE_LEN; i++) {
        int env;
        /* Wavy envelope for rustle feel */
        int wave_env = 80 + ((i * 7) % 47) - 23;
        if (i < 100) {
            env = i * wave_env / 100;
        } else if (i > RUSTLE_LEN - 200) {
            env = (RUSTLE_LEN - i) * wave_env / 200;
        } else {
            env = wave_env;
        }
        /* Bandpass-ish: mix two noise sources */
        int n1 = noise_next();
        int n2 = noise_next();
        int filtered = (n1 + n2) / 2; /* Crude low-pass by averaging */
        smp_rustle[i] = (s8)((filtered * env) >> 7);
    }

    /*--- Pencil scratch (high-freq modulated noise) ---*/
    for (i = 0; i < PENCIL_LEN; i++) {
        int env = 100 - (i * 60 / PENCIL_LEN);
        int n = noise_next();
        /* High-pass effect: difference of consecutive samples */
        int scratch = n - ((i > 0) ? smp_pencil[i-1] : 0);
        if (scratch > 127) scratch = 127;
        if (scratch < -128) scratch = -128;
        smp_pencil[i] = (s8)((scratch * env) >> 7);
    }

    /*--- Whoosh (gravity flip — swept noise) ---*/
    for (i = 0; i < WHOOSH_LEN; i++) {
        int env;
        if (i < WHOOSH_LEN / 4) {
            env = i * 127 / (WHOOSH_LEN / 4);
        } else {
            env = 127 - ((i - WHOOSH_LEN / 4) * 127 / (WHOOSH_LEN * 3 / 4));
        }
        int n = noise_next();
        /* Modulate with a sweep frequency */
        int sweep_freq = 100 + (i * 900 / WHOOSH_LEN);
        int phase = (i * sweep_freq * 256 / SAMPLE_RATE) & 0xFF;
        int mod = (phase < 128) ? 1 : 0;
        smp_whoosh[i] = (s8)((n * mod * env) >> 7);
    }

    /*--- Elastic snap (quick descending tone) ---*/
    for (i = 0; i < TONE_LEN; i++) {
        int env = 127 - (i * 127 / TONE_LEN);
        int freq = 2000 - (i * 1500 / TONE_LEN); /* Descending pitch */
        int phase = (i * freq * 256 / SAMPLE_RATE) & 0xFF;
        int wave = (phase < 128) ? 90 : -90;
        smp_snap[i] = (s8)((wave * env) >> 7);
    }

    /*--- Win jingle (ascending arpeggio) ---*/
    {
        /* Simple major chord arpeggio: C-E-G-C */
        int notes[] = { 523, 659, 784, 1047 };  /* C5, E5, G5, C6 */
        int note_len = JINGLE_LEN / 4;

        for (i = 0; i < JINGLE_LEN; i++) {
            int note_idx = i / note_len;
            if (note_idx > 3) note_idx = 3;
            int pos_in_note = i % note_len;

            int env;
            if (pos_in_note < 50) {
                env = pos_in_note * 120 / 50;
            } else {
                env = 120 - (pos_in_note * 80 / note_len);
            }
            if (env < 0) env = 0;

            int freq = notes[note_idx];
            int phase = (i * freq * 256 / SAMPLE_RATE) & 0xFF;
            /* Triangle wave for softer sound */
            int wave;
            if (phase < 128) {
                wave = phase - 64;
            } else {
                wave = 192 - phase;
            }
            smp_jingle[i] = (s8)((wave * env) >> 7);
        }
    }
}

/*=============================================================================
 * MUSIC SEQUENCER
 * Simple pattern-based sequencer using PSG channels for a gentle,
 * playful notebook-theme melody.
 *===========================================================================*/

/* Musical note frequencies (Hz) */
#define NOTE_REST 0
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_G3   196
#define NOTE_A3   220
#define NOTE_B3   247
#define NOTE_F3   175
#define NOTE_E3   165

/* Melody pattern (gentle, whimsical tune in C major) */
/* Each entry: {note_freq, duration_in_frames} */
typedef struct {
    u16 freq;
    u8  duration; /* in frames (60fps) */
} MusicNote;

/* Melody line (PSG channel 8) */
static const MusicNote melody[] = {
    {NOTE_E4, 15}, {NOTE_G4, 15}, {NOTE_A4, 15}, {NOTE_G4, 15},
    {NOTE_E4, 15}, {NOTE_D4, 15}, {NOTE_C4, 30},
    {NOTE_D4, 15}, {NOTE_E4, 15}, {NOTE_G4, 15}, {NOTE_E4, 15},
    {NOTE_D4, 30}, {NOTE_REST, 30},

    {NOTE_E4, 15}, {NOTE_G4, 15}, {NOTE_A4, 15}, {NOTE_B4, 15},
    {NOTE_C5, 30}, {NOTE_A4, 15}, {NOTE_G4, 15},
    {NOTE_E4, 15}, {NOTE_D4, 15}, {NOTE_C4, 30},
    {NOTE_REST, 30},

    {NOTE_C4, 15}, {NOTE_E4, 15}, {NOTE_G4, 30},
    {NOTE_A4, 15}, {NOTE_G4, 15}, {NOTE_E4, 15}, {NOTE_D4, 15},
    {NOTE_C4, 30}, {NOTE_REST, 15}, {NOTE_D4, 15},
    {NOTE_E4, 30}, {NOTE_C4, 30},
    {NOTE_REST, 30},
};
#define MELODY_LEN (sizeof(melody) / sizeof(melody[0]))

/* Bass line (PSG channel 9) — simple root notes */
static const MusicNote bassline[] = {
    {NOTE_C4/2, 30}, {NOTE_G3, 30}, {NOTE_A3, 30}, {NOTE_E3, 30},
    {NOTE_F3, 30}, {NOTE_G3, 30}, {NOTE_C4/2, 60},
    {NOTE_REST, 30},
    {NOTE_C4/2, 30}, {NOTE_E3, 30}, {NOTE_G3, 30}, {NOTE_A3, 30},
    {NOTE_F3, 30}, {NOTE_G3, 30}, {NOTE_C4/2, 60},
    {NOTE_REST, 30},
};
#define BASS_LEN (sizeof(bassline) / sizeof(bassline[0]))

/* Sequencer state */
static struct {
    bool     playing;
    int      melody_pos;
    int      melody_timer;
    int      bass_pos;
    int      bass_timer;
    int      melody_channel;
    int      bass_channel;
    int      volume;
    bool     muted;
} music;

/*=============================================================================
 * INIT
 *===========================================================================*/
void sound_init(void) {
    generate_samples();

    music.playing = false;
    music.melody_pos = 0;
    music.melody_timer = 0;
    music.bass_pos = 0;
    music.bass_timer = 0;
    music.melody_channel = -1;
    music.bass_channel = -1;
    music.volume = 80;
    music.muted = false;
}

/*=============================================================================
 * PLAY SOUND EFFECT
 *===========================================================================*/
void sound_play(SoundEffect sfx) {
    if (music.muted) return;

    int vol = music.volume;
    int pan = 64; /* Center */

    switch (sfx) {
        case SFX_GRAB:
            soundPlaySample(smp_tone_hi, SoundFormat_8Bit, TONE_LEN,
                           SAMPLE_RATE, vol, pan, false, 0);
            break;

        case SFX_RELEASE:
            soundPlaySample(smp_tone_hi, SoundFormat_8Bit, TONE_LEN,
                           SAMPLE_RATE / 2, vol * 3/4, pan, false, 0);
            break;

        case SFX_IMPACT_LIGHT:
            soundPlaySample(smp_impact, SoundFormat_8Bit, IMPACT_LEN / 2,
                           SAMPLE_RATE, vol / 2, pan, false, 0);
            break;

        case SFX_IMPACT_HARD:
            soundPlaySample(smp_impact, SoundFormat_8Bit, IMPACT_LEN,
                           SAMPLE_RATE, vol, pan, false, 0);
            break;

        case SFX_PAPER_RUSTLE:
            soundPlaySample(smp_rustle, SoundFormat_8Bit, RUSTLE_LEN,
                           SAMPLE_RATE, vol * 2/3, pan, false, 0);
            break;

        case SFX_PENCIL_DRAW:
            soundPlaySample(smp_pencil, SoundFormat_8Bit, PENCIL_LEN,
                           SAMPLE_RATE * 3/2, vol / 2, pan, false, 0);
            break;

        case SFX_PIN_PLACE:
            soundPlaySample(smp_tone_hi, SoundFormat_8Bit, TONE_LEN / 2,
                           SAMPLE_RATE * 2, vol, pan, false, 0);
            break;

        case SFX_PIN_REMOVE:
            soundPlaySample(smp_tone_hi, SoundFormat_8Bit, TONE_LEN / 2,
                           SAMPLE_RATE, vol * 2/3, pan, false, 0);
            break;

        case SFX_ELASTIC_SNAP:
            soundPlaySample(smp_snap, SoundFormat_8Bit, TONE_LEN,
                           SAMPLE_RATE, vol, pan, false, 0);
            break;

        case SFX_ERASER:
            soundPlaySample(smp_rustle, SoundFormat_8Bit, RUSTLE_LEN / 2,
                           SAMPLE_RATE * 2, vol / 2, pan, false, 0);
            break;

        case SFX_GRAVITY_FLIP:
            soundPlaySample(smp_whoosh, SoundFormat_8Bit, WHOOSH_LEN,
                           SAMPLE_RATE, vol, pan, false, 0);
            break;

        case SFX_WIND:
            soundPlaySample(smp_noise, SoundFormat_8Bit, NOISE_LEN,
                           SAMPLE_RATE / 2, vol / 2, pan, false, 0);
            break;

        case SFX_MISSION_START:
            soundPlaySample(smp_tone_hi, SoundFormat_8Bit, TONE_LEN,
                           SAMPLE_RATE * 3/2, vol, pan, false, 0);
            break;

        case SFX_MISSION_WIN:
            soundPlaySample(smp_jingle, SoundFormat_8Bit, JINGLE_LEN,
                           SAMPLE_RATE, vol, pan, false, 0);
            break;

        case SFX_MENU_SELECT:
            soundPlaySample(smp_tone_hi, SoundFormat_8Bit, TONE_LEN / 4,
                           SAMPLE_RATE * 5/4, vol * 2/3, pan, false, 0);
            break;

        default:
            break;
    }
}

/*=============================================================================
 * PLAY IMPACT WITH VARIABLE INTENSITY
 *===========================================================================*/
void sound_play_impact(int intensity) {
    if (music.muted) return;
    if (intensity < 10) return; /* Too weak to hear */

    int vol = intensity;
    if (vol > 127) vol = 127;

    /* Higher intensity = lower pitch (heavier thud) + louder */
    int rate = SAMPLE_RATE + (SAMPLE_RATE * (127 - intensity) / 200);

    soundPlaySample(smp_impact, SoundFormat_8Bit, IMPACT_LEN,
                   rate, vol, 64, false, 0);
}

/*=============================================================================
 * MUSIC CONTROL
 *===========================================================================*/
void sound_music_start(void) {
    music.playing = true;
    music.melody_pos = 0;
    music.melody_timer = 0;
    music.bass_pos = 0;
    music.bass_timer = 0;
}

void sound_music_stop(void) {
    music.playing = false;
    if (music.melody_channel >= 0) {
        soundKill(music.melody_channel);
        music.melody_channel = -1;
    }
    if (music.bass_channel >= 0) {
        soundKill(music.bass_channel);
        music.bass_channel = -1;
    }
}

/*=============================================================================
 * MUSIC SEQUENCER UPDATE (call once per frame)
 *===========================================================================*/
void sound_update(void) {
    if (!music.playing || music.muted) return;

    int vol = music.volume / 3; /* Music quieter than SFX */
    if (vol < 1) vol = 1;

    /*--- Melody ---*/
    if (music.melody_timer <= 0) {
        const MusicNote *note = &melody[music.melody_pos];

        /* Kill previous note */
        if (music.melody_channel >= 0) {
            soundKill(music.melody_channel);
            music.melody_channel = -1;
        }

        /* Play new note */
        if (note->freq > 0) {
            music.melody_channel = soundPlayPSG(DutyCycle_50,
                                                note->freq, vol, 64);
        }

        music.melody_timer = note->duration;
        music.melody_pos = (music.melody_pos + 1) % MELODY_LEN;
    }
    music.melody_timer--;

    /*--- Bass ---*/
    if (music.bass_timer <= 0) {
        const MusicNote *note = &bassline[music.bass_pos];

        if (music.bass_channel >= 0) {
            soundKill(music.bass_channel);
            music.bass_channel = -1;
        }

        if (note->freq > 0) {
            music.bass_channel = soundPlayPSG(DutyCycle_25,
                                              note->freq, vol * 2/3, 64);
        }

        music.bass_timer = note->duration;
        music.bass_pos = (music.bass_pos + 1) % BASS_LEN;
    }
    music.bass_timer--;
}

/*=============================================================================
 * VOLUME / MUTE
 *===========================================================================*/
void sound_set_volume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 127) vol = 127;
    music.volume = vol;
}

void sound_set_mute(bool muted) {
    music.muted = muted;
    if (muted) {
        if (music.melody_channel >= 0) {
            soundKill(music.melody_channel);
            music.melody_channel = -1;
        }
        if (music.bass_channel >= 0) {
            soundKill(music.bass_channel);
            music.bass_channel = -1;
        }
    }
}
