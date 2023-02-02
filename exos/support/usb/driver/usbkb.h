#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <support/usb/driver/hid.h>
#include <kernel/tree.h>

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
} usbkb_modifier_t;

typedef struct
{
	usbkb_modifier_t Modifiers;
	unsigned char Keys[16];
} usbkb_report_t;

typedef struct
{
	hid_function_handler_t Hid;
	unsigned char InstanceIndex;
	unsigned char MaxReportId;
//	enum { USB_KB_DETACHED, USB_KB_STARTING, USB_KB_READY, USB_KB_OPEN, USB_KB_ERROR } State;
	usbkb_report_t Report;
} usbkb_function_handler_t; 

void usbkb_initialize();
bool usbkb_set_report(hid_function_t *func, unsigned char report_id, unsigned char *data, unsigned length);

//void usbkb_push_text(usb_keyboard_handler_t *kb, char *text, int length);
void usbkb_translate(usbkb_function_handler_t *kb, usbkb_modifier_t mask, unsigned char *keys, unsigned char length);

#endif // HID_KEYBOARD_H

