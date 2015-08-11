CC        := avr32-gcc
LD        := avr32-gcc
ASM       := avr32-gcc -x assembler-with-cpp
OBJ_CPY   := avr32-objcopy
OBJ_DUMP  := avr32-objdump
AVR_SZ    := avr32-size

OPT_CC    := -std=gnu99 -mpart=uc3a0512 -O0 -ffunction-sections -masm-addr-pseudos -g3 -Wall
OPT_ASM   := -mpart=uc3a0512 -Wa,-g

#path relative to makefile for src (do NOT put trailing / )
BSRC_DIR := uCOS-II

#directory to include (do NOT put trailing / )
MODULES   := $(shell cd $(BSRC_DIR);  find . -type d | sed -e "s/\.\///g" | sed -e "s/^\.//")

SRC_DIR   := $(addprefix $(BSRC_DIR)/,$(MODULES))
BUILD_DIR := $(addprefix build/,$(MODULES))

SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
SRC_x     := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.x))
SRC_X     := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.X))
SRC_S     := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))

#obj list order will change build order, here we want asm before
OBJ       := $(patsubst $(BSRC_DIR)/%.x,build/%.ox,$(SRC_x))
OBJ       += $(patsubst $(BSRC_DIR)/%.X,build/%.oX,$(SRC_X))
OBJ       += $(patsubst $(BSRC_DIR)/%.S,build/%.oS,$(SRC_S))
OBJ       += $(patsubst $(BSRC_DIR)/%.c,build/%.o,$(SRC))

SRC       += $(SRC_x)
SRC       += $(SRC_X)
SRC       += $(SRC_S)

INCLUDES  := $(addprefix -I,$(SRC_DIR))

vpath %.c $(SRC_DIR)
vpath %.S $(SRC_DIR)
vpath %.x $(SRC_DIR)
vpath %.X $(SRC_DIR)

define make-goal
$1/%.o: %.c
	@echo "CC: $$<"
	@$(CC) -c $(OPT_CC) -MD -MP -MF "$$(@:%.o=%.d)" -MT"$$(@:%.o=%.d)" -MT"$$(@:%.o=%.o)" $(INCLUDES) -o "$$@" "$$<"

$1/%.ox: %.x
	@echo "ASM $$<"
	@$(ASM) -c $(OPT_ASM) -MD -MP -MF "$$(@:%.o=%.d)" -MT"$$(@:%.o=%.d)" -MT"$$(@:%.o=%.o)" $(INCLUDES) -o "$$@" "$$<"

$1/%.oX: %.X
	@echo "ASM $$<"
	@$(ASM) -c $(OPT_ASM) -MD -MP -MF "$$(@:%.o=%.d)" -MT"$$(@:%.o=%.d)" -MT"$$(@:%.o=%.o)" $(INCLUDES) -o "$$@" "$$<"

$1/%.oS: %.S
	@echo "ASM $$<"
	@$(ASM) -c $(OPT_ASM) -MD -MP -MF "$$(@:%.o=%.d)" -MT"$$(@:%.o=%.d)" -MT"$$(@:%.o=%.o)" $(INCLUDES) -o "$$@" "$$<"
endef

.PHONY: all checkdirs clean

all: checkdirs build/uCOS-II.elf dist

build/uCOS-II.elf: $(OBJ)
	@echo "LD: $@"
	@$(LD) $^ -o $@ -nostartfiles -Wl,-Map="$(@:%.elf=%.map)" -Wl,--start-group -lm  -Wl,--end-group -Wl,--gc-sections --rodata-writable -Wl,--direct-data -mpart=uc3a0512 -T $(BSRC_DIR)/BSP/link_newlib_uc3a0512.lds -Wl,-e,_trampoline
	@echo Finished building target:
	@$(OBJ_CPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "$@" "$(@:%.elf=%.hex)"
	@$(OBJ_CPY) -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "$@" "$(@:%.elf=%.eep)" || exit 0
	@$(OBJ_DUMP) -h -S "$@" > "$(@:%.elf=%.lss)"
	@$(OBJ_CPY) -O srec -R .eeprom -R .fuse -R .lock -R .signature  "$@" "$(@:%.elf=%.srec)"
	cp build/uCOS-II.hex dist/uCOS-II.hex
	@$(AVR_SZ) "$@"
	

deploy: all
	@echo "Attempt to deploy into board"
	@cp build/uCOS-II.hex dist/uCOS-II.hex
	@$(shell sudo dist/burn2.sh uCOS-II dist "-y")
	@echo "Deployement sucessfull"

checkdirs: $(BUILD_DIR)
	@echo "Modules=$(MODULES)"
	@mkdir -p dist

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf build

$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))

