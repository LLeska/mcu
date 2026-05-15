#include "protocol-task.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "string.h"

static api_t* api = { 0 };
static int commands_count = 0;

void protocol_task_init(api_t* device_api) {
	api = device_api;
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
	//	Добавляем в обработчик проверку на то, что `command_string` не равно `NULL`.
	//	Если `command_string` равно `NULL`, то выйти из обработчика: строка команды
	//	еще не получена;
	if (!command_string)
	{
		return;
	}

	// логика обработки полученной строки. Делим ее на команду и аргументы:
	const char* command_name = command_string;
	const char* command_args = NULL;

	char* space_symbol = strchr(command_string, ' ');

	if (space_symbol)
	{
		*space_symbol = '\0';
		command_args = space_symbol + 1;
	}
	else
	{
		command_args = "";
	}


	// Добавляем вывод найденных имени команды и ее аргументов;
	printf("received string: '%s', args: '%s'\n", command_name, command_args);


	// в цикле проходим по массиву команд `api` и ищем совпадение имени команды;

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
		// определяем совпадает ли команда  с именем команды в массиве `api`
		// если не совпадает, переходим к следующей итерации

		if (strcmp(command_name, api[i].command_name)!=0) {
			continue;
		}

		// мы нашли команду, вызываем callback найденной команды
		api[i].command_callback(command_args);
		flag = 1;
		return;
	}
	// выводим ошибку, если команда не была найдена в списке команд
	if (!flag) {
		printf("Ошибка, команда не найдена\n");
	}
	return;
}
