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

enum {
    STROBE_PULSE=0,
    STROBE_RANDWALK,
};

static const float c_two_pi = 6.283185307179586;

static inline void redraw(float phase, uint8_t bright, uint8_t color, uint8_t mode)
{

    // coded as g,r,b
    static const uint8_t colors[12] = {
	255,255,255,
	0,255,0,
	255,0,0,
	0,0,255,
    };
	
    
    const float amp = fabsf(sinf(phase*c_two_pi));
    const float lum = amp * (float)bright/10.f;
    
    static uint8_t pattern[24] = {0};
    
    if (mode == STROBE_RANDWALK) {
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
    }
    else {
	const uint8_t *colp = &colors[color*3];
	for(uint8_t i=0; i<sizeof(pattern); i+=3){
	    pattern[i] = colp[0]*lum;
	    pattern[i+1] = colp[1]*lum;
	    pattern[i+2] = colp[2]*lum;
	}
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
		++bright;
		if (bright > 10)
		    bright -= 11;
		break;
	    case BTN_DOWN:
		++mode;
		if (mode >= 2)
		    mode -= 2;
		break;
	    case BTN_LEFT:
		++color;
		if (color >= 4)
		    color -= 4;
		break;
	    case BTN_RIGHT:
		++speed;
		if (speed > 10)
		    speed -= 11;
		break;
	    case BTN_ENTER:
		break;
	}

	redraw(phase,bright,color,mode);
	delay(1000);
	phase += w0*speed;
	if (phase >= 1.f)
	    phase -= 1.f;
    };
    
    return;
};
