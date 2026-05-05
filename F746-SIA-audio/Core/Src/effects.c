/*
 * effects.c
 *
 *  Created on: 17 mars 2026
 *      Author: clara
 */

#include "effects.h"
#include <math.h>
#include <stdio.h>

#define AUDIO_SAMPLING_FREQ 48000

// delay parameters (from main.c / UI)
extern volatile float g_delay_ms;
extern volatile float g_feedback;
extern volatile float g_mix;

// simple chorus parameters
extern volatile int c_depth;
extern volatile int c_rate;
extern volatile int c_delay;
extern volatile float c_feedback;
extern volatile float c_mix;

// SDRAM access (reuse existing functions)
extern int16_t readInt16FromSDRAM(int pos);
extern void writeInt16ToSDRAM(int16_t val, int pos);

// Scratch buffer
extern uint32_t scratch_offset;
#define AUDIO_SCRATCH_SIZE AUDIO_SCRATCH_MAXSZ_WORDS

// Include your SDRAM read/write functions
extern int16_t readInt16FromSDRAM(int pos);
extern void writeInt16ToSDRAM(int16_t val, int pos);

// delay
void processDelay(int16_t *out, int16_t *in, uint32_t buf_size, float feedback, float mix, float delay)
/* ---- parameters ----
 * - delay in ms
 * - feedback in [0, 1]
 * - mix in [0, 1]   */
{
    uint32_t scratch_size = AUDIO_SCRATCH_MAXSZ_WORDS;

    // convert delay from ms to samples (stereo interleaved)
    uint32_t delay_samples = (uint32_t)((delay / 1000.0f) * AUDIO_SAMPLING_FREQ);
    uint32_t delay_words = 2 * delay_samples; // times 2 for stereo


    for (int n = 0; n < AUDIO_BUF_SIZE; n += 2)
    {
        /* ---- Input (int16 → float) ---- */
        float xL = in[n]     / 32768.0f;
        float xR = in[n + 1] / 32768.0f;

        /* ---- Delay read index (ring buffer) ---- */
        uint32_t readIdx = (scratch_offset + scratch_size - delay_words) % scratch_size;

        /* ---- Read delayed samples from SDRAM ---- */
        float dL = readInt16FromSDRAM(readIdx)     / 32768.0f;
        float dR = readInt16FromSDRAM(readIdx + 1) / 32768.0f;

        /* ---- Mix dry/wet ---- */
        float yL = (1.0f - mix) * xL + mix * dL;
        float yR = (1.0f - mix) * xR + mix * dR;

        /* ---- Write feedback into SDRAM ---- */
        int16_t fbL = (int16_t)((xL + feedback * dL) * 32767.0f);
        int16_t fbR = (int16_t)((xR + feedback * dR) * 32767.0f);

        writeInt16ToSDRAM(fbL, scratch_offset);
        writeInt16ToSDRAM(fbR, scratch_offset + 1);

        /* ---- Output ---- */
        out[n]     = (int16_t)(yL * 32767.0f);
        out[n + 1] = (int16_t)(yR * 32767.0f);

        /* ---- Advance ring buffer ---- */
        scratch_offset += 2;
        if (scratch_offset >= scratch_size)
            scratch_offset = 0;
    }
}

