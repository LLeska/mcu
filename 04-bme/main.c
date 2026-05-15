#include "pico/stdio.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task.h"
#include "led-task/led-task.h"
#include "mem-task/mem-task.h"
#include "bme280-driver.h"

#define DEVICE_NAME "my-pico-bme"
#define DEVICE_VRSN "v0.0.4"
#define BME280_ADDR 0x76

uint32_t global_variable = 0;

const uint32_t constant_variable = 42;


void rp2040_i2c_read(uint8_t* buffer, uint16_t length)
{
	i2c_read_timeout_us(i2c1, BME280_ADDR, buffer, length, false, 100000);
}
void rp2040_i2c_write(uint8_t* data, uint16_t size)
{
	i2c_write_timeout_us(i2c1, BME280_ADDR, data, size, false, 100000);
}

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
void read_regs_callback(const char* args)
{
	unsigned int addr = 0, N = 0;
	int scanned = sscanf(args, "%x %x", &addr, &N);
	if (scanned != 2 || addr > 0xFF || N > 0xFF || (addr + N) > 0x100) {
		printf("Error incorrect address or length\n");
		return;
	}

	uint8_t buffer[256] = {0};
	bme280_read_regs(addr, buffer, N);
	for (int i = 0; i < N; i++)
	{
		printf("bme280 register [0x%X] = 0x%X\n", addr + i, buffer[i]);
	}
}
void write_reg_callback(const char* args)
{
	unsigned int addr = 0, value = 0;
	int scanned = sscanf(args, "%x %x", &addr, &value);
	if (scanned != 2 || addr > 0xFF || value > 0xFF) {
		printf("Error incorrect address or value\n");
		return;
	}
	bme280_write_reg(addr, value);
}
void temp_raw_callback(const char* args)
{
	printf("%u\n", bme280_read_temp_raw());
}
void pres_raw_callback(const char* args)
{
	printf("%u\n", bme280_read_pres_raw());
}
void hum_raw_callback(const char* args)
{
	printf("%u\n", bme280_read_hum_raw());
}
void temp_callback(const char* args)
{
	printf("%f\n", bme280_read_temp());
}
void pres_callback(const char* args)
{
	printf("%f\n", bme280_read_pres());
}
void hum_callback(const char* args)
{
	printf("%f\n", bme280_read_hum());
}

int main()
{
	stdio_init_all();
	i2c_init(i2c1, 100000);
	gpio_set_function(14, GPIO_FUNC_I2C);
	gpio_set_function(15, GPIO_FUNC_I2C);
	gpio_pull_up(14);
	gpio_pull_up(15);
	led_task_init();
	stdio_task_init();
	bme280_init(rp2040_i2c_read, rp2040_i2c_write);
	api_t device_api[] =
	{
		{"version", version_callback, "get device name and firmware version"},
		{"on",      led_on_callback,  "switch on led"},
		{"off",     led_off_callback, "switch off led"},
		{"blink",   led_blink_callback, "provide unblocking"},
		{"mem", mem_callback, "Address value printed"},
		{"wmem", wmem_callback, "Addres value changed"},
		{"set_period",   led_blink_set_period_ms_callback, "blinking with arguments"},
		{"read_regs", read_regs_callback, "read bme280 registers"},
		{"read_reg", read_regs_callback, "read bme280 registers"},
		{"write_reg", write_reg_callback, "write bme280 register"},
		{"temp_raw", temp_raw_callback, "get raw temperature"},
		{"pres_raw", pres_raw_callback, "get raw pressure"},
		{"hum_raw", hum_raw_callback, "get raw humidity"},
		{"temp", temp_callback, "get temperature C"},
		{"pres", pres_callback, "get pressure Pa"},
		{"hum", hum_callback, "get humidity percent"},

		{"help", NULL, "show commands list"},
		{NULL, NULL, NULL} 
	};
	protocol_task_init(device_api);
	while (1) {
		protocol_task_handle(stdio_task_handle());
		led_task_handle();
	}
}
