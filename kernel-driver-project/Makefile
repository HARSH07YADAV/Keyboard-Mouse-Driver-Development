# Makefile for Virtual Keyboard and Mouse Drivers
# Educational OS Project

# Kernel module names
obj-m += keyboard_driver.o
obj-m += mouse_driver.o

# Kernel build directory (current running kernel)
KDIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# User-space tools
USERSPACE_READER := userspace/reader

# Default target: build everything
all: modules userspace

# Build kernel modules
modules:
	@echo "Building kernel modules..."
	$(MAKE) -C $(KDIR) M=$(PWD)/drivers modules
	@echo "Kernel modules built successfully!"
	@ls -lh drivers/*.ko

# Build individual modules (for convenience)
keyboard:
	@echo "Building keyboard driver..."
	$(MAKE) -C $(KDIR) M=$(PWD)/drivers keyboard_driver.ko

mouse:
	@echo "Building mouse driver..."
	$(MAKE) -C $(KDIR) M=$(PWD)/drivers mouse_driver.ko

# Build user-space tools
userspace:
	@echo "Building user-space reader..."
	gcc -Wall -Wextra -O2 -o $(USERSPACE_READER) userspace/reader.c
	@echo "User-space tools built successfully!"
	@ls -lh $(USERSPACE_READER)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(MAKE) -C $(KDIR) M=$(PWD)/drivers clean
	rm -f $(USERSPACE_READER)
	rm -f drivers/*.o drivers/*.ko drivers/*.mod* drivers/.*.cmd drivers/Module.symvers
	rm -rf drivers/.tmp_versions
	@echo "Clean complete!"

# Install modules (requires root)
install:
	@echo "Installing modules (requires root)..."
	sudo insmod drivers/keyboard_driver.ko
	sudo insmod drivers/mouse_driver.ko
	@echo "Modules installed. Check dmesg for details."
	@dmesg | tail -20

# Uninstall modules (requires root)
uninstall:
	@echo "Uninstalling modules (requires root)..."
	-sudo rmmod mouse_driver
	-sudo rmmod keyboard_driver
	@echo "Modules uninstalled."

# Show module info
info:
	@echo "=== Keyboard Driver Info ==="
	@modinfo drivers/keyboard_driver.ko 2>/dev/null || echo "Not built yet"
	@echo ""
	@echo "=== Mouse Driver Info ==="
	@modinfo drivers/mouse_driver.ko 2>/dev/null || echo "Not built yet"

# Check if modules are loaded
status:
	@echo "=== Module Status ==="
	@lsmod | grep -E "keyboard_driver|mouse_driver" || echo "No modules loaded"
	@echo ""
	@echo "=== Input Devices ==="
	@cat /proc/bus/input/devices | grep -A 5 "Virtual" || echo "No virtual devices found"

# Help target
help:
	@echo "Virtual Input Driver Project - Makefile Help"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build kernel modules and user-space tools (default)"
	@echo "  make modules      - Build only kernel modules"
	@echo "  make keyboard     - Build only keyboard driver"
	@echo "  make mouse        - Build only mouse driver"
	@echo "  make userspace    - Build only user-space reader"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make install      - Load modules (requires sudo)"
	@echo "  make uninstall    - Unload modules (requires sudo)"
	@echo "  make info         - Show module information"
	@echo "  make status       - Check if modules are loaded"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  1. make              # Build everything"
	@echo "  2. sudo make install # Load the modules"
	@echo "  3. dmesg | tail      # Check kernel messages"
	@echo "  4. Run tests in tests/ directory"

.PHONY: all modules keyboard mouse userspace clean install uninstall info status help
