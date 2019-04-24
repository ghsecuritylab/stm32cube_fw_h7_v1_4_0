/**
  ******************************************************************************
  * @file    Demonstration/STM32H745-Discovery_Demo/CM7/Src/main_menu.c
  * @author  MCD Application Team
  * @brief   This example describes how to use STM32H7 to create a digital oscilloscope
    able to display and detect signals with frequency up to 2 MHz.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main_menu.h"
#include "image_menu.h"
#include "signals.h"
#include <stdio.h>
#include <string.h>


/** @addtogroup STM32H7xx_HAL_Demonstration
  * @{
  */

/** @addtogroup MainMenu
  * @{
  */


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DEMO_OSCILLO_ADDRESS  0x08120000 
#define DEMO_COREMARK_ADDRESS 0x08040000 

/* Private functions ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);
static void CPU_CACHE_Disable(void);
static void Error_Handler(void);

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
static void SystemClock_Config_480MHz(void);
#else
static void SystemClock_Config_400MHz(void);
#endif

static void InitMainMenu(void);
void MenuSelection(void);
void SystemInformationsSlide(void);
/* Private typedef -----------------------------------------------------------*/
typedef  void (*pFunction)(void);
static uint32_t DisplyMainMenu;
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
/* Private variable ------------------------------------------------------------*/
BackgroundWidgetTypeDef MenuImages[9] = 
{
  {
    {
      240,  /* Width */
      56, /* Height */
      220,  /* Xpos */ 
      2,  /* Ypos */
      DMA2D_INPUT_ARGB8888,   /* ColorMode */
      NULL,                   /* Oscillo */
    },
    (uint32_t*)logo,
  },
  {
    {
      90,  /* Width */
      90, /* Height */
      60,  /* Xpos */ 
      110,  /* Ypos */
      DMA2D_INPUT_ARGB8888,   /* ColorMode */
      NULL,                   /* Oscillo */
    },
    (uint32_t*)oscillo,
  },
  {
    {
      90,  /* Width */
      90, /* Height */
      200,  /* Xpos */ 
      110,  /* Ypos */
      DMA2D_INPUT_ARGB8888,   /* ColorMode */
      NULL,                   /* Eembc */
    },
    (uint32_t*)eembc,
  },
  {
    {
      90,  /* Width */
      90, /* Height */
      340,  /* Xpos */ 
      110,  /* Ypos */
      DMA2D_INPUT_ARGB8888,   /* ColorMode */
      NULL,                   /* System Information */
    },
    (uint32_t*)info,
  },
  {
    {
      100,  /* Width */
      140, /* Height */
      16,  /* Xpos */ 
      70,  /* Ypos */
      DMA2D_INPUT_RGB888,   /* ColorMode */
      NULL,                 /* Board */
    },
    (uint32_t*)info1,
  },
  {
    {
      100,  /* Width */
      140, /* Height */
      132,  /* Xpos */ 
      70,  /* Ypos */
      DMA2D_INPUT_RGB888,   /* ColorMode */
      NULL,                   /* MCU */
    },
    (uint32_t*)info2,
  },
  {
    {
      100,  /* Width */
      140, /* Height */
      248,  /* Xpos */ 
      70,  /* Ypos */
      DMA2D_INPUT_RGB888,   /* ColorMode */
      NULL,                   /* CPU Speed */
    },
    (uint32_t*)info3,
  },
  {
    {
      100,  /* Width */
      140, /* Height */
      364,  /* Xpos */ 
      70,  /* Ypos */
      DMA2D_INPUT_RGB888,   /* ColorMode */
      NULL,                   /* Firmware Version */
    },
    (uint32_t*)info4,
  },

  {
    {
      46,  /* Width */
      44, /* Height */
      430,  /* Xpos */ 
      0,  /* Ypos */
      DMA2D_INPUT_ARGB8888,   /* ColorMode */
      NULL,                   /* Return button */
    },
    (uint32_t*)return_button,
  },
};

