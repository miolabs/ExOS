#include "pseudokb.h"
#include <support/usb/driver/usbkb.h>

void pseudokb_service(const char *device)
{
	char buffer_in[16];
	char buffer_out[16];
	int err = 0;
	while(1)
	{
		int offset = 0;
		
		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, device);
		if (dev_node != NULL)
		{
			COMM_IO_ENTRY comm;
			comm_io_create(&comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT); 
			int	err = comm_io_open(&comm);
			if (err == 0)
			{
				while(1)
				{
					int done = exos_io_read((EXOS_IO_ENTRY *)&comm, buffer_in, sizeof(buffer_in));
					if (done < 0) break;

					int i = 0;
					while(i < done &&
						offset < sizeof(buffer_out))
					{
						char a = buffer_in[i++];
						if (a == 13 || a == 10)
						{
							if (offset != 0)
							{
								buffer_out[offset] = '\0';
                                pseudokb_handler(comm.Port, buffer_out, offset);

								offset = 0;
							}
						}
						else if (a != '\0')
						{
							buffer_out[offset++] = a;
						}
					}
				}

				comm_io_close(&comm);
			}
			else exos_thread_sleep(500);
		}
		else exos_thread_sleep(500);
	}
}




void usbkb_translate(USB_KEYBOARD_HANDLER *kb, unsigned char key)
{
	// NOTE: dirty hack to map usb keys to ascii chars
	static const char keytable[] = "\0\0\0\0abcdefghijklmnopqrstuvwxyz1234567890\r\0\0\t -=[]\\\0;,',./\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	if (key < sizeof(keytable))
	usbkb_push_text(kb, (char *)&keytable[key], 1);
}
