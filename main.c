#include <kernel/tests/threading_tests.h>
#include <kernel/posix/tests/basic_pthread_tests.h>

void main()
{
	int result = threading_tests();

	result = pthread_tests();

	while(1);
}
