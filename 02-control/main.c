#include "pico/stdio.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "mem-task/mem-task.h"

#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

uint32_t global_variable = 0;

const uint32_t constant_variable = 42;


void version_callback(const char* args)
{
	printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}
void led_on_callback(const char* args)
{
	printf("Led turned on\n");
	led_task_state_set(LED_STATE_ON);
}
void led_off_callback(const char* args)
{
	printf("Led turned off\n");
	led_task_state_set(LED_STATE_OFF);
}
void led_blink_callback(const char* args)
{
	printf("Led is blinking now\n");
	led_task_state_set(LED_STATE_BLINK);
}

void led_blink_set_period_ms_callback(const char* args){
	uint period_ms = 0;
	sscanf(args, "%u", &period_ms);
	if (period_ms == 0){
		printf("Error incorrect period\n");
		return;
	}
	led_task_set_blink_period_ms(period_ms);
}
void mem_callback(const char* args)
{
	uint32_t addr = 0;
	int scanned = sscanf(args, "%lx", &addr);
	if (scanned == 1) {
		mem(addr);
	}
	else {
		printf("Error incorrect address\n");
	}
}
void wmem_callback(const char* args) {
	uint32_t addr = 0, value = 0;
	int scanned = sscanf(args, "%lx %lx", &addr, &value);
	if (scanned == 2) {
		wmem(addr, value);
	}
	else {
		printf("Error incorrect address or value\n");
	}
}

int main()
{
	stdio_init_all();
	led_task_init();
	stdio_task_init();
	api_t device_api[] =
	{
		{"version", version_callback, "get device name and firmware version"},
		{"on",      led_on_callback,  "switch on led"},
		{"off",     led_off_callback, "switch off led"},
		{"blink",   led_blink_callback, "provide unblocking"},
		{"mem", mem_callback, "Address value printed"},
		{"wmem", wmem_callback, "Addres value changed"},
		{"set_period",   led_blink_set_period_ms_callback, "blinking with arguments"},

		{"help", NULL, "show commands list"},
		{NULL, NULL, NULL} 
	};
	protocol_task_init(device_api);
	while (1) {
		protocol_task_handle(stdio_task_handle());
		led_task_handle();
	}
}
