/*
 * lcd.c
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
  UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards
  Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
  
  This library is the continuation of my ITDB02_Graph, ITDB02_Graph16
  and RGB_GLCD libraries for Arduino and chipKit. As the number of 
  supported display modules and controllers started to increase I felt 
  it was time to make a single, universal library as it will be much 
  easier to maintain in the future.

  Basic functionality of this library was origianlly based on the 
  demo-code provided by ITead studio (for the ITDB02 modules) and 
  NKC Electronics (for the RGB GLCD module/shield).

  This library supports a number of 8bit, 16bit and serial graphic 
  displays, and will work with both Arduino, chipKit boards and select 
  TI LaunchPads. For a full list of tested display modules and controllers,
  see the document UTFT_Supported_display_modules_&_controllers.pdf.

  When using 8bit and 16bit display modules there are some 
  requirements you must adhere to. These requirements can be found 
  in the document UTFT_Requirements.pdf.
  There are no special requirements when using serial displays.

  You can find the latest version of the library at 
  http://www.RinkyDinkElectronics.com/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the 
  examples and tools supplied with the library.
*/

#include "lcd.h"

// Global variables
int fch;
int fcl;
int bch;
int bcl;
struct _current_font cfont;

//array to store note names for findNote
static char notes[12][3] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

// Read data from LCD controller
// FIXME: not work
u32 LCD_Read(char VL)
{
    u32 retval = 0;
    int index = 0;

    Xil_Out32(SPI_DC, 0x0);
    Xil_Out32(SPI_DTR, VL);
    
    //while (0 == (Xil_In32(SPI_SR) & XSP_SR_TX_EMPTY_MASK));
    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
    Xil_Out32(SPI_DC, 0x01);

    while (1 == (Xil_In32(SPI_SR) & XSP_SR_RX_EMPTY_MASK));
    xil_printf("SR = %x\n", Xil_In32(SPI_SR));


    while (0 == (Xil_In32(SPI_SR) & XSP_SR_RX_EMPTY_MASK)) {
       retval = (retval << 8) | Xil_In32(SPI_DRR);
       xil_printf("receive %dth byte\n", index++);
    }

    xil_printf("SR = %x\n", Xil_In32(SPI_SR));
    xil_printf("SR = %x\n", Xil_In32(SPI_SR));
    return retval;
}


// Write command to LCD controller
void LCD_Write_COM(char VL)  
{
    Xil_Out32(SPI_DC, 0x0);
    Xil_Out32(SPI_DTR, VL);
    
    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Write 16-bit data to LCD controller
void LCD_Write_DATA16(char VH, char VL)
{
    Xil_Out32(SPI_DC, 0x01);
    Xil_Out32(SPI_DTR, VH);
    Xil_Out32(SPI_DTR, VL);

    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Write 8-bit data to LCD controller
void LCD_Write_DATA(char VL)
{
    Xil_Out32(SPI_DC, 0x01);
    Xil_Out32(SPI_DTR, VL);

    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Initialize LCD controller
void initLCD(void)
{
    int i;

    // Reset
    LCD_Write_COM(0x01);
    for (i = 0; i < 500000; i++); //Must wait > 5ms


    LCD_Write_COM(0xCB);
    LCD_Write_DATA(0x39);
    LCD_Write_DATA(0x2C);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x34);
    LCD_Write_DATA(0x02);

    LCD_Write_COM(0xCF); 
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0XC1);
    LCD_Write_DATA(0X30);

    LCD_Write_COM(0xE8); 
    LCD_Write_DATA(0x85);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x78);

    LCD_Write_COM(0xEA); 
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x00);
 
    LCD_Write_COM(0xED); 
    LCD_Write_DATA(0x64);
    LCD_Write_DATA(0x03);
    LCD_Write_DATA(0X12);
    LCD_Write_DATA(0X81);

    LCD_Write_COM(0xF7); 
    LCD_Write_DATA(0x20);
  
    LCD_Write_COM(0xC0);   //Power control 
    LCD_Write_DATA(0x23);  //VRH[5:0] 
 
    LCD_Write_COM(0xC1);   //Power control 
    LCD_Write_DATA(0x10);  //SAP[2:0];BT[3:0] 

    LCD_Write_COM(0xC5);   //VCM control 
    LCD_Write_DATA(0x3e);  //Contrast
    LCD_Write_DATA(0x28);
 
    LCD_Write_COM(0xC7);   //VCM control2 
    LCD_Write_DATA(0x86);  //--
 
    LCD_Write_COM(0x36);   // Memory Access Control 
    LCD_Write_DATA(0x48);  

    LCD_Write_COM(0x3A);   
    LCD_Write_DATA(0x55);

    LCD_Write_COM(0xB1);   
    LCD_Write_DATA(0x00); 
    LCD_Write_DATA(0x18);
 
    LCD_Write_COM(0xB6);   // Display Function Control 
    LCD_Write_DATA(0x08);
    LCD_Write_DATA(0x82);
    LCD_Write_DATA(0x27); 

    LCD_Write_COM(0x11);   //Exit Sleep 
    for (i = 0; i < 100000; i++);
                
    LCD_Write_COM(0x29);   //Display on 
    LCD_Write_COM(0x2c); 

    //for (i = 0; i < 100000; i++);

    // Default color and fonts
    fch = 0xFF;
    fcl = 0xFF;
    bch = 0x00;
    bcl = 0x00;
    setFont(SmallFont);
}


