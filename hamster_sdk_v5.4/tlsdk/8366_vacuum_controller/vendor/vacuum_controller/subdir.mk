################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/vacuum_controller/main.c \
../vendor/vacuum_controller/vacuum_controller.c \
../vendor/vacuum_controller/vacuum_controller_batt.c \
../vendor/vacuum_controller/vacuum_controller_button.c \
../vendor/vacuum_controller/vacuum_controller_emi.c \
../vendor/vacuum_controller/vacuum_controller_rf.c \
../vendor/vacuum_controller/vacuum_device_info.c 

OBJS += \
./vendor/vacuum_controller/main.o \
./vendor/vacuum_controller/vacuum_controller.o \
./vendor/vacuum_controller/vacuum_controller_batt.o \
./vendor/vacuum_controller/vacuum_controller_button.o \
./vendor/vacuum_controller/vacuum_controller_emi.o \
./vendor/vacuum_controller/vacuum_controller_rf.o \
./vendor/vacuum_controller/vacuum_device_info.o 


# Each subdirectory must supply rules for building sources it contributes
vendor/vacuum_controller/%.o: ../vendor/vacuum_controller/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: TC32 Compiler'
	tc32-elf-gcc -ffunction-sections -fdata-sections -D__PROJECT_VACUUM__=1 -D__PROJECT_VACUUM_8366__=1 -Wall -Os -fpack-struct -fshort-enums -finline-small-functions -std=gnu99 -fshort-wchar -fms-extensions -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


