/*
 * Fimware for the Galaxy Topper
 * Galaxy Topper is a top hat inspired by Chris Knight's build at:
 *   http://www.instructables.com/id/My-hat-its-full-of-stars/
 *
 * It uses 6 ShiftBrite pixels to drive a "galaxy" of points of light poking
 * through the felt of a simple, felt top hat. The hat has several different
 * modes and more can be added fairly easily. There is a tilt switch taped into
 * the brim of the hat, positioned nearly vertically. Tipping the hat advances
 * the pattern displayed, alternating with the "stealth" pattern. To keep
 * things fresh and interesting we seed the pRNG with the current value of
 * millis() when the switch activates.
 *
 * MODES:
 *  - Stealth:
 *		All pixels are off, leaving the hat dark.
 *  - Nebula:
 *		Each pixel fades randomly through blue/white/purple colors
 *      creating a rippling, twinkling pattern in the colors of the night sky
 *  - Sparkle:
 *		Each pixel flickers quickly from one random value to another
 *  - Rainbow:
 *		Each pixel fades around the color wheel. Pixels are offset from
 *      one another to give a more active pattern
 */


#include "shiftbrite.h"
#include "ticker.h"
#include "math.h"     // only used to get pow(...) for our skew(...) function

/* Constants */
#define LATCH_PIN    9  // replace with pin you use for the ShiftBrite latch
#define SWITCH_PIN   3  // pin used for the switch to cycle modes
#define NUM_LEDS     6  // Number of LEDs in your chain
#define NUM_MODES    6  // Number of light modes the topper supports
#define FPS        100  // number of times per second to update shiftbrites
#define TPS        100  // ticks per second

/* Predeclarations */
// Necessary to avoid confusing particle's preprocessor
// see https://docs.particle.io/reference/firmware/core/#preprocessor
void dark_tick(Ticker *t, unsigned long tick);
void random_tick(Ticker *t, unsigned long tick);
void colorwheel_tick(Ticker *t, unsigned long tick);
void targeted_tick(Ticker *t, unsigned long tick);
void hsv_targeted_tick(Ticker *t, unsigned long tick);

TickerRgb hsv2rgb(double hue, double sat, double value);

/* Define Objects */
ShiftBrite sb(NUM_LEDS, LATCH_PIN);
//TODO: see if this works
//Ticker Tickers[NUM_LEDS];
Ticker Tickers[NUM_LEDS] = {
	Ticker(&dark_tick, 0, 0,  0),
	Ticker(&dark_tick, 0, 0,  0),
	Ticker(&dark_tick, 0, 0,  0),
	Ticker(&dark_tick, 0, 0,  0),
	Ticker(&dark_tick, 0, 0,  0),
	Ticker(&dark_tick, 0, 0,  0),
};

/* Timing */
unsigned long frame_delay = 1000 / FPS;
unsigned long tick_delay  = 1000 / TPS;
unsigned long tick, last_tick, last_frame = 0;

/* Switch to switch modes */
int current_mode = 0;
volatile int           mode_change_requested = 0;
volatile unsigned long switch_last_triggered = 0;

void setup()
{
	// Initialize shiftbrite
	sb.begin();
	sb.show();

	// Set up mode-change switch
	pinMode(SWITCH_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), trigger_change, FALLING);

	// Initialize tickers
	mode_stealth();
}

void loop()
{
	if (mode_change_requested) {
		change_mode();
		mode_change_requested = 0;
	}

	unsigned long now = millis();

	// Is it time to do another tick?
	if (now - last_tick > tick_delay) {
		last_tick = millis();
		for(int i = 0; i < NUM_LEDS; ++i) {
			Tickers[i].Tick(tick);
			TickerRgb c = Tickers[i].current;
			sb.setPixelRGB(i, c.red, c.green, c.blue);
		}
		++tick;
	}

	// Is it time to refresh our shiftbrites?
	if (millis() - last_frame > frame_delay) {
		last_frame = millis();
		sb.show();
	}
}

/*********************
 * MODE SETUP FUNCTIONS *
 ************************/
void trigger_change() {
	if (millis() - switch_last_triggered > 600) {
		switch_last_triggered = millis();
		mode_change_requested = 1;
	}
}

