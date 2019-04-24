/**
  ******************************************************************************
  * @file    FPU/FPU_Fractal/CM7/Src/main.c
  * @author  MCD Application Team
  * @brief   This sample code shows how to use Floating Point Unit.
  *          main program for Cortex-M7.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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

#include "main.h"
#include "button.h"

/** @addtogroup STM32H7xx_HAL_Applications
 * @{
 */

/** @addtogroup FPU_Fractal
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define VSYNC           1  
#define VBP             1 
#define VFP             1
#define VACT            480
#define HSYNC           1
#define HBP             1
#define HFP             1
#define HACT            400

#define LEFT_AREA         1
#define RIGHT_AREA        2

#define __DSI_MASK_TE()   (GPIOJ->AFR[0] &= (0xFFFFF0FFU))   /* Mask DSI TearingEffect Pin*/
#define __DSI_UNMASK_TE() (GPIOJ->AFR[0] |= ((uint32_t)(GPIO_AF13_DSI) << 8)) /* UnMask DSI TearingEffect Pin*/

#define SCREEN_SENSTIVITY 35

/* Private variables ---------------------------------------------------------*/ 

extern LTDC_HandleTypeDef hltdc_discovery;
extern DSI_HandleTypeDef hdsi_discovery;

uint8_t pColLeft[]    = {0x00, 0x00, 0x01, 0x8F}; /*   0 -> 399 */
uint8_t pColRight[]   = {0x01, 0x90, 0x03, 0x1F}; /* 400 -> 799 */
uint8_t pPage[]       = {0x00, 0x00, 0x01, 0xDF}; /*   0 -> 479 */
uint8_t pScanCol[]    = {0x02, 0x15};             /* Scan @ 533 */

DMA2D_HandleTypeDef    DMA2D_Handle;

static __IO uint32_t pending_buffer = 0;
static __IO uint32_t active_area = 0;

DSI_CmdCfgTypeDef CmdCfg;
DSI_LPCmdTypeDef LPCmd;
DSI_PLLInitTypeDef dsiPllInit;
static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

extern LTDC_HandleTypeDef  hltdc_discovery;

/* 32-bytes Alignement is needed for cache maintenance purpose */
ALIGN_32BYTES(uint32_t L8_CLUT[ITERATION]);
ALIGN_32BYTES(uint32_t  buffer[LCD_X_SIZE * LCD_Y_SIZE / 4]);

uint8_t  text[50];

uint8_t SizeIndex = 0;
uint16_t SizeTable[][6]={{600, 252}, {500, 220},{400, 200},{300, 180},{200, 150},{160, 120}};

uint16_t XSize = 600;
uint16_t YSize = 252;

uint32_t  CurrentZoom = 100;  
uint32_t  DirectionZoom = 0; 

TS_StateTypeDef TS_State;
__IO uint8_t TouchdOn = 0;
__IO uint8_t TouchReleased = 0;
__IO uint8_t isplaying = 1;
__IO uint8_t playOneFrame = 0;
__IO  uint32_t score_fpu = 0;
__IO  uint32_t tickstart = 0; 

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
__IO  uint32_t SystemClock_MHz = 480; 
__IO  uint32_t SystemClock_changed = 0;
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */


/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void SystemClock_Config_400MHz(void);
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
static void SystemClock_Config_480MHz(void);
static void FMC_SDRAM_Clock_Config(void);
static void EXTI15_10_IRQHandler_Config(void);
static void SystemClockChange_Handler(void);
#endif
static void InitCLUT(uint32_t * clut);
static void Generate_Julia_fpu(uint16_t size_x,
                       uint16_t size_y,
                       uint16_t offset_x,
                       uint16_t offset_y,
                       uint16_t zoom,
                       uint8_t * buffer);

static void CPU_CACHE_Enable(void);
static void MPU_Config(void);

static void DMA2D_Init(uint32_t ImageWidth, uint32_t ImageHeight);
static void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t ImageWidth, uint16_t ImageHeight);
static void DMA2D_CopyButton(uint32_t Button, uint32_t *pDst);
static uint8_t LCD_Init(void);
static void LCD_LayertInit(uint16_t LayerIndex, uint32_t Address);
static void LTDC_Init(void);
static void LCD_BriefDisplay(void);
static void Touch_Handler(void);
static void print_Size(void);
void Error_Handler(void);


/* Private functions ---------------------------------------------------------*/

/**
 * @brief   Main program
 * @param  None
 * @retval None
 */

