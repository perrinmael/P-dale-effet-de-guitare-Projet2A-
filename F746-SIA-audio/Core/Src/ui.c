/*
 * ui.c
 *
 *  Created on: Nov 21, 2021
 *      Author: sydxrey
 *
 *
 *      LCD and touchscreen management for project User Interface.
 */

#include <ui.h>
#define MAX_DELAY_MS 2000 //2s
#define NUM_EFFECT_BUTTONS (sizeof(effectButtons)/sizeof(effectButtons[0]))


typedef struct {
    int x, y, w, h;
    const char* label;
    uint32_t color;
} Button;

Button effectButtons[] = {
    {240,  60, 110, 45, "Bypass",    LCD_COLOR_LIGHTGRAY},
    {240, 120, 110, 45, "Delay",     LCD_COLOR_CYAN},
    {240, 180, 110, 45, "Chorus",    LCD_COLOR_GREEN},
    {360,  60, 110, 45, "Tremolo",   LCD_COLOR_MAGENTA},
    {360, 120, 110, 45, "Overdrive", LCD_COLOR_ORANGE},
    {360, 180, 110, 45, "Reverb",    LCD_COLOR_BLUE},
};

#define NUM_EFFECT_BUTTONS (sizeof(effectButtons)/sizeof(effectButtons[0]))

// --------------------------------------------------------
// Slider config per effect
// --------------------------------------------------------

typedef struct {
    const char* label1;
    const char* label2;
    const char* label3;
    uint32_t    color;
} SliderConfig;

static const SliderConfig sliderConfigs[] = {
    // EFFECT_NONE
    {"Volume", "---",      "---",      LCD_COLOR_LIGHTGRAY},
    // EFFECT_DELAY
    {"Delay",  "Feedback", "Mix",      LCD_COLOR_CYAN},
    // EFFECT_CHORUS
    {"Depth",  "Rate",     "Mix",      LCD_COLOR_GREEN},
    // EFFECT_TREMOLO
    {"Depth",  "Rate",     "Mix",      LCD_COLOR_MAGENTA},
    // EFFECT_OVERDRIVE
    {"Drive",  "Tone",     "Mix",      LCD_COLOR_ORANGE},
    // EFFECT_REVERB
    {"Size",   "Damp",     "Mix",      LCD_COLOR_BLUE},
};


// Slider positions, range 0..100
extern uint8_t sliderDelayPosition;
extern uint8_t sliderFeedbackPosition;
extern uint8_t sliderMixPosition;

// --------------------------------------------------------
// Externs
// --------------------------------------------------------
extern volatile EffectType currentEffect;

// delay
extern volatile float g_delay_ms;
extern volatile float g_feedback;
extern volatile float g_mix;

// chorus
extern volatile float c_depth;
extern volatile float c_rate;
extern volatile float c_delay;
extern volatile float c_feedback;
extern volatile float c_mix;

// tremolo
extern volatile float t_depth;
extern volatile float t_rate;
extern volatile float t_mix;

// overdrive
extern volatile float od_drive;
extern volatile float od_tone;
extern volatile float od_mix;

// reverb
extern volatile float rv_size;
extern volatile float rv_damp;
extern volatile float rv_mix;

// generic slider values (0..1 normalized), mapped per effect
static float slider1Val = 0.5f;
static float slider2Val = 0.5f;
static float slider3Val = 0.5f;

int volume = 50;


// --------------------------------------------------------
// Helpers
// --------------------------------------------------------

/**
 * Display basic UI information.
 */

static uint32_t getActiveColor(void) {
    return sliderConfigs[currentEffect].color;
}

