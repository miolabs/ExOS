#include "usb.h"

static USB_DESCRIPTOR_HEADER *_next_descriptor(USB_DESCRIPTOR_HEADER *desc)
{
	desc = (USB_DESCRIPTOR_HEADER *)((void *)desc + desc->Length);
	return desc;
}

int usb_parse_conf(USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_TYPE desc_type, USB_DESCRIPTOR_HEADER **current)
{
	USB_DESCRIPTOR_HEADER *desc = *current;
	while(desc != NULL)
	{
		desc = _next_descriptor(desc);
		if (desc >= (USB_DESCRIPTOR_HEADER *)((void *)conf_desc + USB16TOH(conf_desc->TotalLength))) break;

		if (desc->DescriptorType == desc_type
			|| desc_type == USB_DESCRIPTOR_TYPE_ANY)
		{
			*current = desc;
			return 1;
		}
	}
	return 0;
}

int usb_desc2str(USB_DESCRIPTOR_HEADER *str_desc, unsigned char *dest, int max_length)
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
