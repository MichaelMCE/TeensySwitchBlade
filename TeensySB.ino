
#include <Arduino.h>
#include <USBHost_t36.h>
#include "usbh_common.h"
#include "razersb.h"
#include "ibottom.c"



// Still WIP
// TODO: Target top display (keys)
// Add API interface to enable target swtiching from touchpad display & Keys display
// Add Touchpad Input as Mouse


#define println Serial.println

int doTests = 0;


int driver_callback (uint32_t msg, intptr_t *value1, uint32_t value2)
{
	//Serial.printf("driver_callback %i %i %i", (int)msg, value1, value2);
	//println("driver_callback");

	if (msg == USBD_MSG_DEVICEREADY){
		println("Device ready\r\n");
		doTests = 1;
	}
	return 1;
}

void setup ()
{
	Serial.begin(2000000);
	
	while (!Serial) ; // wait for Arduino Serial Monitor
	
	Serial.printf("RazerSB\r\n");
	
	usb_start();
	
	razersb_setCallbackFunc(driver_callback);
	delay(25);
}

void sendBitmapCentered (void *data, const int x, const int y, const int w, const int h)
{
	sbuiwriteop_t idesc;

	idesc.left = (DWIDTH - w) / 2;
	idesc.right = ((DWIDTH - w) / 2) + w-1;
	idesc.top = (DHEIGHT - h) / 2;
	idesc.bottom = ((DHEIGHT - h) / 2) + h-1;
	
	razersb_writeImage(&idesc, data);
}

void delayTask (const uint32_t period)
{
	for (uint32_t i = 0; i < period; i+= 5){
		usb_task();
		delay(5);
	}
}
/*
void printHex32 (const uint8_t *data, int length)
{
	for (int i = 0; i < length; i += 32){
		for (int j = i; j < i+32 && j < length; j++){
			printf("%2.2X ", data[j]);
		}
		printf("\r\n");
	}
	printf("\r\n");
}
*/

void sendBitmap (void *data, const int x, const int y, const int w, const int h)
{
	sbuiwriteop_t idesc;
	
	idesc.left = x;
	idesc.right = x + w-1;
	idesc.top = y;
	idesc.bottom = y + h-1;
	
	razersb_writeImage(&idesc, data);
}

const int ww = 800;
const int hh = 240;
uint32_t ct = 0;

#define len  (ww * hh * 2)
static uint8_t array8t[len];
static uint8_t DMAMEM array8b[len];


void loop ()
{
	usb_task();
	
	if (doTests){
		if (doTests == 1){
			doTests = 2;
			printf("%i %i %p %p\r\n", ww, hh, (void*)array8t, (void*)array8b);
		}
		
		if (++ct&0x01){
			memset(array8t, 0xFF, len);
			memset(array8b, 0xFF, len);
			arm_dcache_flush(array8b, len);
		}else{
			memset(array8t, 0x00, len);
			memset(array8b, 0x00, len);
			arm_dcache_flush(array8b, len);
		}

		sendBitmap(array8t, 0, 0, ww, hh);
		sendBitmap(array8b, 0, 240, ww, hh);

		delayTask(1000);

		printf("start .. \r\n");
		elapsedMillis em = 0;
	
		for (int i = 0; i < 100; i++)
			sendBitmapCentered((void*)ibottom, 0, 0, 800, 480);

		const elapsedMillis tend = em;
		printf("done, tend %.2f\r\n", (float)tend/100.0f);
		
		delayTask(1000);		

	}else{
		delay(30);
	}
}

