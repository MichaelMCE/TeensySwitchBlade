

#ifndef _RAZERSB_H_
#define _RAZERSB_H_


#include "usbh_common.h"



#define RAZERSB_VID		0x1532
#define RAZERSB_PID		0x0117

#define DWIDTH			800
#define DHEIGHT			480



// LFRM_BPP_16 - RGB 565
#define RGB_16_RED		0xF800
#define RGB_16_GREEN	0x07E0
#define RGB_16_BLUE		0x001F	
#define RGB_16_WHITE	(RGB_16_RED|RGB_16_GREEN|RGB_16_BLUE)
#define RGB_16_BLACK	0x0000
#define RGB_16_MAGENTA	(RGB_16_RED|RGB_16_BLUE)
#define RGB_16_YELLOW	(RGB_16_RED|RGB_16_GREEN)
#define RGB_16_CYAN		(RGB_16_GREEN|RGB_16_BLUE)


typedef struct {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
}area_t;


typedef struct {
	uint16_t op;
	uint16_t left;
	uint16_t top;
	uint16_t right;
	uint16_t bottom;
	uint16_t crc;
}sbuiwriteop_t;
	
	
class RazerSB : public USBDriver {
public:
	//enum { SYSEX_MAX_LEN = 60 };
	
	RazerSB (USBHost &host)
	{
		init();
	}
	RazerSB (USBHost *host)
	{
		init();
	}

	int (*callbackFunc) (uint32_t msg, intptr_t *value1, uint32_t value2);

	struct {
		int width;
		int height;
		int pitch;
	}display;
		
	bool usb_bulk_write (USBDriver *drv, Pipe_t *pipe, const void *buffer, const uint32_t len);
	bool usb_control_msg (Device_t *dev, uint32_t orequestType, uint32_t omsg, uint32_t ovalue, uint32_t oindex, void *buff, uint32_t len);
		
	int setBaseAddress (const uint32_t addr);
	int setFrameAddress (const uint32_t addr);
	int setWriteAddress (const uint32_t addr);
	int setConfigValue (const uint32_t cfg, const uint8_t value);
	int getConfigValue (const uint32_t cfg, uint8_t *value);
	int setBrightness (const uint8_t level);
	int setLineLength (uint32_t length);
	int disableStreamDecoder ();
	int enableStreamDecoder ();
	int setTouchMode (const uint32_t mode);
	int setConfigDefaults ();
	int getDeviceDetails (char *name, uint32_t *width, uint32_t *height, uint32_t *version, char *serial);
	int writeData (void *data, const size_t size);
	int drawScreenArea (uint8_t *__restrict__ buffer, const int32_t __restrict__ bwidth, const int32_t __restrict__ bheight, const int32_t __restrict__ dx, const int32_t __restrict__ dy, uint32_t frameAddrOffset = 0, int swap = 0);
	int fillScreen (const uint16_t colour);
	
	//int drawScreenArea2 (uint8_t *buffer, const int32_t bwidth, const int32_t bheight);
	//int drawScreenSetup ();
	//int ustate = 0;
	
	int drawScreenSetup (const int32_t bwidth, const int32_t bheight, const int32_t dx, const int32_t dy, uint32_t frameAddrOffset);
	
	


	int writeImage (sbuiwriteop_t *idesc, void *pixelData);
	
	
	tx_descript_t tx_writeCtx;
	

	
protected:

	virtual void driverReady ();

	virtual void Task ();
	virtual bool claim (Device_t *device, int type, const uint8_t *descriptors, uint32_t len);
	virtual void control (const Transfer_t *transfer);
	virtual void disconnect ();

	void init();

private:

	Device_t *device;
	Pipe_t *rxpipe;
	Pipe_t *txpipe;
	
	uint32_t start = 0;
	uint32_t lineLength = 0;

	enum {
		MAX_PACKET_SIZE = 512,
		MAX_TX_PACKET_SIZE = 512, 
		MAX_RX_PACKET_SIZE = 256
	};
	
	enum { RX_QUEUE_SIZE = 256 }; // must be more than MAX_PACKET_SIZE/4

    uint8_t tx_buffer[MAX_TX_PACKET_SIZE];
    uint8_t rx_buffer[MAX_RX_PACKET_SIZE];
    uint8_t rx_queue[RX_QUEUE_SIZE];

	uint16_t tx_size;
	uint16_t rx_size;
	uint8_t tx_ep;
	uint8_t rx_ep;			

	Pipe_t mypipes[10] __attribute__ ((aligned(32)));
	
	Transfer_t mytransfers[128] __attribute__ ((aligned(32)));
	
	static void rx_callback_keys (const Transfer_t *transfer);
	static void rx_callback_touch0 (const Transfer_t *transfer);
	static void rx_callback_touch (const Transfer_t *transfer);
};


void usb_start ();
void usb_task ();
void razersb_setCallbackFunc (int (*func)(uint32_t msg, intptr_t *value1, uint32_t value2));
int razersb_writeImage (sbuiwriteop_t *idesc, void *pixelData);

#endif