int main(void)
{
  /* System Init, System clock, voltage scaling and L1-Cache configuration are done by CPU1 (Cortex-M7) 
     in the meantime Domain D2 is put in STOP mode(Cortex-M4 in deep-sleep)
  */
  
  /* Enable the CPU Cache */
  CPU_CACHE_Enable();
  MPU_Config();  
  
  /* STM32H7xx HAL library initialization:, instruction and Data caches
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization: global MSP (MCU Support Package) initialization
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)    
  /* By default the FMC source clock is the AXI bus clock
     The maximum FMC allowed frequency is 200MHz
     when switching the system clock to 480MHz , the AXI bus clock is 240MHz:
       - In this case the FMC source clock is switched to PLL2 (DIVR) with 200MHz frequency    
  */
  FMC_SDRAM_Clock_Config();
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */


  /* When system initialization is finished, Cortex-M7 could wakeup (when needed) the Cortex-M4  by means of 
     HSEM notification or by any D2 wakeup source (SEV,EXTI..)   */  
	 
  /* Configure LED4 */
  BSP_LED_Init(LED4);

  InitCLUT(L8_CLUT);
  
  /* Clean Data Cache to update the content of the SRAM to be used bu the DAMA2D */ 
  SCB_CleanDCache_by_Addr( (uint32_t*)L8_CLUT, sizeof(L8_CLUT));
  
 /*##-1- LCD Configuration ##################################################*/ 
  /* Initialize the SDRAM */
  BSP_SDRAM_Init();
  
  /* Initialize the LCD   */
  LCD_Init(); 

  /* Disable DSI Wrapper in order to access and configure the LTDC */
  __HAL_DSI_WRAPPER_DISABLE(&hdsi_discovery);
  
  LCD_LayertInit(0, LCD_FRAME_BUFFER);
  BSP_LCD_SelectLayer(0); 
  
  /* Update pitch : the draw is done on the whole physical X Size */
  HAL_LTDC_SetPitch(&hltdc_discovery, BSP_LCD_GetXSize(), 0);

  /* Enable DSI Wrapper so DSI IP will drive the LTDC */
  __HAL_DSI_WRAPPER_ENABLE(&hdsi_discovery); 
  
  HAL_DSI_LongWrite(&hdsi_discovery, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColLeft);
  HAL_DSI_LongWrite(&hdsi_discovery, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_PASET, pPage);

  LCD_BriefDisplay();
  
  pending_buffer = 1;
  active_area = LEFT_AREA;
  
  HAL_DSI_LongWrite(&hdsi_discovery, 0, DSI_DCS_LONG_PKT_WRITE, 2, OTM8009A_CMD_WRTESCN, pScanCol);
  
  /*Wait until the DSI ends the refresh of previous frame (Left and Right area)*/  
  while(pending_buffer != 0)
  {
  }  

  /*##-2- Initialize the Touch Screen ##################################################*/
  BSP_TS_Init(LCD_X_SIZE, LCD_Y_SIZE);
  TS_State.touchDetected = 0;
  BSP_TS_ITConfig();

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)  
  EXTI15_10_IRQHandler_Config();
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */

  
  /* Init xsize and ysize used by fractal algo */
  XSize = SizeTable[SizeIndex][0];
  YSize = SizeTable[SizeIndex][1];
  
  /*print fractal image size*/
  print_Size();  
  
  /*Copy Pause/Zoom in Off/Zoom out buttons*/
  DMA2D_CopyButton(PAUSE_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);
  DMA2D_CopyButton(ZOOM_IN_OFF_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);
  DMA2D_CopyButton(ZOOM_OUT_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);  
          
  DMA2D_Init(XSize, YSize);
  
  while(1)
  {
    if((isplaying != 0) || (playOneFrame != 0))
    {
      playOneFrame = 0;

      BSP_LED_On(LED4);
      
      tickstart = HAL_GetTick();
      /* Start generating the fractal */
      Generate_Julia_fpu(XSize,
                         YSize,
                         XSize / 2,
                         YSize / 2,
                         CurrentZoom,
                         (uint8_t *)buffer);
      
      /* Get elapsed time */
      score_fpu = (uint32_t)(HAL_GetTick() - tickstart);

      /* Clean Data Cache to update the content of the SRAM to be used bu the DAMA2D */ 
      SCB_CleanDCache_by_Addr( (uint32_t*)buffer, sizeof(buffer));      

      BSP_LED_Off(LED4);
      
      sprintf((char*)text, "%lu ms",score_fpu);
      BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
      BSP_LCD_DisplayStringAt(600 + 32, 370 + 68 , (uint8_t *)"         ", LEFT_MODE); 
      BSP_LCD_DisplayStringAt(600 + 32, 370 + 68 , (uint8_t *)text, LEFT_MODE);  
      
      sprintf((char*)text, "Zoom : %lu",CurrentZoom);
      BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
      BSP_LCD_DisplayStringAt(600 + 8 , 370 + 8 , (uint8_t *)"           ", LEFT_MODE); 
      BSP_LCD_DisplayStringAt(600 + 8 , 370 + 8 , (uint8_t *)text, LEFT_MODE);  
      
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
      sprintf((char*)text,"System Clock = %lu MHz",SystemClock_MHz);
      BSP_LCD_DisplayStringAt(2, 50, (uint8_t *)text, CENTER_MODE);
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */			
      
      
      if(DirectionZoom == 0)
      {
        if (CurrentZoom < 1000) 
        {
          CurrentZoom += 20;
        }
        else
        {
          DirectionZoom = 1;
        }
      }
      
      if(DirectionZoom == 1)
      {
        if (CurrentZoom > 100) 
        {
          CurrentZoom -= 20;
        }
        else
        {
          DirectionZoom = 0;
        }
      }

      /* Copy Result image to the display frame buffer*/      
      DMA2D_CopyBuffer((uint32_t *)buffer, (uint32_t *)LCD_FRAME_BUFFER, XSize, YSize);
    }

    Touch_Handler();
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
    SystemClockChange_Handler();
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */

  }
}

