#include "pico/stdio.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"


#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

uint32_t global_variable = 0;

const uint32_t constant_variable = 42;

const uint LED_PIN = 25;

int main()
{
	stdio_init_all();
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	while (1) {
		char symbol = getchar();
		printf("received char: %c [ ASCII code: %d ]\n", symbol, symbol);
		switch (symbol)
		{
		case 'e':
			gpio_put(LED_PIN, true);
			printf("led enable done\n");
			break;
		case 'd':
			gpio_put(LED_PIN, false);
			printf("led disable done\n");
			break;
		case 'v':
			printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
			break;
		default:
			break;
		}
	}
}