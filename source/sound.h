#ifndef SOUND_H
#define SOUND_H

#include <nds.h>

/*=============================================================================
 * SOUND SYSTEM
 * All sounds are generated procedurally using the NDS sound hardware.
 * No external audio files needed — everything is synthesized in code.
 *
 * The NDS has 16 sound channels:
 *   Ch 0-7:  PCM playback
 *   Ch 8-13: PCM or PSG square wave
 *   Ch 14-15: PSG noise
 *
 * We use PSG for background melody and noise for paper SFX.
 *===========================================================================*/

/*=============================================================================
 * SOUND EFFECT TYPES
 *===========================================================================*/
typedef enum {
    SFX_GRAB,           /* Picking up ragdoll */
    SFX_RELEASE,        /* Releasing ragdoll */
    SFX_IMPACT_LIGHT,   /* Light wall hit */
    SFX_IMPACT_HARD,    /* Hard wall hit */
    SFX_PAPER_RUSTLE,   /* Paper rustling (drawing, dragging) */
    SFX_PENCIL_DRAW,    /* Pencil scribbling */
    SFX_PIN_PLACE,      /* Placing a pin */
    SFX_PIN_REMOVE,     /* Removing a pin */
    SFX_ELASTIC_SNAP,   /* Rubber band snap */
    SFX_ERASER,         /* Erasing sound */
    SFX_GRAVITY_FLIP,   /* Gravity inversion whoosh */
    SFX_WIND,           /* Microphone wind */
    SFX_MISSION_START,  /* Mission begin jingle */
    SFX_MISSION_WIN,    /* Mission success fanfare */
    SFX_MENU_SELECT,    /* Tool switch click */
    SFX_COUNT
} SoundEffect;

/*=============================================================================
 * FUNCTIONS
 *===========================================================================*/

/* Initialize the sound system — call after soundEnable() */
void sound_init(void);

/* Play a sound effect */
void sound_play(SoundEffect sfx);

/* Play impact sound with variable intensity (0-127) */
void sound_play_impact(int intensity);

/* Start/stop background music */
void sound_music_start(void);
void sound_music_stop(void);

/* Update music sequencer — call once per frame */
void sound_update(void);

/* Set master volume (0-127) */
void sound_set_volume(int vol);

/* Mute/unmute all audio */
void sound_set_mute(bool muted);

#endif /* SOUND_H */
