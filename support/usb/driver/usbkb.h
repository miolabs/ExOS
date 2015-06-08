#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <support/usb/driver/hid.h>
#include <kernel/tree.h>
#include <comm/comm.h>

#ifndef USB_KEYBOARD_MAX_INSTANCES
#define USB_KEYBOARD_MAX_INSTANCES 1
#endif

#define USB_KEYBOARD_IO_BUFFER 16

typedef enum
{
	USBKB_IO_NOT_MOUNTED = 0,
	USBKB_IO_CLOSED,
	USBKB_IO_OPENING,
	USBKB_IO_READY,
	USBKB_IO_ERROR,
} USBKB_IO_STATE;

typedef struct
{
	EXOS_TREE_DEVICE KernelDevice;
	COMM_IO_ENTRY *Entry;
    USBKB_IO_STATE State;
	EXOS_IO_BUFFER IOBuffer;
   	unsigned char Buffer[USB_KEYBOARD_IO_BUFFER];	
} USBKB_IO_HANDLE;

typedef enum
{
	USB_KEYBOARD_NOT_PRESENT = 0,
	USB_KEYBOARD_PRESENT,
	USB_KEYBOARD_REMOVED,
} USB_KEYBOARD_STATE;

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
	HID_FUNCTION_HANDLER;
	HID_REPORT_INPUT *Report0;
	HID_REPORT_INPUT *Report1;
	USB_KEYBOARD_STATE State;
	USBKB_IO_HANDLE *IOHandle;
    USBKB_MODIFIERS Modifiers;
} USB_KEYBOARD_HANDLER;


void usbkb_initialize();
void usbkb_push_text(USB_KEYBOARD_HANDLER *kb, char *text, int length);
void usbkb_translate(USB_KEYBOARD_HANDLER *kb, unsigned char key);

#endif // HID_KEYBOARD_H

