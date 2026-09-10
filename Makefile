MAKEFLAGS += --no-print-directory

BUILD_DIR := _build

.PHONY: llvm2ct clean

$(BUILD_DIR)/CMakeCache.txt:
	cmake -S llvm2ct -B $(BUILD_DIR)

llvm2ct: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR) --target llvm2ct

clean:
	rm -rf $(BUILD_DIR)
