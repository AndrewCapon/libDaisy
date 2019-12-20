#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;

static daisy_handle seed;
static phasor ramp;
static oscillator osc_sine;

static void audioCallback(float *in, float *out, size_t size)
{
	float sine, freq;
    for (size_t i = 0; i < size; i += 2)
    {
        // generate phasor value (0-1), and scale it between 0 and 300
        freq = ramp.process()*300;

        osc_sine.set_freq(freq);
    	sine = osc_sine.process();

    	// left out
        out[i] = sine;

        // right out
        out[i+1] = sine;
    }
}

int main(void)
{
	// initialize seed hardware and daisysp modules
    daisy_seed_init(&seed);

    // initialize phasor module
    ramp.init(DSY_AUDIO_SAMPLE_RATE, 1, 0);

    // set parameters for sine oscillator object
    osc_sine.init(DSY_AUDIO_SAMPLE_RATE);
    osc_sine.set_waveform(oscillator::WAVE_SIN);
    osc_sine.set_freq(100);
    osc_sine.set_amp(0.25);

    // define callback
    dsy_audio_set_callback(DSY_AUDIO_INTERNAL, audioCallback);

    // start callback
    dsy_audio_start(DSY_AUDIO_INTERNAL);

    while(1) {}
}