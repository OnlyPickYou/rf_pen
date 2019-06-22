################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/pktsniffer/main.c \
../vendor/pktsniffer/main_pktsniffer.c 

OBJS += \
./vendor/pktsniffer/main.o \
./vendor/pktsniffer/main_pktsniffer.o 


# Each subdirectory must supply rules for building sources it contributes
vendor/pktsniffer/%.o: ../vendor/pktsniffer/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: TC32 Compiler'
	tc32-elf-gcc -ffunction-sections -fdata-sections -D__PROJECT_PKT_SNIFFER__=1 -Wall -O2 -fpack-struct -fshort-enums -finline-small-functions -std=gnu99 -fshort-wchar -fms-extensions -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