static void mapSlidersToEffect(void) {
    switch(currentEffect) {
        case EFFECT_NONE:
            break;
        case EFFECT_DELAY:
            g_delay_ms = slider1Val * MAX_DELAY_MS;
            g_feedback = slider2Val;
            g_mix      = slider3Val;
            break;
        case EFFECT_CHORUS:
            c_depth    = slider1Val * 500.0f;    // 0..500 samples
            c_rate     = slider2Val * 5.0f;      // 0..5 Hz
            c_mix      = slider3Val;
            break;
        case EFFECT_TREMOLO:
            t_depth    = slider1Val * 0.5f;      // keep in [0, 0.5]
            t_rate     = slider2Val * 20.0f;     // 0..20 Hz
            t_mix      = slider3Val;
            break;
        case EFFECT_OVERDRIVE:
            od_drive   = slider1Val;
            od_tone    = slider2Val;
            od_mix     = slider3Val;
            break;
        case EFFECT_REVERB:
            rv_size    = slider1Val;
            rv_damp    = slider2Val;
            rv_mix     = slider3Val;
            break;
    }
}

// --------------------------------------------------------
// Draw a single slider (vertical, fills from bottom)
// --------------------------------------------------------

static void drawSlider(int x, int y, int w, int h, float val, uint32_t color, const char* label) {
    // background
    LCD_SetFillColor(LCD_COLOR_LIGHTGRAY);
    LCD_FillRect(x, y, w, h);

    // fill
    int fill = (int)(val * h);
    LCD_SetFillColor(color);
    LCD_FillRect(x, y + h - fill, w, fill);

    // border
    LCD_SetStrokeColor(LCD_COLOR_BLACK);
    LCD_DrawRect(x, y, w, h);

    // label
    LCD_SetFont(&Font12);
    LCD_SetBackColor(LCD_COLOR_WHITE);
    LCD_SetStrokeColor(LCD_COLOR_BLACK);
    LCD_DrawString(x, y + h + 5, (uint8_t*)label, LEFT_MODE, true);
}

// --------------------------------------------------------
// Touch a slider, return new value or unchanged
// --------------------------------------------------------
static float touchSlider(uint16_t xt, uint16_t yt, int x, int y, int w, int h, float current) {
    if(xt > x && xt < x+w && yt > y && yt < y+h) {
        return 1.0f - ((float)(yt - y) / (float)h);
    }
    return current;
}

// --------------------------------------------------------
// Public API
// --------------------------------------------------------
void uiDisplayBasic(void) {
    LCD_Clear(LCD_COLOR_WHITE);
    LCD_SetStrokeColor(LCD_COLOR_BLACK);
    LCD_SetBackColor(LCD_COLOR_WHITE);
    uiDrawEffectButtons();
    uiDrawSliderLabels();
}

void uiDrawSliderLabels(void) {
    const SliderConfig* cfg = &sliderConfigs[currentEffect];
    LCD_SetFont(&Font12);
    LCD_SetBackColor(LCD_COLOR_WHITE);
    LCD_SetStrokeColor(LCD_COLOR_BLACK);
    // clear label area first
    LCD_SetFillColor(LCD_COLOR_WHITE);
    LCD_FillRect(20, 230, 280, 20);
    LCD_DrawString(30,  230, (uint8_t*)cfg->label1, LEFT_MODE, true);
    LCD_DrawString(100, 230, (uint8_t*)cfg->label2, LEFT_MODE, true);
    LCD_DrawString(170, 230, (uint8_t*)cfg->label3, LEFT_MODE, true);
}

void uiDrawEffectButtons(void) {
    for(int i = 0; i < NUM_EFFECT_BUTTONS; i++) {
        Button* b = &effectButtons[i];

        // highlight active effect
        EffectType e = (EffectType)i; // assumes enum order matches button order
        if(e == currentEffect) {
            LCD_SetFillColor(b->color);
            LCD_FillRect(b->x, b->y, b->w, b->h);
        } else {
            LCD_SetFillColor(LCD_COLOR_WHITE);
            LCD_FillRect(b->x, b->y, b->w, b->h);
        }

        LCD_SetStrokeColor(LCD_COLOR_BLACK);
        LCD_DrawRect(b->x, b->y, b->w, b->h);
        LCD_SetFont(&Font12);
        LCD_SetBackColor(e == currentEffect ? b->color : LCD_COLOR_WHITE);
        LCD_DrawString(b->x + 8, b->y + 15, (uint8_t*)b->label, LEFT_MODE, true);
    }
}

