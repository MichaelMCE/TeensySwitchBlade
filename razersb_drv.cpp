

//#include <Arduino.h>
#include <USBHost_t36.h>
#include "usbh_common.h"
#include "razersb.h"


static USBHost myusb;
static USBHub hub1(myusb);
static RazerSB razersb(myusb);



// #define print   Serial.println
#define println Serial.println


typedef struct _sb {
	Device_t *device;

	Pipe_t *txpipe;
	Pipe_t *rxpipe;
		
	uint16_t tx_size;		// wMaxPacketSize
	uint16_t rx_size;		// wMaxPacketSize
	
	uint8_t tx_ep;
	uint8_t rx_ep;
	
	uint8_t tx_ep_adr;
	uint8_t rx_ep_adr;
	
	int32_t pipeType;
	int32_t pipeDir;
	
	uint8_t rxpacket[4];
	
	USBDriver *driver;
}razerdev_t;

static razerdev_t razerdev[10];


#define LIBSBUI_EP_DISPLAY_PAD			0x01
#define LIBSBUI_EP_DISPLAY_KEYS			0x02
#define LIBSBUI_EP_IN_TOUCH				0x81
#define LIBSBUI_EP_IN_KEYS				0x82

#define RAZERSB_INT_0					0
#define RAZERSB_INT_DISPLAY_PAD			1
#define RAZERSB_INT_DISPLAY_KEYS		2
#define RAZERSB_INT_INPUT_TOUCH			3
#define RAZERSB_INT_INPUT_KEYS			4




volatile static int klen = 0;
volatile static int tlen0 = 0;
volatile static int tlen = 0;





void usb_start ()
{
	myusb.begin();
}

void usb_task ()
{
	myusb.Task();
}

void razersb_setCallbackFunc (int (*func)(uint32_t msg, intptr_t *value1, uint32_t value2))
{
	razersb.callbackFunc = func;
}


void RazerSB::init ()
{
	lineLength = 0;
	start = 0;
	callbackFunc = NULL;
	
	//contribute_Pipes(mypipes, sizeof(mypipes)/sizeof(Pipe_t));
	contribute_Transfers(mytransfers, sizeof(mytransfers)/sizeof(Transfer_t));
	
	driver_ready_for_device(this);
}

void RazerSB::Task ()
{
	//println("RazerSB Task:  enum_state = ", device->enum_state);
	//Serial.printf("RazerSB Task:  enum_state %d\r\n", device->enum_state);
	
	if (device->enum_state == USBH_ENUMSTATE_END){	// 15
		if (!start){
			start = millis();
		}else if (millis() - start > 2500){	// display startup time
			device->enum_state++;
			driverReady();
		}
	}
}

void RazerSB::driverReady ()
{
	// println("RazerSB driverReady  = ", (uint32_t)this, HEX);
	
	if (callbackFunc)
		(*callbackFunc)(USBD_MSG_DEVICEREADY, NULL, 0);
}


