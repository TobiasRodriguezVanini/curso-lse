#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

#define LED_RED	2
#define USER_BOTTON 4

int main(void) {
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    gpio_pin_config_t config = { kGPIO_DigitalOutput, 1 };
    gpio_pin_config_t config1 = { kGPIO_DigitalInput, 0};

    GPIO_PortInit(GPIO, 0);
    GPIO_PortInit(GPIO, 1);

    GPIO_PinInit(GPIO, 1, LED_RED, &config);
    GPIO_PinInit(GPIO, 0, USER_BOTTON, &config1);

    while(1) {
    	if (GPIO_PinRead(GPIO, 0, USER_BOTTON) == 1){
    		for(uint32_t i = 0; i < 20000; i++);
    		if (GPIO_PinRead(GPIO, 0, USER_BOTTON) == 1){
    			GPIO_PinWrite(GPIO, 1, LED_RED, 1);
    		}
    		else{
    			GPIO_PinWrite(GPIO, 1, LED_RED, 0);
    		}
    	}
    	for(uint32_t i = 0; i < 1000; i++);

    }
    return 0;
}

