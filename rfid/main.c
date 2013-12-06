#include <usb/host.h>
#include <net/adapter.h>
#include <support/usb/driver/hid.h>
#include "pseudokb.h"
#include <comm/comm.h>

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

unsigned char _buffer[128];

void main()
{
	usb_host_initialize();
	int err = 0;

#ifdef BOARD_E2468
	NET_ADAPTER *adapter = NULL;
	if (net_adapter_enum(&adapter))
	{
		adapter->IP = (IP_ADDR) { 10, 0, 1, 10 };
	}
#endif

	while(1)
	{
		int offset = 0;
		
		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, "dev/usbkb0");
		if (dev_node != NULL)
		{
			COMM_IO_ENTRY comm;
			comm_io_create(&comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT); 
			int	err = comm_io_open(&comm);
			if (err == 0)
			{
				while(1)
				{
					if (offset >= sizeof(_buffer))
						break;

					int done = exos_io_read((EXOS_IO_ENTRY *)&comm, _buffer + offset, sizeof(_buffer) - offset);
					if (done < 0) break;

					offset += done;
				}

				comm_io_close(&comm);
			}
			else exos_thread_sleep(500);
		}
		else exos_thread_sleep(500);
	}
}

void usb_host_add_drivers()
{
	usbd_hid_initialize();
	keyboard_initialize();
}

void keyboard_translate(HID_KEYBOARD_HANDLER *kb, unsigned char key)
{
	// NOTE: dirty hack to map usb keys to ascii chars
	static const char keytable[] = "\0\0\0\0abcdefghijklmnopqrstuvwxyz1234567890\r\0\0\t -=[]\\\0;,',./\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	if (key < sizeof(keytable))
	keyboard_push_text(kb, (char *)&keytable[key], 1);
}

