################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../CDataPool.cpp \
../CLogUtil.cpp \
../CSDLDisplay.cpp \
../CVideoCamera.cpp \
../CVideoReceiver.cpp \
../main.cpp 

OBJS += \
./CDataPool.o \
./CLogUtil.o \
./CSDLDisplay.o \
./CVideoCamera.o \
./CVideoReceiver.o \
./main.o 

CPP_DEPS += \
./CDataPool.d \
./CLogUtil.d \
./CSDLDisplay.d \
./CVideoCamera.d \
./CVideoReceiver.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


