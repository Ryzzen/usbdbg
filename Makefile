TARGET_NAME ?= app
TARGET_ELF  ?= $(TARGET_NAME).elf
TARGET_HEX  ?= $(TARGET_NAME).hex
TARGET_BIN  ?= $(TARGET_NAME).bin

AS := riscv-none-elf-gcc
CC := riscv-none-elf-gcc
CXX := riscv-none-elf-g++
OBJCOPY := riscv-none-elf-objcopy

BUILD_DIR ?= ./build
SRC_DIRS ?= ./app/Core \
						./app/USBHost_App \
						./vendor/openwch/Core \
						./vendor/openwch/Debug \
						./vendor/openwch/Peripheral \
						./vendor/openwch/Startup \
						./vendor/openwch/User \
						./vendor/openwch/USB_Host \

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.S)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

FLAGS ?= -march=rv32imafc -mabi=ilp32f -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized  -g
ASFLAGS ?= $(FLAGS) -x assembler $(INC_FLAGS) -MMD -MP
CPPFLAGS ?=  $(FLAGS) $(INC_FLAGS) -std=gnu99 -MMD -MP
LDFLAGS ?= $(FLAGS) -T ./vendor/openwch/Ld/Link.ld -nostartfiles -Xlinker --gc-sections -Wl,-Map,"$(BUILD_DIR)/CH32V203.map" --specs=nano.specs --specs=nosys.specs


$(BUILD_DIR)/$(TARGET_ELF): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	$(OBJCOPY) -Oihex   $@ $(BUILD_DIR)/$(TARGET_HEX)
	$(OBJCOPY) -Obinary $@ $(BUILD_DIR)/$(TARGET_BIN)

# assembly
$(BUILD_DIR)/%.S.o: %.S
	$(MKDIR_P) $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
