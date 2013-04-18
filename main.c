#include <kernel/tests/threading_tests.h>
#include <kernel/tests/memory_tests.h>
#include <kernel/posix/tests/basic_pthread_tests.h>

#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>

void main()
{
	int result;

	//result = memory_tests();
	
	//result = threading_tests();
	
	//result = pthread_tests();

	LPC_GPIO2->DIR |= (1<<6) | (1<<7);
	while(1)
	{
		exos_thread_sleep(1000);
		LPC_GPIO2->MASKED_ACCESS[1<<6] = 1<<6;
		LPC_GPIO2->MASKED_ACCESS[1<<7] = 0;
		exos_thread_sleep(1000);
		LPC_GPIO2->MASKED_ACCESS[1<<6] = 0;
		LPC_GPIO2->MASKED_ACCESS[1<<7] = 1<<7;
	}
}