// simple chorus (modulated delay)
void processChorus(int16_t *out, int16_t *in, uint32_t buf_size, float rate, float depth, float delay, float feedback, float mix)
/* ---- parameters ----
 * - rate:     LFO frequency in Hz
 * - depth:    max delay modulation in samples
 * - delay:    base delay in samples
 * - feedback: in [0, 1]
 * - mix:      dry/wet in [0, 1]   */
{
    uint32_t scratch_size = AUDIO_SCRATCH_MAXSZ_WORDS / 2; // split scratch in half
    static float lfo_phase = 0.0f;
    static uint32_t sample_count = 0;
    static uint32_t out_scratch_offset = 0;

    const float g = 0.15f;
    const float lfo_phase_offset = 3.0f;
    const float lfo_increment = (2.0f * M_PI * rate) / AUDIO_SAMPLING_FREQ;

    // second half of scratch for output history
    uint32_t out_scratch_base = AUDIO_SCRATCH_MAXSZ_WORDS / 2;

    for (int n = 0; n < buf_size; n += 2)
    {
        /* ---- LFO ---- */
        float lfo = sinf(lfo_phase + lfo_phase_offset);
        float M   = delay + depth * lfo;
        uint32_t d = (uint32_t)M;

        lfo_phase += lfo_increment;
        if (lfo_phase >= 2.0f * M_PI) lfo_phase -= 2.0f * M_PI;

        /* ---- Input (int16 → float) ---- */
        float xL = in[n]     / 32768.0f;
        float xR = in[n + 1] / 32768.0f;

        uint32_t delay_words = 2 * d;

        float yL = (1.0f - mix) * xL;
        float yR = (1.0f - mix) * xR;

        if (sample_count > d && delay_words < scratch_size)
        {
            /* ---- Read delayed INPUT from first half of scratch ---- */
            uint32_t readIdx = (scratch_offset + scratch_size - delay_words) % scratch_size;
            float dL = readInt16FromSDRAM(readIdx)     / 32768.0f;
            float dR = readInt16FromSDRAM(readIdx + 1) / 32768.0f;

            /* ---- Read delayed OUTPUT from second half of scratch ---- */
            uint32_t outReadIdx = (out_scratch_offset + scratch_size - delay_words) % scratch_size + out_scratch_base;
            float outL_delayed = readInt16FromSDRAM(outReadIdx)     / 32768.0f;
            float outR_delayed = readInt16FromSDRAM(outReadIdx + 1) / 32768.0f;

            /* ---- Mix + feedback ---- */
            yL += mix * g * dL + feedback * outL_delayed;
            yR += mix * g * dR + feedback * outR_delayed;

            /* ---- Soft clip (tanh) ---- */
            yL = tanhf(yL);
            yR = tanhf(yR);
        }

        /* ---- Write input into first half ring buffer ---- */
        writeInt16ToSDRAM((int16_t)(xL * 32767.0f), scratch_offset);
        writeInt16ToSDRAM((int16_t)(xR * 32767.0f), scratch_offset + 1);

        /* ---- Write output into second half ring buffer ---- */
        writeInt16ToSDRAM((int16_t)(yL * 32767.0f), out_scratch_base + out_scratch_offset);
        writeInt16ToSDRAM((int16_t)(yR * 32767.0f), out_scratch_base + out_scratch_offset + 1);

        /* ---- Output ---- */
        out[n]     = (int16_t)(yL * 32767.0f);
        out[n + 1] = (int16_t)(yR * 32767.0f);

        /* ---- Advance ring buffers ---- */
        scratch_offset += 2;
        if (scratch_offset >= scratch_size) scratch_offset = 0;

        out_scratch_offset += 2;
        if (out_scratch_offset >= scratch_size) out_scratch_offset = 0;

        sample_count++;
    }
}

// tremolo
void processTremolo(int16_t *out, int16_t *in, uint32_t buf_size, float depth, float rate, float mix)
/* ---- parameters ----
 * - depth: LFO amplitude, keep in [0, 0.5] to avoid overmodulation
 * - rate:  LFO frequency in Hz (slow: 2-5Hz, fast: 10-20Hz)
 * - mix:   dry/wet in [0, 1]   */
{
    static float counter    = 0.0f;
    static float c_direction = 1.0f;

    // quarter period in samples, same formula as Python
    float c_limit = (1.0f / 4.0f) * ((float)AUDIO_SAMPLING_FREQ / rate);

    for (int n = 0; n < buf_size; n += 2)
    {
        /* ---- Advance triangular LFO counter ---- */
        counter += c_direction;

        if (counter >= c_limit) {
            counter     = c_limit;
            c_direction = -1.0f;
        } else if (counter <= -c_limit) {
            counter     = -c_limit;
            c_direction  = 1.0f;
        }

        /* ---- Normalize to [-1, 1] then shift to [0, 1], scale by depth ---- */
        float lfo = (counter / c_limit + 1.0f) * depth;

        /* ---- Input (int16 → float) ---- */
        float xL = in[n]     / 32768.0f;
        float xR = in[n + 1] / 32768.0f;

        /* ---- Apply tremolo ---- */
        float yL = (1.0f - mix) * xL + mix * xL * lfo;
        float yR = (1.0f - mix) * xR + mix * xR * lfo;

        /* ---- Output ---- */
        out[n]     = (int16_t)(yL * 32767.0f);
        out[n + 1] = (int16_t)(yR * 32767.0f);
    }
}

// Dispatcher: call the right effect
void processAudioEffect(EffectType effect, int16_t *out, int16_t *in, uint32_t buf_size)
{
    switch(effect)
    {
        case EFFECT_DELAY:  processDelay(out, in, buf_size, g_feedback, g_mix, g_delay_ms); break;
        case EFFECT_CHORUS: processChorus(out, in, buf_size, 0.5f, 240.0f, 480.0f, 0.3f, 0.5f); break;
        case EFFECT_TREMOLO: processTremolo(out, in, buf_size, 0.5f, 8.0f, 1.0f); break;
        case EFFECT_OVERDRIVE:
        case EFFECT_REVERB:
        case EFFECT_NONE:
        default:
            // just passthrough
            for(int n=0;n<buf_size;n++) out[n] = in[n];
            break;
    }

    // Apply gain of 15 to all outputs safely
    for(int n=0; n<buf_size; n++) {
        int32_t temp = 15 * (int32_t)out[n]; // promote to 32-bit to avoid overflow
        if(temp > 32767) temp = 32767;
        if(temp < -32768) temp = -32768;
        out[n] = (int16_t)temp;
    }
}
