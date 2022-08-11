/*
 * interrupts.c
 *
 *  Created on: Dec 17, 2020
 *      Author: noahc
 */

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

#define GPIO_CHANNEL1 1
#define INTC_GPIO_INTERRUPT_ID	XPAR_INTC_0_GPIO_0_VEC_ID
#define INTC_DEVICE_ID			XPAR_INTC_0_DEVICE_ID
#define INTC_HANDLER			XIntc_InterruptHandler

XIntc intc;
XStatus Status;
XTmrCtr axiTimer;
XGpio encoder;
XGpio btn;
XGpio dc;
XTmrCtr timer;
XTmrCtr timerDebounce;
uint32_t Control;
XSpi spi;

XSpi_Config *spiConfig;	/* Pointer to Configuration data */

u32 controlReg;

// state definitions for encoder FSM
u32 encoderData = 0;
int pinA = 0;
int pinB = 0;
int pressed = 0;
int fullCCW = 0; 	// initial (signify complete CW)
int fullCW = 0;		// initial (signify complete CCW)
int ccw1 = 0;  		// s1
int ccw2 = 0;  		// s2
int ccw3 = 0;  		// s3
int cw1 = 0; 		// s4
int cw2 = 0; 		// s5
int cw3 = 0; 		// s6
int done = 1;		// flag for completion

int menuStatus = 0; // 0 = hidden, 1 = octave selection, 2 = tune a4
int a4 = 440; 		// a4 initially at 440Hz
int a4Tmp = 440;
int octSel = 10; 	// auto-ranging initially on (octaves 0-9, 10 = auto)
int octSelTmp = 10;	// temp oct sel
int rangeMode = 0;	// mode = 0, 1, 2 for 3 different effective sample rates
int octMode = 1;

