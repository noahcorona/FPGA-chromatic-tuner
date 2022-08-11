/*
 * interrupts.h
 *
 *  Created on: Dec 17, 2020
 *      Author: noahc
 */

#ifndef SRC_INTERRUPTS_H_
#define SRC_INTERRUPTS_H_

#include <stdio.h>
#include "xil_cache.h"
#include <mb_interface.h>
#include "xparameters.h"
#include <xgpio.h>
#include <xil_types.h>
#include <xil_assert.h>
#include "xil_exception.h"
#include <xio.h>
#include "xspi.h"
#include "xtmrctr.h"
#include "xintc.h"
#include "lcd.h"

extern XIntc intc;
XStatus Status;
XTmrCtr axiTimer;
XGpio encoder;
XGpio btn;
XGpio dc;
extern XTmrCtr timer;
XTmrCtr timerDebounce;
uint32_t Control;
XSpi spi;

XSpi_Config *spiConfig;	/* Pointer to Configuration data */

u32 controlReg;

// state definitions for encoder FSM
u32 encoderData;
int pinA;
int pinB;
int pressed;
int fullCCW; 	// initial (signify complete CW)
int fullCW;		// initial (signify complete CCW)
int ccw1;  		// s1
int ccw2;  		// s2
int ccw3;  		// s3
int cw1; 		// s4
int cw2; 		// s5
int cw3; 		// s6
int done;		// flag for completion

extern int menuStatus; // 0 = hidden, 1 = octave selection, 2 = tune a4
extern int a4; 		// a4 initially at 440Hz
int a4Tmp;
extern int octSel; 	// auto-ranging initially on (octaves 0-9, 10 = auto)
int octSelTmp;	// temp oct sel
int rangeMode;	// mode = 0, 1, 2 for 3 different effective sample rates
extern int octMode;

void initInterrupts();
void startInterrupts();
void GpioHandler(void *CallbackRef);
void TwistHandler(void *CallbackRef);
void debounceInterrupt();


#endif /* SRC_INTERRUPTS_H_ */
