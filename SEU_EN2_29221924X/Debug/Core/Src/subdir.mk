################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/cJSON.c \
../Core/Src/cJSON_Utils.c \
../Core/Src/freertos.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/tareas.c \
../Core/Src/task_COMM.c \
../Core/Src/task_CONSOLE.c \
../Core/Src/task_EJER3.c \
../Core/Src/task_TIME.c \
../Core/Src/utility_buff.c 

OBJS += \
./Core/Src/cJSON.o \
./Core/Src/cJSON_Utils.o \
./Core/Src/freertos.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/tareas.o \
./Core/Src/task_COMM.o \
./Core/Src/task_CONSOLE.o \
./Core/Src/task_EJER3.o \
./Core/Src/task_TIME.o \
./Core/Src/utility_buff.o 

C_DEPS += \
./Core/Src/cJSON.d \
./Core/Src/cJSON_Utils.d \
./Core/Src/freertos.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/tareas.d \
./Core/Src/task_COMM.d \
./Core/Src/task_CONSOLE.d \
./Core/Src/task_EJER3.d \
./Core/Src/task_TIME.d \
./Core/Src/utility_buff.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG '-DPPB_PRJ="SEU_EN2_29221924X.elf"' -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/cJSON.cyclo ./Core/Src/cJSON.d ./Core/Src/cJSON.o ./Core/Src/cJSON.su ./Core/Src/cJSON_Utils.cyclo ./Core/Src/cJSON_Utils.d ./Core/Src/cJSON_Utils.o ./Core/Src/cJSON_Utils.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/tareas.cyclo ./Core/Src/tareas.d ./Core/Src/tareas.o ./Core/Src/tareas.su ./Core/Src/task_COMM.cyclo ./Core/Src/task_COMM.d ./Core/Src/task_COMM.o ./Core/Src/task_COMM.su ./Core/Src/task_CONSOLE.cyclo ./Core/Src/task_CONSOLE.d ./Core/Src/task_CONSOLE.o ./Core/Src/task_CONSOLE.su ./Core/Src/task_EJER3.cyclo ./Core/Src/task_EJER3.d ./Core/Src/task_EJER3.o ./Core/Src/task_EJER3.su ./Core/Src/task_TIME.cyclo ./Core/Src/task_TIME.d ./Core/Src/task_TIME.o ./Core/Src/task_TIME.su ./Core/Src/utility_buff.cyclo ./Core/Src/utility_buff.d ./Core/Src/utility_buff.o ./Core/Src/utility_buff.su

.PHONY: clean-Core-2f-Src

