/*
 * encoder.c
 *
 * Created: 6/27/2020 12:48:35 AM
 * Author : asara
 */ 

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
//#include <avr/pgmspace.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "oddebug.h"


static uchar    reportBuffer[2];  
#ifndef NULL
#define NULL    ((void *)0)
#endif
//static uchar    idleRate;           /* in 4 ms units */

struct encoder {
	uchar button;
	uchar Left1;
	uchar Right1;
	uchar Left2;
	uchar Right2;
	uchar Q1c0_pin;
	uchar Q1c1_pin;
	uchar Q2c0_pin;
	uchar Q2c1_pin;
	uchar button_pin;
	uchar Q1c0_state;
	uchar Q1c1_state;
	uchar Q2c0_state;
	uchar Q2c1_state;
};




const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = { /* USB report descriptor */
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,                    // USAGE (Keyboard)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x95, 0x08,                    //   REPORT_COUNT (8)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
	0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
	0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
	0xc0                           // END_COLLECTION
};

static void buildReport(struct encoder *e1, struct encoder *e2)
{
uchar key = 0;  /* 0 means no key is sent */
uchar mod = 0;  /* Modifier 0x02 will make upper case */

if (e1->Left1 == 1){
	key = 0x04;			/* Encoder 1: main knob, left click, send the A key */
	mod = 0x02;
	e1->Left1 = 0;}
	
if (e1->Right1 == 1){   /* Encoder 1: main knob, right click, send the B key */
	key = 0x05;
	mod = 0x02;
	e1->Right1 = 0;}
	
if (e1->Left2 == 1){
	key = 0x07;			/* Encoder 1: secondary knob, left click, send the D key */
	mod = 0x02;
	e1->Left2 = 0;}
	
if (e1->Right2 == 1){   /* Encoder 1: secondary knob, right click, send the E key */
	key = 0x08;
	mod = 0x02;
	e1->Right2 = 0;}
		
if  (e1->button == 1){	/* Encoder 1: button push, send the C key */
	key = 0x06;	
	mod = 0x02;
	e1->button = 0;}
	
	////
	
if (e2->Left1 == 1){
	key = 0x04;			/* Encoder 2: main knob, left click, send the a key */
	e2->Left1 = 0;}

if (e2->Right1 == 1){   /* Encoder 2: main knob, right click, send the b key */
	key = 0x05;
	e2->Right1 = 0;}

if (e2->Left2 == 1){
	key = 0x07;			/* Encoder 2: secondary knob, left click, send the d key */
	e2->Left2 = 0;}

if (e2->Right2 == 1){   /* Encoder 2: secondary knob, right click, send the e key */
	key = 0x08;
	e2->Right2 = 0;}

if  (e2->button == 1){  /* Encoder 2: button push, send the c key */
	key = 0x06;
	e2->button = 0;}

	reportBuffer[0] = mod;    /* modifiers */
	reportBuffer[1] = key;
}

uchar	usbFunctionSetup(uchar data[8])
{
	//usbRequest_t    *rq = (void *)data;

	//usbMsgPtr = reportBuffer;
	//if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
		//if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
			///* we only have one report type, so don't look at wValue */
			//buildReport();
			//return sizeof(reportBuffer);
			//}else if(rq->bRequest == USBRQ_HID_GET_IDLE){
			//usbMsgPtr = &idleRate;
			//return 1;
			//}else if(rq->bRequest == USBRQ_HID_SET_IDLE){
			//idleRate = rq->wValue.bytes[1];
		//}
		//}else{
		///* no vendor specific requests implemented */
	//}
	return 0;
}

void    usbEventResetReady(void)
{
    /* Disable interrupts during oscillator calibration since
     * usbMeasureFrameLength() counts CPU cycles.
     */
   // cli();
   // sei();
   // eeprom_write_byte(0, OSCCAL);   /* store the calibrated value in EEPROM */
}

uchar getinput(uchar AorB, uchar pin){
	uchar tmp;
	
	if (AorB == 'A'){
		tmp = PINA & (1 << pin);
	}
	else{
		tmp = PINB & (1 << pin);
	}
	return (tmp);
}


uchar LeftTurn(struct encoder *en, uchar Q1orQ2, uchar AorB){
	uchar tmp;
	unsigned int delay = 1; /* A 1ms delay is used to allow for switch debounce */
	uchar ret = 0;
	uchar pin0, pin1;
	uchar *state0, *state1;	
	
	if (Q1orQ2 == '1'){
		pin0 = en->Q1c0_pin;
		pin1 = en->Q1c1_pin;
		state0 = &(en->Q1c0_state);
		state1 = &(en->Q1c1_state);
	}
	else{
		pin0 = en->Q2c0_pin;
		pin1 = en->Q2c1_pin;
		state0 = &(en->Q2c0_state);
		state1 = &(en->Q2c1_state);
	}

	tmp = getinput(AorB,pin0);   /* Check Connection 0 */
	if (tmp != *state0){ /* If Connection 0 has changed state */
		_delay_ms(delay);   /* Wait for the debounce time */
		tmp = getinput(AorB,pin0);  /* Recheck the Connection 0 state */
		if (*state0 != tmp){ /* If the changed state of Connection 0 is still valid, we can move on to Connection 1 */
			*state0 = tmp; /* Update the state of Connection 0 */
			tmp = getinput(AorB,pin1); /* Check the state of Connection 1 */
			while (tmp == *state1){  /* Wait for Connection 1 to change state */
				tmp = getinput(AorB,pin1);}
				*state1 = tmp;             /* Update the state of Connection 1 */
				ret = 1;}}	/* Now we can confirm one click to the right */
	return (ret);}
	
	
