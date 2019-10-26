################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/accelerate.cpp \
../src/loop_acceleration.cpp \
../src/util.cpp 

OBJS += \
./src/accelerate.o \
./src/loop_acceleration.o \
./src/util.o 

CPP_DEPS += \
./src/accelerate.d \
./src/loop_acceleration.d \
./src/util.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	@g++ -I$(CBMC_DIR)"/src" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-deprecated -Wno-deprecated-declarations -DSATCHECK_MINISAT2 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


