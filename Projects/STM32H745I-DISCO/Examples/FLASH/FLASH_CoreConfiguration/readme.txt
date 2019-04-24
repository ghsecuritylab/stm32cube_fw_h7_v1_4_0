/**
  @page Templates  Description of the Templates FLASH_CoreConfiguration example

  @verbatim
  ******************** (C) COPYRIGHT 2019 STMicroelectronics *******************
  * @file    FLASH/FLASH_CoreConfiguration/readme.txt
  * @author  MCD Application Team
  * @brief   Description the FLASH CoreConfiguration example.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  @endverbatim

@par Example Description

Guide through the configuration steps to copy a dedicated program by CPU1 (Cortex-CM7)
in Flash Bank2, to be executed by CPU2 (Cortex-CM4).

This example is based in the following sequence:
System Init, System clock configuration, erase Flash Bank2 and program dedicated code to CPU2 are done by CPU1(Cortex-M7) seen as the master CPU.
Once done, CPU2 (Cortex-M4) will be released through hold boot function (an RCC feature)

This projects is configured for STM32H745xx devices using STM32CubeH7 HAL and running on
STM32H745I-DISCO board from STMicroelectronics.

The SystemClock_Config() function is used to set the Flash latency and  to configure the system clock :
  - The Cortex-M7 at 400MHz
  - Cortex-M4 at 200MHz.
  - The HCLK for D1 Domain AXI/AHB3 peripherals , D2 Domain AHB1/AHB2 peripherals
    and D3 Domain AHB4  peripherals at 200MHz.
  - The APB clock dividers for D1 Domain APB3 peripherals, D2 Domain APB1/APB2 peripherals
    and D3 Domain APB4 peripherals to run at 100MHz.


For Cortex-M7 L1-Cache configuration, two functions used to configure MPU feature and to enable the CPU Cache,
respectively MPU_Config() and CPU_CACHE_Enable().
These functions are provided as template implementation that User may integrate in his application,
to enhance the performance in case of use of AXI interface with several masters.

@note For Cortex-M7, Some code parts (for instance ISR routines, vector table, critical routines )
      can be executed from the ITCM-RAM (64 KB) allowing zero wait state access.
      Which decreases critical task execution time compared to code execution from internal Flash memory.
      This configuratuion can be done using '#pragma location = ".itcmram"' to be placed above function declaration,
      or using the toolchain GUI (file options) to execute a whole source file in the ITCM-RAM.
      For fast data access, critical variables, application RAM, heap, stack could be put as well in the DTCM-RAM (128 KB).

@note Care must be taken when using HAL_Delay(), this function provides accurate delay (in milliseconds)
      based on variable incremented in SysTick ISR. This implies that if HAL_Delay() is called from
      a peripheral ISR process, then the SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.

@note The application needs to ensure that the SysTick time base is always set to 1 millisecond
      to have correct HAL operation.

@par Directory contents

  - FLASH/FLASH_CoreConfiguration/Common/Src/system_stm32h7xx.c     STM32H7xx system configuration file

  - FLASH/FLASH_CoreConfiguration/CM7/Src/main.c                 Main program for Cortex-M7
  - FLASH/FLASH_CoreConfiguration/CM7/Src/stm32h7xx_it.c         Interrupt handlers for Cortex-M7
  - FLASH/FLASH_CoreConfiguration/CM7/Src/stm32h7xx_hal_msp.c    HAL MSP module for Cortex-M7
  - FLASH/FLASH_CoreConfiguration/CM7/Inc/main.h                 Main program header file for Cortex-M7
  - FLASH/FLASH_CoreConfiguration/CM7/Inc/stm32h7xx_hal_conf.h   HAL Configuration file for Cortex-M7
  - FLASH/FLASH_CoreConfiguration/CM7/Inc/stm32h7xx_it.h         Interrupt handlers header file for Cortex-M7

  - FLASH/FLASH_CoreConfiguration/CM4/Src/main.c                 Main program for Cortex-M4
  - FLASH/FLASH_CoreConfiguration/CM4/Src/stm32h7xx_it.c         Interrupt handlers for Cortex-M4
  - FLASH/FLASH_CoreConfiguration/CM4/Src/stm32h7xx_hal_msp.c    HAL MSP module for Cortex-M4
  - FLASH/FLASH_CoreConfiguration/CM4/Inc/main.h                 Main program header file for Cortex-M4
  - FLASH/FLASH_CoreConfiguration/CM4/Inc/stm32h7xx_hal_conf.h   HAL Configuration file for Cortex-M4
  - FLASH/FLASH_CoreConfiguration/CM4/Inc/stm32h7xx_it.h         Interrupt handlers header file for Cortex-M4


@par Hardware and Software environment

  - This example runs on STM32H745xx devices.

  - This example has been tested with STM32H745I-Discovery rev.A board with SMPS (SD Convertor) power supply config and can be
    easily tailored to any other supported device and development board.
  - LED1 toggle when process  is complete.
  - In case of error, LED2 is toggling each 100 ms.

@par How to use it ?

In order to make the program work, you must do the following :
 - Open your preferred toolchain
 - For each target configuration (STM32H745I_DISCO_CM7 and STM32H745I_DISCO_CM4) :
     - Rebuild all files
     - Load images into target memory (CM4 program will be copied by CM7 program)
 - After loading the CM7 image, you have to reset the board in order to boot (Cortex-M7) and CPU2 (Cortex-M4) at once.
 - Run the example



 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
