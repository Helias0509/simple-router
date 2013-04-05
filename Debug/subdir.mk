################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../sha1.o \
../sr_arpcache.o \
../sr_dumper.o \
../sr_if.o \
../sr_main.o \
../sr_router.o \
../sr_rt.o \
../sr_utils.o \
../sr_vns_comm.o 

C_SRCS += \
../sha1.c \
../sr_arpcache.c \
../sr_dumper.c \
../sr_if.c \
../sr_main.c \
../sr_router.c \
../sr_rt.c \
../sr_utils.c \
../sr_vns_comm.c 

OBJS += \
./sha1.o \
./sr_arpcache.o \
./sr_dumper.o \
./sr_if.o \
./sr_main.o \
./sr_router.o \
./sr_rt.o \
./sr_utils.o \
./sr_vns_comm.o 

C_DEPS += \
./sha1.d \
./sr_arpcache.d \
./sr_dumper.d \
./sr_if.d \
./sr_main.d \
./sr_router.d \
./sr_rt.d \
./sr_utils.d \
./sr_vns_comm.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


