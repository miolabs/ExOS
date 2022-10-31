#include "iap2.h"
#include <kernel/panic.h>
#include <string.h>

void iap2_helper_init_parameters(iap2_control_parameters_t *params, void *buffer, unsigned short length)
{
	*params = (iap2_control_parameters_t) { .Buffer = buffer, .MaxLength = length };
} 

void *iap2_helper_add_parameter(iap2_control_parameters_t *params, unsigned short id, unsigned short length)
{
	void *ptr = params->Buffer + params->Length;
	iap2_control_session_message_parameter_t *param = (iap2_control_session_message_parameter_t *)ptr;
	unsigned short param_length = sizeof(iap2_control_session_message_parameter_t) + length;
	param->Length = HTOIAP2S(param_length);
	param->Id = HTOIAP2S(id);
	params->Length += param_length;
	ASSERT(params->Length <= params->MaxLength, KERNEL_ERROR_KERNEL_PANIC);
	return ptr + sizeof(iap2_control_session_message_parameter_t);
}

void iap2_helper_add_param_string(iap2_control_parameters_t *params, unsigned short id, const char *str)
{
	void *payload = iap2_helper_add_parameter(params, id, strlen(str) + 1);
	strcpy((char *)payload, str);
}