// type 0: claim complete device
// type 1: claim per interface
bool RazerSB::claim (Device_t *dev, int type, const uint8_t *descriptors, uint32_t len)
{
	// only claim at interface level
	if (type != 1) return false;

	if (dev->idVendor != RAZERSB_VID || dev->idProduct != RAZERSB_PID){
		//println("  device is not a RazerSB");
		return false;
	}else{
		//println("  found a RazerSB");
	}

	device = dev;

	//Serial.printf("\r\n\r\n\r\n###############\r\nRazerSB claim this %p\r\n", (uint32_t)this);
	
	//Serial.printf("claimType = %d\r\n", type);
	//Serial.printf("idVendor = %X\r\n" , dev->idVendor);
	//Serial.printf("idProduct = %X\r\n" , dev->idProduct);


	const uint8_t *p = descriptors;
	const uint8_t *end = p + len;

	// http://www.beyondlogic.org/usbnutshell/usb5.shtml
	int descriptorLength = p[0];
	int descriptorType = p[1];
	
	//Serial.printf("descriptorType = %d\r\n", descriptorType);
	//Serial.printf("descriptorLength = %d\r\n",  descriptorLength);
	
	if (!descriptorLength){
		//Serial.printf("3\r\n");
		return false;
	}
	if (descriptorType != USBH_DESCRIPTORTYPE_INTERFACE /*|| descriptorLength != 9*/){
		//Serial.printf("2\r\n");
		return false;
	}

	//descriptor_interface_t *interface = (descriptor_interface_t*)&p[0];

	//Serial.printf("bInterfaceClass = %d\r\n", interface->bInterfaceClass);
	//Serial.printf("bInterfaceSubClass = %d\r\n",  interface->bInterfaceSubClass);
	
	//if (interface->bInterfaceClass != USBH_DEVICECLASS_VENDOR || interface->bInterfaceSubClass != 0)
	//	return false;

	//println("  Interface is RazerSB");
	
	p += descriptorLength;	// == sizeof(descriptor_interface_t)
	rx_ep = 0;
	tx_ep = 0;
	//int interfaceCt = 0;

	while (p < end){
		int interfaceLength = p[0];
		if (p + interfaceLength > end){
			//Serial.printf("1\r\n");
			return false; // reject if beyond end of data
		}

		int interfaceType = p[1];
		//Serial.printf(":: interfaceType = %d\r\n", interfaceType);

		if (interfaceType == USBH_DESCRIPTORTYPE_ENDPOINT){
	
			println(" ");
			//Serial.printf("interface number : %d\r\n", interfaceCt++);
			//Serial.printf("interfaceType = %d\r\n", interfaceType);
			//Serial.printf("interfaceLength = %d\r\n", interfaceLength);
			descriptor_endpoint_t *endpoint = (descriptor_endpoint_t*)&p[0];
			
			//Serial.printf("bEndpointAddress = %X\r\n", endpoint->bEndpointAddress);
			//Serial.printf("bmAttributes = %X\r\n", endpoint->bmAttributes);
			//Serial.printf("wMaxPacketSize = %d\r\n", endpoint->wMaxPacketSize);
			//Serial.printf("bInterval = %d\r\n", endpoint->bInterval);

			//const int pipeType = endpoint->bmAttributes&0x03;
			//const int pipeDir = (endpoint->bEndpointAddress&0x80) >> 7;
			
			// type: 0 = Control, 1 = Isochronous, 2 = Bulk, 3 = Interrupt
			//Serial.printf("  endpoint type : %d\r\n", pipeType);
			//Serial.printf("  endpoint dir  : %d\r\n", pipeDir);
			//Serial.printf("  endpoint addr : %X\r\n", endpoint->bEndpointAddress/*&0x0F*/);

			if (endpoint->bEndpointAddress == LIBSBUI_EP_DISPLAY_PAD && endpoint->wMaxPacketSize == 512){
				razerdev_t *rdev = &razerdev[RAZERSB_INT_DISPLAY_PAD];
				
				rdev->device = dev;
				rdev->tx_ep = endpoint->bEndpointAddress&0x0F;
				rdev->tx_size = endpoint->wMaxPacketSize;
				rdev->pipeType = endpoint->bmAttributes&0x03;
				rdev->pipeDir = (endpoint->bEndpointAddress&0x80) >> 7;
				
				if (!rdev->txpipe)
					rdev->txpipe = new_Pipe(rdev->device, rdev->pipeType, rdev->tx_ep, rdev->pipeDir, rdev->tx_size);
				
				//Serial.printf("###  pad rdev->txpipe = %p\r\n", rdev->txpipe);
				
				rdev->driver = this;
				
			}else if (endpoint->bEndpointAddress == LIBSBUI_EP_DISPLAY_KEYS && endpoint->wMaxPacketSize == 512){
				razerdev_t *rdev = &razerdev[RAZERSB_INT_DISPLAY_KEYS];
				
				rdev->device = dev;
				rdev->tx_ep = endpoint->bEndpointAddress&0x0F;
				rdev->tx_size = endpoint->wMaxPacketSize;
				rdev->pipeType = endpoint->bmAttributes&0x03;
				rdev->pipeDir = (endpoint->bEndpointAddress&0x80) >> 7;
				
				if (!rdev->txpipe)
					rdev->txpipe = new_Pipe(rdev->device, rdev->pipeType, rdev->tx_ep, rdev->pipeDir, rdev->tx_size);
					
				//Serial.printf("###  keys rdev->txpipe = %p\r\n", rdev->txpipe);
				rdev->driver = this;
				
			}
#if 0
			else if (endpoint->bEndpointAddress == LIBSBUI_EP_IN_TOUCH && endpoint->wMaxPacketSize == 64){
				razerdev_t *rdev = &razerdev[RAZERSB_INT_INPUT_TOUCH];
				rdev->device = dev;
				rdev->rx_ep = endpoint->bEndpointAddress&0x0F;
				rdev->rx_size = endpoint->wMaxPacketSize;
				rdev->pipeType = endpoint->bmAttributes&0x03;
				rdev->pipeDir = (endpoint->bEndpointAddress&0x80) >> 7;
				
				rdev->rxpipe = new_Pipe(rdev->device, rdev->pipeType, rdev->rx_ep, rdev->pipeDir, rdev->rx_size, endpoint->bInterval);
				if (rdev->rxpipe){
					rdev->rxpipe->callback_function = rx_callback_touch;
					queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, rdev->rx_size, this);
					rdev->driver = this;
				}
				
			}else if (endpoint->bEndpointAddress == LIBSBUI_EP_IN_KEYS){
				razerdev_t *rdev = &razerdev[RAZERSB_INT_INPUT_KEYS];
				
				rdev->device = dev;
				rdev->rx_ep = endpoint->bEndpointAddress&0x0F;
				rdev->rx_size = endpoint->wMaxPacketSize;
				rdev->pipeType = endpoint->bmAttributes&0x03;
				rdev->pipeDir = (endpoint->bEndpointAddress&0x80) >> 7;
				
				rdev->rxpipe = new_Pipe(rdev->device, rdev->pipeType, rdev->rx_ep, rdev->pipeDir, rdev->rx_size, endpoint->bInterval);
				if (rdev->rxpipe){
					rdev->rxpipe->callback_function = rx_callback_keys;
					//queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, rdev->rx_size, this);
					rdev->driver = this;
				}

			}else if (endpoint->bEndpointAddress == LIBSBUI_EP_IN_TOUCH && endpoint->wMaxPacketSize == 8){
				razerdev_t *rdev = &razerdev[RAZERSB_INT_0];
				
				rdev->device = dev;
				rdev->rx_ep = endpoint->bEndpointAddress&0x0F;
				rdev->rx_size = endpoint->wMaxPacketSize;
				rdev->pipeType = endpoint->bmAttributes&0x03;
				rdev->pipeDir = (endpoint->bEndpointAddress&0x80) >> 7;
				
				rdev->rxpipe = new_Pipe(rdev->device, rdev->pipeType, rdev->rx_ep, rdev->pipeDir, rdev->rx_size, endpoint->bInterval);
				if (rdev->rxpipe){
					rdev->rxpipe->callback_function = rx_callback_touch0;
					queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, rdev->rx_size, this);
					
					rdev->driver = this;
				}

			}else if (0){
				razerdev_t *rdev = &razerdev[interfaceCt];
				
				rdev->device = dev;
				rdev->rx_ep = endpoint->bEndpointAddress&0x0F;
				rdev->rx_size = endpoint->wMaxPacketSize;
				rdev->pipeType = endpoint->bmAttributes&0x03;
				rdev->pipeDir = (endpoint->bEndpointAddress&0x80) >> 7;
				
				rdev->rxpipe = new_Pipe(rdev->device, rdev->pipeType, rdev->rx_ep, rdev->pipeDir, rdev->rx_size, endpoint->bInterval);
				if (rdev->rxpipe){
					rdev->rxpipe->callback_function = rx_callback_touch;
					queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, rdev->rx_size, this);
					
					rdev->driver = this;
				}
			
		}
