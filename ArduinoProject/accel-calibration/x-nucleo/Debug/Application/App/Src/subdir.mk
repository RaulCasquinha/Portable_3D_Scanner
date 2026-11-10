################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/App/Src/app.c \
../Application/App/Src/app_accel.c \
../Application/App/Src/app_link.c \
../Application/App/Src/app_log.c \
../Application/App/Src/app_protocol.c 

OBJS += \
./Application/App/Src/app.o \
./Application/App/Src/app_accel.o \
./Application/App/Src/app_link.o \
./Application/App/Src/app_log.o \
./Application/App/Src/app_protocol.o 

C_DEPS += \
./Application/App/Src/app.d \
./Application/App/Src/app_accel.d \
./Application/App/Src/app_link.d \
./Application/App/Src/app_log.d \
./Application/App/Src/app_protocol.d 


# Each subdirectory must supply rules for building sources it contributes
Application/App/Src/%.o Application/App/Src/%.su Application/App/Src/%.cyclo: ../Application/App/Src/%.c Application/App/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_NUCLEO_64 -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32WLxx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL345/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL345/stm32wl/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL355/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/ADXL355/stm32wl/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/AccelCalib/Inc" -I"C:/Users/raulcasquinha/Documents/Solvit/wise-sensor/applications/accel-calibration/x-nucleo/Application/App/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Application-2f-App-2f-Src

clean-Application-2f-App-2f-Src:
	-$(RM) ./Application/App/Src/app.cyclo ./Application/App/Src/app.d ./Application/App/Src/app.o ./Application/App/Src/app.su ./Application/App/Src/app_accel.cyclo ./Application/App/Src/app_accel.d ./Application/App/Src/app_accel.o ./Application/App/Src/app_accel.su ./Application/App/Src/app_link.cyclo ./Application/App/Src/app_link.d ./Application/App/Src/app_link.o ./Application/App/Src/app_link.su ./Application/App/Src/app_log.cyclo ./Application/App/Src/app_log.d ./Application/App/Src/app_log.o ./Application/App/Src/app_log.su ./Application/App/Src/app_protocol.cyclo ./Application/App/Src/app_protocol.d ./Application/App/Src/app_protocol.o ./Application/App/Src/app_protocol.su

.PHONY: clean-Application-2f-App-2f-Src

