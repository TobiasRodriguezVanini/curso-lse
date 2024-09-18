#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

// Etiqueta para el LED azul
#define LED_RED
#define USER_BUTTON


/*
 * @brief   Application entry point.
*/
int main(void) {
	// Inicializacion
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Estructura de configuracion para salida
    gpio_pin_config_t config = { kGPIO_DigitalOutput, 2 };
    gpio_pin_config_t config1 = { kGPIO_DigitalInput, 4 };

    GPIO_PortInit(GPIO, 1);
    GPIO_PortInit(GPIO, 0);

    GPIO_PinInit(GPIO, 2, LED_RED, &config);
    GPIO_PinInit(GPIO, 4, USER_BUTTOM, &config1);

    while(1){
    	if (GPIO_PinRead(GPIO,0 USER_BUTTOM)==1){
    		for(uint32 t i=0 <20000; i++)
    		(GPIO_PinWrite(GPIO,1,LED_RED);
    	}
    	else{
    		GPIO_PinWrite(GPIO,1, LED_RED, 0);
    	}
    }
}
return 0;