static void Touch_Handler(void)
{
  if(TouchReleased != 0)  
  {
    TouchReleased = 0;
    
    /*************************Pause/Play buttons *********************/
    if((TS_State.touchX[0] + SCREEN_SENSTIVITY >= PLAY_PAUSE_BUTTON_XPOS) && \
      (TS_State.touchX[0] <= (PLAY_PAUSE_BUTTON_XPOS + PLAY_PAUSE_BUTTON_WIDTH + SCREEN_SENSTIVITY)) && \
        (TS_State.touchY[0] + SCREEN_SENSTIVITY >= PLAY_PAUSE_BUTTON_YPOS) && \
          (TS_State.touchY[0] <= (PLAY_PAUSE_BUTTON_YPOS + PLAY_PAUSE_BUTTON_HEIGHT + SCREEN_SENSTIVITY)))
    {
      isplaying = 1 - isplaying;
      
      if(isplaying == 0)
      {
        DMA2D_CopyButton(PLAY_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);
      }
      else
      {
        DMA2D_CopyButton(PAUSE_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);
      }
      
      DMA2D_Init(XSize, YSize);
    }
    /*************************Zoom In button *********************/
    else if((TS_State.touchX[0] + SCREEN_SENSTIVITY >= ZOOM_IN_BUTTON_XPOS) && \
      (TS_State.touchX[0] <= (ZOOM_IN_BUTTON_XPOS + ZOOM_BUTTON_WIDTH + SCREEN_SENSTIVITY)) && \
        (TS_State.touchY[0] + SCREEN_SENSTIVITY >= ZOOM_IN_BUTTON_YPOS) && \
          (TS_State.touchY[0] <= (ZOOM_IN_BUTTON_YPOS + ZOOM_BUTTON_HEIGHT + SCREEN_SENSTIVITY)))
    {
      if (SizeIndex > 0) 
      {
        SizeIndex--;
        XSize = SizeTable[SizeIndex][0];
        YSize = SizeTable[SizeIndex][1];

        if(SizeIndex == 0)
        {
          /*zoom in limit reached */
          DMA2D_CopyButton(ZOOM_IN_OFF_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);
        }
        else if(SizeIndex == 4)
        {
          /* zoom out limit unreached display zoom out button */
          DMA2D_CopyButton(ZOOM_OUT_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);  
        }
        
        /*Clear Fractal Display area */
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        BSP_LCD_FillRect(2, 112 + 1, 800 - 4, 254);
        
        BSP_LCD_SetBackColor(0xFF0080FF);
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);        
        
        print_Size();
        CurrentZoom = 100;
        
        playOneFrame = 1;
        DMA2D_Init(XSize, YSize);
      }  
    }
    /*************************Zoom Out button *********************/
    else if((TS_State.touchX[0] + SCREEN_SENSTIVITY >= ZOOM_OUT_BUTTON_XPOS) && \
      (TS_State.touchX[0] <= (ZOOM_OUT_BUTTON_XPOS + ZOOM_BUTTON_WIDTH + SCREEN_SENSTIVITY)) && \
        (TS_State.touchY[0] + SCREEN_SENSTIVITY >= ZOOM_OUT_BUTTON_YPOS) && \
          (TS_State.touchY[0] <= (ZOOM_OUT_BUTTON_YPOS + ZOOM_BUTTON_HEIGHT + SCREEN_SENSTIVITY)))
    {
      if (SizeIndex < 5) 
      {
        SizeIndex++;
        XSize = SizeTable[SizeIndex][0];
        YSize = SizeTable[SizeIndex][1];
        
        if(SizeIndex == 5)
        {
          /* zoom out limit reached */
          DMA2D_CopyButton(ZOOM_OUT_OFF_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);
        }
        else if (SizeIndex == 1)
        {
          /*zoom in limit unreached  Display zoom in button */
          DMA2D_CopyButton(ZOOM_IN_BUTTON,(uint32_t *)LCD_FRAME_BUFFER);          
        }          
        
        /*Clear Fractal Display area */
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        BSP_LCD_FillRect(2, 112 + 1, 800 - 4, 254);
        
        BSP_LCD_SetBackColor(0xFF0080FF);
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        
        print_Size();
        CurrentZoom = 100;
        
        playOneFrame = 1;
        DMA2D_Init(XSize, YSize);
      }
    }
  }    
}

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
static void SystemClockChange_Handler(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
  if(SystemClock_changed != 0)  
  {        
    /* Select HSE  as system clock source to allow modification of the PLL configuration */
    RCC_ClkInitStruct.ClockType       = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_HSE;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
      /* Initialization Error */
      Error_Handler();
    }
    
    
    if(SystemClock_MHz == 400)
    {
      __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);  
      while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
      
      SystemClock_Config_480MHz();
      SystemClock_MHz = 480; 				
    }
    else
    {
      SystemClock_Config_400MHz();    
      
      __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  
      while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

      SystemClock_MHz = 400;				
    }

    /* PLL1  as system clock source to allow modification of the PLL configuration */
    RCC_ClkInitStruct.ClockType       = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_PLLCLK;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
      /* Initialization Error */
      Error_Handler();
    }
    
    CurrentZoom = 100;
    playOneFrame = 1;
    
    SystemClock_changed = 0;
    
  }
}
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */
/**
  * @brief  EXTI line detection callbacks
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == TS_INT_PIN)
  {
    BSP_TS_GetState(&TS_State);
    
    if(TS_State.touchDetected != 0) /*Touch Detected */
    {
      TouchdOn = 1;
      TS_State.touchDetected = 0;
    }
    else if(TouchdOn == 1) /*TouchReleased */
    {        
      TouchdOn = 0;
      TouchReleased = 1;
    }
  }
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
  else if (GPIO_Pin == WAKEUP_BUTTON_PIN)
  {
    SystemClock_changed = 1;
  }
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */
}

