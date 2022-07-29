#ifndef PARSER_H
#define PARSER_H

typedef struct
{
	unsigned char *Buffer;
	int Length;
	int Offset;
} LINE_PARSER;

typedef void (* LINE_PARSER_CALLBACK)(unsigned char *buffer, int length);

// prototypes
void parser_create(LINE_PARSER *state, unsigned char *buffer, int length);
int parser_parse_line(LINE_PARSER *state, LINE_PARSER_CALLBACK callback, unsigned char *buffer, int length);
int parse_string(unsigned char **pbuffer_in, unsigned char *buffer_out, int max_length);
int parse_dec(unsigned char **pbuffer_in, unsigned int *pvalue);
int parse_hex(unsigned char **pbuffer_in, unsigned int *pvalue);

#endif // PARSER_H
