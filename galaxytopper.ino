/*
 * Fimware for the Galaxy Topper
 * Galaxy Topper is a top hat inspired by Chris Knight's build at:
 *   http://www.instructables.com/id/My-hat-its-full-of-stars/
 *
 * It uses 6 ShiftBrite pixels to drive a "galaxy" of points of light poking
 * through the felt of a simple, felt top hat. The hat has several different
 * modes and more can be added fairly easily. There is a momentary pushbutton
 * hidden under the bow on the band to cycle through the modes. To keep things
 * fresh and interesting the pushbutton seeds the pRNG with the current value
 * of millis() when pushed.
 *
 * MODES:
 *  - Stealth: All pixels are off, leaving the hat dark.
 *  - Sparkle: each pixel picks a random color, waits a short time, and picks a
 *      new one
 *  - Rainbow: Each pixel fades around the color wheel. Pixels are offset from
 *      one another to give a more active pattern
 *  - Nebula: each pixel fades randomly through blue/white/purple colors
 *      creating a rippling, twinkling pattern in the colors of the night sky
 *  - Christmas Lights: Pixels cycle through the soft colors of old strings of
 *      Christmas lights. Colors are nominally red, yellow, green, and blue,
 *      TODO: but in practice are washed out and warmed over
 *  - Christmas: Pixels alternate between red and green.
 *
 * TODO:
 * I'd really like to add a couple more pixels tied directly into certain
 * patterns (alchemical symbols?) that I can have illuminate at random or on
 * command, but I'm not sure if that'll work out.
 */


#include "shiftbrite.h"
#include "ticker.h"
#include "math.h"     // only used to get pow(...) for our skew(...) function

/* Constants */
#define LATCH_PIN    9  // replace with pin you use for the ShiftBrite latch
#define BUTTON_PIN   3  // pin used for the button to cycle modes
#define NUM_LEDS     6  // Number of LEDs in your chain
#define NUM_MODES    6  // Number of light modes the topper supports
#define FPS        100  // number of times per second to update shiftbrites
#define TPS        100  // ticks per second

/* Predeclarations */
// Necessary to avoid confusing particle's preprocessor
// see https://docs.particle.io/reference/firmware/core/#preprocessor
void colorwheel_tick(Ticker *t, unsigned long tick);
void cycling_tick(Ticker *t, unsigned long tick);
void fading_cycling_tick(Ticker *t, unsigned long tick);
void targeted_tick(Ticker *t, unsigned long tick);
void dark_tick(Ticker *t, unsigned long tick);
TickerRgb hsv2rgb(double hue, double sat, double value);

/* Define Objects */
ShiftBrite sb(NUM_LEDS, LATCH_PIN);
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

/* Button to switch modes */
int current_mode = 0;
volatile int           mode_change_requested = 0;
volatile unsigned long button_last_triggered = 0;

void setup()
{
	// Initialize shiftbrite
	sb.begin();
	sb.show();

	// Set up mode-change button
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), trigger_change, FALLING);

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
	if (millis() - button_last_triggered > 600) {
		button_last_triggered = millis();
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
			mode_sparkle();
			break;
		case 2:
			mode_rainbow();
			break;
		case 3:
			mode_nebula();
			break;
		case 4:
			mode_christmas_lights();
			break;
		case 5:
			mode_christmas();
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

		Tickers[i].targets[0] = {red: random(0,1023), green: random(0,1023), blue: random(0,1023)};
		Tickers[i].targets[1] = {red: 0, green: 0, blue: 511};
		Tickers[i].targets[2] = {red: 1023, green: 1023, blue: 1023};
	}
}

void mode_sparkle()
{
	for (int i = 0; i < NUM_LEDS; ++i) {
		Tickers[i] = Ticker(&random_tick, 0, 2, i);
	}
}

void mode_christmas_lights()
{
	for (int i = 0; i < NUM_LEDS; ++i) {
		// Slow by 2^6
		Tickers[i] = Ticker(&cycling_tick, 4, 6, i);

		//TODO: Adjust these values to taste to simulate christmas lights
		Tickers[i].targets[0] = {red: 1023, green:    0, blue:    0}; // Red
		Tickers[i].targets[1] = {red: 1023, green: 1023, blue:    0}; // Yellow
		Tickers[i].targets[2] = {red:    0, green: 1023, blue:    0}; // Green
		Tickers[i].targets[3] = {red:    0, green:    0, blue: 1023}; // Blue
	}
}