void GpioHandler(void *CallbackRef) {
	// disable btn interrupts (will be re-enabled by debounce timer)
	XIntc_Disable(&intc, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	XGpio_InterruptDisable(&btn, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalDisable(&btn);

	XGpio *GpioPtr = (XGpio *) CallbackRef;

	XGpio_InterruptClear(GpioPtr, GPIO_CHANNEL1);

	u32 data = XGpio_DiscreteRead(GpioPtr, GPIO_CHANNEL1);

	if (data == 1) { // down
		if (menuStatus == 0) { // enter oct sel menu
			menuStatus = 1;
			paintBgMode1();
			paintMode1(octSel);
		} else if (menuStatus == 1) { // close oct sel menu
			octSelTmp = octSel;
			menuStatus = 0;
			hideMode1();
		}
	} else if (data == 2) { // right
		if (menuStatus == 1) { // increase oct sel
			octSelTmp++;

			if(octSelTmp > 10)
				octSelTmp = 0;

			repaintMode1(octSelTmp);
		} else if (menuStatus == 2) { // increase a4
			if(a4Tmp < 460)
				a4Tmp++;

			repaintMode2(a4Tmp);
		}
	} else if (data == 4) { // left
		if (menuStatus == 1) { // decrease oct sel
			octSelTmp--;

			if(octSelTmp < 0)
				octSelTmp = 10;

			repaintMode1(octSelTmp);
		} else if (menuStatus == 2) { // decrease a4
			if(a4Tmp > 420)
				a4Tmp--;

			repaintMode2(a4Tmp);
		}
	} else if (data == 8) { // up
		if (menuStatus == 0) { 	// enter tune a4 menu
			menuStatus = 2;
			paintBgMode2();
			paintMode2(a4);
		} else if (menuStatus == 2) { // close tune a4 menu
			a4Tmp = a4;
			menuStatus = 0;
			hideMode2();
		}
	} else if (data == 16) { // center
		if (menuStatus == 1) { // set oct to selection
			octSel = octSelTmp;
			hideMode1();
		} else if (menuStatus == 2) { // set a4 to selection
			a4 = a4Tmp;
			hideMode2();
		}
	}

	// restart debounce timer
	XTmrCtr_Reset(&timerDebounce, 1);
	XTmrCtr_Start(&timerDebounce, 1);

	XIntc_Enable(&intc, XPAR_AXI_GPIO_BTN_DEVICE_ID);

}

void TwistHandler(void *CallbackRef) {
	XIntc_Disable(&intc, XPAR_ENCODER_DEVICE_ID);

	encoderData = XGpio_DiscreteRead(&encoder, 1);
	pinA = (encoderData & 1);
	pinB = (encoderData & 2) / 2;
	pressed = (encoderData & 4) / 4;

	if(pressed) {
		XGpio_InterruptDisable(&encoder, XGPIO_IR_CH1_MASK);
		XGpio_InterruptGlobalDisable(&encoder);

		if (menuStatus == 1) { // set oct to selection
			octSel = octSelTmp;
			hideMode1();
			menuStatus = 0;
		} else if (menuStatus == 2) { // set a4 to selection
			a4 = a4Tmp;
			hideMode2();
			menuStatus = 0;
		}

		// restart debounce timer
		XTmrCtr_Reset(&timerDebounce, 1);
		XTmrCtr_Start(&timerDebounce, 1);
	}

	if(pinA == 1 && pinB == 1) {
		if(cw1 == 1 && cw2 == 1 && cw3 == 1) {
			// last step complete
			//xil_printf("CW");
			fullCW = 1;
			done = 1;
			cw1 = cw2 = cw3 = 0;


			if (menuStatus == 0) { // enter oct sel menu
				menuStatus = 1;
				paintBgMode1();
				paintMode1(octSel);
			} else if (menuStatus == 1) { // decrease oct sel
				octSelTmp++;

				if (octSelTmp > 10)
					octSelTmp = 0;

				repaintMode1(octSelTmp);
			} else if (menuStatus == 2) { // decrease a4
				if (a4Tmp < 460)
					a4Tmp++;

				repaintMode2(a4Tmp);
			}
		} else if (ccw1 == 1 && ccw2 == 1 && ccw3) {
			// last step complete
			//xil_printf("CCW");
			fullCCW = 1;
			done = 1;
			ccw1 = ccw2 = ccw3 = 0;

			if (menuStatus == 0) { 	// enter tune a4 menu
				menuStatus = 2;
				paintBgMode2();
				paintMode2(a4);
			} else if (menuStatus == 1) { // increase oct sel
				octSelTmp--;

				if(octSelTmp < 0)
					octSelTmp = 10;

				repaintMode1(octSelTmp);
			} else if (menuStatus == 2) { // increase a4
				if(a4Tmp > 420)
					a4Tmp--;

				repaintMode2(a4Tmp);
			}
		} else if(cw1 == 1) // we are bouncing cw1 -> initial
			cw1 = 0;
		else if(ccw1 == 1) // we are bouncing ccw1 -> initial
			ccw1 = 0;
	} else if(pinA == 1 && pinB == 0) {
		// we could be completing first cw step,
		// or moving from ccw2 -> ccw3
		// or in a bounce back from cw2 -> cw1
		if(done == 1) // first cw step
			cw1 = 1;

		if(cw2 == 1) // cw2 -> cw1
			cw2 = 0;

		if(ccw2 == 1) // ccw2 -> ccw3
			ccw3 = 1;
	} else if (pinA == 0 && pinB == 1) {
		// we could be completing first ccw step,
		// or moving from cw2 -> cw3,
		// or bouncing back ccw2 -> ccw1
		if(done)
			ccw1 = 1;

		if(cw2 == 1)
			cw3 = 1;

		if(ccw2 == 1)
			ccw2 = 0;
	} else if (pinA == 0 && pinB == 0) {
		// we could be moving cw1 -> cw2,
		// bouncing back cw3 -> cw2,
		// moving ccw1 -> ccw2,
		// or bouncing back ccw3 -> ccw2
		if(cw3 == 1)
			cw3 = 0;
		else if (cw1 == 1) // cw3 is 0 so must be moving forward
			cw2 = 1;

		if(ccw3 == 1)
			ccw3 = 0;
		else if(ccw1 == 1) // ccw3 is 0 so must be moving forward
			ccw2 = 1;

	}

	XGpio_InterruptClear(&encoder, XGPIO_IR_CH1_MASK);
	XIntc_Enable(&intc, XPAR_ENCODER_DEVICE_ID);
}

void debounceInterrupt() {
	//re-enable interrupts
	XGpio_InterruptClear(&btn, XGPIO_IR_CH1_MASK);
	XGpio_InterruptEnable(&btn, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalEnable(&btn);

	XGpio_InterruptClear(&encoder, XGPIO_IR_CH1_MASK);
	XGpio_InterruptEnable(&encoder, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalEnable(&encoder);

	// stop timer after 0.5 sec timer has passed
	XTmrCtr_Stop(&timerDebounce, 1);
}

void initInterrupts() {
	// initialize interrupt controller
	Status = XIntc_Initialize(&intc, XPAR_INTC_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to initialize Interrupt Controller\n\r");
	} else {
		xil_printf("Successfully initialized Interrupt Controller\n\r");
	}

	// initialize push buttons
	Status = XGpio_Initialize(&btn, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to initialize Button GPIO driver\n\r");
	} else {
		xil_printf("Successfully initialized Button GPIO driver\n\r");
	}

	XGpio_SetDataDirection(&btn, GPIO_CHANNEL1, 0x0000001F);

	// initialize rotary encoder
	Status = XGpio_Initialize(&encoder, XPAR_ENCODER_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to initialize rotary encoder\n\r");
	} else {
		xil_printf("Successfully initialized rotary encoder\n\r");
	}

	XGpio_SetDataDirection(&encoder, GPIO_CHANNEL1, 0x00000003);

	//set up program timer
	Status = XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to initialize program timer\n\r");
	} else {
		xil_printf("Successfully initialized program timer\n\r");
	}

	Control = XTmrCtr_GetOptions(&timer,
			0) | XTC_CAPTURE_MODE_OPTION | XTC_INT_MODE_OPTION;
	XTmrCtr_SetOptions(&timer, 0, Control);

	// set up debounce timer
	Status = XTmrCtr_Initialize(&timerDebounce, XPAR_AXI_TIMER_1_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to initialize debounce timer\n\r");
	} else {
		xil_printf("Successfully initialized debounce timer\n\r");
	}

	XTmrCtr_SetOptions(&timerDebounce, 1, XTC_INT_MODE_OPTION);
	XTmrCtr_SetResetValue(&timerDebounce, 1, 0xFD050F80); // 0.5 sec

	/*
	 * Initialize the GPIO driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h
	 */
	Status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialize GPIO dc fail!\n");
	}

	/*
	 * Set the direction for all signals to be outputs
	 */
	XGpio_SetDataDirection(&dc, 1, 0x0);

	/*
	 * Initialize the SPI driver so that it is  ready to use.
	 */
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (spiConfig == NULL) {
		xil_printf("Can't find spi device!\n");
	}

	Status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");
	}

	/*
	 * Reset the SPI device to leave it in a known good state.
	 */
	XSpi_Reset(&spi);

	/*
	 * Setup the control register to enable master mode
	 */
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) & (~XSP_CR_TRANS_INHIBIT_MASK));

	// Select 1st slave device
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	/* Hook up interrupt service routines */
	XIntc_Connect(&intc,
			XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR,
			(Xil_ExceptionHandler) GpioHandler, &btn);
	XIntc_Connect(&intc,
			XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR,
			(XInterruptHandler) TwistHandler, &encoder);
	XIntc_Connect(&intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_1_INTERRUPT_INTR,
			(XInterruptHandler) debounceInterrupt, (void *) &timerDebounce);

	initLCD();
	clrScr();
}

void startInterrupts() {
	// first we enable each of our 3 interrupts
	// gpio/push button interrupt
	XIntc_Enable(&intc,
			XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR);

	XGpio_InterruptEnable(&btn, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalEnable(&btn);

	// rotary encoder interrupt
	XIntc_Enable(&intc,
	XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR);

	XGpio_InterruptEnable(&encoder, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalEnable(&encoder);

	// enable program timer
	XIntc_Enable(&intc,
			XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);

	// enable debounce timer
	XIntc_Enable(&intc,
			XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_1_INTERRUPT_INTR);

	// then we start the interrupt controller
	XIntc_Start(&intc, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to start Interrupt Controller\n\r");
	} else {
		xil_printf("Successfully started Interrupt Controller\n\r");
	}

	// enable microblaze interrupts
	microblaze_enable_interrupts();

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) INTC_HANDLER, &intc);
	Xil_ExceptionEnable();
}
