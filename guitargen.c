#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdint.h>
#include <termios.h>
#include "rawaudio/audioplay.h"

#define dump(b,s) //write(1,(b),(s))

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
const struct Timing _t_D3S[] = {
#  include "timings/D3S.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_E3[] = {
#  include "timings/E3.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_F3[] = {
#  include "timings/F3.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_F3S[] = {
#  include "timings/F3S.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_G3[] = {
#  include "timings/G3.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_G3S[] = {
#  include "timings/G3S.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_A3[] = {
#  include "timings/A3.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_A3S[] = {
#  include "timings/A3S.c"
	{0, 1, 0, {0}}
};
const struct Timing _t_B3[] = {
#  include "timings/B3.c"
	{0, 1, 0, {0}}
};

// Ordered by frequency, non-overlapping
const struct FrequencyTimingRange freqTimeMap[] = {
	TIMING_MAP(106, 113, _t_A2),
	TIMING_MAP(113, 120, _t_A2S),
	TIMING_MAP(120, 126, _t_B2),
	TIMING_MAP(126, 134, _t_C3),
	TIMING_MAP(134, 142, _t_C3S),
	TIMING_MAP(142, 150, _t_D3),
	TIMING_MAP(150, 160, _t_D3S),
	TIMING_MAP(160, 170, _t_E3),
	TIMING_MAP(170, 180, _t_F3),
	TIMING_MAP(180, 190, _t_F3S),
	TIMING_MAP(190, 201, _t_G3),
	TIMING_MAP(201, 213, _t_G3S),
	TIMING_MAP(213, 226, _t_A3),
	TIMING_MAP(226, 240, _t_A3S),
	TIMING_MAP(240, 254, _t_B3)
};

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
		val += sin_scaled(fundFreq*(harmonic+1)*time) *
			(timings[timing].harmonics[harmonic] * not_ratioIntoTiming +
			 timings[timing+1].harmonics[harmonic] * ratioIntoTiming);
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

#define NUM_NOTES (sizeof(state->notes)/sizeof(*state->notes))

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
			for(j = 0; j < NUM_NOTES; ++j) {
				struct NoteState *note = state->notes[j];
				if(note == (void*)-1U) {
					stop = 1;
					break;
				} else if(note != NULL) {
					if(!getNoteSample(note, &currSample))
						__sync_bool_compare_and_swap(&state->notes[j], note,
													 NULL, note,
													 &state->notes[j]);
					samples[i] += currSample / NUM_NOTES;
				}
			}
		}

		if(stop) break;

		audioplay_play_buffer(player, samples, i*sizeof(*samples));
		dump(samples, i*sizeof(*samples));
		if(i < n_samples)
			break;

		// If more than 12.5ms left, wait until no more than 12ms left
		// (don't wait for 10ms because of delay in being woken)
		uint32_t pendingSamples = audioplay_get_latency(player);
		if(pendingSamples >= SAMPLE_RATE/80)
			usleep(pendingSamples * (1000000/SAMPLE_RATE) - 12000);
	}
#undef NUM_NOTES

	audioplay_delete(player);
	return NULL;
}

int main() {
	if(isatty(0)) {
		struct termios t;
		tcgetattr(0, &t);
		cfmakeraw(&t); // Raw input mode
		t.c_cc[VMIN] = 3; // Return after each 3-byte packet
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

	uint8_t buffer[3];
	while(read(0, buffer, sizeof(buffer)) == sizeof(buffer)) {
		static unsigned freqs[19+(NUM_STRINGS-1)*5] = {
			11000, 11654, 12347, 13081, 13859, 14683, 15556, 16481,
			17461, 18500, 19600, 20765, 22000, 23308, 24694, 26163,
			27718, 29366, 31113, 32963, 34923, 36999, 39200, 41530
		};

		uint8_t string = buffer[0] - 1;
		uint8_t isRelease = 0;
		if(string & 0x80) {
			string &= 0x7F;
			isRelease = 1;
		}

		if(string >= NUM_STRINGS || buffer[1] > 18 ||
		   buffer[2] != 0xff)
			continue;

		if(!isRelease) {
			unsigned freq = freqs[string*5+buffer[1]];

			// Initial volume 0.3 to keep below 1 when all sine waves are added
			initNote(&notes[string][nextNote[string]], (freq+50) / 100, 0.3);
			__sync_synchronize();
			state.notes[string] = &notes[string][nextNote[string]];
			__sync_synchronize();
			nextNote[string] = (nextNote[string] + 1) & 1;
		} else {
			__sync_synchronize();
			state.notes[string] = NULL;
			__sync_synchronize();
		}
	}

	__sync_synchronize();
	state.notes[0] = (void*)-1U; // Only need to set 1 to stop the player
	__sync_synchronize();
	pthread_join(thread, NULL);
	return 0;
}
