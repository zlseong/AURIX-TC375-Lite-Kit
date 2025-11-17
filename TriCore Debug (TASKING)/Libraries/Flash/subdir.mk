################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/Flash/FlashBankManager.c" \
"../Libraries/Flash/external_flash.c"

COMPILED_SRCS += \
"Libraries/Flash/FlashBankManager.src" \
"Libraries/Flash/external_flash.src"

C_DEPS += \
"./Libraries/Flash/FlashBankManager.d" \
"./Libraries/Flash/external_flash.d"

OBJS += \
"Libraries/Flash/FlashBankManager.o" \
"Libraries/Flash/external_flash.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/Flash/FlashBankManager.src":"../Libraries/Flash/FlashBankManager.c" "Libraries/Flash/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Flash/FlashBankManager.o":"Libraries/Flash/FlashBankManager.src" "Libraries/Flash/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

"Libraries/Flash/external_flash.src":"../Libraries/Flash/external_flash.c" "Libraries/Flash/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Flash/external_flash.o":"Libraries/Flash/external_flash.src" "Libraries/Flash/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-Flash

clean-Libraries-2f-Flash:
	-$(RM) ./Libraries/Flash/FlashBankManager.d ./Libraries/Flash/FlashBankManager.o ./Libraries/Flash/FlashBankManager.src ./Libraries/Flash/external_flash.d ./Libraries/Flash/external_flash.o ./Libraries/Flash/external_flash.src

.PHONY: clean-Libraries-2f-Flash

