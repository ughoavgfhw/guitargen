#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdint.h>
#include <termios.h>
#include "rawaudio/audioplay.h"

#define SAMPLE_RATE 44100
#define SAMPLE_BITS 16

#define NUM_STRINGS 2

// Returns the result of sin with the positive argument scaled by samples/s/2pi
// x must be no more than 2^32/SIN_SIZE (currently ~190.2 seconds)
int sin_scaled(unsigned x) {
	// The first 1/4 of a sine wave. Next it goes to 1, then back down, then repeats negative
#define SIN_SIZE_BITS 9
#define SIN_SIZE (1<<SIN_SIZE_BITS) // Power of 2
	static const unsigned short sinarr[SIN_SIZE/4 + 1] = {
		    0,   402,   804,  1206,  1608,  2009,  2411,  2811,
		 3212,  3612,  4011,  4410,  4808,  5205,  5602,  5998,
		 6393,  6787,  7180,  7571,  7962,  8351,  8740,  9127,
		 9512,  9896, 10279, 10660, 11039, 11417, 11793, 12167,
		12540, 12910, 13279, 13646, 14010, 14373, 14733, 15091,
		15447, 15800, 16151, 16500, 16846, 17190, 17531, 17869,
		18205, 18538, 18868, 19195, 19520, 19841, 20160, 20475,
		20788, 21097, 21403, 21706, 22006, 22302, 22595, 22884,
		23170, 23453, 23732, 24008, 24279, 24548, 24812, 25073,
		25330, 25583, 25833, 26078, 26320, 26557, 26791, 27020,
		27246, 27467, 27684, 27897, 28106, 28311, 28511, 28707,
		28899, 29086, 29269, 29448, 29622, 29792, 29957, 30118,
		30274, 30425, 30572, 30715, 30853, 30986, 31114, 31238,
		31357, 31471, 31581, 31686, 31786, 31881, 31972, 32057,
		32138, 32214, 32286, 32352, 32413, 32470, 32522, 32568,
		32610, 32647, 32679, 32706, 32729, 32746, 32758, 32766,

		32768 // Extra for the reverse part
	};

	x %= SAMPLE_RATE;
	unsigned pos = ((x * SIN_SIZE + SAMPLE_RATE/2) / SAMPLE_RATE);
	unsigned section = pos / (SIN_SIZE/4);
	pos = pos % (SIN_SIZE/4);
	switch(section) {
		case 0: return sinarr[pos];
		case 1: return sinarr[SIN_SIZE/4 - pos];
		case 2: return -(int)sinarr[pos];
		case 3: return -(int)sinarr[SIN_SIZE/4 - pos];
	}
#undef SIN_SIZE
#undef SIN_SIZE_BITS
	return 0;
}

#define MAX_HARMONICS 58
struct Timing {
	unsigned length; // In samples
	double multiplier;
	unsigned numHarmonics;
	double harmonics[MAX_HARMONICS];
};
#define TIMING(len,vol,num,vals...) {SAMPLE_RATE*len, vol, num, vals}

struct TimingInfo {
	unsigned timingCount; // Should exclude an empty timing at the end
	const struct Timing *timings;
};

struct FrequencyTimingRange {
	unsigned minFreq, maxFreq; // Max is excluded
	struct TimingInfo timingInfo;
};
#define TIMING_MAP(min,max,tim) {min, max, {sizeof(tim)/sizeof(*tim)-1, tim}}

#ifndef OLD_TIMINGS
const struct Timing _t_A2[] = {
#  include "timings/A2.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_A2S[] = {
#  include "timings/A2S.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_B2[] = {
#  include "timings/B2.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_C3[] = {
#  include "timings/C3.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_C3S[] = {
#  include "timings/C3S.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_D3[] = {
#  include "timings/D3.c"
	{0, 1, 0, {0}}
};