#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
/**
  * @brief  Configures EXTI lines 15 to 10 (connected to PC.13 pin) in interrupt mode
  * @param  None
  * @retval None
  */
static void EXTI15_10_IRQHandler_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;
  
  /* Enable GPIOC clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PC.13 pin as the EXTI input event line in interrupt mode for both CPU1 and CPU2*/
  GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;    /* current CPU (CM7) config in IT rising */
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* Enable and set EXTI lines 15 to 10 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0xF, 0xF);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */

static void print_Size(void)
{
  while(pending_buffer != 0)
  {
  } 
  
  sprintf((char*)text, "%lu x %lu",(uint32_t)XSize,(uint32_t)YSize);


  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAt(64, 370 + 24 , (uint8_t *)"Size", LEFT_MODE); 
  BSP_LCD_DisplayStringAt(24, 370 + 48 , (uint8_t *)text, LEFT_MODE);

  /* Frame Buffer updated , unmask the DSI TE pin to ask for a DSI refersh*/ 
  pending_buffer = 1;
  /* UnMask the TE */
  __DSI_UNMASK_TE();
  
  while(pending_buffer != 0)
  {
  }   
  
}

static void InitCLUT(uint32_t * clut)
{
  uint32_t  red = 0, green = 0, blue = 0;
  uint32_t  i = 0x00;
  
  /* Color map generation */
  for (i = 0; i < ITERATION; i++)
  {
    /* Generate red, green and blue values */
    red = (i * 8 * 256 / ITERATION) % 256;
    green = (i * 6 * 256 / ITERATION) % 256;
    blue = (i * 4 * 256 / ITERATION) % 256;
    
    red = red << 16;
    green = green << 8;
    
    /* Store the 32-bit value */
    clut[i] = 0xFF000000 | (red + green + blue);
  }
}

static void Generate_Julia_fpu(uint16_t size_x, uint16_t size_y, uint16_t offset_x, uint16_t offset_y, uint16_t zoom, uint8_t * buffer)
{
  float       tmp1, tmp2;
  float       num_real, num_img;
  float       rayon;
  
  uint8_t       i;
  uint16_t      x, y;
  
  for (y = 0; y < size_y; y++)
  {
    for (x = 0; x < size_x; x++)
    {
      num_real = y - offset_y;
      num_real = num_real / zoom;
      num_img = x - offset_x;
      num_img = num_img / zoom;
      i = 0;
      rayon = 0;
      while ((i < ITERATION - 1) && (rayon < 4))
      {
        tmp1 = num_real * num_real;
        tmp2 = num_img * num_img;
        num_img = 2 * num_real * num_img + IMG_CONSTANT;
        num_real = tmp1 - tmp2 + REAL_CONSTANT;
        rayon = tmp1 + tmp2;
        i++;
      }
      /* Store the value in the buffer */
      buffer[x+y*size_x] = i;
    }
  }
}

static void LCD_BriefDisplay(void)
{
  char message[64];	
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(0xFF0080FF);
  BSP_LCD_SetTextColor(0xFF0080FF);
  BSP_LCD_FillRect(2, 2, 800 - 4, 112 - 2);  
  
  /*Title*/
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  sprintf(message, " STM32H7xx Fractal Benchmark  : %s", SCORE_FPU_MODE);
  BSP_LCD_DisplayStringAt(2, 24, (uint8_t *)message, CENTER_MODE);
  
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)  
  sprintf((char*)message,"System Clock = %lu MHz",SystemClock_MHz);
  BSP_LCD_DisplayStringAt(2, 50, (uint8_t *)message, CENTER_MODE);
  BSP_LCD_DisplayStringAt(2, 72, (uint8_t *)"Press Wakeup button to switch the System Clock", CENTER_MODE);  

#else
  BSP_LCD_DisplayStringAt(2, 50, (uint8_t *)"System Clock = 400MHz", CENTER_MODE);  
#endif
  
  /*Fractal Display area */
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_FillRect(2, 112 + 1, 800 - 4, 254);
   

  
  /*image Size*/  
  BSP_LCD_SetBackColor(0xFF0080FF);
  BSP_LCD_SetTextColor(0xFF0080FF);  
  BSP_LCD_FillRect(2, 370, 200 - 2, 112 - 4);

  /*Calculation Time*/      
  BSP_LCD_SetBackColor(0xFF0080FF);
  BSP_LCD_SetTextColor(0xFF0080FF); 
  BSP_LCD_FillRect(202, 370, 400 - 2, 112 - 4);
  
  BSP_LCD_SetBackColor(0xFF0080FF);
  BSP_LCD_SetTextColor(0xFF0080FF);   
  BSP_LCD_FillRect(602, 370, 200 - 4, 112 - 4);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAt(600 + 32, 370 + 48 , (uint8_t *)"Duration:", LEFT_MODE);   

}

/**
  * @brief  Initializes the DSI LCD. 
  * The ititialization is done as below:
  *     - DSI PLL ititialization
  *     - DSI ititialization
  *     - LTDC ititialization
  *     - OTM8009A LCD Display IC Driver ititialization
  * @param  None
  * @retval LCD state
  */
