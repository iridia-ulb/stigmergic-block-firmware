########################################################################
# Target

BOARD_TAG = smartblock-mainboard
TARGET = firmware
MCU = atmega328p
F_CPU = 8000000UL
HEX_MAXIMUM_SIZE = 30720

########################################################################
# AVR Tool names

CC_NAME      = avr-gcc
CXX_NAME     = avr-g++
OBJCOPY_NAME = avr-objcopy
OBJDUMP_NAME = avr-objdump
AR_NAME      = avr-ar
SIZE_NAME    = avr-size
NM_NAME      = avr-nm

# Paths
MAKEFILE_PATH := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

AVR_TOOLCHAIN_DIR = $(MAKEFILE_PATH)/../../toolchain/install/
AVR_TOOLS_PATH = $(AVR_TOOLCHAIN_DIR)/bin
#OBJDIR = $(MAKEFILE_PATH)/build
#SRCDIR = $(MAKEFILE_PATH)/source

OBJDIR = build
SRCDIR = source


########################################################################
# Local sources

LOCAL_SRCS      = $(wildcard $(SRCDIR)/*.cpp)
LOCAL_DEPS	= $(wildcard $(SRCDIR)/*.h)
LOCAL_OBJ_FILES = $(notdir $(LOCAL_SRCS:.cpp=.o))
LOCAL_OBJS      = $(patsubst %,$(OBJDIR)/%,$(LOCAL_OBJ_FILES))

# Dependency files
DEPS = $(LOCAL_OBJS:.o=.d)

########################################################################
# Rules for making stuff

# The name of the main targets
TARGET_HEX = $(OBJDIR)/$(TARGET).hex
TARGET_ELF = $(OBJDIR)/$(TARGET).elf
TARGET_EEP = $(OBJDIR)/$(TARGET).eep
TARGETS    = $(OBJDIR)/$(TARGET).*

CC      = $(AVR_TOOLS_PATH)/$(CC_NAME)
CXX     = $(AVR_TOOLS_PATH)/$(CXX_NAME)
AS      = $(AVR_TOOLS_PATH)/$(AS_NAME)
OBJCOPY = $(AVR_TOOLS_PATH)/$(OBJCOPY_NAME)
OBJDUMP = $(AVR_TOOLS_PATH)/$(OBJDUMP_NAME)
AR      = $(AVR_TOOLS_PATH)/$(AR_NAME)
SIZE    = $(AVR_TOOLS_PATH)/$(SIZE_NAME)
NM      = $(AVR_TOOLS_PATH)/$(NM_NAME)

REMOVE  = rm -rf
MV      = mv -f
CAT     = cat
ECHO    = printf
MKDIR   = mkdir -p

OPTIMIZATION_LEVEL = s

# -I$(AVR_TOOLCHAIN_DIR)/avr/include
# probably need to give the path to libavr here
CPPFLAGS += -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Wall -ffunction-sections -fdata-sections

OPTIMIZATION_FLAGS = -O$(OPTIMIZATION_LEVEL)

CPPFLAGS += $(OPTIMIZATION_FLAGS)

CFLAGS_STD    = -std=c++11

CFLAGS        += $(EXTRA_FLAGS) $(EXTRA_CFLAGS)
CXXFLAGS      += -fno-exceptions $(CFLAGS_STD) -I$(SRCDIR) $(EXTRA_FLAGS) $(EXTRA_CXXFLAGS)
ASFLAGS       += -x assembler-with-cpp
LDFLAGS       += -mmcu=$(MCU) -Wl,--gc-sections -Wl,-Tsource/avr5.xn -O$(OPTIMIZATION_LEVEL) $(EXTRA_FLAGS) $(EXTRA_CXXFLAGS) $(EXTRA_LDFLAGS)
SIZEFLAGS     ?= --mcu=$(MCU) -C

avr_size = $(SIZE) $(2)

# Implicit rules for building everything (needed to get everything in
# the right directory)
#
# Rather than mess around with VPATH there are quasi-duplicate rules
# here for building e.g. a system C++ file and a local C++
# file. Besides making things simpler now, this would also make it
# easy to change the build options in future

ifdef COMMON_DEPS
    COMMON_DEPS := $(COMMON_DEPS) $(MAKEFILE_LIST)
else
    COMMON_DEPS := $(MAKEFILE_LIST)
endif

#$(warning COMMON_DEPS=$(COMMON_DEPS), MAKEFILE_LIST=$(MAKEFILE_LIST), OBJDIR=$(OBJDIR), LOCAL_DEPS=$(LOCAL_DEPS))

# normal local sources

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(LOCAL_DEPS)
	@$(MKDIR) $(dir $@)
	$(CXX) -MMD -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

#$(OBJDIR)/%.lst: $(OBJDIR)/%.s
#	$(AS) -$(MCU_FLAG_NAME)=$(MCU) -alhnd $< > $@

# various object conversions
$(OBJDIR)/%.hex: $(OBJDIR)/%.elf $(COMMON_DEPS)
	@$(MKDIR) $(dir $@)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	@$(ECHO) '\n'
	$(call avr_size,$<,$@)
ifneq ($(strip $(HEX_MAXIMUM_SIZE)),)
	@if [ `$(SIZE) $@ | awk 'FNR == 2 {print $$2}'` -le $(HEX_MAXIMUM_SIZE) ]; then touch $@.sizeok; fi
else
	@$(ECHO) "Maximum flash memory of $(BOARD_TAG) is not specified. Make sure the size of $@ is less than $(BOARD_TAG)\'s flash memory"
	@touch $@.sizeok
endif

$(OBJDIR)/%.eep: $(OBJDIR)/%.elf $(COMMON_DEPS)
	@$(MKDIR) $(dir $@)
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
		--change-section-lma .eeprom=0 -O ihex $< $@

$(OBJDIR)/%.lss: $(OBJDIR)/%.elf $(COMMON_DEPS)
	@$(MKDIR) $(dir $@)
	$(OBJDUMP) -h --source --demangle --wide $< > $@

$(OBJDIR)/%.sym: $(OBJDIR)/%.elf $(COMMON_DEPS)
	@$(MKDIR) $(dir $@)
	$(NM) --size-sort --demangle --reverse-sort --line-numbers $< > $@

########################################################################
# Explicit targets start here

all: 		$(TARGET_EEP) $(TARGET_HEX)

# Rule to create $(OBJDIR) automatically. All rules with recipes that
# create a file within it, but do not already depend on a file within it
# should depend on this rule. They should use a "order-only
# prerequisite" (e.g., put "| $(OBJDIR)" at the end of the prerequisite
# list) to prevent remaking the target when any file in the directory
# changes.
$(OBJDIR):
		$(MKDIR) $(OBJDIR)

$(TARGET_ELF): $(LOCAL_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OTHER_OBJS) $(LOCAL_OBJS) -lc -lm

clean:
		$(REMOVE) $(LOCAL_OBJS) $(CORE_OBJS) $(LIB_OBJS) $(TARGETS) $(DEPS) $(USER_LIB_OBJS) ${OBJDIR}

size:	$(TARGET_HEX)
		$(call avr_size,$(TARGET_ELF),$(TARGET_HEX))

disasm: $(OBJDIR)/$(TARGET).lss
		@$(ECHO) "The compiled ELF file has been disassembled to $(OBJDIR)/$(TARGET).lss\n\n"

symbol_sizes: $(OBJDIR)/$(TARGET).sym
		@$(ECHO) "A symbol listing sorted by their size have been dumped to $(OBJDIR)/$(TARGET).sym\n\n"

verify_size:
ifeq ($(strip $(HEX_MAXIMUM_SIZE)),)
	@$(ECHO) "\nMaximum flash memory of $(BOARD_TAG) is not specified. Make sure the size of $(TARGET_HEX) is less than $(BOARD_TAG)\'s flash memory\n\n"
endif
	@if [ ! -f $(TARGET_HEX).sizeok ]; then echo >&2 "\nThe size of the compiled binary file is greater than the $(BOARD_TAG)'s flash memory. \
See http://www.arduino.cc/en/Guide/Troubleshooting#size for tips on reducing it."; false; fi

generate_assembly: $(OBJDIR)/$(TARGET).s
		@$(ECHO) "Compiler-generated assembly for the main input source has been dumped to $(OBJDIR)/$(TARGET).s\n\n"

.PHONY: all clean depends size disasm symbol_sizes \
        generate_assembly verify_size

# added - in the beginning, so that we don't get an error if the file is not present
-include $(DEPS)