// Ordered by frequency, non-overlapping
const struct FrequencyTimingRange freqTimeMap[] = {
	TIMING_MAP(106, 113, _t_A2),
	TIMING_MAP(113, 120, _t_A2S),
	TIMING_MAP(120, 126, _t_B2),
	TIMING_MAP(126, 134, _t_C3),
	TIMING_MAP(134, 142, _t_C3S),
	TIMING_MAP(142, 150, _t_D3)
};
#else
# warning "Old timings in use"
const struct Timing timings[] = {
		// Unused partials: 34, 28, 10, 29, 17
		// Harmonic			 2       3       4       5       6       7      8   9      10      11      12      13      14      15      16     17 18  19      20      21      22      23      24     25 26 27  28      29      30      31      32      33     34 35 36 37 38  39      40     41  42      43      44
		// Partial Number	32      35      36      31      24      25      -  01      02      19      20      26      27      00      03      -  -  04      05      06      21      07      08      -  -  -  09      22      23      30      11      12      -  -  -  -  -  13      14      -  15      16      18
	// 1.000 second up to 5.0kHz split to 0.1 + 0.15 + 0.25 + 0.5 seconds
	//{ SAMPLE_RATE/10, 1.0000, 0, {0}},
	{SAMPLE_RATE*3/20, 0.7638, 44, {1, 0.9394, 0.2622, 0.3507, 0.2504, 0.1056, 0.1134, 0, 0.0140, 0.0783, 0.1048, 0.0434, 0.0680, 0.1010, 0.0654, 0.0293, 0, 0, 0.0339, 0.0702, 0.0902, 0.1042, 0.0628, 0.0323, 0, 0, 0, 0.0293, 0.0262, 0.1166, 0.0376, 0.0374, 0.0610, 0, 0, 0, 0, 0, 0.0298, 0.0387, 0, 0.0289, 0.0295, 0.0331}},
	{SAMPLE_RATE  / 4, 0.7094, 44, {1, 0.8860, 0.5025, 0.5161, 0.3476, 0.1453, 0.1351, 0, 0.0354, 0.0904, 0.1348, 0.0569, 0.1348, 0.0588, 0.0623, 0.0290, 0, 0, 0.0254, 0.0312, 0.0830, 0.1015, 0.0621, 0.0245, 0, 0, 0, 0.0156, 0.0101, 0.0550, 0.0172, 0.0189, 0.0364, 0, 0, 0, 0, 0, 0.0112, 0.0108, 0, 0.0112, 0.0113, 0.0028}},
	{SAMPLE_RATE  / 2, 0.7273, 44, {1, 0.4717, 0.4832, 0.5992, 0.4270, 0.1353, 0.1380, 0, 0.0375, 0.0723, 0.1002, 0.0874, 0.1811, 0.0272, 0.0328, 0.0254, 0, 0, 0.0257, 0.0393, 0.0411, 0.0568, 0.0204, 0.0124, 0, 0, 0, 0.0053, 0.0094, 0.0142, 0.0076, 0.0095, 0.0097, 0, 0, 0, 0, 0, 0.0022, 0.0013, 0, 0.0023, 0.0024, 0.0007}},
	// 0.500 second up to 4.0kHz
	{SAMPLE_RATE  / 2, 0.7678, 33, {1, 0.2216, 0.3029, 0.5942, 0.4486, 0.0752, 0.0961, 0, 0.0230, 0.0216, 0.0622, 0.0908, 0.0703, 0.0251, 0.0270, 0.0152, 0, 0, 0.0074, 0.0130, 0.0175, 0.0238, 0.0103, 0.0054, 0, 0, 0, 0.0015, 0.0028, 0.0048, 0.0020, 0.0022, 0.0016}},
	// 0.375 second up to 3.35kHz
	{SAMPLE_RATE*3/ 8, 0.8046, 30, {1, 0.2113, 0.2332, 0.5782, 0.4158, 0.0518, 0.0381, 0, 0.0153, 0.0387, 0.0944, 0.0372, 0.0874, 0.0292, 0.0273, 0.0103, 0, 0, 0.0081, 0.0093, 0.0064, 0.0096, 0.0030, 0.0033, 0, 0, 0, 0.0000, 0.0000, 0.0018}},
	// 0.250 second up to 2.6kHz
	{SAMPLE_RATE  / 4, 0.7014, 23, {1, 0.1221, 0.2184, 0.5863, 0.3852, 0.0537, 0.0115, 0, 0.0144, 0.0494, 0.0854, 0.0399, 0.1212, 0.0308, 0.0087, 0.0080, 0, 0, 0.0032, 0.0046, 0.0048, 0.0063, 0.0025}},
	// 0.625 second up to 2.5kHz
	{SAMPLE_RATE*5/ 8, 0.5755, 22, {1, 0.1933, 0.1782, 0.6735, 0.3806, 0.0680, 0.0478, 0, 0.0074, 0.0454, 0.0398, 0.0705, 0.0637, 0.0295, 0.0148, 0.0064, 0, 0, 0.0024, 0.0056, 0.0026, 0.0033}},
	// 0.500 second up to 2.0kHz
	{SAMPLE_RATE  / 2, 0.2765, 16, {1, 0.3174, 0.1389, 0.9126, 0.5333, 0.0827, 0.0715, 0, 0.0114, 0.0185, 0.0865, 0.0429, 0.0878, 0.0325, 0.0149, 0.0052}},
	// 1.750 second up to 1.6kHz
	{SAMPLE_RATE*7/ 4, 0.2287, 14, {1, 0.9405, 0.1277, 1.4852, 1.4344, 0.1408, 0.0746, 0, 0.0396, 0.0708, 0.1134, 0.0952, 0.1302, 0.0471}},
	{0, 1, 0, {0}}
	// Fund. Freq. Volumes: 0.109247 0.083442 0.059196 0.043054 0.033059 0.026599 0.018656 0.010737 0.002969 end=0.000679
	
	// or 1.0 to 5k; 0.5 to 4k; 0.5 to 3.35k; 0.5 to 2.5k; 0.75 to 2.0k; 1.75 to 1.6k
};
const struct Timing t[] = {{SAMPLE_RATE, 0.5, 1, {1}}, {0, 1, 0, {0}}};

