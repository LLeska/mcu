#include "pico/stdio.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "mem-task/mem-task.h"
#include "adc-task/adc-task.h"

#define DEVICE_NAME "my-pico-adc"
#define DEVICE_VRSN "v0.0.3"

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
void get_adc_callback(const char* args)
{
	float voltage_V = adc_task_get_voltage();
	printf("%f\n", voltage_V);
}
void get_temp_callback(const char* args)
{
	float temp_C = adc_task_get_temp();
	printf("%f\n", temp_C);
}
void tm_start_callback(const char* args)
{
	adc_task_set_state(ADC_TASK_STATE_RUN);
}
void tm_stop_callback(const char* args)
{
	adc_task_set_state(ADC_TASK_STATE_IDLE);
}

int main()
{
	stdio_init_all();
	led_task_init();
	adc_task_init();
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
		{"get_adc", get_adc_callback, "get adc voltage"},
		{"get_temp", get_temp_callback, "get rp2040 temperature"},
		{"tm_start", tm_start_callback, "start telemetry"},
		{"tm_stop", tm_stop_callback, "stop telemetry"},

		{"help", NULL, "show commands list"},
		{NULL, NULL, NULL} 
	};
	protocol_task_init(device_api);
	while (1) {
		protocol_task_handle(stdio_task_handle());
		led_task_handle();
		adc_task_handle();
	}
}