#endif
		}

		p += interfaceLength;
	}

	//Serial.printf("  endpoint txpipe : %d\r\n", (uint32_t)txpipe);
	//Serial.printf("  endpoint rxpipe  : %d\r\n", (uint32_t)rxpipe);
	//Serial.printf("***********************************************\r\n");

	return true;//(rxpipe || txpipe);
}


#if 0
static int tx_state = 0;


void RazerSB::tx_callback (const Transfer_t *transfer)
{
	//// println("RazerSB tx_callback");
	
	if (transfer->driver){
		if (tx_state == -1){
			//// println("RazerSB tx_callback tx_state ", tx_state);
			((RazerSB*)(transfer->driver))->drawScreenSetup();

		}else if (tx_state == 1){
			tx_descript_t *context = &((RazerSB*)(transfer->driver))->tx_writeCtx;
				
			int ret = (*((RazerSB*)(transfer->driver))->callbackFunc)(USBD_MSG_WRITEREADY, (intptr_t*)context, 0);
			if (ret){
				//printf("tx_callback %i %i\r\n", (int)context->frame, (int)context->row);
				
				((RazerSB*)(transfer->driver))->writeData(context->buffer, context->len);
				context->row += context->rows;
			}else{
				tx_state = 0;
				(*((RazerSB*)(transfer->driver))->callbackFunc)(USBD_MSG_WRITEEND, (intptr_t*)context, 0);
			}
		}
	}
}

#endif

/*
void dump (char *buffer, int len)
{
	for (int j = 0; j < len; j++)
		Serial.printf("%.2x ", buffer[j]);
	Serial.printf("\r\n");
}
*/

#if 0

void RazerSB::rx_callback_touch0 (const Transfer_t *transfer)
{
	//println("RazerSB rx_callback_touch0");
	
	if (transfer->driver){
		
		//((RazerSB *)(transfer->driver))->new_data(transfer);
		int32_t len = transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF);
		
		Serial.printf("rx_callback_touch0 %d\r\n", len);
		//dump((char*)transfer->buffer, len);
		razerdev_t *rdev = &razerdev[RAZERSB_INT_0];
		queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, len, rdev->driver);
					
		if (len < 1 || len > 8){
			//razerdev_t *rdev = &razerdev[RAZERSB_INT_0];
			//queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, 8, rdev->driver);
			tlen0 = 0;
		}else{
			tlen0 = len;
		}
	}
}

