#ifndef USB_DEVICE_HAL_H
#define USB_DEVICE_HAL_H

#include <usb/device.h>

void hal_usbd_initialize();
void hal_usbd_connect(bool connect);
bool hal_usbd_is_connected();
bool hal_usbd_disconnected();
void hal_usbd_suspend();
void hal_usbd_resume();
void hal_usbd_set_address(unsigned char addr);
void hal_usbd_set_address_early(unsigned char addr) __weak;	// NOTE: rare - for devices that require the address setup before handshake 
void hal_usbd_configure(bool configure);
void hal_usbd_prepare_setup_ep(usb_io_buffer_t *iob);
void hal_usbd_enable_out_ep(unsigned ep_num, usb_transfer_type_t tt, unsigned max_packet_size);
void hal_usbd_enable_in_ep(unsigned ep_num, usb_transfer_type_t tt, unsigned max_packet_size);
void hal_usbd_prepare_out_ep(unsigned ep_num, usb_io_buffer_t *iob);
void hal_usbd_prepare_in_ep(unsigned ep_num, usb_io_buffer_t *iob);
void hal_usbd_stall_out_ep(unsigned ep_num, bool stall);
void hal_usbd_stall_in_ep(unsigned ep_num, bool stall);
bool hal_usbd_is_halted(unsigned ep_num);

#endif // USB_DEVICE_HAL_H
