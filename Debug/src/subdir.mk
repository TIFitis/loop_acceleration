################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/accelerate.cpp \
../src/loop_acceleration.cpp \
../src/util.cpp  \
../src/z3_formula.cpp

OBJS += \
./src/accelerate.o \
./src/loop_acceleration.o \
./src/util.o \
./src/z3_formula.o

CPP_DEPS += \
./src/accelerate.d \
./src/loop_acceleration.d \
./src/util.d \
./src/z3_formula.d


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	g++ -I$(CBMC_DIR)"/src" $(CXXFLAGS) -Wall -c -fmessage-length=0 -Wno-deprecated -Wno-deprecated-declarations -DSATCHECK_MINISAT2 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


