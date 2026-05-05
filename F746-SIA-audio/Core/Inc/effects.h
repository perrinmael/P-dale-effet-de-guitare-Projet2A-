/*
 * effects.h
 *
 *  Created on: 17 mars 2026
 *      Author: clara
 */

#ifndef SRC_EFFECTS_H_
#define SRC_EFFECTS_H_

#ifndef EFFECTS_H_
#define EFFECTS_H_

#include <stdint.h>
#include "bsp/disco_base.h" // for AUDIO_SCRATCH etc

typedef enum {
	EFFECT_NONE      = 0,  // Bypass   — button 0
	EFFECT_DELAY     = 1,  // Delay    — button 1
	EFFECT_CHORUS    = 2,  // Chorus   — button 2
	EFFECT_TREMOLO   = 3,  // Tremolo  — button 3
	EFFECT_OVERDRIVE = 4,  // Overdrive— button 4
	EFFECT_REVERB    = 5,  // Reverb   — button 5
} EffectType;

// global variables controlling effects (like sliders)
extern volatile float g_delay_ms;
extern volatile float g_feedback;
extern volatile float g_mix;

// Function prototypes
void processDelay(int16_t *out, int16_t *in, uint32_t buf_size, float feedback, float mix, float delay);
void processChorus(int16_t *out, int16_t *in, uint32_t buf_size, float rate, float depth, float delay, float feedback, float mix);
void processTremolo(int16_t *out, int16_t *in, uint32_t buf_size, float depth, float rate, float mix);

// The main dispatcher: call this in audio.c
void processAudioEffect(EffectType effect, int16_t *out, int16_t *in, uint32_t buf_size);

#endif

#endif /* SRC_EFFECTS_H_ */
