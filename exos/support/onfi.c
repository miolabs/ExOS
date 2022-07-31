#include "onfi.h"

ONFI_PARAMETER_PAGE _params;

static unsigned char _read_status(const ONFI_DRIVER *driver)
{
	unsigned char status;
	driver->Cmd(ONFI_CMD1_READ_STATUS, 0, (void *)0);
	driver->Read(&status, 1);
	return status;
}

int onfi_initialize(const ONFI_DRIVER *driver, ONFI_STATUS *param)
{
	driver->Cmd(ONFI_CMD1_RESET, 0, (void *)0);
	while(1)
	{
		unsigned char status = _read_status(driver);
		if ((status & 0x60) == 0x60) break;
	}
	
	unsigned char signature[4];
	driver->Cmd(ONFI_CMD1_READ_ID, 1, (unsigned char[]){ 0x20 });
	driver->Read(&signature, 4);

	if (signature[0] == 'O' &&
		signature[1] == 'N' &&
		signature[2] == 'F' &&
		signature[3] == 'I')
	{
		driver->Cmd(ONFI_CMD1_READ_ID, 1, (unsigned char[]){ 0 } );
		driver->Read(param->Id, 4);

		unsigned char addr = 0;
		driver->Cmd(ONFI_CMD1_READ_PARAMETER_PAGE, 1, &addr);
		addr += driver->Read(&_params.Features, sizeof(_params.Features));
		addr += driver->Read((void *)0, 32 - addr);
		addr += driver->Read(&_params.Manufacturer, sizeof(_params.Manufacturer));
		addr += driver->Read((void *)0, 80 - addr);
		addr += driver->Read(&_params.MemoryOrganization, sizeof(_params.MemoryOrganization));
		addr += driver->Read((void *)0, 128 - addr);
		addr += driver->Read(&_params.ElectricalParameters, sizeof(_params.ElectricalParameters));

		// TODO: save required params in user struct
		return 1;
	}
	return 0;
}
