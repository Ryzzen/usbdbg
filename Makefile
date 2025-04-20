TARGET 			:= usbdbg
TARGET_HEX  ?= $(TARGET).hex
TARGET_BIN  ?= $(TARGET).bin

DEBUG ?= 1
BUILD_DIR	?= build
SRC_DIRS ?= ./app/Core \
						./app/USBHost_App \
						./vendor/openwch/Core \
						./vendor/openwch/Debug \
						./vendor/openwch/Peripheral \
						./vendor/openwch/Startup \
						./vendor/openwch/User \
						./vendor/openwch/USB_Host
INC_DIRS 			:= $(shell find $(SRC_DIRS) -type d)

AS := riscv-none-elf-gcc
CC := riscv-none-elf-gcc
CXX := riscv-none-elf-g++
OBJCOPY := riscv-none-elf-objcopy

INC_FLAGS := $(addprefix -I,$(INC_DIRS))
FLAGS = -march=rv32imafc -mabi=ilp32f -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized  -g
FLAGS_DEBUG := -g3 \
    -O
FLAGS_RELEASE := -O2 \
    -march=native \
    -mtune=native \
    -ftree-vectorize

ASFLAGS ?= $(FLAGS) -x assembler $(INC_FLAGS) -MMD -MP
CFLAGS ?=  $(FLAGS) $(INC_FLAGS) -std=gnu99 -MMD -MP
CPPFLAGS ?=  $(FLAGS) $(INC_FLAGS) -std=gnu99 -MMD -MP
LDFLAGS ?= $(FLAGS) -T ./vendor/openwch/Ld/Link.ld -nostartfiles -Xlinker --gc-sections -Wl,-Map,"$(BUILD_DIR)/CH32V203.map" --specs=nano.specs --specs=nosys.specs

ifeq ($(DEBUG), 1)
    FLAGS += $(FLAGS_DEBUG) -DDEBUG
else
    FLAGS += $(FLAGS_RELEASE) -DRELEASE
endif

SRCS	:= $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.S)
OBJS	:= $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	$(OBJCOPY) -Oihex   $@ $(BUILD_DIR)/$(TARGET_HEX)
	$(OBJCOPY) -Obinary $@ $(BUILD_DIR)/$(TARGET_BIN)

# assembly
$(BUILD_DIR)/%.S.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean

-include $(DEPS)
