#include "terminal.h"
#include <string.h>
#include <stdio.h>
#include <kernel/io.h>
#include <kernel/panic.h>
#include <kernel/posix/posix.h>
//#include <kernel/ascii.h>

#ifndef TERMINAL_POLL_TIME
#define TERMINAL_POLL_TIME EXOS_TIMEOUT_NEVER
#endif
#ifndef TERMINAL_CR_CHAR
#define TERMINAL_CR_CHAR 13
#endif


static void _rx_callback(dispatcher_context_t *context, dispatcher_t *dispatcher);

void terminal_context_create(terminal_context_t *terminal, const terminal_cmd_t cmd_array[], terminal_prompt_t prompt, void *user_state)
{
	ASSERT(terminal != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(cmd_array != nullptr && prompt != nullptr, KERNEL_ERROR_NULL_POINTER);
	*terminal = (terminal_context_t) {
		.CmdArray = cmd_array, .Prompt = prompt, .UserState = user_state };
}

void terminal_set_driver(terminal_context_t *terminal, terminal_driver_t driver)
{
	ASSERT(terminal != nullptr, KERNEL_ERROR_NULL_POINTER);
	terminal->Driver = driver;
}

void terminal_run(terminal_context_t *terminal, dispatcher_context_t *context, FILE *input, FILE *output)
{
	ASSERT(terminal != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(input != nullptr && output != nullptr, KERNEL_ERROR_NULL_POINTER);

	terminal->Input = input;
	terminal->Output = output;

    io_entry_t *io = posix_get_io_from_file(input);
	exos_dispatcher_create(&terminal->RxDispatcher, &io->InputEvent, _rx_callback, terminal);
	exos_dispatcher_add(context, &terminal->RxDispatcher, TERMINAL_POLL_TIME);
}

static bool _parse(const char **pbuf, const char *match)
{
	const char *buf = *pbuf;
	unsigned len = 0;
	char c;
	do 
	{ 
		c = match[len]; 
		if (c == '\0')
		{
			char b = buf[len];
			if (len != 0 && (b == '\0' || b == ' '))
			{
				if (b != '\0') len++;
				*pbuf = &buf[len];
				return true;
			}
			break;
		}
	} while(c == buf[len++]);
	return false;
}

static void _terminal(const terminal_context_t *terminal, dispatcher_context_t *context)
{
	fputs("\x1b[12h\n", terminal->Output); // disable local echo

	bool cmd_ok = false;
	const char *buf2 = terminal->Buffer;
	for(const terminal_cmd_t *cmd = terminal->CmdArray; cmd->Cmd != nullptr; cmd++)
	{
		if (_parse(&buf2, cmd->Cmd))
		{
			cmd->Func(context, buf2, terminal->UserState);
			cmd_ok = true;
			break;
		}
	}

	if (!cmd_ok)
	{
		if (0 == strcmp(terminal->Buffer, "help"))
		{
			for(const terminal_cmd_t *cmd = terminal->CmdArray; cmd->Cmd != nullptr; cmd++)
				fprintf(terminal->Output, "%s\n", cmd->Cmd);
		}
		else if (terminal->Buffer[0] != '\0') fprintf(terminal->Output, "%s?\n", terminal->Buffer);
	}

	if (terminal->Prompt != nullptr)
		terminal->Prompt(terminal);

	fputs("\x1b[12l", terminal->Output);	// enable local echo
	fputs("\x1b[s", terminal->Output);	// save cursor pos
}

static void _refresh(terminal_context_t *terminal)
{
	terminal->Buffer[terminal->Length] = '\0';
	fprintf(terminal->Output, "\x1b[u%s \x1b[%dD", terminal->Buffer, terminal->Length + 1 - terminal->Offset);
}

static void _rx_callback(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	terminal_context_t *terminal = (terminal_context_t *)dispatcher->CallbackState;
	io_entry_t *io = posix_get_io_from_file(terminal->Input);
    int done = 0;
	while(1)
	{
		char c;
		done = exos_io_read(io, &c, 1);
		if (done > 0)
		{
			switch(terminal->Escape)
			{
				case 0:
					if (c == TERMINAL_CR_CHAR)
					{
						terminal->Buffer[terminal->Length] = '\0';
						if (terminal->Length != 0 && terminal->Driver != nullptr)
						{
							void *args[] = { terminal->Buffer };
							terminal->Driver(terminal, TERM_FEAT_HISTORY, args);
						}

						_terminal(terminal, context);
						terminal->Offset = terminal->Length = 0;
					}
					else if (c == 27)
					{
						terminal->Escape = 1;
					}
					else if (c == 127) // backspace
					{
						if (terminal->Offset > 0)
						{
							for (unsigned i = terminal->Offset; i < terminal->Length; i++)
								terminal->Buffer[i - 1] = terminal->Buffer[i];
							terminal->Offset--;
							terminal->Length--;
						}
						_refresh(terminal);
					}
					else if (c >= 32 && terminal->Offset < TERMINAL_BUFFER_SIZE)
					{
						terminal->Buffer[terminal->Offset++] = c;
						if (terminal->Offset > terminal->Length) terminal->Length = terminal->Offset;
					}
					break;
				case 1:
					switch(c)
					{
						case '[':	terminal->Escape = 2;	break;
						default:
							terminal->Escape = 0;
							break;
					}
					break;
				case 2:
					switch(c)
					{
						case 'D':	if (terminal->Offset > 0) // arrow left 
									{
										terminal->Offset--;
									}
									break;
						case 'C':	if (terminal->Offset < terminal->Length) 	// arrow right
									{
										terminal->Offset++; 
									}
									break;
						case 'A':	if (terminal->Driver != nullptr)		// up
									{
										//fputs("\x1b[B", terminal->Output);
										terminal->Driver(terminal, TERM_FEAT_HISTORY, nullptr);
									}
						default:
							break;
					}
					terminal->Escape = 0;
					break;
			}
		}
		else break;		
	}

	if (done >= 0)
		exos_dispatcher_add(context, dispatcher, TERMINAL_POLL_TIME);
}


void terminal_set_buffer(terminal_context_t *terminal, const char *buffer)
{
	ASSERT(terminal != nullptr, KERNEL_ERROR_NULL_POINTER);
	unsigned length = buffer != nullptr ? strlen(buffer) : 0;
	if (length > TERMINAL_BUFFER_SIZE) length = TERMINAL_BUFFER_SIZE;
	strncpy(terminal->Buffer, buffer, length);
	terminal->Offset = terminal->Length = length;
	_refresh(terminal);
}



