#include "comm_service.h"

static void *_service(void *arg);

void comm_service_start(COMM_DEVICE *device)
{
	exos_thread_create(&device->Thread, 1, &device->Stack, COMM_DEVICE_THREAD_STACK, 
		_service, device);
}

static void *_service(void *arg)
{
	COMM_DEVICE *device = (COMM_DEVICE *)arg;
	device->InputSignal = exos_signal_alloc();

	// TODO: enable input now or prevent usage of the driver signal before this thread initialization

	const COMM_DRIVER *driver = device->Driver;
	while(1)
	{


		exos_signal_wait(1 << device->InputSignal, 1000);
	}
}

