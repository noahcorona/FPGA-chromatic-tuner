#include "note.h"

//finds and prints note of frequency and deviation from note
void findNote(float f, int a4, int* oct, int* note, int* error) {
	//float c = 261.63;
	float f_a4 = (float) a4;
	float c = f_a4 * (float) pow(2.0, -0.75);
	float r;

	//determine which octave frequency is in
	if (f >= c) {
		while (f > c * 2) {
			c = c * 2;
			*oct = *oct + 1;
		}
	} else { //f < C4
		while (f < c) {
			c = c / 2;
			*oct = *oct - 1;
		}
	}

	if (*oct < 0 || *oct > 9)
		*oct = 0;

	//find note below frequency
	//c=middle C
	r = c * root2;
	while (f > r) {
		c = c * root2;
		r = r * root2;
		*note = *note + 1;
	}

	//determine which note frequency is closest to
	if ((f - c) <= (r - f)) { //closer to left note
		*error = (int) 1200.0 * log2f(f/c);
	} else { //f closer to right note
		*error = (int) 1200.0 * log2f(f/r);
		*note = *note + 1;
		if(*note == 12) {
			*note = 0;
			*oct = *oct + 1;
		}
	}

}
