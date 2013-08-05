#include <support/misc/sdcard/sdcard.h>

void main()
{
	int done = sd_initialize();

	SD_INFO info;
	sd_get_info(&info);
}