void change_mode() {
	randomSeed(millis());

	++current_mode;
	current_mode %= NUM_MODES;
	switch(current_mode) {
		case 0:
			mode_stealth();
			break;
		case 1:
			mode_nebula();
			break;
		case 3:
			mode_rainbow();
			break;
		case 5:
			mode_sparkle();
			break;
		default:
			mode_stealth();
			break;
	}

}

void mode_stealth()
{
	for (int i = 0; i < NUM_LEDS; ++i) {
		Tickers[i] = Ticker(&dark_tick, 0, 0, 0);
	}
}

void mode_nebula()
{
	// Use targeting tickers constrained to be colors with strong blue channels
	// to make a flickering "nebula" effect.
	for (int i = 0; i < NUM_LEDS; ++i) {
		Tickers[i] = Ticker(&targeted_tick, 3, 0, i * 5 + 1);

		Tickers[i].current    = {red: random(0, 1023), green: random(0, 1023), blue: random(511, 1023)};
		Tickers[i].targets[0] = {red: random(0, 1023), green: random(0, 1023), blue: random(511, 1023)};

		Tickers[i].targets[1] = {red: 0, green: 0, blue: 511};
		Tickers[i].targets[2] = {red: 1023, green: 1023, blue: 1023};
	}
}

//FIXME: Make it flicker faster than nebula.
void mode_stars()
{
	// Use targeting tickers constrained to be white
	// to make a twinkling starfield
	for (int i = 0; i < NUM_LEDS; ++i) {
		Tickers[i] = Ticker(&hsv_targeted_tick, 3, 0, i * 5 + 1);

		int16_t value = random(0, 1023);
		Tickers[i].current    = {red: value, green: value, blue: value};
		Tickers[i].targets[0] = {red: value, green: value, blue: value};

		// We're lying here and using TickerRgb's "red", "green", and "blue"
		// data members for "hue", "saturation", and "value" respectively.
		Tickers[i].targets[1] = {red: 0, green: 0, blue: 0};
		Tickers[i].targets[2] = {red: 0, green: 0, blue: 1023};
	}
}

void mode_sparkle()
{
	for (int i = 0; i < NUM_LEDS; ++i) {
		Tickers[i] = Ticker(&random_tick, 0, 3, i);
	}
}

void mode_rainbow()
{
	for (int i = 0; i < NUM_LEDS; ++i) {
		Tickers[i] = Ticker(&colorwheel_tick, 0, 0, i * 15);
	}
}

/*********************
 *  TICKER FUNCTIONS *
 *********************/

/* Do nothing */
void dark_tick(Ticker *t, unsigned long tick)
{
	t->current = {red: 0, green: 0, blue: 0};
}

/*
 * Picks a random color and switches to it.
 * Also randomizes how quickly it moves to the next color.
 * This tick function does not use any targets.
 */
void random_tick(Ticker *t, unsigned long tick)
{
	if (tick % (1 << t->speed_adjust)) {return;}
	t->speed_adjust = random(2,5);	
	t->current = hsv2rgb((double)random(0,1023)/1023, (double)random(767,1023)/1023, (double)random(0,1023)/1023);
}

/*
 * Cycles around the color wheel by using tick to compute the hue (value and
 * saturation are held constant at 1).
 *
 * Ticker's speed_adjust is used to slow the cycle by powers of 2
 *
 * Ticker's offset is used to set the color *ahead* of the tick's base hue.
 * This can be used to sync multiple colorwheel_tick Tickers into a ribbon.
 *
 * This tick function does not use any target colors.
 */
void colorwheel_tick(Ticker *t, unsigned long tick)
{
	int16_t hue = ((tick >> t->speed_adjust) + t->offset) % 360;
	t->current = hsv2rgb((double)hue/360, 1.0, 1.0);
}

/*
 * Fades to a target color. Once target color is reached, the target and fade
 * speed are randomized. Target randomization is controllable within min and
 * max values on each color channel.
 *
 * Ticker's offset is used as the step size to control how quickly the color
 * moves towards the target.
 *
 * Ticker's speed_adjust is used to skip cycles by powers of 2:
 * 0 executes every cycle
 * 1 executes every other cycle
 * 2 executes every fourth cycle...
 *
 * This tick function uses three "target" colors:
 *   0: The color to fade towards
 *   1: The minimum value for each color channel when randomizing
 *   2: The maximum value for each color channel when randomizing
 */