// Set boundary for drawing
void setXY(int x1, int y1, int x2, int y2)
{
    LCD_Write_COM(0x2A); 
    LCD_Write_DATA(x1 >> 8);
    LCD_Write_DATA(x1);
    LCD_Write_DATA(x2 >> 8);
    LCD_Write_DATA(x2);
    LCD_Write_COM(0x2B); 
    LCD_Write_DATA(y1 >> 8);
    LCD_Write_DATA(y1);
    LCD_Write_DATA(y2 >> 8);
    LCD_Write_DATA(y2);
    LCD_Write_COM(0x2C);
}


// Remove boundry
void clrXY(void)
{
    setXY(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}


// Set foreground RGB color for next drawing
void setColor(u8 r, u8 g, u8 b)
{
    // 5-bit r, 6-bit g, 5-bit b
    fch = (r & 0x0F8) | g >> 5;
    fcl = (g & 0x1C) << 3 | b >> 3;
}


// Set background RGB color for next drawing
void setColorBg(u8 r, u8 g, u8 b)
{
    // 5-bit r, 6-bit g, 5-bit b
    bch = (r & 0x0F8) | g >> 5;
    bcl = (g & 0x1C) << 3 | b >> 3;
}


// Clear display
void clrScr(void)
{
    // Black screen
    setColor(0, 0, 0);

    fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}


// Draw horizontal line
void drawHLine(int x, int y, int l)
{
    int i;

    if (l < 0) {
        l = -l;
        x -= l;
    }

    setXY(x, y, x + l, y);
    for (i = 0; i < l + 1; i++) {
        LCD_Write_DATA16(fch, fcl);
    }

    clrXY();
}


// Fill a rectangular 
void fillRect(int x1, int y1, int x2, int y2)
{
    int i;

    if (x1 > x2)
        swap(int, x1, x2);

    if (y1 > y2)
        swap(int, y1, y2);

    setXY(x1, y1, x2, y2);
    for (i = 0; i < (x2 - x1 + 1) * (y2 - y1 + 1); i++) {
        LCD_Write_DATA16(fch, fcl);
    }

   clrXY();
}


// Select the font used by print() and printChar()
void setFont(u8* font)
{
	cfont.font=font;
	cfont.x_size = font[0];
	cfont.y_size = font[1];
	cfont.offset = font[2];
	cfont.numchars = font[3];
}


// Print a character
void printChar(u8 c, int x, int y)
{
    u8 ch;
    int i, j, pixelIndex;


    setXY(x, y, x + cfont.x_size - 1,y + cfont.y_size - 1);

    pixelIndex =
            (c - cfont.offset) * (cfont.x_size >> 3) * cfont.y_size + 4;
    for(j = 0; j < (cfont.x_size >> 3) * cfont.y_size; j++) {
        ch = cfont.font[pixelIndex];
        for(i = 0; i < 8; i++) {   
            if ((ch & (1 << (7 - i))) != 0)   
                LCD_Write_DATA16(fch, fcl);
            else
                LCD_Write_DATA16(bch, bcl);
        }
        pixelIndex++;
    }

    clrXY();
}


// Print string
void lcdPrint(char *st, int x, int y)
{
    int i = 0;

    while(*st != '\0')
        printChar(*st++, x + cfont.x_size * i++, y);
}

void paintBg(int displayDetails) {
	// light gray background color
	setColor(200, 200, 200);
	fillRect(0, 0, 240, 320);

	if(displayDetails) {
		setColor(255, 87, 51);
		setColorBg(200, 200, 200);
		lcdPrint("ms", 204, 300);

		setColor(255, 87, 51);
		setColorBg(200, 200, 200);
		lcdPrint("F:", 4, 300);
	}

	paintBgMode0();
}

void paintBgMode0() {
	setColor(255, 255, 255);
	fillRect(10, 10, 230, 40);
	setColor(230, 230, 230);
	fillRect(10, 40, 230, 90);

	setColor(255, 87, 51);
	setColorBg(255, 255, 255);
	lcdPrint("Note", 90, 18);
	setColorBg(230, 230, 230);
	lcdPrint("cents", 127, 68);
}

void paintMode0(int note, int oct, int error) {
	if(note != 1 && note != 3 &&
		note != 6 && note != 8 && note != 10) {
		// repaint # to bg
		setColor(230, 230, 230);
		fillRect(45, 55, 60, 70);
	}


	setColor(255, 87, 51);
	setColorBg(230, 230, 230);
	char sOct[2];
	sprintf(sOct, "%d", oct);

	lcdPrint(notes[note], 30, 55);
	lcdPrint(sOct, 62, 55);

	char sError[4];

	if(error >= 0)
		sprintf(sError, "+%02d", error);
	else
		sprintf(sError, "%03d", error);

	lcdPrint(sError, 150, 50);
}

void paintBgMode1() {
	setColor(255, 255, 255);
	fillRect(10, 125, 230, 173);
	setColor(230, 230, 230);
	fillRect(10, 173, 230, 285);

	setColor(255, 87, 51);
	setColorBg(255, 255, 255);
	lcdPrint("Octave", 75, 133);
	lcdPrint("Selection", 50, 151);

	setColorBg(230, 230, 230);
	lcdPrint("A=auto-range", 25, 181);
	lcdPrint("Current:", 25, 219);
	lcdPrint("Selected: ", 25, 255);

}

void hideMode1() {
	setColor(200, 200, 200);
	fillRect(10, 125, 230, 285);
}

void paintMode1(int currentOctaveSel) {
	setColor(255, 87, 51);
	setColorBg(230, 230, 230);

	char sOctSel[1];

	if(currentOctaveSel == 10)
		sprintf(sOctSel, "A", currentOctaveSel);
	else
		sprintf(sOctSel, "%d", currentOctaveSel);

	lcdPrint(sOctSel, 180, 219);
	lcdPrint(sOctSel, 180, 255);
}

void repaintMode1(int newOctSel) {
	setColor(255, 87, 51);
	setColorBg(230, 230, 230);

	char sOctSel[2];

	if(newOctSel == 10)
			sprintf(sOctSel, "A", newOctSel);
	else {
		sprintf(sOctSel, "%d", newOctSel);
	}

	lcdPrint(sOctSel, 180, 255);
}

void paintBgMode2() {
	setColor(255, 255, 255);
	fillRect(10, 125, 230, 155);
	setColor(230, 230, 230);
	fillRect(10, 155, 230, 227);

	setColor(255, 87, 51);
	setColorBg(255, 255, 255);
	lcdPrint("Tune A4", 65, 133);

	setColorBg(230, 230, 230);
	lcdPrint("Current:", 25, 165);
	lcdPrint("Selected: ", 25, 201);

}

void hideMode2() {
	setColor(200, 200, 200);
	fillRect(10, 125, 230, 227);
}

void paintMode2(int a4) {
	setColor(255, 87, 51);
	setColorBg(230, 230, 230);
	char sA4[4];
	sprintf(sA4, "%d", a4);
	lcdPrint(sA4, 175, 165);
	lcdPrint(sA4, 175, 201);
}

void repaintMode2(int newA4) {
	setColor(255, 87, 51);
	setColorBg(230, 230, 230);
	char sNewA4[4];
	sprintf(sNewA4, "%d", newA4);
	lcdPrint(sNewA4, 175, 201);
}

// 200 px wide in total, 100px for each +/- 50 cents
// meaning 2px per cent
// midway point = 120px horizontal
// bar is 20px tall (95-115px)
void drawError(int prevError, int currError) {
	if (currError >= 0 && prevError < 0) {
		setColor(200, 200, 200);
		fillRect(120 + 2 * prevError, 95, 120, 115);
	} else if (currError < 0 && prevError >= 0) {
		setColor(200, 200, 200);
		fillRect(120, 95, 120 + 2 * prevError, 115);
	}

	if (currError <= 15 && currError >= -15)
		setColor(0, 255, 0); // green
	else
		setColor(255, 0, 0); // red

	if (currError >= 0) {
		//draw bar appropriate color
		fillRect(120, 95, 120 + 2 * currError, 115);

		if(currError < prevError) {
			//cover difference
			setColor(200, 200, 200);
			fillRect(120 + 2 * currError, 95, 120 + 2 * prevError, 115);
		}
	} else {
		// draw bar appropriate color
		fillRect(120 + 2 * currError, 95, 120, 115);

		if(currError > prevError) {
			//cover difference
			setColor(200, 200, 200);
			fillRect(120 + 2 * prevError, 95, 120 + 2 * currError, 115);
		}
	}
}


void drawTime(int progTime) {
	setColor(255, 87, 51);
	setColorBg(200, 200, 200);
	char sTime[5];
	sprintf(sTime, "%03d", progTime);
	lcdPrint(sTime, 156, 300);

}

void drawFreq(int freq) {
	setColor(255, 87, 51);
	setColorBg(200, 200, 200);
	char sFreq[6];
	sprintf(sFreq, "%05d", freq);
	lcdPrint(sFreq, 36, 300);
}