void uiCheckEffectTouch(TS_StateTypeDef *TS_State) {
    if(!TS_State->touchDetected) return;

    uint16_t xt = TS_State->touchX[0];
    uint16_t yt = TS_State->touchY[0];

    for(int i = 0; i < NUM_EFFECT_BUTTONS; i++) {
        Button* b = &effectButtons[i];
        if(xt > b->x && xt < b->x+b->w && yt > b->y && yt < b->y+b->h) {
            currentEffect = (EffectType)i;
            uiDrawEffectButtons();
            uiDrawSliderLabels();
        }
    }
}

void uiUpdateSliders(TS_StateTypeDef *TS_State) {
    int x1 = 30,  y = 60, w = 28, h = 160;
    int x2 = 100, x3 = 170;

    uint32_t color = getActiveColor();

    if(TS_State->touchDetected) {
        uint16_t xt = TS_State->touchX[0];
        uint16_t yt = TS_State->touchY[0];

        slider1Val = touchSlider(xt, yt, x1, y, w, h, slider1Val);
        slider2Val = touchSlider(xt, yt, x2, y, w, h, slider2Val);
        slider3Val = touchSlider(xt, yt, x3, y, w, h, slider3Val);

        mapSlidersToEffect();
    }

    const SliderConfig* cfg = &sliderConfigs[currentEffect];
    drawSlider(x1, y, w, h, slider1Val, color, cfg->label1);
    drawSlider(x2, y, w, h, slider2Val, color, cfg->label2);
    drawSlider(x3, y, w, h, slider3Val, color, cfg->label3);
}

/**
 * Displays line or microphones input level on the LCD.
 */
void uiDisplayInputLevel(double inputLevelL, double inputLevelR) {

	uint8_t buf[50];

	LCD_SetStrokeColor(LCD_COLOR_BLACK);
	LCD_SetBackColor(LCD_COLOR_WHITE);
	LCD_SetFont(&Font12);

	/*sprintf((char *)buf, "%d     ", (int)(inputLevelL));
	 LCD_DisplayStringAt(90, 30, (uint8_t *)buf, LEFT_MODE);

	 sprintf((char *)buf, "%d     ", (int)(inputLevelR));
	 LCD_DisplayStringAt(90, 50, (uint8_t *)buf, LEFT_MODE);*/

	if (inputLevelL > 0) {
		int lvl_db = (int) (20. * log10(inputLevelL));
		sprintf((char*) buf, "%d dB   ", lvl_db);
		LCD_DrawString(90, 30, (uint8_t*) buf, LEFT_MODE, true);
	} else
		LCD_DrawString(90, 30, (uint8_t*) "-inf dB", LEFT_MODE, true);

	if (inputLevelR > 0) {
		int lvl_db = (int) (20. * log10(inputLevelR));
		sprintf((char*) buf, "%d dB   ", lvl_db);
		LCD_DrawString(90, 50, (uint8_t*) buf, LEFT_MODE, true);
	} else
		LCD_DrawString(90, 50, (uint8_t*) "-inf dB", LEFT_MODE, true);

}

/**
 * Displays the volume in %
 */
void dispVolume(uint32_t v){
	char volume_char[10];
	sprintf(volume_char, "%lu  ",v);
	uint16_t X = 440;
	uint16_t Y = 245;
	LCD_DrawString(X,Y,(uint8_t *)volume_char, LEFT_MODE, true);
}

/**
 * Displays a slider to control the volume
 */