static uint8_t LCD_Init(void){
  
  GPIO_InitTypeDef GPIO_Init_Structure;
  DSI_PHY_TimerTypeDef  PhyTimings;  
  
  /* Toggle Hardware Reset of the DSI LCD using
     its XRES signal (active low) */
  BSP_LCD_Reset();
  
  /* Call first MSP Initialize only in case of first initialization
  * This will set IP blocks LTDC, DSI and DMA2D
  * - out of reset
  * - clocked
  * - NVIC IRQ related to IP blocks enabled
  */
  BSP_LCD_MspInit();

  /* LCD clock configuration */
  /* LCD clock configuration */
  /* PLL3_VCO Input = HSE_VALUE/PLL3M = 5 Mhz */
  /* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 800 Mhz */
  /* PLLLCDCLK = PLL3_VCO Output/PLL3R = 800/19 = 42 Mhz */
  /* LTDC clock frequency = PLLLCDCLK = 42 Mhz */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLL3.PLL3M = 5;    
  PeriphClkInitStruct.PLL3.PLL3N = 160;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 2;  
  PeriphClkInitStruct.PLL3.PLL3R = 19;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);   
  
  /* Base address of DSI Host/Wrapper registers to be set before calling De-Init */
  hdsi_discovery.Instance = DSI;
  
  HAL_DSI_DeInit(&(hdsi_discovery));
  
  dsiPllInit.PLLNDIV  = 100;
  dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
  dsiPllInit.PLLODF  = DSI_PLL_OUT_DIV1;  

  hdsi_discovery.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
  hdsi_discovery.Init.TXEscapeCkdiv = 0x4;
  
  
  HAL_DSI_Init(&(hdsi_discovery), &(dsiPllInit));
    
  /* Configure the DSI for Command mode */
  CmdCfg.VirtualChannelID      = 0;
  CmdCfg.HSPolarity            = DSI_HSYNC_ACTIVE_HIGH;
  CmdCfg.VSPolarity            = DSI_VSYNC_ACTIVE_HIGH;
  CmdCfg.DEPolarity            = DSI_DATA_ENABLE_ACTIVE_HIGH;
  CmdCfg.ColorCoding           = DSI_RGB888;
  CmdCfg.CommandSize           = HACT;
  CmdCfg.TearingEffectSource   = DSI_TE_EXTERNAL;
  CmdCfg.TearingEffectPolarity = DSI_TE_RISING_EDGE;
  CmdCfg.VSyncPol              = DSI_VSYNC_FALLING;
  CmdCfg.AutomaticRefresh      = DSI_AR_DISABLE;
  CmdCfg.TEAcknowledgeRequest  = DSI_TE_ACKNOWLEDGE_DISABLE;
  HAL_DSI_ConfigAdaptedCommandMode(&hdsi_discovery, &CmdCfg);
  
  LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_ENABLE;
  LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_ENABLE;
  LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_ENABLE;
  LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_ENABLE;
  LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_ENABLE;
  LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_ENABLE;
  LPCmd.LPGenLongWrite        = DSI_LP_GLW_ENABLE;
  LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_ENABLE;
  LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_ENABLE;
  LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_ENABLE;
  LPCmd.LPDcsLongWrite        = DSI_LP_DLW_ENABLE;
  HAL_DSI_ConfigCommand(&hdsi_discovery, &LPCmd);

  /* Initialize LTDC */
  LTDC_Init();
  
  /* Start DSI */
  HAL_DSI_Start(&(hdsi_discovery));

  /* Configure DSI PHY HS2LP and LP2HS timings */
  PhyTimings.ClockLaneHS2LPTime = 35;
  PhyTimings.ClockLaneLP2HSTime = 35;
  PhyTimings.DataLaneHS2LPTime = 35;
  PhyTimings.DataLaneLP2HSTime = 35;
  PhyTimings.DataLaneMaxReadTime = 0;
  PhyTimings.StopWaitTime = 10;
  HAL_DSI_ConfigPhyTimer(&hdsi_discovery, &PhyTimings);  
    
  /* Initialize the OTM8009A LCD Display IC Driver (KoD LCD IC Driver)
  */
  OTM8009A_Init(OTM8009A_COLMOD_RGB888, LCD_ORIENTATION_LANDSCAPE);
  
  LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_DISABLE;
  LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_DISABLE;
  LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_DISABLE;
  LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_DISABLE;
  LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_DISABLE;
  LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_DISABLE;
  LPCmd.LPGenLongWrite        = DSI_LP_GLW_DISABLE;
  LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_DISABLE;
  LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_DISABLE;
  LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_DISABLE;
  LPCmd.LPDcsLongWrite        = DSI_LP_DLW_DISABLE;
  HAL_DSI_ConfigCommand(&hdsi_discovery, &LPCmd);
  
  HAL_DSI_ConfigFlowControl(&hdsi_discovery, DSI_FLOW_CONTROL_BTA);
  HAL_DSI_ForceRXLowPower(&hdsi_discovery, ENABLE);  
  
  
  /* Enable GPIOJ clock */
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  
  /* Configure DSI_TE pin from MB1166 : Tearing effect on separated GPIO from KoD LCD */
  /* that is mapped on GPIOJ2 as alternate DSI function (DSI_TE)                      */
  /* This pin is used only when the LCD and DSI link is configured in command mode    */
  GPIO_Init_Structure.Pin       = GPIO_PIN_2;
  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull      = GPIO_NOPULL;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
  GPIO_Init_Structure.Alternate = GPIO_AF13_DSI;
  HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);   
    
  return LCD_OK;
}

/**
  * @brief  Initialize the LTDC
  * @param  None
  * @retval None
  */
