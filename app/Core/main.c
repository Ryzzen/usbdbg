#include "debug.h"
#include "usb_host_app.h"

/* Global define */

/* Global Variable */

/**
 * @brief Main function
 *
 * @return none
 */
int main(void) {
  RCC_ClocksTypeDef RCC_ClocksStatus = {0};

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  Delay_Init();
  Delay_Ms(100);
  USART_Printf_Init(115200);
  printf("SystemClk: %dHz\r\n", SystemCoreClock);

  SystemCoreClockUpdate();
  printf("SystemClk: %dHz\r\n", SystemCoreClock);

  RCC_GetClocksFreq(&RCC_ClocksStatus);
  printf("SYSCLK_Frequency: %dHz\r\n", RCC_ClocksStatus.SYSCLK_Frequency);
  printf("HCLK_Frequency: %dHz\r\n", RCC_ClocksStatus.HCLK_Frequency);
  printf("PCLK1_Frequency: %dHz\r\n", RCC_ClocksStatus.PCLK1_Frequency);
  printf("PCLK2_Frequency: %dHz\r\n", RCC_ClocksStatus.PCLK2_Frequency);

  USBH_HostInit();

  while (1) {
    USBH_App();
  }
}
