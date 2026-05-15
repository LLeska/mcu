#include "protocol-task.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "string.h"

static api_t* api = { 0 };
static int commands_count = 0;

static char* skip_spaces(char* str)
{
	while (*str == ' ' || *str == '\t')
	{
		str++;
	}
	return str;
}

void protocol_task_init(api_t* device_api) {
	api = device_api;
	commands_count = 0;
	int i = 0;
	while (1) {
		if (device_api[i].command_name != NULL) {
			commands_count += 1;
			i++;
		}
		else {
			break;
		}
	}
}

void protocol_task_handle(char* command_string)
{
	bool flag = 0;
	if (!command_string)
	{
		return;
	}

	command_string = skip_spaces(command_string);
	if (*command_string == '\0')
	{
		return;
	}

	const char* command_name = command_string;
	const char* command_args = NULL;

	char* space_symbol = strpbrk(command_string, " \t");
	if (space_symbol)
	{
		*space_symbol = '\0';
		command_args = skip_spaces(space_symbol + 1);
	}
	else
	{
		command_args = "";
	}

	printf("received string: '%s', args: '%s'\n", command_name, command_args);

	if (strcmp(command_name, "help") == 0) {
		printf("Available commands:\n");
		for (int i = 0; i < commands_count; i++) {
			if (api[i].command_name == NULL) break;
			if (api[i].command_help != NULL) {
				printf("  %-16s - %s\n", api[i].command_name, api[i].command_help);
			}
		}
		return;
	}

	for (int i = 0; i < commands_count ; i++)
	{
		if (strcmp(command_name, api[i].command_name)!=0) {
			continue;
		}
		api[i].command_callback(command_args);
		flag = 1;
		return;
	}

	if (!flag) {
		printf("Error command not found\n");
		printf("unknown command bytes:");
		for (const char* p = command_name; *p != '\0'; p++) {
			printf(" 0x%02X", (unsigned char)*p);
		}
		printf("\n");
	}
	return;
}
