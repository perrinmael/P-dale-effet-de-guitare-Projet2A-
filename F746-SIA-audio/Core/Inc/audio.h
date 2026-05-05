/*
 * audio_processing.h
 *
 *  Created on: May 17, 2021
 *      Author: sydxrey
 */

#ifndef INC_AUDIO_H_
#define INC_AUDIO_H_

#include "stdint.h"

void audioLoop();
void audioLoop_no_ui();

int16_t readInt16FromSDRAM(int pos);
void writeInt16ToSDRAM(int16_t val, int pos);

void audioCallback();
static void processAudio();

#endif /* INC_AUDIO_H_ */
