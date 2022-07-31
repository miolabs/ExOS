#include "usb.h"

static usb_descriptor_header_t *_next_descriptor(usb_descriptor_header_t *desc)
{
	desc = (usb_descriptor_header_t *)((void *)desc + desc->Length);
	return desc;
}

int usb_parse_conf(usb_configuration_descriptor_t *conf_desc, usb_descriptor_type_t desc_type, usb_descriptor_header_t **current)
{
	usb_descriptor_header_t *desc = *current;
	while(desc != nullptr)
	{
		desc = _next_descriptor(desc);
		if (desc >= (usb_descriptor_header_t *)((void *)conf_desc + USB16TOH(conf_desc->TotalLength))) break;

		if (desc->DescriptorType == desc_type
			|| desc_type == USB_DESCRIPTOR_TYPE_ANY)
		{
			*current = desc;
			return 1;
		}
	}
	return 0;
}

int usb_desc2str(usb_descriptor_header_t *str_desc, unsigned char *dest, int max_length)
{
	int done = 0;
	if (str_desc->DescriptorType == USB_DESCRIPTOR_TYPE_STRING)
	{
		int length = str_desc->Length;
		unsigned char *data = (unsigned char *)str_desc;
		int off = 2; 
		while(off < length && done < max_length)
		{
			dest[done++] = data[off];
			off += 2;
		}
		if (done < max_length) dest[done++] = '\0';
	}
	return done;
}
