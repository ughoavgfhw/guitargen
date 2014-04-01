#include <math.h>
#include <unistd.h>
#include <stdio.h>

#define FUND_FREQ 110
#define SAMPLE_RATE 44100

// Returns the result of sin with the positive argument scaled by 2pi
double sin_scaled(double x) {
	// The first 1/4 of a sine wave. Next it goes to 1, then back down, then repeats negative
	static const double sinarr[] = {
		0.000000, 0.012272, 0.024541, 0.036807, 0.049068, 0.061321, 0.073565, 0.085797,
		0.098017, 0.110222, 0.122411, 0.134581, 0.146730, 0.158858, 0.170962, 0.183040,
		0.195090, 0.207111, 0.219101, 0.231058, 0.242980, 0.254866, 0.266713, 0.278520,
		0.290285, 0.302006, 0.313682, 0.325310, 0.336890, 0.348419, 0.359895, 0.371317,
		0.382683, 0.393992, 0.405241, 0.416430, 0.427555, 0.438616, 0.449611, 0.460539,
		0.471397, 0.482184, 0.492898, 0.503538, 0.514103, 0.524590, 0.534998, 0.545325,
		0.555570, 0.565732, 0.575808, 0.585780, 0.595699, 0.605511, 0.615232, 0.624859,
		0.634393, 0.643832, 0.653173, 0.662416, 0.671559, 0.680601, 0.689541, 0.698376,
		0.707107, 0.715731, 0.724247, 0.732654, 0.740951, 0.749136, 0.757209, 0.765167,
		0.773010, 0.780737, 0.788346, 0.795837, 0.803208, 0.810457, 0.817585, 0.824589,
		0.831470, 0.838225, 0.844854, 0.851355, 0.857729, 0.863973, 0.870087, 0.876070,
		0.881921, 0.887640, 0.893224, 0.898674, 0.903989, 0.909168, 0.914210, 0.919114,
		0.923880, 0.928506, 0.932993, 0.937339, 0.941544, 0.945607, 0.949528, 0.953306,
		0.956940, 0.960431, 0.963776, 0.966976, 0.970031, 0.972940, 0.975702, 0.978317,
		0.980785, 0.983105, 0.985278, 0.987301, 0.989177, 0.990903, 0.992480, 0.993907,
		0.995185, 0.996313, 0.997290, 0.998118, 0.998795, 0.999322, 0.999699, 0.999925,

		1.0 // Extra for the reverse part
	};
#define numVals (sizeof(sinarr)/sizeof(*sinarr) - 1)
	double intPart;
	unsigned pos = round(modf(x*4, &intPart) * numVals);
	switch(((unsigned)intPart) % 4) {
		case 0: return sinarr[pos];
		case 1: return sinarr[numVals - pos];
		case 2: return -sinarr[pos];
		case 3: return -sinarr[numVals - pos];
	}
#undef numVals
	return 0;
}
#define sin(x) sin_scaled((x)/(2*M_PI))

