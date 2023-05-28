################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ECXX.cpp \
../src/gpio.cpp \
../src/main.cpp \
../src/tools.cpp 

OBJS += \
./src/ECXX.o \
./src/gpio.o \
./src/main.o \
./src/tools.o 

CPP_DEPS += \
./src/ECXX.d \
./src/gpio.d \
./src/main.d \
./src/tools.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	mipsel-openwrt-linux-g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