uint32_t * Oscillo_signature = (uint32_t *)SIGNATURE_OSCILLO_LOCATION;
uint32_t * Coremark_signature = (uint32_t *)SIGNATURE_COREMARK_LOCATION;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{  
  
  /*After Wakeup from Standby Mode , return to Oscilloscope Demo */
  if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB_D1) != 0)
  {
    /* Clear Standby flag */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB_D1);
    
   if ( (*Oscillo_signature & SIGNATURE_OSCILLO_MASK) == SIGNATURE_OSCILLO_VALUE)
   {
    /* Initialize user application's Stack Pointer & Jump to user application */
    JumpToApplication = (pFunction) (*(__IO uint32_t*) (DEMO_OSCILLO_ADDRESS + 4));
    __set_MSP(*(__IO uint32_t*) DEMO_OSCILLO_ADDRESS);
    JumpToApplication();
   }
   else
   {
     Error_Handler();
   }
     
  }
  
  MPU_Config();
  
  /* Enable the CPU Cache */
  CPU_CACHE_Enable();
  
  /* STM32H7xx HAL library initialization:
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
   */
  HAL_Init();

  /*  Configure the system */
  SystemClock_Config();  

  /* Initialize LED1 */
  BSP_LED_Init(LED1);
  BSP_LED_On(LED1);

  BSP_QSPI_Init();
  BSP_QSPI_EnableMemoryMappedMode();
  DisplyMainMenu = 0; 
  InitMainMenu();
  /* Infinite loop */
  while (1)
  {
    if (DisplyMainMenu == 1)
    {
      /* Clear the LCD */
      BSP_LCD_Clear(LCD_COLOR_BLACK);
      
      BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
      BSP_LCD_FillRect(0,0,480,60);
      /* Display background image */
      DisplayMenuImages(MenuImages,4); 
      /* Display Demo title */
      
      BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
      BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
      BSP_LCD_SetFont(&Font24);
      BSP_LCD_DisplayStringAt(30, 16,(uint8_t *) "STM32H745", LEFT_MODE);

      BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
      BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
      BSP_LCD_SetFont(&Font16);
      BSP_LCD_DisplayStringAt(32, 36,(uint8_t *) "Discovery Kit", LEFT_MODE);
      
      BSP_LCD_SetTextColor(LCD_COLOR_GRAY);
      BSP_LCD_SetBackColor(LCD_COLOR_BLACK);

      BSP_LCD_SetFont(&Font12);
      BSP_LCD_DisplayStringAt(2, 256,(uint8_t *) "Copyright (c) STMicroelectronics 2018", RIGHT_MODE);
      DisplyMainMenu = 0;
    }

    MenuSelection();

    BSP_LED_Toggle(LED1);
  }  
}

/**
  * @brief  Init the main generator menu
  * @param  None
  * @retval None
  */
static void InitMainMenu(void)
{

  /* Init the touch screen */
  /* Touchscreen initialization */
  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());


  /* Init the Display */  
  InitMenuDisplay();
  
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(0,0,480,60);  

  /* Display background image */
  DisplayMenuImages(MenuImages,4); 
  /* Display Demo title */

  BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_DisplayStringAt(30, 16,(uint8_t *) "STM32H745", LEFT_MODE);
  
  BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAt(32, 36,(uint8_t *) "Discovery Kit", LEFT_MODE);

  BSP_LCD_SetTextColor(LCD_COLOR_GRAY);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(2, 256,(uint8_t *) "Copyright (c) STMicroelectronics 2018", RIGHT_MODE);
 
}


void MenuSelection(void)
{
  uint16_t x, y;
  TS_StateTypeDef  TS_State = {0};
  
  
  /* Check the touch screen coordinates */
  BSP_TS_GetState(&TS_State);
  
  if(TS_State.touchDetected)
  {
    /* One or dual touch have been detected          */
    /* Only take into account the first touch so far */
    
    /* Get X and Y position of the first touch post calibrated */
    x = TS_State.touchX[0];
    y = TS_State.touchY[0];
    
    if ((x > 320) && (x < 430) && (y > 110) && (y < 200) )
    {
      /* Info Slide */
      SystemInformationsSlide();
    }  
    
    
    if ((x > 60) && (x < 150) && (y > 110) && (y < 200) )
    {
      if ( (*Oscillo_signature & SIGNATURE_OSCILLO_MASK) == SIGNATURE_OSCILLO_VALUE)
      {
        /* go to demo 1 (Oscillo) */
        BSP_LCD_DeInit();
        CPU_CACHE_Disable();
        
        /* Initialize user application's Stack Pointer & Jump to user application */
        JumpToApplication = (pFunction) (*(__IO uint32_t*) (DEMO_OSCILLO_ADDRESS + 4));
        __set_MSP(*(__IO uint32_t*) DEMO_OSCILLO_ADDRESS);
        JumpToApplication();
      }
      else
      {
        BSP_LCD_SetTextColor(LCD_COLOR_RED);
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        
        BSP_LCD_SetFont(&Font12);
        BSP_LCD_DisplayStringAt(0, 50, (uint8_t *)" Please Load Oscilloscope binary! ", CENTER_MODE);
      }
   
    }
    if ((x>200) && (x <290) && (y > 110) && (y < 200) )
    {
      if ( (*Coremark_signature & SIGNATURE_COREMARK_MASK) == SIGNATURE_COREMARK_VALUE)
      {
        /* go to demo 2 (CoreMark)*/
        BSP_LCD_DeInit();
        CPU_CACHE_Disable();
        /* Initialize user application's Stack Pointer & Jump to user application */
        JumpToApplication = (pFunction) (*(__IO uint32_t*) (DEMO_COREMARK_ADDRESS + 4));
        __set_MSP(*(__IO uint32_t*) DEMO_COREMARK_ADDRESS);
        JumpToApplication();
      }
      else
      {
        BSP_LCD_SetTextColor(LCD_COLOR_RED);
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        
        BSP_LCD_SetFont(&Font12);
        BSP_LCD_DisplayStringAt(0, 50,(uint8_t *) "   Please Load Coremark binary!   ", CENTER_MODE);

      }
    }    
  }

}