void targeted_tick(Ticker *t, unsigned long tick)
{
	// speed_adjust keeps us from ticking on certain cycles
	if (tick % (1 << t->speed_adjust)) {return;}

	TickerRgb* current = &t->current;
	TickerRgb* target  = &t->targets[0];
	TickerRgb  minima  = t->targets[1];
	TickerRgb  maxima  = t->targets[2];
	int step_size = t->offset;

	if (   current->red   == target->red
		&& current->green == target->green
		&& current->blue  == target->blue)
	{
		// We've reached the target! Pick a new one.
		target->red   = random(minima.red,   maxima.red);
		target->green = random(minima.green, maxima.green);
		target->blue  = random(minima.blue,  maxima.blue);
		// And just for fun let's also randomize the fade speed!
		t->offset       = skew(random(1, 100), 5, 50, 100);
		t->speed_adjust = random(0, 1);
	}
	// Still working towards the target. Take a step in the right direction
	// TODO: Should we adjust the step sizes so that each channel reaches
	//       its target at the same time?
	current->red   += stepToward(target->red,   current->red,   step_size);
	current->green += stepToward(target->green, current->green, step_size);
	current->blue  += stepToward(target->blue,  current->blue,  step_size);
}

/*
 * As targeted_tick except the minima and maxima are given in HSV coordinates.
 * This is a very lazy bit of copypasta from targeted_tick -- only the
 * constraints are actually HSV colors; the current and target colors are still
 * RGB and the steps occur in RGB space.
 */
void hsv_targeted_tick(Ticker *t, unsigned long tick)
{
	// speed_adjust keeps us from ticking on certain cycles
	if (tick % (1 << t->speed_adjust)) {return;}

	TickerRgb* current = &t->current;
	TickerRgb* target  = &t->targets[0];
	TickerRgb  minima  = t->targets[1];
	TickerRgb  maxima  = t->targets[2];
	int step_size = t->offset;

	if (   current->red   == target->red
		&& current->green == target->green
		&& current->blue  == target->blue)
	{
		// We've reached the target! Pick a new one. We're lying here and using
		// the "red", "green", and "blue" data members of minima and maxima for
		// "hue", "saturation", and "value" respectively.
		double hue        = (double)random(minima.red,   maxima.red)   / (double)1023;
		double saturation = (double)random(minima.green, maxima.green) / (double)1023;
		double value      = (double)random(minima.blue,  maxima.blue)  / (double)1023;
		*target = hsv2rgb(hue, saturation, value);

		// And just for fun let's also randomize the fade speed!
		t->offset       = skew(random(1, 100), 5, 50, 100);
		t->speed_adjust = random(0, 1);
	}
	// Still working towards the target. Take a step in the right direction
	current->red   += stepToward(target->red,   current->red,   step_size);
	current->green += stepToward(target->green, current->green, step_size);
	current->blue  += stepToward(target->blue,  current->blue,  step_size);
}

/*** HELPER FUNCTIONS ***/

/*
 * Given a target value, current value, and step size, calculate the value to
 * add to the current value to move it towards its target by the step size
 * without going past the target.
 */
int16_t stepToward(int16_t target, int16_t current, int step_size)
{
	int16_t delta = target - current;
	delta = abs(delta);

	int direction = 0;
	if (target > current) {direction =  1;}
	if (target < current) {direction = -1;}

	return (delta > step_size ? step_size : delta) * direction;
}

/*
 * This is the gamma correction function that I included a lookup table for in
 * shiftbrite.h. It's used here to map a uniform distribution to something skewed
 * heavily towards the low end.
 *
 * Basically I want my targeting Tickers to fade slowly most of the time but do a
 * really quick flicker every once in a while.
 */
int16_t skew(int16_t x, int16_t exponent, int16_t max_out, int16_t range)
{
	return (int16_t)(pow((float)x / (float)range, exponent) * max_out + 1);
}

/*
 * This function converts an HSV value into an RGB value.
 * Code taken from github user ratkins:
 *   https://github.com/ratkins/RGBConverter
 */
TickerRgb hsv2rgb(double h, double s, double v)
{
	double r, g, b;
	TickerRgb out;

	int i = int(h * 6);
	double f = h * 6 - i;
	double p = v * (1 - s);
	double q = v * (1 - f * s);
	double t = v * (1 - (1 - f) * s);

	switch(i % 6){
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}

	out.red   = (int16_t)(r * 1023);
	out.green = (int16_t)(g * 1023);
	out.blue  = (int16_t)(b * 1023);
	return out;
}
