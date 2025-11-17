################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/DataCollection/readiness_aggregator.c" \
"../Libraries/DataCollection/vci_aggregator.c" 

COMPILED_SRCS += \
"Libraries/DataCollection/readiness_aggregator.src" \
"Libraries/DataCollection/vci_aggregator.src" 

C_DEPS += \
"./Libraries/DataCollection/readiness_aggregator.d" \
"./Libraries/DataCollection/vci_aggregator.d" 

OBJS += \
"Libraries/DataCollection/readiness_aggregator.o" \
"Libraries/DataCollection/vci_aggregator.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/DataCollection/readiness_aggregator.src":"../Libraries/DataCollection/readiness_aggregator.c" "Libraries/DataCollection/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/DataCollection/readiness_aggregator.o":"Libraries/DataCollection/readiness_aggregator.src" "Libraries/DataCollection/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/DataCollection/vci_aggregator.src":"../Libraries/DataCollection/vci_aggregator.c" "Libraries/DataCollection/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/DataCollection/vci_aggregator.o":"Libraries/DataCollection/vci_aggregator.src" "Libraries/DataCollection/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-DataCollection

clean-Libraries-2f-DataCollection:
	-$(RM) ./Libraries/DataCollection/readiness_aggregator.d ./Libraries/DataCollection/readiness_aggregator.o ./Libraries/DataCollection/readiness_aggregator.src ./Libraries/DataCollection/vci_aggregator.d ./Libraries/DataCollection/vci_aggregator.o ./Libraries/DataCollection/vci_aggregator.src

.PHONY: clean-Libraries-2f-DataCollection