void RazerSB::rx_callback_touch (const Transfer_t *transfer)
{
	//println("RazerSB rx_callback_touch");
	
	if (transfer->driver){
		
		//((RazerSB *)(transfer->driver))->new_data(transfer);
		int32_t len = transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF);
		
		Serial.printf("rx_callback_touch %d\r\n", len);
		//dump((char*)transfer->buffer, len);
		razerdev_t *rdev = &razerdev[RAZERSB_INT_INPUT_TOUCH];
		queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, len, rdev->driver);
		
		if (len < 1 || len > 64){
			//razerdev_t *rdev = &razerdev[RAZERSB_INT_INPUT_TOUCH];
			//queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, 64, rdev->driver);
			tlen = 0;
		}else{
			tlen = len;
		}
	}
}

void RazerSB::rx_callback_keys (const Transfer_t *transfer)
{
	//println("RazerSB rx_callback_keys");
	
	if (transfer->driver){
		//((RazerSB *)(transfer->driver))->new_data(transfer);
		
		int32_t len = transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF);
		Serial.printf("rx_callback_keys %d\r\n", len);
		//dump((char*)transfer->buffer, len);
		
		razerdev_t *rdev = &razerdev[RAZERSB_INT_INPUT_KEYS];
		queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, len, rdev->driver);

		if (len < 1 || len > 16){
			//razerdev_t *rdev = &razerdev[RAZERSB_INT_INPUT_KEYS];
			//queue_Data_Transfer(rdev->rxpipe, rdev->rxpacket, 16, rdev->driver);
			klen = 0;
		}else{
			klen = len;
		}
	}
}
#endif

void RazerSB::control (const Transfer_t *transfer)
{
	// println("RazerSB control()");
}

void RazerSB::disconnect ()
{
	// println("RazerSB disconnect()");
	
	// TODO: free resources
	device = NULL;
	start = 0;
	lineLength = 0;
}

bool RazerSB::usb_control_msg (Device_t *dev, uint32_t orequestType, uint32_t omsg, uint32_t ovalue, uint32_t oindex, void *buff, uint32_t len)
{
	setup_t setup;
	mk_setup(setup, orequestType, omsg, ovalue, oindex, len);
	return queue_Control_Transfer(dev, &setup, buff, NULL);
}

bool RazerSB::usb_bulk_write (USBDriver *drv, Pipe_t *pipe, const void *buffer, const uint32_t len)
{
	//printf("usb_bulk_write %i\r\n", len);
	return queue_Data_Transfer(pipe, (void*)buffer, len, drv);
}

int RazerSB::writeData (void *data, const size_t size)
{
	
	razerdev_t *rdev = &razerdev[RAZERSB_INT_DISPLAY_PAD];
	
	//__disable_irq();
	//NVIC_DISABLE_IRQ(IRQ_USBHS);
	int ret = usb_bulk_write(this, rdev->txpipe, data, size);
	//NVIC_ENABLE_IRQ(IRQ_USBHS);
	//__enable_irq();
	return ret;
}

volatile uint16_t swap16 (const uint16_t src)
{
	volatile unsigned char *tmp = (unsigned char*)&src;
	return (tmp[0] << 8 | tmp[1]);
}

int razersb_writeImage (sbuiwriteop_t *idesc, void *pixelData)
{
	return razersb.writeImage(idesc, pixelData);
}

// todo: int RazerSB::writeImage (sbuiwriteop_t *idesc, void *pixelData, flags)
int RazerSB::writeImage (sbuiwriteop_t *idesc, void *pixelData)
{
	uint16_t header[6];
	header[0] = swap16(1);
	header[1] = swap16(idesc->left);
	header[2] = swap16(idesc->top);
	header[3] = swap16(idesc->right);
	header[4] = swap16(idesc->bottom);
	header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

	if (writeData((void*)header, sizeof(header))){
		delayMicroseconds(500);
		
		sbuiwriteop_t r = *idesc;
		const int len = ((r.right - r.left)+1) * ((r.bottom - r.top)+1) * 2;
		int ret = writeData((uint8_t*)pixelData, len);
		
		// ISV requires at least 18ms to complete the write otherwise jittering will occur
		// if your design does not allow successive calls within this period, then this delay may not be necessary 
		delay(19);		

		arm_dcache_flush((void*)pixelData, len);	// shouldn't be here
		return (ret == len);
	}
	return 0;
}

