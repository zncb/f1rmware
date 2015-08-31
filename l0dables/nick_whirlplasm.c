/* 
 * partially based on nick_plasma.c
 * original provided by Grigori Goronzy <greg@geekmind.org> */
/* adapted for rad1o by Hans-Werner "hw" Hilse <hwhilse@gmail.com> */
#include <math.h>

#include <r0ketlib/config.h>
#include <r0ketlib/print.h>
#include <r0ketlib/render.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <rad1olib/pins.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include "usetable.h"

#define HRESX 65
#define HRESY 65

#define SIN_TAB_SIZE 512
static float g_sintab[SIN_TAB_SIZE] = {0};

#define TYPE_CMD    0
#define TYPE_DATA   1

static const float c_pi = 3.141592653589793f;
static const float c_two_pi = 6.283185307179586f;

#define MAX(x,y) ((x>y)?x:y)
#define MIN(x,y) ((x>y)?y:x)
#define CLAMP(m,x,M) ((x<m)?m:(x>M)?M:x)

#define GETRED(x) ((x & 0b11100000) >> 5)
#define GETGRN(x) ((x & 0b00011100) >> 2)
#define GETBLU(x) (x & 0b00000011)

#define MKRGB(r,g,b) ((((r & 0b111) << 5) | ((g & 0b111) << 2) | (b & 0b11)))

// from nick_colplasm
static inline unsigned isqrt(unsigned val) {
    unsigned temp, g = 0, b = 0x8000, bshft = 15;
    do {
        if (val >= (temp = (((g << 1) + b)<<bshft--))) {
           g += b;
           val -= temp;
        }
    } while (b >>= 1);
    return g;
}

static inline int modpow2u(unsigned int x, unsigned int m) {
    return x & (m-1);
}

static inline float sintab(float p) {
    const int i = modpow2u((unsigned int)(p * (float)SIN_TAB_SIZE),SIN_TAB_SIZE);
    const float fr = p - (int)p;
    return (1.f-fr)*g_sintab[i] + fr*g_sintab[i];
}



static inline void plasma(const unsigned int t, const uint8_t complexity, const float offset, const float fluid_speed) {

    static const float color_inten = 1.f;
    static const float maxres = MAX(RESX,RESY);
    
    const float ta = (float)t / fluid_speed;

    static uint8_t dbg_pattern[24] = {0};
    
    lcdWrite(TYPE_CMD,0x2C);
    
    for (uint8_t j = 0; j < RESY; ++j) {
	const float py = (float)(2*j - RESY) / maxres;
	
	for (uint8_t i = 0; i < RESX; ++i) {
	    const float px = (float)(2*i - RESX) / maxres;
	    float npx = px, npy = py;
	    
	    for (uint8_t c = 1; c < complexity; ++c) {
	    	float p = ((float)c * npy + ta + 0.3f * (float)c);
	    	npx += 0.6f / (float)c * sintab(p) + offset;
		
	    	p = ((float)c * npx + ta + 0.3f * (float)(c + 10.f));
	    	npy += 0.6 / (float)c * sintab(p) + offset;
	    }
	    
	    const float sinpx = color_inten * sintab(0.15f*npx);
	    const float sinpy = color_inten * sintab(0.25f*npy);
	    
	    const int r = modpow2u(sinpx * 255.f, 256);
	    const int g = modpow2u(sinpy * 255.f, 256);
	    const int b = modpow2u((color_inten * sintab(0.20*(npx+npy)) + color_inten) * 255.f,256);
	    int v = (r & 0b11100000) | ((g & 0b11100000) >> 3) | ((b & 0b11000000) >> 6);
	    
	    const int ic = (i-HRESX);
	    const int ii = ic*ic;
	    const int jc = (j-HRESY);
	    const int jj = jc*jc;

	    const int rr = ii + jj;
	    
            if (rr < 2304) { // 48x48
		v = (v ^ 0xFF);

		static const int dx = 10;
		static const int dy = -10;
		const int dix = (ic - dx);
		const int djy = (jc - dy);
		const int drr = dix * dix + djy * djy;
		float darken = 1.08f - (float)MIN((0.2*drr),2304)/2304.f;
		
		if(lcdGetPixel(i, j) == GLOBAL(nickfg)) {
		    darken = 0.95f;
		}
		
		v = MKRGB(MIN((int)(darken*GETRED(v)),7),MIN((int)(darken*GETGRN(v)),7),MIN((int)(darken*GETBLU(v)),3));

	    }
	    
            lcdWrite(TYPE_DATA, v);

        }
    }


}

static void reset_leds()
{
    uint8_t pattern[24] = {
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0
    };

    ws2812_sendarray(pattern,sizeof(pattern));
}

void ram(void)
{
    unsigned int t = 0;

    float * __restrict sp = &g_sintab[0];
    const float* spe = sp + SIN_TAB_SIZE;
    const float w0 = 1.f/(float)SIN_TAB_SIZE;
    
    for (float p = 0.f; sp != spe; ++sp, p+=w0) {
	*sp = sinf(p*c_two_pi);
    }
    
    setExtFont(GLOBAL(nickfont));
    int dx=DoString(0,0,GLOBAL(nickname));
    dx=(RESX-dx)/2;
    if(dx<0)
        dx=0;
    int dy=(RESY-getFontHeight())/2;

    lcdClear();
    lcdFill(GLOBAL(nickbg));
    setTextColor(GLOBAL(nickbg),GLOBAL(nickfg));
    lcdSetCrsr(dx,dy);
    lcdPrint(GLOBAL(nickname));
    lcdDisplay();

    // display plasma
    lcd_select();

    SETUPgout(RGB_LED);
    reset_leds();

    unsigned int complexity = 3;
    unsigned int offset = 75.f;
    unsigned int speed = 70.f;
    
    for (;;) {
        plasma(t++,complexity,offset,speed);
        delayms(10);
        char key = getInputRaw();
        switch (key) {
	    case BTN_UP:
		complexity = (complexity + 1) % 6;
		if (complexity == 0)
		    ++complexity;
		break;
	    case BTN_DOWN:
		offset = (offset + 5) % 100;
		break;
	    case BTN_RIGHT:
		speed = (speed + 10) % 100;
		if (speed == 0)
		    speed += 10;
		break;
	    case BTN_ENTER:
		lcd_deselect();
		setTextColor(0xFF,0x00);
		return;
        }
    }

}
