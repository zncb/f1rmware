#include <math.h>

#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#include <r0ketlib/random.h>

#include "usetable.h"



enum {
    STROBE_WHITE=0,
    STROBE_RED,
    STROBE_BLUE,
    STROBE_GREEN,
};

// coded as g,r,b
static const uint8_t g_strobe_colors[12] = {
    255,255,255,
    0,255,0,
    255,0,0,
    0,0,255,
};


enum {
    STROBE_PULSE=0,
    STROBE_RANDWALK,
    STROBE_SHIFTWAVE,
    STROBE_MORPH,
};

#define MAX(x,y) ((x>y)?x:y)
#define MIN(x,y) ((x>y)?y:x)
#define CLAMP(m,x,M) ((x<m)?m:(x>M)?M:x)

static const float c_two_pi = 6.283185307179586;

static inline void draw_randwalk(float phase, uint8_t bright, uint8_t color)
{
    static uint8_t pattern[24] = {0};

    const float amp = fabsf(sinf(phase*c_two_pi));
    const float lum = amp * (float)bright/10.f;
    
    const uint8_t peak = lum * 255.f;
    const float dist = 16.f;
    for (uint8_t i=0; i<sizeof(pattern); i+=3) {
	uint8_t rand = getRandom();
	int8_t sign = (rand & 0x1)?1:-1;
	pattern[i] += sign * (float)((uint8_t)rand)/255.f * dist;
	if (pattern[i] > peak)
	    pattern[i] = peak;
	
	rand = getRandom();
	sign = (rand & 0x1)?1:-1;
	pattern[i+1] += sign * (float)((uint8_t)rand)/255.f * dist;
	if (pattern[i+1] > peak)
	    pattern[i+1] = peak;
	
	rand = getRandom();
	sign = (rand & 0x1)?1:-1;
	pattern[i+2] += sign * (float)((uint8_t)rand)/255.f * dist;
	if (pattern[i+2] > peak)
	    pattern[i+2] = peak;
    }

    ws2812_sendarray(pattern, sizeof(pattern));
}

static inline void draw_shiftwave(float phase, uint8_t bright, uint8_t color)
{
    // tbh, this is a bit shit... 
    
    static uint8_t pattern[24] = {0};
    static uint8_t color_states[8] = {0};
    static const float phase_ofsts[8] = {0.f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.750f, 0.875f};
    
    const float amp = 0.4 * (float)bright/10.f;
    const uint8_t dist = 8;
    
    for (uint8_t i=0; i<sizeof(color_states); ++i) {
	uint8_t rand = getRandom();
	int8_t sign = (rand & 0x1)?1:-1;
	color_states[i] += sign * (float)((uint8_t)rand)/255.f * dist;
	color_states[i] = CLAMP(0,color_states[i],0x3f);
    }

    const uint8_t *colorp = &color_states[0];
    const float *ofstp = &phase_ofsts[0];
    for(uint8_t i=0; i<sizeof(pattern); i+=3,++colorp){
	const float mod = amp * sinf((phase + *ofstp) * c_two_pi);
	uint8_t color = CLAMP(0,mod * *colorp,0x3f);
	
	pattern[i] = (color & 0x3) * 64;
	pattern[i+1] = (color>>2 & 0x3) * 64;
	pattern[i+2] = (color>>4 & 0x3) * 64;
    }
    
    ws2812_sendarray(pattern, sizeof(pattern));
}

static inline void draw_morph(float phase, uint8_t bright, uint8_t color)
{
    static uint8_t pattern[24] = {0}
}

static inline void draw_pulse(float phase, uint8_t bright, uint8_t color)
{
    static uint8_t pattern[24] = {0};
    
    const float amp = fabsf(sinf(phase*c_two_pi));
    const float lum = amp * (float)bright/10.f;

    const uint8_t *colp = &g_strobe_colors[color*3];

    for(uint8_t i=0; i<sizeof(pattern); i+=3){
	pattern[i] = colp[0]*lum;
	pattern[i+1] = colp[1]*lum;
	pattern[i+2] = colp[2]*lum;
    }
    
    ws2812_sendarray(pattern, sizeof(pattern));    
}


//# MENU strobe
void ram(void)
{
    uint8_t brightness = 0;

    getInputWaitRelease();

    SETUPgout(RGB_LED);
    
    const float w0 = 0.008f;
    
    float phase = 0.f;
    uint8_t color = STROBE_WHITE;
    uint8_t bright = 5;
    uint8_t speed = 5;
    uint8_t mode = STROBE_PULSE;
    
    while(1){
	lcdClear(0xff);
        lcdPrintln("Strobe");

	lcdPrint("UP: brightness (");
	lcdPrint(IntToStr(bright,2,F_LONG));
	lcdPrintln(")");

	lcdPrint("DOWN: mode (");
	lcdPrint(IntToStr(mode,2,F_LONG));
	lcdPrintln(")");

	lcdPrint("LEFT: color (");
	lcdPrint(IntToStr(color,2,F_LONG));
	lcdPrintln(")");

	lcdPrint("RIGHT: speed (");
	lcdPrint(IntToStr(speed,2,F_LONG));
	lcdPrintln(")");

	lcdPrint("Phase: ");
	lcdPrintln(IntToStr(phase*255,3,F_LONG));
	
        lcdDisplay();
	

	switch (getInput()) {
	    case BTN_UP:
		bright = (bright + 1) % 11;
		break;
	    case BTN_DOWN:
		mode = (mode + 1) % 4;
		break;
	    case BTN_LEFT:
		color = (color + 1) % 5;
		break;
	    case BTN_RIGHT:
		speed = (speed + 1) % 11;
		break;
	    case BTN_ENTER:
		return;
		break;
	}

	switch(mode) {
	    case STROBE_RANDWALK:
		draw_randwalk(phase,bright,color);
		break;
	    case STROBE_SHIFTWAVE:
		draw_shiftwave(phase,bright,color);
		break;
	    case STROBE_PULSE:
	    default:
		draw_pulse(phase,bright,color);
	}
	
	delay(1000);
	phase += w0*speed;
	if (phase >= 1.f)
	    phase -= 1.f;
    };
    
    return;
};
