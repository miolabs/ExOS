#include <usb/host.h>
#include <net/adapter.h>
#include <support/usb/driver/hid.h>
#include <support/usb/driver/usbkb.h>
#include "pseudokb.h"
#include <comm/comm.h>
#include <kernel/dispatch.h>
#include <kernel/thread.h>

#define THREAD_STACK 1024
static unsigned char _thread1_stack[THREAD_STACK] __attribute__((__aligned__(16)));
static unsigned char _thread2_stack[THREAD_STACK] __attribute__((__aligned__(16)));
static EXOS_THREAD _thread1;
static EXOS_THREAD _thread2;

static EXOS_DISPATCHER_CONTEXT _context;

#ifdef BOARD_E2468
// NOTE: hook called by net stack
void net_board_set_mac_address(NET_ADAPTER *adapter, int index)
{
        // MAC 1 (0:18:1b:5:1c:13)
	adapter->MAC = (HW_ADDR) { 0x00, 0x18, 0x1b, 0x05, 0x1c, 0x13 };
        // MAC 2 (0:1:38:38:a8:20)
	// adapter->MAC = (HW_ADDR) { 0x00, 0x1, 0x38, 0x38, 0xa8, 0x20 };
        // MAC 3 (0:14:bf:71:ac:88)
        //adapter->MAC = (HW_ADDR) { 0x00, 0x14, 0xbf, 0x71, 0xac, 0x88 };  
}
#endif

void *_service_thread(void *arg);

struct rfid_msg
{
	char *Data;
	int Length;
	EXOS_EVENT *Done;
};

void main()
{
	usb_host_initialize();

#ifdef BOARD_E2468
	NET_ADAPTER *adapter = NULL;
	if (net_adapter_enum(&adapter))
	{
		adapter->IP = (IP_ADDR) { 10, 0, 1, 10 };
	}
#endif

	exos_dispatcher_context_create(&_context);

	exos_thread_create(&_thread1, 1, _thread1_stack, sizeof(_thread1_stack), NULL, _service_thread, "dev/usbkb0");
	exos_thread_create(&_thread2, 1, _thread2_stack, sizeof(_thread2_stack), NULL, _service_thread, "dev/usbkb1");
}

void *_service_thread(void *arg)
{
	pseudokb_service((const char *)arg);
}

void _send_rfid(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
}

void pseudokb_handler(char *text, int length)
{
	EXOS_EVENT done;
	struct rfid_msg msg = (struct rfid_msg) { .Data = text, .Length = length, .Done = &done };
	exos_event_create(&done);
	
	EXOS_DISPATCHER dispatcher = (EXOS_DISPATCHER) { .Callback = _send_rfid, .CallbackState = &msg };
	exos_dispatcher_add(&_context, &dispatcher, 0);

	exos_event_wait(&done, EXOS_TIMEOUT_NEVER);
}

void usb_host_add_drivers()
{
	usbd_hid_initialize();
	usbkb_initialize();
}



