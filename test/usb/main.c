#include "board.h"
#include "terminal.h"
#include <stdio.h>

static terminal_cmd_t _cmds[] = {
	{ }};
static void _prompt(const terminal_context_t *term);

void main()
{
	dispatcher_context_t context;
	exos_dispatcher_context_create(&context);

	//board_output(BOARD_OUTF_DCDC_EN, BOARD_OUTF_DCDC_EN);
	
	printf("\nExOS R2 on stm32f405!\n");

	board_usb_host_start(&context);

	terminal_context_t term; 
	terminal_context_create(&term, _cmds, _prompt, NULL);
	terminal_run(&term, &context, stdin, stdout);

	while(1)
	{
		if (!exos_dispatch(&context, 1000))
		{
			//printf("\ntick!");
		}
	}
}

static void _prompt(const terminal_context_t *term)
{
	printf("$ ");
}