uchar RightTurn(struct encoder *en, uchar Q1orQ2, uchar AorB){
	uchar tmp;
	unsigned int delay = 1; /* A 1ms delay is used to allow for switch debounce */
	uchar ret = 0;
	uchar pin0, pin1;
	uchar *state0, *state1;
	
	if (Q1orQ2 == '1'){
		pin0 = en->Q1c0_pin;
		pin1 = en->Q1c1_pin;
		state0 = &(en->Q1c0_state);
		state1 = &(en->Q1c1_state);
	}
	else{
		pin0 = en->Q2c0_pin;
		pin1 = en->Q2c1_pin;
		state0 = &(en->Q2c0_state);
		state1 = &(en->Q2c1_state);}
	
	tmp = getinput(AorB,pin1); /* Repeat everything in the reverse order - check Connection 1 followed by Connection 0 */
	if (tmp != *state1){
		_delay_ms(delay);
		tmp = getinput(AorB,pin1);
		if (*state1 != tmp){
			*state1 = tmp;
			tmp = getinput(AorB,pin0);
			while (tmp == *state0){
				tmp = getinput(AorB,pin0);}
				*state0 = tmp;
				ret = 1;}}
	return (ret);}
	
uchar PushButton(struct encoder *en, uchar AorB){
	uchar tmp;
	unsigned int delay = 5; /* A 5ms delay is used to allow for switch debounce */
	uchar ret = 0;
	tmp = getinput(AorB,en->button_pin);
	if (tmp == (1 << en->button_pin)){       /* If button has been pushed, its state will be 1 */
		_delay_ms(delay);				  	 /* Debounce wait */
		tmp = getinput(AorB,en->button_pin);  /* Recheck after the debounce delay */
		if (tmp == (1 << en->button_pin)){ /* If the push button state is maintained */
			ret = 1;}}
	return (ret);}


int main(void)
{
	/* Fuse bits LOW.CKDIV8 should be unset*/
	/* Fuse bits CKSEL(0:3) should be set for 64 MHz */
	
	struct encoder e1, e2;
	OSCCAL = 150;		/* This value is experimentally determined to give slightly higher than 16.5 MHz clock rate */
	e1.button_pin = PB1;  /* Encoder 1 Push button pin */
	e1.Q1c0_pin = PB2;   /* Encoder 1 Connection 0 pin */
	e1.Q1c1_pin = PB3;   /* Encoder 1 Connection 1 pin */
	e1.Q2c0_pin = PB4;
	e1.Q2c1_pin = PB5;
	
	e2.button_pin = PA0; /* Encoder 2 */
	e2.Q1c0_pin = PA1;
	e2.Q1c1_pin = PA2;
	e2.Q2c0_pin = PA3;
	e2.Q2c1_pin = PA4;
	

	uchar i;
	//odDebugInit();
	usbDeviceDisconnect();
	for(i=0;i<20;i++){  /* 300 ms disconnect */
		_delay_ms(15);
	}
	usbDeviceConnect();
	usbInit();
	sei();

	e1.Q1c0_state = PINB & (1 << e1.Q1c0_pin);  /* Encoder 1, Knob A, Connection 0 */
	e1.Q1c1_state = PINB & (1 << e1.Q1c1_pin);  /* Encoder 1, Knob A, Connection 1 */
	e1.Q2c0_state = PINB & (1 << e1.Q2c0_pin);  /* Encoder 1, Knob B, Connection 0 */
	e1.Q2c1_state = PINB & (1 << e1.Q2c1_pin);  /* Encoder 1, Knob B, Connection 1 */
	
	e2.Q1c0_state = PINA & (1 << e2.Q1c0_pin);  /* Encoder 2, Knob A, Connection 0 */
	e2.Q1c1_state = PINA & (1 << e2.Q1c1_pin);  /* Encoder 2, Knob A, Connection 1 */
	e2.Q2c0_state = PINA & (1 << e2.Q2c0_pin);  /* Encoder 2, Knob B, Connection 0 */
	e2.Q2c1_state = PINA & (1 << e2.Q2c1_pin);  /* Encoder 2, Knob B, Connection 1 */
	
	
    while (1) {			/* Look for a change in Connection 0, followed by a change in Connection 1 */
		if (LeftTurn(&e1,'1','B') == 1){e1.Left1 = 1; e1.Right1 = 0;}
		if (RightTurn(&e1,'1','B') == 1){e1.Left1 = 0; e1.Right1 = 1;}
		if (LeftTurn(&e1,'2','B') == 1){e1.Left2 = 1; e1.Right2 = 0;}	
		if (RightTurn(&e1,'2','B') == 1){e1.Left2 = 0; e1.Right2 = 1;}	
		if (PushButton(&e1,'B') == 1){e1.button = 1;}
						/* Do the same for Encoder 2 */
		if (LeftTurn(&e2,'1','A') == 1){e2.Left1 = 1; e2.Right1 = 0;}
		if (RightTurn(&e2,'1','A') == 1){e2.Left1 = 0; e2.Right1 = 1;}
		if (LeftTurn(&e2,'2','A') == 1){e2.Left2 = 1; e2.Right2 = 0;}
		if (RightTurn(&e2,'2','A') == 1){e2.Left2 = 0; e2.Right2 = 1;}
		if (PushButton(&e2,'A') == 1){e2.button = 1;}
	
	usbPoll();
	if(usbInterruptIsReady()){ /* we can send another key */
		buildReport(&e1,&e2);
		usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
}}}
