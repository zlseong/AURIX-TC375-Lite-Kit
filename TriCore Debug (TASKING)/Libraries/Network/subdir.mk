################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/Network/udp_link.c" 

COMPILED_SRCS += \
"Libraries/Network/udp_link.src" 

C_DEPS += \
"./Libraries/Network/udp_link.d" 

OBJS += \
"Libraries/Network/udp_link.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/Network/udp_link.src":"../Libraries/Network/udp_link.c" "Libraries/Network/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Network/udp_link.o":"Libraries/Network/udp_link.src" "Libraries/Network/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-Network

clean-Libraries-2f-Network:
	-$(RM) ./Libraries/Network/udp_link.d ./Libraries/Network/udp_link.o ./Libraries/Network/udp_link.src

.PHONY: clean-Libraries-2f-Network

