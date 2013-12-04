#include "canopen.h"

static void *_service(void *args);

int canopen_create(CANOPEN_INSTANCE *ci, const CANOPEN_NODE_IDENTITY *identity, int can_module, int bitrate)
{
	can_receiver_initialize();
	
	int done = hal_can_initialize(can_module, bitrate);
	if (done)
	{
		can_receiver_add_handler(&ci->Handler, can_module, 0, 0);

		return 1;
	}
	return 0;
}

int canopen_run(CANOPEN_INSTANCE *ci, int pri, void *stack, int stack_size)
{
	exos_thread_create(&ci->Thread, pri, stack, stack_size, NULL, _service, ci);
	return 1;
}

int canopen_nmt_send_cmd(CANOPEN_INSTANCE *ci, int cmd)
{
	//TODO
}

int canopen_sdo_download(CANOPEN_INSTANCE *ci, int cob_id, CANOPEN_MUX mux, void *data)
{
	//TODO
}

int canopen_sdo_download_expedited(CANOPEN_INSTANCE *ci, int cob_id, CANOPEN_MUX mux, unsigned long data)
{
	
	//TODO
}

static void *_service(void *args)
{
	CANOPEN_INSTANCE *ci = (CANOPEN_INSTANCE *)args;

	CAN_MSG msg;
	while(1)
	{
		if (can_receiver_read(&ci->Handler, &msg, 1000))
		{
			
		}
	}
}



