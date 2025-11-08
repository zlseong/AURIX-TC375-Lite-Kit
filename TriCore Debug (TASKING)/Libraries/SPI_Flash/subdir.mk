################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/SPI_Flash/Flash4_Driver.c" 

COMPILED_SRCS += \
"Libraries/SPI_Flash/Flash4_Driver.src" 

C_DEPS += \
"./Libraries/SPI_Flash/Flash4_Driver.d" 

OBJS += \
"Libraries/SPI_Flash/Flash4_Driver.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/SPI_Flash/Flash4_Driver.src":"../Libraries/SPI_Flash/Flash4_Driver.c" "Libraries/SPI_Flash/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/Zonal_Gateway/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/SPI_Flash/Flash4_Driver.o":"Libraries/SPI_Flash/Flash4_Driver.src" "Libraries/SPI_Flash/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-SPI_Flash

clean-Libraries-2f-SPI_Flash:
	-$(RM) ./Libraries/SPI_Flash/Flash4_Driver.d ./Libraries/SPI_Flash/Flash4_Driver.o ./Libraries/SPI_Flash/Flash4_Driver.src

.PHONY: clean-Libraries-2f-SPI_Flash

