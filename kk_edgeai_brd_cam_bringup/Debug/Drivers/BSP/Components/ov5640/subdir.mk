################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/Components/ov5640/ov5640.c \
../Drivers/BSP/Components/ov5640/ov5640_reg.c 

OBJS += \
./Drivers/BSP/Components/ov5640/ov5640.o \
./Drivers/BSP/Components/ov5640/ov5640_reg.o 

C_DEPS += \
./Drivers/BSP/Components/ov5640/ov5640.d \
./Drivers/BSP/Components/ov5640/ov5640_reg.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/ov5640/%.o Drivers/BSP/Components/ov5640/%.su Drivers/BSP/Components/ov5640/%.cyclo: ../Drivers/BSP/Components/ov5640/%.c Drivers/BSP/Components/ov5640/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U585xx -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ka-gu/STM32CubeIDE/workspace_2.0.0/stm32u5_custom_pcb_fw/kk_edgeai_brd_cam_bringup/Drivers/BSP/Components/ov5640" -I"C:/Users/ka-gu/STM32CubeIDE/workspace_2.0.0/stm32u5_custom_pcb_fw/kk_edgeai_brd_cam_bringup/Drivers/BSP/B-U585I-IOT02A" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-Components-2f-ov5640

clean-Drivers-2f-BSP-2f-Components-2f-ov5640:
	-$(RM) ./Drivers/BSP/Components/ov5640/ov5640.cyclo ./Drivers/BSP/Components/ov5640/ov5640.d ./Drivers/BSP/Components/ov5640/ov5640.o ./Drivers/BSP/Components/ov5640/ov5640.su ./Drivers/BSP/Components/ov5640/ov5640_reg.cyclo ./Drivers/BSP/Components/ov5640/ov5640_reg.d ./Drivers/BSP/Components/ov5640/ov5640_reg.o ./Drivers/BSP/Components/ov5640/ov5640_reg.su

.PHONY: clean-Drivers-2f-BSP-2f-Components-2f-ov5640

