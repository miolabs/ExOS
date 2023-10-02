#ifndef STM32F4_USB_OTG_HS_HOST_CORE_H
#define STM32F4_USB_OTG_HS_HOST_CORE_H

#include <usb/host.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/dispatch.h>
#include <kernel/event.h>

typedef enum
{
	PID_DATA0 = 0,
	PID_DATA2, PID_DATA1, PID_SETUP,
} stm32_usbh_pid_t;

typedef struct
{
	usb_request_buffer_t *Request;
	unsigned char LastPacketLength;
} stm32_usbh_xfer_t;

typedef struct
{
	uint8_t Index;
	uint8_t EndpointNumber;
	usb_transfer_type_t EndpointType;
    usb_direction_t Direction;
	stm32_usbh_pid_t Pid;
	unsigned char Toggle;
	unsigned char Period;
	stm32_usbh_xfer_t Current;
} stm32_usbh_channel_t;

typedef enum
{
	STM32_EP_STA_IDLE = 0,
    STM32_EP_STA_BUSY,
	STM32_EP_STA_STOPPING,
} stm32_usbh_ep_state_t;

typedef struct
{
	node_t Node;	// for pooling
	stm32_usbh_channel_t *Tx;
	stm32_usbh_channel_t *Rx;
	stm32_usbh_ep_state_t Status;
} stm32_usbh_ep_t;

typedef enum
{
	STM32_USBERR_OK = 0,
	STM32_USBERR_CANCEL,
	STM32_USBERR_TXERR,
	STM32_USBERR_STALL,
	STM32_USBERR_HALTED,
} stm32_usbh_error_t;

void usb_hs_host_initialize(usb_host_controller_t *hc, dispatcher_context_t *context);
bool usb_hs_request_role_switch(usb_host_controller_t *hc);

void usb_hs_host_port_reset();
bool usb_hs_host_start_pipe(usb_host_pipe_t *pipe);
void usb_hs_host_update_control_pipe(usb_host_pipe_t *pipe);
void usb_hs_host_stop_pipe(usb_host_pipe_t *pipe);

bool usb_hs_host_begin_xfer(usb_request_buffer_t *urb, usb_direction_t dir, bool setup);

#endif // STM32F4_USB_OTG_HS_HOST_CORE_H

