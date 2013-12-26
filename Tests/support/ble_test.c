#include <support/bluetooth/nordic/aci.h>

void main()
{
	aci_initialize();
	while(1)
	{
		aci_connect(0, 100);
	}
}
