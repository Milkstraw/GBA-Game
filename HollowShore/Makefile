#-------------------------------------------------------------------------------
# Hollow Shore GBA — devkitPro build
#-------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM not set. Install devkitPro and export DEVKITARM=<path>/devkitARM")
endif

include $(DEVKITARM)/gba_rules

#-------------------------------------------------------------------------------
TARGET   := HollowShore
BUILD    := build
SOURCES  := source \
            source/core \
            source/world \
            source/world/maps \
            source/player \
            source/systems \
            source/sanctums \
            source/ui \
            source/graphics
INCLUDES := source \
            source/core \
            source/world \
            source/world/maps \
            source/player \
            source/systems \
            source/sanctums \
            source/ui \
            source/graphics

#-------------------------------------------------------------------------------
ARCH     := -mthumb -mthumb-interwork

CFLAGS   := -g -Wall -O2 \
             -mcpu=arm7tdmi -mtune=arm7tdmi \
             $(ARCH)

CFLAGS   += $(INCLUDE)
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS  := -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS     := -lgba
LIBDIRS  := $(LIBGBA)

#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

export OUTPUT  := $(CURDIR)/$(TARGET)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export LD      := $(CC)
export OFILES  := $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(SFILES:.s=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo "Cleaning..."
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba

#-------------------------------------------------------------------------------
else
#-------------------------------------------------------------------------------

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).gba : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)

-include $(DEPENDS)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------