void SystemInformationsSlide(void)
{
  
  TS_StateTypeDef  TS_State = {0};
  /* Clear the LCD */
  BSP_LCD_Clear(0x00000000);
  
  /* Display background image */
  DisplayMenuImages(&MenuImages[4],5); 
  
  /* Display Slide title */
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_DisplayStringAt(0, 16,(uint8_t *) "System Information", CENTER_MODE);
  
  BSP_LCD_SetTextColor(LCD_COLOR_GRAY);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(2, 256,(uint8_t *) "Copyright (c) STMicroelectronics 2018", RIGHT_MODE);
  
  BSP_TS_GetState(&TS_State);
  
  while((TS_State.touchDetected == 0) || ((TS_State.touchX[0]<430)||(TS_State.touchY[0] > 50)) )
  {
    TS_State.touchDetected = 0;
    BSP_TS_GetState(&TS_State);
    HAL_Delay(10);
  }
  DisplyMainMenu = 1; 
}


/**
  * @brief  System Clock Configuration
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
  
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)

#if (BOARD_HW_CONFIG_IS_LDO == 1) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 0)
  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);   
#else
  #error "Please make sure that the STM32H745I-DISCO Board has been modified to match the LDO configuration then set the define BOARD_HW_CONFIG_IS_LDO to 1 and BOARD_HW_CONFIG_IS_DIRECT_SMPS set to 0 to confirm the HW config"
#endif  /* (BOARD_HW_CONFIG_IS_LDO == 1) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 0) */

#else

#if (BOARD_HW_CONFIG_IS_LDO == 0) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 1)
  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
#else
  #error "Please make sure that the STM32H745I-DISCO Board has been modified to match the Direct SMPS configuration then set the define BOARD_HW_CONFIG_IS_LDO to 0 and BOARD_HW_CONFIG_IS_DIRECT_SMPS set to 1 to confirm the HW config"
#endif  /* (BOARD_HW_CONFIG_IS_LDO == 0) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 1) */  

#endif /* USE_VOS0_480MHZ_OVERCLOCK */

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
  
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);  
  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  SystemClock_Config_480MHz();
#else
  SystemClock_Config_400MHz();
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */
  
/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
  /* By default the FMC source clock is the AXI bus clock
     The maximum FMC allowed frequency is 200MHz
     when switching the system clock to 480MHz , the AXI bus clock is 240MHz:
       - In this case the FMC source clock is switched to PLL2 (DIVR) with 200MHz frequency    
  */

  /* PLL2_VCO Input = HSE_VALUE/PLL2_M = 5 Mhz */
  /* PLL2_VCO Output = PLL2_VCO Input * PLL_N = 800 Mhz */
  /* FMC Kernel Clock = PLL2_VCO Output/PLL_R = 800/4 = 200 Mhz */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC;
  PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL1VCIRANGE_2;
  PeriphClkInitStruct.PLL2.PLL2M = 5;
  PeriphClkInitStruct.PLL2.PLL2N = 160;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 4;
  PeriphClkInitStruct.PLL2.PLL2Q = 4;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  ret = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
#endif

/* Select PLL2 As an independant clock source generator for ADC 1 (PLL2 output = 36 MHz) */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 5;
  PeriphClkInitStruct.PLL2.PLL2N = 160;
  PeriphClkInitStruct.PLL2.PLL2P = 22;
  PeriphClkInitStruct.PLL2.PLL2Q = 4;
  PeriphClkInitStruct.PLL2.PLL2R = 4;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

 /*
  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
          (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)       
          The I/O Compensation Cell activation  procedure requires :
        - The activation of the CSI clock
        - The activation of the SYSCFG clock
        - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR
 */
 
  /*activate CSI clock mondatory for I/O Compensation Cell*/  
  __HAL_RCC_CSI_ENABLE() ;
    
  /* Enable SYSCFG clock mondatory for I/O Compensation Cell */
  __HAL_RCC_SYSCFG_CLK_ENABLE() ;
  
  /* Enables the I/O Compensation Cell */    
  HAL_EnableCompensationCell();  
}

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)

/**
  * @brief  System Clock Configuration to 480MHz
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 480000000 (CM7 CPU Clock)
  *            HCLK(Hz)                       = 240000000 (CM4 CPU, AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  120MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  120MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  120MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  120MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 192
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  * @param  None
  * @retval None
  */
static void SystemClock_Config_480MHz(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}

#else
/**
  * @brief  System Clock Configuration to 400MHz
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (CM7 CPU Clock)
  *            HCLK(Hz)                       = 200000000 (CM4 CPU, AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  * @param  None
  * @retval None
  */
static void SystemClock_Config_400MHz(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}

#endif /* USE_VOS0_480MHZ_OVERCLOCK */

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WT for SRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = SDRAM_DEVICE_ADDR;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Disable(void)
{
  /* Enable I-Cache */
  SCB_DisableICache();

  /* Enable D-Cache */
  SCB_DisableDCache();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED REDon */
  BSP_LED_On(LED_RED);
  while(1)
  {
  }
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
