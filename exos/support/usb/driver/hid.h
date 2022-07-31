#ifndef USB_DRIVER_HID_H
#define USB_DRIVER_HID_H

#include <usb/classes/hid.h>
#include <usb/host.h>
#include <kernel/dispatch.h>
#include <stdbool.h>

typedef struct __hid_function_handler hid_function_handler_t;

typedef struct
{
	usb_host_function_t;
	usb_host_pipe_t InputPipe;
	unsigned char InterfaceSubClass;
	unsigned char Protocol;
   	unsigned char Interface;

	hid_function_handler_t *Handler;

	unsigned char InputBuffer[64];
	unsigned char OutputBuffer[64];
	
	bool ExitFlag;
	unsigned char InstanceIndex;

	usb_request_buffer_t Request;
	dispatcher_t Dispatcher;
} hid_function_t;

typedef enum
{
	HID_PARSE_FOUND_END,
	HID_PARSE_FOUND_LOCAL,
	HID_PARSE_FOUND_INPUT,
	HID_PARSE_FOUND_OUTPUT,
} hid_parse_found_t;

typedef struct
{
	unsigned char InputOffset, NextInputOffset;
	unsigned char OutputOffset, NextOutputOffset;
	// global data
	unsigned char UsagePage;
	unsigned char ReportId;
	unsigned char ReportSize;
	unsigned char ReportCount;
	unsigned char Min;
	unsigned char Max;
	unsigned long InputFlags;
} hid_report_parser_global_state_t;

typedef struct
{
	unsigned char *Ptr;
	unsigned short Offset;
	unsigned short Length;
	hid_report_parser_global_state_t *Global;
} hid_report_parser_t;

typedef struct
{
	unsigned char Type;
	unsigned char Tag;
	unsigned char Length;
	union 
	{
		unsigned Value;
		unsigned char *Data;
	};
} hid_report_parser_item_t;

typedef struct 
{
	hid_function_handler_t *(*MatchDevice)(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
		hid_report_parser_t *parser);
	void (*Start)(hid_function_handler_t *handler);
	void (*Stop)(hid_function_handler_t *handler);
	void (*Notify)(hid_function_handler_t *handler, unsigned char report_id, unsigned char *data, unsigned length);
} hid_driver_t;

typedef struct
{
	node_t Node;
	const hid_driver_t *Driver;
} hid_driver_node_t;

struct __hid_function_handler
{
	node_t Node;
	hid_driver_node_t *DriverNode;
	hid_function_t *Function;
	unsigned char MaxReportId;
};

typedef struct
{
	unsigned char Size;
	unsigned char Offset;
} hid_report_field_t;


void usb_hid_initialize(dispatcher_context_t *context);
void usb_hid_add_driver(hid_driver_node_t *node);
bool usb_hid_parse_report_descriptor(hid_report_parser_t *parser, hid_report_parser_item_t *item, hid_parse_found_t *pfound);
bool usb_hid_parse_find_collection(hid_report_parser_t *parser, unsigned char collection_type);
void usb_hid_parse_local_item(const hid_report_parser_item_t *item, usb_hid_desktop_usage_t *pusage, usb_hid_desktop_usage_t *pusage_min, usb_hid_desktop_usage_t *pusage_max);
bool usb_hid_set_idle(hid_function_t *func, unsigned char report_id, unsigned char idle);
bool usb_hid_set_report(hid_function_t *func, unsigned char report_type, unsigned char report_id, void *data, int length);
unsigned usb_hid_read_field(unsigned bit_offset, unsigned bit_length, const unsigned char *report, unsigned char *data);

#endif // USB_DRIVER_HID_H