static void LTDC_Init(void)
{
  /* DeInit */
  HAL_LTDC_DeInit(&hltdc_discovery);
  
  /* LTDC Config */
  /* Timing and polarity */
  hltdc_discovery.Init.HorizontalSync = HSYNC;
  hltdc_discovery.Init.VerticalSync = VSYNC;
  hltdc_discovery.Init.AccumulatedHBP = HSYNC+HBP;
  hltdc_discovery.Init.AccumulatedVBP = VSYNC+VBP;
  hltdc_discovery.Init.AccumulatedActiveH = VSYNC+VBP+VACT;
  hltdc_discovery.Init.AccumulatedActiveW = HSYNC+HBP+HACT;
  hltdc_discovery.Init.TotalHeigh = VSYNC+VBP+VACT+VFP;
  hltdc_discovery.Init.TotalWidth = HSYNC+HBP+HACT+HFP;
  
  /* background value */
  hltdc_discovery.Init.Backcolor.Blue = 0;
  hltdc_discovery.Init.Backcolor.Green = 0;
  hltdc_discovery.Init.Backcolor.Red = 0;
  
  /* Polarity */
  hltdc_discovery.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc_discovery.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc_discovery.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc_discovery.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc_discovery.Instance = LTDC;

  HAL_LTDC_Init(&hltdc_discovery);
}

/**
  * @brief  Initializes the LCD layers.
  * @param  LayerIndex: Layer foreground or background
  * @param  FB_Address: Layer frame buffer
  * @retval None
  */
static void LCD_LayertInit(uint16_t LayerIndex, uint32_t Address)
{
  LCD_LayerCfgTypeDef  Layercfg;

  /* Layer Init */
  Layercfg.WindowX0 = 0;
  Layercfg.WindowX1 = BSP_LCD_GetXSize()/2 ;
  Layercfg.WindowY0 = 0;
  Layercfg.WindowY1 = BSP_LCD_GetYSize(); 
  Layercfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  Layercfg.FBStartAdress = Address;
  Layercfg.Alpha = 255;
  Layercfg.Alpha0 = 0;
  Layercfg.Backcolor.Blue = 0;
  Layercfg.Backcolor.Green = 0;
  Layercfg.Backcolor.Red = 0;
  Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  Layercfg.ImageWidth = BSP_LCD_GetXSize() / 2;
  Layercfg.ImageHeight = BSP_LCD_GetYSize();
  
  HAL_LTDC_ConfigLayer(&hltdc_discovery, &Layercfg, LayerIndex); 
}

void BSP_LCD_MspInit(void)
{
  /** @brief Enable the LTDC clock */
  __HAL_RCC_LTDC_CLK_ENABLE();

  /** @brief Toggle Sw reset of LTDC IP */
  __HAL_RCC_LTDC_FORCE_RESET();
  __HAL_RCC_LTDC_RELEASE_RESET();

  /** @brief Enable the DMA2D clock */
  __HAL_RCC_DMA2D_CLK_ENABLE();

  /** @brief Toggle Sw reset of DMA2D IP */
  __HAL_RCC_DMA2D_FORCE_RESET();
  __HAL_RCC_DMA2D_RELEASE_RESET();

  /** @brief Enable DSI Host and wrapper clocks */
  __HAL_RCC_DSI_CLK_ENABLE();

  /** @brief Soft Reset the DSI Host and wrapper */
  __HAL_RCC_DSI_FORCE_RESET();
  __HAL_RCC_DSI_RELEASE_RESET();

  /** @brief NVIC configuration for LTDC interrupt that is now enabled */
  HAL_NVIC_SetPriority(LTDC_IRQn, 9, 0xf);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);

  /** @brief NVIC configuration for DMA2D interrupt that is now enabled */
  HAL_NVIC_SetPriority(DMA2D_IRQn, 9, 0xf);
  HAL_NVIC_EnableIRQ(DMA2D_IRQn);

  /** @brief NVIC configuration for DSI interrupt that is now enabled */
  HAL_NVIC_SetPriority(DSI_IRQn, 9, 0xf);
  HAL_NVIC_EnableIRQ(DSI_IRQn);
}

/**
  * @brief  Initialize the DMA2D in memory to memory with PFC.
  * @param  ImageWidth: image width
  * @param  ImageHeight: image Height 
  * @retval None
  */
static void DMA2D_Init(uint32_t ImageWidth, uint32_t ImageHeight)
{
  DMA2D_CLUTCfgTypeDef CLUTCfg;
  
  /* Init DMA2D */
  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/ 
  DMA2D_Handle.Init.Mode         = DMA2D_M2M_PFC;
  DMA2D_Handle.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
  DMA2D_Handle.Init.OutputOffset = LCD_X_SIZE - ImageWidth;     
  
  /*##-2- DMA2D Callbacks Configuration ######################################*/
  DMA2D_Handle.XferCpltCallback  = NULL;
  
  /*##-3- Foreground Configuration ###########################################*/
  DMA2D_Handle.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  DMA2D_Handle.LayerCfg[1].InputAlpha = 0xFF;

  DMA2D_Handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_L8;
  DMA2D_Handle.LayerCfg[1].InputOffset = 0;
  
  DMA2D_Handle.Instance          = DMA2D; 

  /*##-4- DMA2D Initialization     ###########################################*/  
  HAL_DMA2D_Init(&DMA2D_Handle);
  HAL_DMA2D_ConfigLayer(&DMA2D_Handle, 1);
  
  /* Load DMA2D Foreground CLUT */
  CLUTCfg.CLUTColorMode = DMA2D_CCM_ARGB8888;     
  CLUTCfg.pCLUT = (uint32_t *)L8_CLUT;
  CLUTCfg.Size = 255;
  
  HAL_DMA2D_CLUTLoad(&DMA2D_Handle, CLUTCfg, 1);
  HAL_DMA2D_PollForTransfer(&DMA2D_Handle, 100);  
}

