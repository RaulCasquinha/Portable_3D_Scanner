################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/ADXL345/stm32wl/Src/adxl345_stm32wl.c 

OBJS += \
./Application/ADXL345/stm32wl/Src/adxl345_stm32wl.o 

C_DEPS += \
./Application/ADXL345/stm32wl/Src/adxl345_stm32wl.d 


# Each subdirectory must supply rules for building sources it contributes
Application/ADXL345/stm32wl/Src/%.o Application/ADXL345/stm32wl/Src/%.su Application/ADXL345/stm32wl/Src/%.cyclo: ../Application/ADXL345/stm32wl/Src/%.c Application/ADXL345/stm32wl/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_NUCLEO_64 -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32WLxx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL345/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL345/stm32wl/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL355/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL355/stm32wl/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/AccelCalib/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/App/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Application-2f-ADXL345-2f-stm32wl-2f-Src

clean-Application-2f-ADXL345-2f-stm32wl-2f-Src:
	-$(RM) ./Application/ADXL345/stm32wl/Src/adxl345_stm32wl.cyclo ./Application/ADXL345/stm32wl/Src/adxl345_stm32wl.d ./Application/ADXL345/stm32wl/Src/adxl345_stm32wl.o ./Application/ADXL345/stm32wl/Src/adxl345_stm32wl.su

.PHONY: clean-Application-2f-ADXL345-2f-stm32wl-2f-Src

