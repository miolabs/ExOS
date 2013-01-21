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