// Ordered by frequency, non-overlapping
const struct FrequencyTimingRange freqTimeMap[] = {
	{0, 100, {1, t}},
	{100, 200, {sizeof(timings)/sizeof(*timings) - 1, timings}},
	{200, 20000, {1, t}}
};
#endif //ndef OLD_TIMINGS

struct NoteState {
	unsigned time, timingEnd;
	double volume, dVolume;
	double ratioPerSample, ratioIntoTiming;
	unsigned timing;
	unsigned fundFreq;
	struct TimingInfo timingInfo;
};

void initNote(struct NoteState *note, unsigned fundFreq, double initialVol) {
	unsigned min = 0, max = sizeof(freqTimeMap)/sizeof(*freqTimeMap);
	unsigned i;
	// Binary search for the frequency. If not found, returns the next position
	// higher or lower (but may return either unless at an end)
	do {
		i = ((max - min) >> 1) + min;
		if(fundFreq < freqTimeMap[i].minFreq) {
			max = i;
		} else if(fundFreq >= freqTimeMap[i].maxFreq) {
			min = i+1;
		} else break;
	} while(min < max);
	struct TimingInfo timingInfo = freqTimeMap[i].timingInfo;

	double ratioPerSample = 1.0 / timingInfo.timings[0].length;
	double dVolume = (timingInfo.timings[0].multiplier - 1) *
		initialVol * ratioPerSample;

	note->time = 0;
	note->timingEnd = timingInfo.timings[0].length;
	note->volume = initialVol;
	note->dVolume = dVolume;
	note->ratioPerSample = ratioPerSample;
	note->ratioIntoTiming = 0;
	note->timing = 0;
	note->fundFreq = fundFreq;
	note->timingInfo = timingInfo;
}

// Returns true if the note is still playing and false if it is finished
_Bool getNoteSample(struct NoteState *note, short *outputSample) {
	double val;
	unsigned harmonic, harmonicLimit;

	// Local copies
	unsigned timing = note->timing;
	unsigned fundFreq = note->fundFreq;
	unsigned time = note->time;
	double ratioIntoTiming = note->ratioIntoTiming;
	double volume = note->volume;
	double not_ratioIntoTiming = 1 - ratioIntoTiming;
	const struct Timing *timings = note->timingInfo.timings;

	val = 0;
	harmonicLimit = timings[timing].numHarmonics;
	for(harmonic = 0; harmonic < harmonicLimit; ++harmonic) {
		val += sin_scaled(fundFreq*(harmonic+1)*time) * (timings[timing].harmonics[harmonic] * not_ratioIntoTiming + timings[timing+1].harmonics[harmonic] * ratioIntoTiming);
	}
	val = val * volume;
	*outputSample = val;

	volume += note->dVolume;
	note->volume = volume;
	time += 1;
	note->time = time;
	note->ratioIntoTiming = ratioIntoTiming + note->ratioPerSample;

	if(time >= note->timingEnd) {
		++timing;
		if(timing < note->timingInfo.timingCount) {
			note->timing = timing;
			note->timingEnd += timings[timing].length;
			note->ratioPerSample = 1.0 / timings[timing].length;
			note->ratioIntoTiming = 0;
			note->dVolume = (timings[timing].multiplier - 1) * volume * note->ratioPerSample;
		} else return 0;
	}
	return 1;
}

