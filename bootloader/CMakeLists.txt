cmake_minimum_required(VERSION 3.10)

set(prj_name uds_bootloader_server)
message(STATUS "Building ${prj_name}")

set(lib_path "")

if(NOT WIN32)
    # Set the project name
    project(${prj_name} C ASM)
    set(CMAKE_AR           $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-ar)
    set(CMAKE_ASM_COMPILER $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-gcc)
    set(CMAKE_C_COMPILER   $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-gcc)
    set(CMAKE_LINKER       $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-gcc)
    set(CMAKE_OBJCOPY      $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-objcopy CACHE INTERNAL "")
    set(CMAKE_RANLIB       $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-ranlib CACHE INTERNAL "")
    set(CMAKE_SIZE         $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-size CACHE INTERNAL "")
    set(CMAKE_STRIP        $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-strip CACHE INTERNAL "")
    set(lib_path "${CMAKE_CURRENT_SOURCE_DIR}/linux_lib")
else()
    set(CMAKE_AR           arm-none-eabi-ar.exe)
    set(CMAKE_ASM_COMPILER arm-none-eabi-gcc.exe)
    set(CMAKE_C_COMPILER   arm-none-eabi-gcc.exe)
    set(CMAKE_LINKER       arm-none-eabi-gcc.exe)
    set(CMAKE_OBJCOPY      arm-none-eabi-objcopy.exe CACHE INTERNAL "")
    set(CMAKE_RANLIB       arm-none-eabi-ranlib.exe CACHE INTERNAL "")
    set(CMAKE_SIZE         arm-none-eabi-size.exe CACHE INTERNAL "")
    set(CMAKE_STRIP        arm-none-eabi-strip.exe CACHE INTERNAL "")
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
    set(lib_path "${CMAKE_CURRENT_SOURCE_DIR}/win_lib")
    # Set the project name
    project(${prj_name} C ASM)
endif()

set(cube_path "${CMAKE_CURRENT_SOURCE_DIR}/../STM32CubeC0")
set(build_flags "")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Debug build enabled")
        set(build_flags "-g3 -O0")
else()
        message(STATUS "Release")
        set(build_flags "-Os")
endif()

set(CMAKE_C_FLAGS  "-mcpu=cortex-m0plus \
                    -std=gnu11 \
                    -DUSE_HAL_DRIVER -DSTM32C092xx \
                    ${build_flags} \
                    --specs=nano.specs \
                    -ffunction-sections -fdata-sections \
                    -Wall -fstack-usage \
                    -mfloat-abi=soft -mthumb" CACHE INTERNAL "")

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/STM32C092RCTX_FLASH.ld \
                            -mcpu=cortex-m0plus \
                            --specs=nosys.specs \
                            -Wl,-Map=${prj_name}.map \
                            -Wl,--gc-sections -static \
                            -Wl,--print-memory-usage \
                            -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group" CACHE INTERNAL "")

set_property(SOURCE startup_stm32c092rctx.s APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

set(
    HAL_DRIVER_SRCS
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_fdcan.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc_ex.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_cortex.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart_ex.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_gpio.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_flash_ex.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_flash.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_tim.c"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_tim_ex.c"
)

file(GLOB ISOTP_SRCS "isotp/*.c")
file(GLOB CUBE_SRCS "cube/*.c")

add_executable(
    ${prj_name}
    main.c
    hw.c
    startup_stm32c092rctx.s
    abs_tim_config.c
    uds_config.c
    ${CUBE_SRCS}
    ${HAL_DRIVER_SRCS}
    ${ISOTP_SRCS}
)

add_custom_command(TARGET ${prj_name} POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${prj_name}> ${prj_name}.hex
	COMMENT "Generating HEX file"
)

target_include_directories(
    ${prj_name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Inc"
    "${cube_path}/Drivers/STM32C0xx_HAL_Driver/Inc/Legacy"
    "${cube_path}/Drivers/CMSIS/Include"
    "${cube_path}/Drivers/BSP/STM32C0xx_Nucleo"
    "${cube_path}/Drivers/CMSIS/Device/ST/STM32C0xx/Include"
    "cube"
    "isotp"
    "uds"
    ${lib_path}
)

target_link_libraries(${prj_name} PRIVATE ${lib_path}/libuds.a)