void uiSliderVolume(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

		/* Get X and Y position of the touch post calibrated */
		uint16_t xt = TS_State.touchX[0];
		uint16_t yt = TS_State.touchY[0];
		/* Init position and size of elements */
		int x = 435;
		int y = 36;
		int w = 30;
		int h = 200;

		/* If touch is in the volume bar */
		if(xt>x && xt<x+w && yt>y && yt<y+h){
			/* Position yt into volume value in % */
			volume = 100-((yt-y))/2;
			/* Display new volume value */
			dispVolume((uint32_t) volume);

			/* RED Bar for level */
			LCD_SetFillColor(LCD_COLOR_RED);
			LCD_FillRect(x+1, yt, w-1, h-yt+y);
			/* WHITE Bar for update */
			LCD_SetFillColor(LCD_COLOR_WHITE);
			LCD_FillRect(x+1, y+1, w-1, yt-y-1);
		}
	}
}

void uiSliderDelay(void) {
    TS_StateTypeDef TS_State;
    TS_GetState(&TS_State);

    int x = 50, y = 50, w = 28, h = 140;

    // --- Update value if touched ---
    if(TS_State.touchDetected){
        uint16_t xt = TS_State.touchX[0];
        uint16_t yt = TS_State.touchY[0];

        if(xt > x && xt < x+w && yt > y && yt < y+h){
            g_delay_ms = MAX_DELAY_MS - ((yt - y) * MAX_DELAY_MS / h);
        }
    }

    // --- ALWAYS draw slider ---
    LCD_SetFillColor(LCD_COLOR_LIGHTGRAY);
    LCD_FillRect(x, y, w, h); // background

    LCD_SetFillColor(LCD_COLOR_RED);
    int fill = (g_delay_ms * h) / MAX_DELAY_MS;
    LCD_FillRect(x, y + h - fill, w, fill);
}

void uiSliderFeedback(void) {
    TS_StateTypeDef TS_State;
    TS_GetState(&TS_State);

    if(TS_State.touchDetected){
        uint16_t xt = TS_State.touchX[0];
        uint16_t yt = TS_State.touchY[0];

        int x = 100, y = 50, w = 28, h = 200;
        if(xt > x && xt < x+w && yt > y && yt < y+h){
            g_feedback = 1.0f - ((yt - y) / (float)h); // scale 0..1

            LCD_SetFillColor(LCD_COLOR_RED);
            LCD_FillRect(x+1, y+h - (g_feedback*h), w-1, (g_feedback*h));
            LCD_SetFillColor(LCD_COLOR_WHITE);
            LCD_FillRect(x+1, y+1, w-1, h - (g_feedback*h) - 1);
        }
    }
}

void uiSliderMix(void) {
    TS_StateTypeDef TS_State;
    TS_GetState(&TS_State);

    if(TS_State.touchDetected){
        uint16_t xt = TS_State.touchX[0];
        uint16_t yt = TS_State.touchY[0];

        int x = 150, y = 50, w = 28, h = 200;
        if(xt > x && xt < x+w && yt > y && yt < y+h){
            g_mix = 1.0f - ((yt - y) / (float)h); // scale 0..1

            LCD_SetFillColor(LCD_COLOR_RED);
            LCD_FillRect(x+1, y+h - (g_mix*h), w-1, (g_mix*h));
            LCD_SetFillColor(LCD_COLOR_WHITE);
            LCD_FillRect(x+1, y+1, w-1, h - (g_mix*h) - 1);
        }
    }
}

extern volatile EffectType currentEffect;


/**
 * Call this function periodically in the UI task.
 * It updates sliders, checks button touches, and redraws UI elements if needed.
 */

void uiUpdate(void) {
    TS_StateTypeDef TS_State;
    TS_GetState(&TS_State);  // read touchscreen state

    uiCheckEffectTouch(&TS_State);  // Check which effect button was touched

    uiUpdateSliders(&TS_State);
}
