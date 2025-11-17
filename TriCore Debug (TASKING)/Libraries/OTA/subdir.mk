# Automatically-generated file. Do not edit!

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../Libraries/OTA/ota_manager.c \
../Libraries/OTA/zone_package.c

COMPILED_SRCS += \
./Libraries/OTA/ota_manager.src \
./Libraries/OTA/zone_package.src

C_DEPS += \
./Libraries/OTA/ota_manager.d \
./Libraries/OTA/zone_package.d

OBJS += \
./Libraries/OTA/ota_manager.o \
./Libraries/OTA/zone_package.o


# Each subdirectory must supply rules for building sources it contributes
Libraries/OTA/%.src: ../Libraries/OTA/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: TASKING C/C++ Compiler'
	cctc -cs --dep-file="$(@:.src=.d)" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Libraries/OTA/%.o: ./Libraries/OTA/%.src
	@echo 'Building file: $<'
	@echo 'Invoking: TASKING Assembler'
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

