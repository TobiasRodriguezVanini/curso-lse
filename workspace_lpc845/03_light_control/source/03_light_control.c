#include "board.h"
#include "fsl_swm.h"
#include "fsl_i2c.h"
#include "fsl_sctimer.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"

// Direccion del BH1750
#define BH1750_ADDR	0x5c
#define PWM_FREQ	1000
uint32_t duty_cycle;
uint32_t event;

int main(void) {

	// Arranco de 30 MHz
	BOARD_BootClockFRO30M();

	// Inicializo el clock del I2C1
	CLOCK_Select(kI2C1_Clk_From_MainClk);
	CLOCK_EnableClock(kCLOCK_Swm);
	SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
	SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
    CLOCK_DisableClock(kCLOCK_Swm);

    uint32_t sctimer_clock = CLOCK_GetFreq(kCLOCK_Fro);
        sctimer_config_t sctimer_config;
        SCTIMER_GetDefaultConfig(&sctimer_config);
        SCTIMER_Init(SCT0, &sctimer_config);

        // Configuro el PWM
        sctimer_pwm_signal_param_t pwm_config = {
    		.output = kSCTIMER_Out_4,		// Salida del Timer
    		.level = kSCTIMER_LowTrue,		// Logica Negativa
    		.dutyCyclePercent = 50 // 50% de ancho de pulso
        };

        uint32_t event;
        // Inicializo el PWM
        SCTIMER_SetupPwm(
    		SCT0,
    		&pwm_config,
    		kSCTIMER_CenterAlignedPwm,
    		PWM_FREQ,
    		sctimer_clock,
    		&event
    	);
        // Inicializo el Timer
        SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);
    i2c_master_config_t config;
    I2C_MasterGetDefaultConfig(&config);
    config.baudRate_Bps = 400000;
    I2C_MasterInit(I2C1, &config, SystemCoreClock);

	if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
		// Comando de power on
		uint8_t cmd = 0x01;
		I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
		I2C_MasterStop(I2C1);
	}
	if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
		// Comando de medicion continua a 1 lux de resolucion
		uint8_t cmd = 0x10;
		I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
		I2C_MasterStop(I2C1);
	}

	while(1) {
		// Lectura del sensor
		if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Read) == kStatus_Success) {
			// Resultado
			uint8_t res[2] = {0};
			I2C_MasterReadBlocking(I2C1, res, 2, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
			// Devuelvo el resultado
			float lux = ((res[0] << 8) + res[1]) / 1.2;
			PRINTF("LUX : %d \r\n",(uint16_t) lux);
			uint32_t duty = (float) lux / 65535 * 100000;
			PRINTF("DUTY_CYCLE: %d\n", duty);

			    	// Verifico que este entre 0 y 100
			    	if(lux < 300 && lux > 0){
			    		duty_cycle = duty;
			    		// Actualizo el ancho de pulso
			    		SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, lux, event);


			    }
		}
    }
    return 0;
}