void mode_christmas()
{
	for (int i = 0; i < NUM_LEDS; ++i) {
		// Slow by 2^6
		Tickers[i] = Ticker(&cycling_tick, 2, 6, i);

		Tickers[i].targets[0] = {red: 1023, green:    0, blue:    0}; // Red
		Tickers[i].targets[1] = {red:    0, green: 1023, blue:    0}; // Green
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
	t->current = hsv2rgb((double)hue, 1.0, 1.0);
}

/*
 * Picks a random color and switches to it.
 *
 * By default it switches every tick. Slow this by 2^n by setting Ticker's
 * speed_adjust to n.
 *
 * This tick function does not use any targets
 */
void random_tick(Ticker *t, unsigned long tick)
{
	if (tick % (1 << t->speed_adjust)) {return;}
	t->current = {red: random(0,1023), green: random(0,1023), blue: random(0,1023)};
}

/*
 * Cycles through a fixed list of target colors.
 *
 * By default moves to the next color in the cycle every tick. Slow this
 * by 2^n by setting Ticker's speed_adjust to n.
 *
 * Advance the position in the cycle with Ticker's offset. This can be used to
 * sync multiple cycling_tick tickers into a string.
 *
 * This tick function uses as many targets as you choose to give the Ticker.
 */
void cycling_tick(Ticker *t, unsigned long tick)
{
	int current_index = ((tick >> t->speed_adjust) + t->offset) % t->num_targets;
	t->current = t->targets[current_index];
}

/*
 * Cycles through a fixed list of target colors, fading between them.
 *
 * Fading from one color to the next takes about a second. TODO: make this
 * dynamic via Ticker's speed_adjust.
 *
 * Advance the position in the cycle with Ticker's offset. This can be used to
 * sync multiple cycling_tick tickers into a string.
 *
 * This tick function uses as many targets as you choose to give the Ticker.
 */
void fading_cycling_tick(Ticker *t, unsigned long tick)
{
	int previous_index = ((tick / TPS) + t->offset) % t->num_targets; // Update target once per second.
	TickerRgb previous_color = t->targets[previous_index];
	TickerRgb next_color     = t->targets[(previous_index + 1) % t->num_targets];

	int16_t red_delta     = next_color.red       - previous_color.red;
	int16_t current_red   = previous_color.red   + ((red_delta * (tick % TPS)) / TPS);
	int16_t green_delta   = next_color.green     - previous_color.green;
	int16_t current_green = previous_color.green + ((green_delta * (tick % TPS)) / TPS);
	int16_t blue_delta    = next_color.blue      - previous_color.blue;
	int16_t current_blue  = previous_color.blue  + ((blue_delta * (tick % TPS)) / TPS);

	t->current = {red: current_red, green: current_green, blue: current_blue};
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
 * Code taken from StackOverflow user DavidH:
 *   http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
 */
TickerRgb hsv2rgb(double hue, double sat, double value)
{
	double sextant, chroma, q, t, ff;
	double red, blue, green;
	long  i;
	TickerRgb   out;

	if(sat <= 0.0) {
		red   = value;
		green = value;
		blue  = value;
		return out;
	}

	sextant = hue;
	if(sextant >= 360.0) sextant = 0.0;
	sextant /= 60.0;
	i = (long)sextant;
	ff = sextant - i;
	chroma = value * (1.0 - sat);
	q = value * (1.0 - (sat * ff));
	t = value * (1.0 - (sat * (1.0 - ff)));

	switch(i) {
		case 0:
			red   = value;
			green = t;
			blue  = chroma;
			break;
		case 1:
			red   = q;
			green = value;
			blue = chroma;
			break;
		case 2:
			red   = chroma;
			green = value;
			blue  = t;
			break;
		case 3:
			red   = chroma;
			green = q;
			blue  = value;
			break;
		case 4:
			red   = t;
			green = chroma;
			blue  = value;
			break;
		case 5:
		default:
			red   = value;
			green = chroma;
			blue  = q;
			break;
	}
	out.red   = (int16_t)(red   * 1023);
	out.green = (int16_t)(green * 1023);
	out.blue  = (int16_t)(blue  * 1023);
	return out;
}
