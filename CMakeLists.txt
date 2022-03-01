
cmake_minimum_required(VERSION 3.15.3)

project(stm32f103Rtos)

enable_language(C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS -faligned-new)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
# SETUP ARM TOOLCHAIN 
if(MINGW OR CYGWIN OR WIN32)
    set(UTIL_SEARCH_CMD where)
elseif(UNIX OR APPLE)
    set(UTIL_SEARCH_CMD which)
endif()

set(TOOLCHAIN_PREFIX arm-none-eabi-)

execute_process(
  COMMAND ${UTIL_SEARCH_CMD} ${TOOLCHAIN_PREFIX}gcc
  OUTPUT_VARIABLE BINUTILS_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
get_filename_component(ARM_TOOLCHAIN_DIR ${BINUTILS_PATH} DIRECTORY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)

set(CMAKE_OBJCOPY ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}size CACHE INTERNAL "size tool")

set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# set up source and include directories 

set(DRIVER_SRC Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx)
set(DRIVER_INC Drivers/STM32F1xx_HAL_Driver/Inc)
set(CORE_SRC Core/Src)
set(CORE_INC Core/Inc)
set(CORE_STARTUP Core/Startup)
set(DRIVER_CMSIS_INC Drivers/CMSIS/Device/ST/STM32F1xx/Include ) 
set(CMSIS_INC Drivers/CMSIS/Include )
set(FREERTOS Middlewares/Third_Party/FreeRTOS/Source )
set(FREERTOS_INC Middlewares/Third_Party/FreeRTOS/Source/include)
set(FREERTOS_INC2 Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3) 
set(TINYCBOR ../tinycbor)
set(COMP ../components)

set(STM32CUBEMX_GENERATED_FILES
        ${DRIVER_SRC}_hal_adc.c
        ${DRIVER_SRC}_hal_cortex.c  
        ${DRIVER_SRC}_hal_exti.c      
        ${DRIVER_SRC}_hal_gpio.c     
        ${DRIVER_SRC}_hal_iwdg.c  
        ${DRIVER_SRC}_hal_rcc_ex.c  
        ${DRIVER_SRC}_hal_uart.c
        ${DRIVER_SRC}_hal_adc_ex.c  
        ${DRIVER_SRC}_hal_crc.c     
        ${DRIVER_SRC}_hal_flash.c     
        ${DRIVER_SRC}_hal_gpio_ex.c  
        ${DRIVER_SRC}_hal_pwr.c   
        ${DRIVER_SRC}_hal_tim.c
        ${DRIVER_SRC}_hal.c         
        ${DRIVER_SRC}_hal_dma.c     
        ${DRIVER_SRC}_hal_flash_ex.c  
        ${DRIVER_SRC}_hal_i2c.c      
        ${DRIVER_SRC}_hal_rcc.c   
        ${DRIVER_SRC}_hal_tim_ex.c

        ${CORE_INC}/main.h
        ${CORE_INC}/stm32f1xx_hal_conf.h
        ${CORE_INC}/stm32f1xx_it.h

        ${CORE_SRC}/main.c
        ${CORE_SRC}/stm32f1xx_hal_msp.c
        ${CORE_SRC}/stm32f1xx_it.c
        ${CORE_SRC}/system_stm32f1xx.c
        ${CORE_SRC}/app_main.cpp
        ${CORE_SRC}/LedBlinker.cpp
        ${CORE_SRC}/Uart.cpp
        ${CORE_SRC}/syscalls.c
        ${CORE_SRC}/freertos.c
        ${CORE_SRC}/sysmem.c

        ${COMP}/src/CborWriter.cpp
        ${COMP}/src/CborReader.cpp
        ${COMP}/src/Spine.cpp
        ${COMP}/src/StringUtility.cpp
        ${COMP}/src/Frame.cpp
        ${COMP}/src/frame.cpp
        ${COMP}/src/Crc32.cpp
        ${COMP}/stm32/Log.cpp 
        ${COMP}/stm32/Sys.cpp
        ${COMP}/stm32/limero.cpp

        ${TINYCBOR}/src/cborencoder.c                          
        ${TINYCBOR}/src/cborerrorstrings.c       
        ${TINYCBOR}/src/cborparser_float.c  
        ${TINYCBOR}/src/cbortojson.c
        ${TINYCBOR}/src/cborencoder_close_container_checked.c  
        ${TINYCBOR}/src/cborparser.c             
        ${TINYCBOR}/src/cborpretty.c        
        ${TINYCBOR}/src/cborvalidation.c
        ${TINYCBOR}/src/cborencoder_float.c                    
        ${TINYCBOR}/src/cborparser_dup_string.c  
        ${TINYCBOR}/src/cborpretty_stdio.c  
        ${TINYCBOR}/src/open_memstream.c

        ${FREERTOS}/croutine.c
        ${FREERTOS}/event_groups.c
        ${FREERTOS}/list.c
        ${FREERTOS}/queue.c
        ${FREERTOS}/tasks.c
        ${FREERTOS}/timers.c
        ${FREERTOS}/stream_buffer.c
        ${FREERTOS}/portable/GCC/ARM_CM3/port.c
        ${FREERTOS}/portable/MemMang/heap_4.c
        ${FREERTOS}/CMSIS_RTOS/cmsis_os.c


        ${CORE_STARTUP}/startup_stm32f103c8tx.s)

set(EXECUTABLE ${PROJECT_NAME}.out)

add_executable(${EXECUTABLE} ${STM32CUBEMX_GENERATED_FILES})

target_compile_definitions(${EXECUTABLE} PRIVATE
        -DUSE_DRIVER_SRC
        -DSTM32F413xx
        )

target_include_directories(${EXECUTABLE} PRIVATE
        ${CORE_INC}
        ${DRIVER_INC}
        ${CMSIS_INC}
        ${DRIVER_CMSIS_INC}
        ${FREERTOS}/CMSIS_RTOS
        ${FREERTOS_INC}
        ${FREERTOS_INC2}
        ${TINYCBOR}/src
        ${COMP}/inc
        )

target_compile_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m3
        -mthumb
        -fdata-sections
        -ffunction-sections
        -DSTM32F103xB
        -Wall
        -O3
        -DSTM32_OPENCM3
        $<$<CONFIG:Debug>:-Og>
        )

target_link_options(${EXECUTABLE} PRIVATE
        -T${CMAKE_SOURCE_DIR}/STM32F103C8TX_FLASH.ld
        -mcpu=cortex-m3
        -mthumb
        -specs=nano.specs
        -lc
        -lm
        -lnosys
        -Wl,-Map=${PROJECT_NAME}.map,--cref
        -Wl,--print-memory-usage
        -Wl,--gc-sections
        )

# Print executable size
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND arm-none-eabi-size ${EXECUTABLE})

# Create hex file
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
        COMMAND arm-none-eabi-objcopy -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin)
