#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <support/usb/driver/hid.h>
#include <kernel/tree.h>
#include <kernel/io.h>
#include <kernel/iobuffer.h>

#ifndef USB_KEYBOARD_MAX_INSTANCES
#define USB_KEYBOARD_MAX_INSTANCES 1
#endif

#define USB_KEYBOARD_IO_BUFFER 16

typedef struct
{
	io_entry_t *Entry;
	io_buffer_t IOBuffer;
	unsigned char Buffer[USB_KEYBOARD_IO_BUFFER];	
} USBKB_IO_HANDLE;

typedef enum
{
	USBKB_MODIFIER_LEFT_CTRL = (1<<0),
	USBKB_MODIFIER_LEFT_SHIFT = (1<<1),
	USBKB_MODIFIER_LEFT_ALT = (1<<2),
	USBKB_MODIFIER_LEFT_GUI = (1<<3),
	USBKB_MODIFIER_RIGHT_CTRL = (1<<4),
	USBKB_MODIFIER_RIGHT_SHIFT = (1<<5),
	USBKB_MODIFIER_RIGHT_ALT = (1<<6),
	USBKB_MODIFIER_RIGHT_GUI = (1<<7),
} USBKB_MODIFIERS;

typedef struct
{
	hid_function_handler_t Hid;
	unsigned char InstanceIndex;
	unsigned char MaxReportId;
	enum { USB_KB_DETACHED, USB_KB_STARTING, USB_KB_READY, USB_KB_OPEN, USB_KB_ERROR } State;
	char DeviceName[8];
	io_tree_device_t Device;
	io_entry_t *IOEntry;
	io_buffer_t IOBuffer;
	unsigned char Buffer[USB_KEYBOARD_IO_BUFFER];	
 //   USBKB_MODIFIERS Modifiers;
} usb_kb_function_handler_t; 

void usbkb_initialize();
bool usbkb_set_report(hid_function_t *func, unsigned char report_id, unsigned char *data, unsigned length);

//void usbkb_push_text(usb_keyboard_handler_t *kb, char *text, int length);
//void usbkb_translate(usb_keyboard_handler_t *kb, unsigned char key);

#endif // HID_KEYBOARD_H