/**
  * @brief  Copy the Decoded image to the display Frame buffer.
  * @param  pSrc: Pointer to source buffer
  * @param  pDst: Pointer to destination buffer
  * @param  ImageWidth: image width
  * @param  ImageHeight: image Height 
  * @retval None
  */
static void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t ImageWidth, uint16_t ImageHeight)
{
  
  uint32_t xPos, yPos, destination;       
  
  /*##-1- calculate the destination transfer address  ############*/
  xPos = (LCD_X_SIZE - ImageWidth)/2;
  yPos = ((LCD_Y_SIZE - ImageHeight)/2);  
  
  destination = (uint32_t)pDst + (yPos * LCD_X_SIZE + xPos) * 4;
 
  /*##-2- Wait until the DSI ends the refresh of previous frame (Left and Right area)##*/  
  while(pending_buffer != 0)
  {
  }
  

  HAL_DMA2D_Start(&DMA2D_Handle, (uint32_t)pSrc, destination, ImageWidth, ImageHeight);
  HAL_DMA2D_PollForTransfer(&DMA2D_Handle, 100);

  /* Frame Buffer updated , unmask the DSI TE pin to ask for a DSI refersh*/ 
  pending_buffer = 1;
  /* UnMask the TE */
  __DSI_UNMASK_TE();
  
  while(pending_buffer != 0)
  {
  }  

}

static void DMA2D_CopyButton(uint32_t Button, uint32_t *pDst)
{
  uint32_t xPos, yPos, destination, buttonWidth;
  uint32_t *pSrc;

  while(pending_buffer != 0)
  {
  }  
  
  if(PLAY_BUTTON == Button)
  {
    xPos = PLAY_PAUSE_BUTTON_XPOS;
    yPos = PLAY_PAUSE_BUTTON_YPOS;
    
    buttonWidth = PLAY_PAUSE_BUTTON_WIDTH;
    
    pSrc = (uint32_t *)Play_Button;
  }
  else if(PAUSE_BUTTON == Button)
  {
    xPos = PLAY_PAUSE_BUTTON_XPOS;
    yPos = PLAY_PAUSE_BUTTON_YPOS;

    buttonWidth = PLAY_PAUSE_BUTTON_WIDTH;
    
    pSrc = (uint32_t *)Pause_Button;
  }
  else if(ZOOM_IN_BUTTON == Button)
  {
    xPos = ZOOM_IN_BUTTON_XPOS;
    yPos = ZOOM_IN_BUTTON_YPOS;

    buttonWidth = ZOOM_BUTTON_WIDTH;    
    
    pSrc = (uint32_t *)ZOOM_IN_Button;
  }
  else if(ZOOM_OUT_BUTTON == Button)
  {
    xPos = ZOOM_OUT_BUTTON_XPOS;
    yPos = ZOOM_OUT_BUTTON_YPOS;
    
    buttonWidth = ZOOM_BUTTON_WIDTH;      
    
    pSrc = (uint32_t *)ZOOM_Out_Button;
  }
  else if(ZOOM_IN_OFF_BUTTON == Button)
  {
    xPos = ZOOM_IN_BUTTON_XPOS;
    yPos = ZOOM_IN_BUTTON_YPOS;

    buttonWidth = ZOOM_BUTTON_WIDTH;    
    
    pSrc = (uint32_t *)ZOOM_IN_Off_Button;
  }
  else if(ZOOM_OUT_OFF_BUTTON == Button)
  {
    xPos = ZOOM_OUT_BUTTON_XPOS;
    yPos = ZOOM_OUT_BUTTON_YPOS;

    buttonWidth = ZOOM_BUTTON_WIDTH;    
    
    pSrc = (uint32_t *)ZOOM_Out_Off_Button;
  }  
  
  /* Init DMA2D */
  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/ 
  DMA2D_Handle.Init.Mode         = DMA2D_M2M_PFC;
  DMA2D_Handle.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
  DMA2D_Handle.Init.OutputOffset = LCD_X_SIZE - buttonWidth;     
  
  /*##-2- DMA2D Callbacks Configuration ######################################*/
  DMA2D_Handle.XferCpltCallback  = NULL;
  
  /*##-3- Foreground Configuration ###########################################*/
  DMA2D_Handle.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  DMA2D_Handle.LayerCfg[1].InputAlpha = 0xFF;
  DMA2D_Handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  DMA2D_Handle.LayerCfg[1].InputOffset = 0;
  
  DMA2D_Handle.Instance          = DMA2D; 

  /*##-4- DMA2D Initialization     ###########################################*/
  HAL_DMA2D_Init(&DMA2D_Handle);
  HAL_DMA2D_ConfigLayer(&DMA2D_Handle, 1);     
  

  
  destination = (uint32_t)pDst + (yPos * LCD_X_SIZE + xPos) * 4;
  
  HAL_DMA2D_Start(&DMA2D_Handle, (uint32_t)pSrc, destination, PLAY_PAUSE_BUTTON_WIDTH, PLAY_PAUSE_BUTTON_HEIGHT);
  HAL_DMA2D_PollForTransfer(&DMA2D_Handle, 100);

  /* Frame Buffer updated , unmask the DSI TE pin to ask for a DSI refersh*/ 
  pending_buffer = 1;
  /* UnMask the TE */
  __DSI_UNMASK_TE();
  
  while(pending_buffer != 0)
  {
  } 
  
}

