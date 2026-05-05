/*
 * ui.h
 *
 *  Created on: Nov 21, 2021
 *      Author: sydxrey
 */

#ifndef INC_UI_H_
#define INC_UI_H_

#include "bsp/disco_lcd.h"
#include "effects.h"
#include <bsp/disco_ts.h>
#include <math.h>
#include <stdio.h>


void uiDisplayBasic(void);
void uiDisplayInputLevel(double inputLevelL, double inputLevelR);
void dispVolume(uint32_t v);
void uiSliderVolume();

void uiSliderDelay(void);
void uiSliderFeedback(void);
void uiSliderMix(void);

void uiDrawSliderLabels(void);
void uiUpdateSliders(TS_StateTypeDef *TS_State);

void uiDrawEffectButtons(void);
void uiCheckEffectTouch();
void uiUpdate();

#endif /* INC_UI_H_ */
