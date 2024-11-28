#include "app_tasks.h"
#include "fsl_debug_console.h"


xQueueHandle queue_display_variable1;
xQueueHandle queue_display_variable2;
xQueueHandle queue_adc;
xQueueHandle queue_lux;
xQueueHandle queue_pwm;
xQueueHandle queue_time;

TaskHandle_t handle_display;


/**
 * @brief Inicializa todos los perifericos y colas
 */
void task_init(void *params) {
	wrapper_gpio_init(0);
	wrapper_adc_init();
	wrapper_display_init();
	wrapper_btn_init();
	wrapper_pwm_init();
    wrapper_i2c_init();
	wrapper_bh1750_init();

	queue_adc = xQueueCreate(1, sizeof(adc_data_t));
	queue_lux = xQueueCreate(1, sizeof(uint8_t));
	queue_pwm = xQueueCreate(1, sizeof(uint8_t));
	queue_time = xQueueCreate(1, sizeof(uint32_t));
	queue_display_variable1 = xQueueCreate(1, sizeof(display_variable_t));
	queue_display_variable2 = xQueueCreate(1, sizeof(display_variable_t));
	vTaskDelete(NULL);
}

void task_time(void *params){
	uint32_t time = 0;
	while(1){
		vTaskDelay(1);
		time++;
		xQueueOverwrite(queue_time, &time);
	}
}

void task_bh1750(void *params) {
		CLOCK_Select(kI2C1_Clk_From_MainClk);
		CLOCK_EnableClock(kCLOCK_Swm);
	    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
	    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
	    CLOCK_DisableClock(kCLOCK_Swm);

	    i2c_master_config_t config;
	    I2C_MasterGetDefaultConfig(&config);
	    config.baudRate_Bps = 400000;
	    I2C_MasterInit(I2C1, &config, SystemCoreClock);

		if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
		uint8_t cmd = 0x01;
			I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
		}
		if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
			uint8_t cmd = 0x10;
			I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
		}

		while(1) {
			if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Read) == kStatus_Success) {
				uint8_t res[2] = {0};
				I2C_MasterReadBlocking(I2C1, res, 2, kI2C_TransferDefaultFlag);
				I2C_MasterStop(I2C1);
				float lux = ((res[0] << 8) + res[1]) / 1.2;
				uint8_t percentLux = (lux * 100.00) / 30000.00;
				xQueueOverwrite(queue_lux, &percentLux);
			}
	    }
}

void task_consola(void *params){
	uint32_t time = 0;
	uint8_t lux = 0;
	display_variable_t val = 50;
	uint8_t pwm = 0;
	PRINTF("Tiempo transcurrido | Luz | SP | PWM\n");
	while(1) {

		xQueueReceive(queue_time, &time, portMAX_DELAY);
		xQueueReceive(queue_lux, &lux, portMAX_DELAY);
		xQueuePeek(queue_display_variable2, &val, portMAX_DELAY);
		xQueuePeek(queue_pwm, &pwm, portMAX_DELAY);

		PRINTF("  %d    %d    %d    %d\n",
				time,
				lux,
				val,
				pwm
				);
		vTaskDelay(1000);

	}
}

void task_adc(void *params) {

	while(1) {
		ADC_DoSoftwareTriggerConvSeqA(ADC0);
		vTaskDelay(250);
	}
}

void task_boton(void *params) {


	display_variable_t val = 50;
	display_variable_t change = 0;
	uint8_t lux = 0;

	while(1) {
		xQueueReceive(queue_lux, &lux, portMAX_DELAY);
		if(!wrapper_btn_get(USER)) {
			vTaskDelay(20);
			if(change == 1){
				change = 0;
			}
			else{
				change = 1;
			}
		}
		if(!wrapper_btn_get(S1)) {
			vTaskDelay(20);
			if(!wrapper_btn_get(S1) && val < 75) {
				val++;
			}
		}
		if(!wrapper_btn_get(S2)) {
			vTaskDelay(20);
			if(!wrapper_btn_get(S2) && val > 25) {
				val--;
			}
		}

		xQueueOverwrite(queue_display_variable1, &change);
		xQueueOverwrite(queue_display_variable2, &val);
		vTaskDelay(200);
	}
}

/**
 * @brief Escribe valores en el display
 */
void task_display_write(void *params) {
	display_variable_t val = 50;
	display_variable_t change = 0;
	uint8_t lux = 0;
	uint8_t show = 0;

	while(1) {
		xQueuePeek(queue_display_variable1, &change, portMAX_DELAY);
		xQueuePeek(queue_display_variable2, &val, portMAX_DELAY);
		xQueuePeek(queue_lux, &lux, portMAX_DELAY);
		if(change == 0){
			show = val;
		}
		else{
			show = lux;
		}

    	wrapper_display_off();
		wrapper_display_write((uint8_t)(show / 10));
		wrapper_display_on(COM_1);
		vTaskDelay(10);
		wrapper_display_off();
		wrapper_display_write((uint8_t)(show % 10));
		wrapper_display_on(COM_2);
		vTaskDelay(10);
	}
}

/**
 * @brief Actualiza el duty del PWM
 */
void task_pwm(void *params) {
	adc_data_t data = {0};
	uint8_t pwm = 0;

	while(1) {
		xQueueReceive(queue_adc, &data, portMAX_DELAY);
		pwm = (uint8_t)((data.ref_raw * 100) / 4095);
    	wrapper_pwm_update(pwm);
		xQueueOverwrite(queue_pwm, &pwm);
	}
}

void ADC0_SEQA_IRQHandler(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC0))) {
		ADC_ClearStatusFlags(ADC0, kADC_ConvSeqAInterruptFlag);
		adc_result_info_t temp_info, ref_info;
		ADC_GetChannelConversionResult(ADC0, REF_POT_CH, &ref_info);
		ADC_GetChannelConversionResult(ADC0, LM35_CH, &temp_info);
		adc_data_t data = {
			.temp_raw = (uint16_t)temp_info.result,
			.ref_raw = (uint16_t)ref_info.result
		};
		xQueueOverwriteFromISR(queue_adc, &data, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}