struct PlayerState {
	struct NoteState *notes[NUM_STRINGS];
};

void *playerThread(void *input) {
	bcm_host_init();

	// Write 10ms worth of samples at once to sync with system clock rate
	const n_samples = SAMPLE_RATE / 100;
	AUDIOPLAY_STATE_T *player;
	struct PlayerState *state = input;
	audioplay_create(&player, SAMPLE_RATE, 1, SAMPLE_BITS,
					 2, SAMPLE_BITS/8 * n_samples);
	audioplay_set_dest(player, "local");

	while(1) {
		unsigned i, j;
		unsigned char stop;
		uint64_t ign;
		short *samples, currSample;

		// Get one of the buffers. This shouldn't ever wait long, so just spin
		while((samples = audioplay_get_buffer(player)) == NULL) ;

		stop = 0;
		for(i = 0; i < n_samples && !stop; ++i) {
			samples[i] = 0;
			for(j = 0; j < sizeof(state->notes)/sizeof(*state->notes); ++j) {
				struct NoteState *note = state->notes[j];
				if(note == (void*)-1U) {
					stop = 1;
					break;
				} else if(note != NULL) {
					if(!getNoteSample(note, &currSample))
						__sync_bool_compare_and_swap(&state->notes[j], note,
													 NULL, note,
													 &state->notes[j]);
					samples[i] += currSample;
				}
			}
		}

		if(stop) break;

		audioplay_play_buffer(player, samples, i*sizeof(*samples));
		if(i < n_samples)
			break;

		// If more than 12.5ms left, wait until no more than 12ms left
		// (don't wait for 10ms because of delay in being woken)
		uint32_t pendingSamples = audioplay_get_latency(player);
		if(pendingSamples >= SAMPLE_RATE/80)
			usleep(pendingSamples * (1000000/SAMPLE_RATE) - 12000);
	}

	audioplay_delete(player);
	return NULL;
}

int main() {
	if(isatty(0)) {
		struct termios t;
		tcgetattr(0, &t);
		t.c_lflag &= ~ICANON;
		t.c_cc[VMIN] = 1; // Return after each byte
		t.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &t);
	}

	struct NoteState notes[NUM_STRINGS][2];
	struct PlayerState state;
	memset(&state, 0, sizeof(state));
	unsigned char nextNote[NUM_STRINGS];
	memset(&nextNote, 0, sizeof(nextNote));

	pthread_t thread;
	pthread_create(&thread, NULL, playerThread, &state);

	int c, octave = 4;
	unsigned char string = 0;
	while((c = getchar()) != EOF) {
		static unsigned freq0[] = {2750, 3087, 1635, 1835, 2060, 2183, 2450};
		unsigned sharpMult = 271;

		if('a' <= c && 'g' >= c) {
			c -= 'a' - 'A';
			sharpMult = 256;
		}
		if('0' <= c && '8' >= c) octave = c - '0';
		else if('A' <= c && 'G' >= c) {
			unsigned freq = freq0[c - 'A'] * sharpMult;
			freq >>= 8 - octave;

			// Initial volume 0.3 to keep below 1 when all sine waves are added
			initNote(&notes[string][nextNote[string]], freq / 100, 0.3);
			__sync_synchronize();
			state.notes[string] = &notes[string][nextNote[string]];
			__sync_synchronize();
			nextNote[string] = (nextNote[string] + 1) & 1;
			string = (string + 1) % NUM_STRINGS;
		} else if(c == 0x04 && isatty(0)) {
			// Non-canonical terminal mode breaks ^D for EOF, so handle it here
			fputc('\n', stderr);
			break;
		}
	}

	__sync_synchronize();
	state.notes[0] = (void*)-1U; // Only need to set 1 to stop the player
	__sync_synchronize();
	pthread_join(thread, NULL);
	return 0;
}
