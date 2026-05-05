ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set — source /etc/profile.d/devkit-env.sh or see devkitpro.org)
endif

# ------------------------------------------------------------
# Toolchain
# ------------------------------------------------------------
DEVKITARM := $(DEVKITPRO)/devkitARM
PREFIX    := $(DEVKITARM)/bin/arm-none-eabi-
CC        := $(PREFIX)gcc
OBJCOPY   := $(PREFIX)objcopy
GBAFIX    := $(DEVKITPRO)/tools/bin/gbafix

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------
TARGET   := supermarket
SRCDIR   := src
BUILDDIR := build

TONC_INC  := $(DEVKITPRO)/libtonc/include
TONC_LIB  := $(DEVKITPRO)/libtonc/lib
LDSCRIPT  := $(DEVKITARM)/arm-none-eabi/lib/gba.ld
CRT0      := $(DEVKITARM)/arm-none-eabi/lib/crt0.o

# ------------------------------------------------------------
# Flags
# ------------------------------------------------------------
ARCH := -mthumb -mthumb-interwork

CFLAGS := $(ARCH) \
          -O2 \
          -mcpu=arm7tdmi \
          -ffunction-sections \
          -fdata-sections \
          -Wall \
          -Wextra \
          -Iinclude \
          -I$(TONC_INC)

LDFLAGS := $(ARCH) \
           -nostartfiles \
           -T$(LDSCRIPT) \
           -Wl,--gc-sections \
           -L$(TONC_LIB) \
           -ltonc \
           -lgcc

# ------------------------------------------------------------
# Sources → objects
# Finds every .c under src/ recursively, mirrors tree in build/
# ------------------------------------------------------------
SRCS := $(shell find $(SRCDIR) -name '*.c' | sort)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# ------------------------------------------------------------
# Targets
# ------------------------------------------------------------
.PHONY: all clean

all: $(TARGET).gba

# Strip ELF → raw binary, then fix the ROM header
$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@

# crt0.o must be first so _start is the entry point
$(TARGET).elf: $(OBJS)
	$(CC) $(CRT0) $^ $(LDFLAGS) -o $@

# Compile: create build subdir on demand, emit dep file alongside .o
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET).elf $(TARGET).gba

# Pull in header dependency files from previous builds
-include $(DEPS)
