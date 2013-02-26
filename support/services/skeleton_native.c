#include <kernel/port.h>
#include <kernel/thread.h>

#ifndef EXAMPLE_SERVICE_STACK_SIZE
#define EXAMPLE_SERVICE_STACK_SIZE 1024
#endif

#ifndef EXAMPLE_SERVICE_PRIORITY
#define EXAMPLE_SERVICE_PRIORITY 1
#endif

typedef struct 
{
	unsigned char Stack[EXAMPLE_SERVICE_STACK_SIZE];
	EXOS_THREAD Thread;
	EXOS_PORT Port;
	EXOS_EVENT ExitEvent;
} EXAMPLE_SERVICE_INSTANCE;

typedef enum
{
	EXAMPLE_SERVICE_REQ_NONE = 0,
	EXAMPLE_SERVICE_REQ_QUIT,
} EXAMPLE_SERVICE_REQ_TYPE;

typedef struct
{
	EXOS_MESSAGE Message;
	EXAMPLE_SERVICE_REQ_TYPE ReqType;
} EXAMPLE_SERVICE_REQUEST;

static void *_service(void *arg);

int service_start(EXAMPLE_SERVICE_INSTANCE *instance, const char *name)
{
	int done = exos_port_create(&instance->Port, name);
	if (done)
	{
		exos_event_create(&instance->ExitEvent);
		exos_thread_create(&instance->Thread, EXAMPLE_SERVICE_PRIORITY, instance->Stack, EXAMPLE_SERVICE_STACK_SIZE, NULL, _service, instance);
	}
	return done;
}

static void _do_request(EXAMPLE_SERVICE_INSTANCE *instance, int *quit)
{
	EXAMPLE_SERVICE_REQUEST *req = (EXAMPLE_SERVICE_REQUEST *)exos_port_get_message(&instance->Port, EXOS_TIMEOUT_NEVER);
	switch(req->ReqType)
	{
		case EXAMPLE_SERVICE_REQ_QUIT:
			*quit = 1;
			break;
	}
	exos_port_reply_message((EXOS_MESSAGE *)req);
}


static void *_service(void *arg)
{
	EXAMPLE_SERVICE_INSTANCE *instance = (EXAMPLE_SERVICE_INSTANCE *)arg;
	
	int quit = 0;
	while(!quit)
	{
#ifdef SERVICE_WAITS_IO
		// NOTE: your io must be ready to receive (created and bound)

		EXOS_EVENT *events[] = { io->InputEvent, instance->Port.Event };
		int rcv_index = exos_event_wait_multiple(events, 2, EXOS_TIMEOUT_NEVER);
		switch(rcv_index)
		{
			case 0: // io input
				// TODO: read io
				// NOTE: use a socket/io that doesn't wait, you can be pretty sure that some data is ready to read as InputEvent is set.
				break;
			case 1:	// req input
				_do_request(instance, &quit);
				break;
		}
#else
		_do_request(instance, &quit);
#endif
	}

	exos_port_remove(instance->Port.Name);
	exos_event_set(&instance->ExitEvent);
}