/**
  * @brief  Tearing Effect DSI callback.
  * @param  hdsi: pointer to a DSI_HandleTypeDef structure that contains
  *               the configuration information for the DSI.
  * @retval None
  */
void HAL_DSI_TearingEffectCallback(DSI_HandleTypeDef *hdsi)
{
  /* Mask the TE */
  __DSI_MASK_TE();
  
  /* Refresh the right part of the display */
  HAL_DSI_Refresh(hdsi);   
}

/**
  * @brief  End of Refresh DSI callback.
  * @param  hdsi: pointer to a DSI_HandleTypeDef structure that contains
  *               the configuration information for the DSI.
  * @retval None
  */
void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi)
{
  if(pending_buffer > 0)
  {
    if(active_area == LEFT_AREA)
    {     
      /* Disable DSI Wrapper */
      __HAL_DSI_WRAPPER_DISABLE(hdsi);
      /* Update LTDC configuaration */
      LTDC_LAYER(&hltdc_discovery, 0)->CFBAR = LCD_FRAME_BUFFER + 400 * 4;
      __HAL_LTDC_RELOAD_CONFIG(&hltdc_discovery);
      /* Enable DSI Wrapper */
      __HAL_DSI_WRAPPER_ENABLE(hdsi);
      
      HAL_DSI_LongWrite(hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColRight);
      /* Refresh the right part of the display */
      HAL_DSI_Refresh(hdsi);    
      
    }
    else if(active_area == RIGHT_AREA)
    {

      /* Disable DSI Wrapper */
      __HAL_DSI_WRAPPER_DISABLE(&hdsi_discovery);
      /* Update LTDC configuaration */
      LTDC_LAYER(&hltdc_discovery, 0)->CFBAR = LCD_FRAME_BUFFER;
      __HAL_LTDC_RELOAD_CONFIG(&hltdc_discovery);
      /* Enable DSI Wrapper */
      __HAL_DSI_WRAPPER_ENABLE(&hdsi_discovery);
      
      HAL_DSI_LongWrite(hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColLeft); 
      pending_buffer = 0; 

      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
    }
    
      active_area = (active_area == LEFT_AREA)? RIGHT_AREA : LEFT_AREA; 
  }

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
  
#if (USE_VOS0_480MHZ_OVERCLOCK == 1)

#if (BOARD_HW_CONFIG_IS_LDO == 1) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 0)
  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);   
#else
  #error "Please make sure that the STM32H747I-DISCO Board has been modified to match the LDO configuration then set the define BOARD_HW_CONFIG_IS_LDO to 1 and BOARD_HW_CONFIG_IS_DIRECT_SMPS set to 0 to confirm the HW config"
#endif  /* (BOARD_HW_CONFIG_IS_LDO == 1) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 0) */

#else

#if (BOARD_HW_CONFIG_IS_LDO == 0) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 1)
  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
#else
  #error "Please make sure that the STM32H747I-DISCO Board has been modified to match the Direct SMPS configuration then set the define BOARD_HW_CONFIG_IS_LDO to 0 and BOARD_HW_CONFIG_IS_DIRECT_SMPS set to 1 to confirm the HW config"
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


/**
  * @brief  Configure the FMC/SDRAM source clock to PLL2 at 200MHz.
  * @param  None
  * @retval None
  */
static void FMC_SDRAM_Clock_Config(void)
{
  RCC_PeriphCLKInitTypeDef  RCC_PeriphCLKInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /* PLL2_VCO Input = HSE_VALUE/PLL2_M = 5 Mhz */
  /* PLL2_VCO Output = PLL2_VCO Input * PLL_N = 800 Mhz */
  /* FMC Kernel Clock = PLL2_VCO Output/PLL_R = 800/4 = 200 Mhz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC;
  RCC_PeriphCLKInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
  RCC_PeriphCLKInitStruct.PLL2.PLL2RGE = RCC_PLL1VCIRANGE_2;
  RCC_PeriphCLKInitStruct.PLL2.PLL2M = 5;
  RCC_PeriphCLKInitStruct.PLL2.PLL2N = 160;
  RCC_PeriphCLKInitStruct.PLL2.PLL2FRACN = 0;
  RCC_PeriphCLKInitStruct.PLL2.PLL2P = 2;
  RCC_PeriphCLKInitStruct.PLL2.PLL2R = 4;
  RCC_PeriphCLKInitStruct.PLL2.PLL2Q = 4;
  RCC_PeriphCLKInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}
#endif /* USE_VOS0_480MHZ_OVERCLOCK */


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
  * @brief  Configure the MPU attributes as Write Through for External SDRAM.
  * @note   The Base Address is SDRAM_DEVICE_ADDR .
  *         The Configured Region Size is 32MB because same as SDRAM size.
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WT for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = SDRAM_DEVICE_ADDR;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
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
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  BSP_LED_On(LED3);
  while(1);
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
 * @}
 */

/**
 * @}
 */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
