#ifndef TERMINAL_H
#define TERMINAL_H

#include <kernel/dispatch.h>
#include <stdio.h>

typedef void (terminal_handler_t)(dispatcher_context_t *context, const char *buf, void *userstate); 
typedef struct 
{ 
	const char *Cmd; 
	terminal_handler_t *Func;
} terminal_cmd_t;

typedef enum
{
	TERM_FEAT_NONE = 0,
	TERM_FEAT_HISTORY = 1,
} terminal_driver_feature_t;

typedef struct __terminal_context terminal_context_t;
typedef void (*terminal_prompt_t)(const terminal_context_t *);
typedef void (*terminal_driver_t)(terminal_context_t *terminal, terminal_driver_feature_t feat, void *args);

#ifndef TERMINAL_BUFFER_SIZE
#define TERMINAL_BUFFER_SIZE 63
#endif

struct __terminal_context
{
	unsigned char Offset;
	unsigned char Length;
	unsigned char Escape;
	FILE *Input;
	FILE *Output;
	dispatcher_t RxDispatcher;
	const  terminal_cmd_t *CmdArray;
	terminal_prompt_t Prompt;
	terminal_driver_t Driver;
	void *UserState;
	char Buffer[TERMINAL_BUFFER_SIZE + 1];
};

void terminal_context_create(terminal_context_t *terminal, const terminal_cmd_t cmd_array[], terminal_prompt_t prompt, void *user_state);
void terminal_set_driver(terminal_context_t *terminal, terminal_driver_t driver);
void terminal_run(terminal_context_t *terminal, dispatcher_context_t *context, FILE *input, FILE *output);
void terminal_set_buffer(terminal_context_t *terminal, const char *buffer);

#endif // TERMINAL_H


 