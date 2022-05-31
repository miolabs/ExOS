#include "aci_support.h"
#include "services.h"	// NOTE: setup data generated with nrfgo
#include "aci.h"

static const ACI_SETUP_DATA _setup[] = SETUP_MESSAGES_CONTENT; 

int aci_support_setup()
{
	int setup = 0;
	int complete = 0;
	while(setup < NB_SETUP_MESSAGES)
	{
		int cont;
		const unsigned char *data = _setup[setup].Data;
		ACI_REQUEST req = (ACI_REQUEST) { .Length = data[0] - 1, .Command = data[1] };
		for(int i = 0; i < req.Length; i++) req.Data[i] = data[2 + i];
		setup++;

		if (!aci_send_setup(&req, &complete)) break;
		if (complete) break;
	}

	return complete;
}





