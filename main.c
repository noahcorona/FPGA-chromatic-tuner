/*
 * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include "lcd.h"
#include "interrupts.h"

#define CLOCK 100000000.0 //clock speed

int showDetails = 0;// show/hide freq. and run time (0 = hide, 1 = show)

int samples;
int m;
int decimation_factor;
float decimated_sample_f;
int m_decimated;
int samples_decimated;

int int_buffer[MAX_SAMPLES];
static float q[MAX_SAMPLES];
static float w[MAX_SAMPLES];

void read_fsl_values(float* q) {
	int i, j;
	unsigned int x;
	stream_grabber_start();

	j = 0;
	for (i = 0; i < samples_decimated; i++) {
		int_buffer[i] = stream_grabber_read_sample(j);
		x = int_buffer[i];
		q[i] = 0.000000049173831939 * x; // = 3.3/67108864.0 * x; (3.3V and 2^26 bit precision.)
		j += decimation_factor;
	}
}


int main() {
	Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();

	//configure default effective sample rate and sample freq
	samples = 4096;
	m = 12;
	decimation_factor = 4;
	m_decimated = m - 2;
	samples_decimated = samples / decimation_factor;
	decimated_sample_f = CLOCK / decimation_factor / 2048.0;

	//program timer variables
	int ticks;
	float tot_time;

	// measurement result variables
	float frequency;
	int octave;
	int note;
	int error;
	int prevError = 0;

	//configure and start interrupts
	initInterrupts();
	startInterrupts();

	//generate sin/cos LUT
	generateLUT(samples_decimated);

	// use largest font, paint background color
	setFont(BigFont);
	paintBg(showDetails);

	while (1) {
		XTmrCtr_Start(&timer, 0);

		// configure effective sample rate and f
		// to one of the configurations
		if (octMode == 1) {
			samples = 4096;
			m = 12;
			decimation_factor = 4;
			m_decimated = m - 2;
			samples_decimated = samples / decimation_factor;
			decimated_sample_f = CLOCK / decimation_factor / 2048.0;
		} else if (octMode == 2) {
			// configure effective sample rate and f
			samples = 2048;
			m = 11;
			decimation_factor = 2;
			m_decimated = m - 1;
			samples_decimated = samples / decimation_factor;
			decimated_sample_f = CLOCK / decimation_factor / 2048.0;
		}

		//Read Values from Microblaze buffer, which is continuously populated by AXI4 Streaming Data FIFO.
		read_fsl_values(q);

		//zero w array
		memset(w, 0, sizeof(w));

		octave = 4;
		note = 0;

		XIntc_Disable(&intc, XPAR_ENCODER_DEVICE_ID);

		//compute FFT and write to freq
		frequency = fft(q, w, samples_decimated, m_decimated,
				decimated_sample_f);

		if (frequency > 1.0) {
			// compute note, octave, and error in cents
			findNote(frequency, a4, &octave, &note, &error);

			if (octSel == 10) { //auotmatic ranging
				if (octave > 7 || frequency > 8000)
					octMode = 2;
				else
					octMode = 1;
			} else if (octSel > 7)
				octMode = 2;
			else
				octMode = 1;

			microblaze_disable_interrupts();

			ticks = XTmrCtr_GetValue(&timer, 0);
			XTmrCtr_Stop(&timer, 0);
			tot_time = ticks / CLOCK;

			//draw extra info if wanted
			if(showDetails) {
				drawFreq((int) frequency + 0.5);
				drawTime((int) (1000 * tot_time));
			}

			paintMode0(note, octave, error);
			drawError(prevError, error);
			prevError = error;

			microblaze_enable_interrupts();
		}

		XIntc_Enable(&intc, XPAR_ENCODER_DEVICE_ID);

	}

	return 0;
}
