################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/DoIP/doip_client.c" \
"../Libraries/DoIP/doip_link.c" \
"../Libraries/DoIP/doip_message.c" \
"../Libraries/DoIP/uds_handler.c" 

COMPILED_SRCS += \
"Libraries/DoIP/doip_client.src" \
"Libraries/DoIP/doip_link.src" \
"Libraries/DoIP/doip_message.src" \
"Libraries/DoIP/uds_handler.src" 

C_DEPS += \
"./Libraries/DoIP/doip_client.d" \
"./Libraries/DoIP/doip_link.d" \
"./Libraries/DoIP/doip_message.d" \
"./Libraries/DoIP/uds_handler.d" 

OBJS += \
"Libraries/DoIP/doip_client.o" \
"Libraries/DoIP/doip_link.o" \
"Libraries/DoIP/doip_message.o" \
"Libraries/DoIP/uds_handler.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/DoIP/doip_client.src":"../Libraries/DoIP/doip_client.c" "Libraries/DoIP/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/DoIP/doip_client.o":"Libraries/DoIP/doip_client.src" "Libraries/DoIP/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/DoIP/doip_link.src":"../Libraries/DoIP/doip_link.c" "Libraries/DoIP/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/DoIP/doip_link.o":"Libraries/DoIP/doip_link.src" "Libraries/DoIP/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/DoIP/doip_message.src":"../Libraries/DoIP/doip_message.c" "Libraries/DoIP/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/DoIP/doip_message.o":"Libraries/DoIP/doip_message.src" "Libraries/DoIP/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/DoIP/uds_handler.src":"../Libraries/DoIP/uds_handler.c" "Libraries/DoIP/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/DoIP/uds_handler.o":"Libraries/DoIP/uds_handler.src" "Libraries/DoIP/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-DoIP

clean-Libraries-2f-DoIP:
	-$(RM) ./Libraries/DoIP/doip_client.d ./Libraries/DoIP/doip_client.o ./Libraries/DoIP/doip_client.src ./Libraries/DoIP/doip_link.d ./Libraries/DoIP/doip_link.o ./Libraries/DoIP/doip_link.src ./Libraries/DoIP/doip_message.d ./Libraries/DoIP/doip_message.o ./Libraries/DoIP/doip_message.src ./Libraries/DoIP/uds_handler.d ./Libraries/DoIP/uds_handler.o ./Libraries/DoIP/uds_handler.src

.PHONY: clean-Libraries-2f-DoIP

