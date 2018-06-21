#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

const uint16_t OutPin[16] = {GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6};
GPIO_TypeDef *OutPort[16] = {GPIOD, GPIOD, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOE, GPIOE, GPIOE, GPIOE, GPIOE, GPIOE, GPIOE};
    
const uint16_t InPin[16] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_15, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_15, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14};
GPIO_TypeDef *InPort[16] = {GPIOD, GPIOD, GPIOD, GPIOD, GPIOA, GPIOC, GPIOC, GPIOC, GPIOB, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD};

void HY8005InitGpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    int i = 0;
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();


    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   
    for (i=0; i<16; i++)
    {
        GPIO_InitStruct.Pin = OutPin[i];
        HAL_GPIO_Init(OutPort[i], &GPIO_InitStruct);
    }

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   
    for (i=0; i<16; i++)
    {
        GPIO_InitStruct.Pin = InPin[i];
        HAL_GPIO_Init(InPort[i], &GPIO_InitStruct);
    }
  
}



void HY8005SetOut(uint16_t Out)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    int i = 0;
    
    for (i=0; i<16; i++)
    {
        GPIO_InitStruct.Pin = OutPin[i];
        HAL_GPIO_Init(OutPort[i], &GPIO_InitStruct);
    }
}

void HY8005GetIn(uint16_t *pIn)
{

}
