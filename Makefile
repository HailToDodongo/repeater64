BUILD_DIR=build
include $(N64_INST)/include/n64.mk

PROJECT_NAME = "repeat64"

N64_CXXFLAGS += -std=gnu++20 -Os -fno-exceptions -fsingle-precision-constant

src = $(wildcard src/*.cpp) $(wildcard src/demos/*.cpp) $(wildcard src/rdp/*.cpp)

assets_png = $(wildcard assets/*.rgba16.png)
assets_test = $(wildcard assets/*.test)
assets_conv = $(patsubst assets/%,filesystem/%,$(assets_png:%.png=%.sprite))
assets_conv += $(patsubst assets/%,filesystem/%,$(assets_test:%.test=%.test))

all: $(PROJECT_NAME).z64

# Generate demoList.h by scanning files in src/demos/
src/demoList.h: $(wildcard src/demos/*.cpp)
	@echo "    [DEMO LIST] $@"
	@echo "// This file is auto-generated. Do not edit directly." > $@
	@for f in $(wildcard src/demos/*.cpp); do \
		name=$$(basename $$f .cpp); \
		echo "DEMO_ENTRY($$name)" >> $@; \
	done

$(BUILD_DIR)/src/main.o: src/demoList.h

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o $(dir $@) "$<"

filesystem/%.test: assets/%.test
	@mkdir -p $(dir $@)
	@echo "    [TEST-DUMP] $@ $<"
	cp "$<" $@
	$(N64_BINDIR)/mkasset -c 1 -o $(dir $@) $@

$(BUILD_DIR)/$(PROJECT_NAME).dfs: $(assets_conv)
$(BUILD_DIR)/$(PROJECT_NAME).elf: $(src:%.cpp=$(BUILD_DIR)/%.o)

$(PROJECT_NAME).z64: N64_ROM_TITLE="Repeat64"
$(PROJECT_NAME).z64: $(BUILD_DIR)/$(PROJECT_NAME).dfs

fonts:
	node tools/createFont.mjs assets/font.png src/font.h
	node tools/createFontDelta.mjs assets/font64.png src/font64.h

sc64:
	make -j8
	curl 192.168.0.6:9065/off
	sleep 1
	sc64deployer --remote 192.168.0.6:9064 upload --tv ntsc *.z64
	curl 192.168.0.6:9065/on

sc64_pal:
	make -j8
	curl 192.168.0.6:9065/off
	sleep 1
	sc64deployer --remote 192.168.0.6:9064 upload --tv pal *.z64
	curl 192.168.0.6:9065/on

clean:
	rm -rf $(BUILD_DIR) *.z64 src/demoList.h
	rm -rf filesystem

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
