#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "drivers/nvic.h"
#include "drivers/system.h"

#define AIRCR_VECTKEY_MASK    ((uint32_t)0x05FA0000)

// from system_stm32f10x.c
void SetSysClock(bool overclock);

void systemReset(void)
{
    // Generate system reset
    SCB->AIRCR = AIRCR_VECTKEY_MASK | (uint32_t)0x04;
}

void systemResetToBootloader(void)
{
    // 1FFFF000 -> 20000200 -> SP
    // 1FFFF004 -> 1FFFF021 -> PC

    *((uint32_t *)0x20004FF0) = 0xDEADBEEF; // 20KB STM32F103
    systemReset();
}

void enableGPIOPowerUsageAndNoiseReductions(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Mode = GPIO_Mode_AIN,
        .GPIO_Pin = GPIO_Pin_All
    };

    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

bool isMPUSoftReset(void)
{
    if (cachedRccCsrValue & RCC_CSR_SFTRSTF)
        return true;
    else
        return false;
}

void systemInit(void)
{
    checkForBootLoaderRequest();

    SetSysClock(false);

#if defined(OPBL)
    /* Accounts for OP Bootloader, set the Vector Table base address as specified in .ld file */
    extern void *isr_vector_table_base;

    NVIC_SetVectorTable((uint32_t)&isr_vector_table_base, 0x0);
#endif
    // Configure NVIC preempt/priority groups
    NVIC_PriorityGroupConfig(NVIC_PRIORITY_GROUPING);

    // Turn on clocks for stuff we use
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // cache RCC->CSR value to use it in isMPUSoftReset() and others
    cachedRccCsrValue = RCC->CSR;
    RCC_ClearFlag();

    enableGPIOPowerUsageAndNoiseReductions();

    // Set USART1 TX (PA9) to output and high state to prevent a rs232 break condition on reset.
    // See issue https://github.com/cleanflight/cleanflight/issues/1433
    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Mode = GPIO_Mode_Out_PP,
        .GPIO_Pin = GPIO_Pin_9,
        .GPIO_Speed = GPIO_Speed_2MHz
    };

    GPIOA->BSRR = GPIO_InitStructure.GPIO_Pin;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Turn off JTAG port 'cause we're using the GPIO for leds
#define AFIO_MAPR_SWJ_CFG_NO_JTAG_SW            (0x2 << 24)
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_NO_JTAG_SW;

    // Init cycle counter
    cycleCounterInit();

    // SysTick
    SysTick_Config(SystemCoreClock / 1000);
}

void checkForBootLoaderRequest(void)
{
    void(*bootJump)(void);

    if (*((uint32_t *)0x20004FF0) == 0xDEADBEEF) {

        *((uint32_t *)0x20004FF0) = 0x0;

        __enable_irq();
        __set_MSP(*((uint32_t *)0x1FFFF000));

        bootJump = (void(*)(void))(*((uint32_t *) 0x1FFFF004));
        bootJump();
        while (1);
    }
}