#define MAX_HARMONICS 45
const struct {
	double length;
	double multiplier;
	unsigned numHarmonics;
	double harmonics[MAX_HARMONICS];
} timings[] = {
		// Unused partials: 34, 28, 10, 29, 17
		// Harmonic			 2       3       4       5       6       7      8   9      10      11      12      13      14      15      16     17 18  19      20      21      22      23      24     25 26 27  28      29      30      31      32      33     34 35 36 37 38  39      40     41  42      43      44
		// Partial Number	32      35      36      31      24      25      -  01      02      19      20      26      27      00      03      -  -  04      05      06      21      07      08      -  -  -  09      22      23      30      11      12      -  -  -  -  -  13      14      -  15      16      18
	// 1.000 second up to 5.0kHz split to 0.1 + 0.15 + 0.25 + 0.5 seconds
	//{0.100, 1.0000, 0, {0}},
	{0.150, 0.7638, 44, {1, 0.9394, 0.2622, 0.3507, 0.2504, 0.1056, 0.1134, 0, 0.0140, 0.0783, 0.1048, 0.0434, 0.0680, 0.1010, 0.0654, 0.0293, 0, 0, 0.0339, 0.0702, 0.0902, 0.1042, 0.0628, 0.0323, 0, 0, 0, 0.0293, 0.0262, 0.1166, 0.0376, 0.0374, 0.0610, 0, 0, 0, 0, 0, 0.0298, 0.0387, 0, 0.0289, 0.0295, 0.0331}},
	{0.250, 0.7094, 44, {1, 0.8860, 0.5025, 0.5161, 0.3476, 0.1453, 0.1351, 0, 0.0354, 0.0904, 0.1348, 0.0569, 0.1348, 0.0588, 0.0623, 0.0290, 0, 0, 0.0254, 0.0312, 0.0830, 0.1015, 0.0621, 0.0245, 0, 0, 0, 0.0156, 0.0101, 0.0550, 0.0172, 0.0189, 0.0364, 0, 0, 0, 0, 0, 0.0112, 0.0108, 0, 0.0112, 0.0113, 0.0028}},
	{0.500, 0.7273, 44, {1, 0.4717, 0.4832, 0.5992, 0.4270, 0.1353, 0.1380, 0, 0.0375, 0.0723, 0.1002, 0.0874, 0.1811, 0.0272, 0.0328, 0.0254, 0, 0, 0.0257, 0.0393, 0.0411, 0.0568, 0.0204, 0.0124, 0, 0, 0, 0.0053, 0.0094, 0.0142, 0.0076, 0.0095, 0.0097, 0, 0, 0, 0, 0, 0.0022, 0.0013, 0, 0.0023, 0.0024, 0.0007}},
	// 0.500 second up to 4.0kHz
	{0.500, 0.7678, 33, {1, 0.2216, 0.3029, 0.5942, 0.4486, 0.0752, 0.0961, 0, 0.0230, 0.0216, 0.0622, 0.0908, 0.0703, 0.0251, 0.0270, 0.0152, 0, 0, 0.0074, 0.0130, 0.0175, 0.0238, 0.0103, 0.0054, 0, 0, 0, 0.0015, 0.0028, 0.0048, 0.0020, 0.0022, 0.0016}},
	// 0.375 second up to 3.35kHz
	{0.375, 0.8046, 30, {1, 0.2113, 0.2332, 0.5782, 0.4158, 0.0518, 0.0381, 0, 0.0153, 0.0387, 0.0944, 0.0372, 0.0874, 0.0292, 0.0273, 0.0103, 0, 0, 0.0081, 0.0093, 0.0064, 0.0096, 0.0030, 0.0033, 0, 0, 0, 0.0000, 0.0000, 0.0018}},
	// 0.250 second up to 2.6kHz
	{0.250, 0.7014, 23, {1, 0.1221, 0.2184, 0.5863, 0.3852, 0.0537, 0.0115, 0, 0.0144, 0.0494, 0.0854, 0.0399, 0.1212, 0.0308, 0.0087, 0.0080, 0, 0, 0.0032, 0.0046, 0.0048, 0.0063, 0.0025}},
	// 0.625 second up to 2.5kHz
	{0.625, 0.5755, 22, {1, 0.1933, 0.1782, 0.6735, 0.3806, 0.0680, 0.0478, 0, 0.0074, 0.0454, 0.0398, 0.0705, 0.0637, 0.0295, 0.0148, 0.0064, 0, 0, 0.0024, 0.0056, 0.0026, 0.0033}},
	// 0.500 second up to 2.0kHz
	{0.500, 0.2765, 16, {1, 0.3174, 0.1389, 0.9126, 0.5333, 0.0827, 0.0715, 0, 0.0114, 0.0185, 0.0865, 0.0429, 0.0878, 0.0325, 0.0149, 0.0052}},
	// 1.750 second up to 1.6kHz
	{1.750, 0.2287, 14, {1, 0.9405, 0.1277, 1.4852, 1.4344, 0.1408, 0.0746, 0, 0.0396, 0.0708, 0.1134, 0.0952, 0.1302, 0.0471}},
	{0, 1, 0, {0}}
	// Fund. Freq. Volumes: 0.109247 0.083442 0.059196 0.043054 0.033059 0.026599 0.018656 0.010737 0.002969 end=0.000679
	
	// or 1.0 to 5k; 0.5 to 4k; 0.5 to 3.35k; 0.5 to 2.5k; 0.75 to 2.0k; 1.75 to 1.6k
};
#define NUM_TIMINGS (sizeof(timings)/sizeof(*timings))
#define SAMPLE_BITS 16

void playNote(unsigned fundFreq) {
	double time = 0, end;
	double val;
	double dVolume, volume = 0.3; // Start at 0.5 so combining sin waves does not cross 1
	double ratioPerSample, ratioIntoTiming;
	unsigned short sample;
	
	unsigned timing,harmonic;
	time = 0;
	timing = 0;
	end = timings[0].length;
	ratioPerSample = 1.0 / timings[0].length / SAMPLE_RATE;
	dVolume = (volume*timings[0].multiplier - volume) * ratioPerSample;
	ratioIntoTiming = 0;
	
	for(;;) {
		if(time >= end) {
			++timing;
			if(timing < NUM_TIMINGS) {
				end += timings[timing].length;
				ratioPerSample = 1.0 / timings[timing].length / SAMPLE_RATE;
				ratioIntoTiming = 0;
				dVolume = (volume*timings[timing].multiplier - volume) * ratioPerSample;
			} else {
				break;
			}
		}
		val = 0;
		unsigned harmonicLimit = timings[timing].numHarmonics;
		for(harmonic = 0; harmonic < harmonicLimit; ++harmonic) {
			val += sin_scaled(fundFreq*(harmonic+1)*time) * (timings[timing].harmonics[harmonic] * (1-ratioIntoTiming) + timings[timing+1].harmonics[harmonic] * ratioIntoTiming);
		}
		val = val * volume;
		val = (val/2) + 0.5; // Scale into 0-1 range
		if(val < 1.0) sample = val * (1<<SAMPLE_BITS) - (1<<(SAMPLE_BITS-1));
		else sample = (1<<SAMPLE_BITS)-1;
		
		write(1, &sample, 2);
		
		volume += dVolume;
		time += 1.0/SAMPLE_RATE;
		ratioIntoTiming += ratioPerSample;
	}
}

int main() {
	int c;
	while((c = getchar()) != EOF) {
		static unsigned freq0[] = {2750, 3087, 1635, 1835, 2060, 2183, 2450};
		unsigned freq = freq0[c - 'A'];
		c = getchar();
		freq <<= c - '0';
		playNote(freq / 100);
	}

	return 0;
}
