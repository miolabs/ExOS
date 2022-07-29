#include "parser.h"
#include <kernel/panic.h>

void parser_create(LINE_PARSER *state, unsigned char *buffer, int length)
{
	*state = (LINE_PARSER) { .Buffer = buffer, .Length = length };
}

int parser_parse_line(LINE_PARSER *state, LINE_PARSER_CALLBACK callback, unsigned char *buffer, int length)
{
	if (state == NULL || callback == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);

	for(int index = 0; index < length; index++)
	{
		unsigned char c = buffer[index];
		if (c == '\0') break;

		if (c == '\n')
		{
			state->Buffer[state->Offset] = '\0';
			callback(state->Buffer, state->Offset);
			
			state->Offset = 0;
		}
		else if (c != '\r')
		{
			if (state->Offset < (state->Length - 1))
			{
				state->Buffer[state->Offset++] = c;
			}
			else break;
		}
	}
	return 0;
}

int parse_string(unsigned char **pbuffer_in, unsigned char *buffer_out, int max_length)
{
	unsigned char *buf = *pbuffer_in;
	unsigned char c;
	int length = 0;
	while(c = *buf, c == ' ') buf++;
	while(c != '\0')
	{
		if (length < max_length) 
			buffer_out[length++] = c;
		
		c = *++buf;
		if (c == ' ') break;
	}
	*pbuffer_in = buf;
	buffer_out[length] = '\0';
	return length;
}

int parse_dec(unsigned char **pbuffer_in, unsigned int *pvalue)
{
	unsigned char *buf = *pbuffer_in;
	unsigned char c;
   	while(c = *buf, c == ' ') buf++;
	int length = 0;
	unsigned int value = 0;
	while(1)
	{
		c = buf[length];
		if (c >= '0' && c <= '9')
		{
			value = (value * 10) + (c - '0');
			length++;
		}
		else break;
	}
	*pbuffer_in = buf + length;
	*pvalue = value;
	return length;
}

int parse_hex(unsigned char **pbuffer_in, unsigned int *pvalue)
{
	unsigned char *buf = *pbuffer_in;
	unsigned char c;
   	while(c = *buf, c == ' ') buf++;
	int length = 0;unsigned int value = 0;
	while(1)
	{
		c = buf[length];
		if (c >= '0' && c <= '9')
		{
			value = (value << 4) + (c - '0');
			length++;
		}
		else if (c >= 'A' && c <= 'F')
		{
			value = (value << 4) + (c + 10 - 'A');
			length++;
		}
		else if (c >= 'a' && c <= 'f')
		{
			value = (value << 4) + (c + 10 - 'a');
			length++;
		}
		else break;
	}
	*pbuffer_in = buf + length;
	*pvalue = value;
	return length;
}
