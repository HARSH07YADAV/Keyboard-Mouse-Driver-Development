#!/bin/bash
# install.sh - Automated installation script for virtual input drivers
# Educational OS Project

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "Virtual Input Driver Installation Script"
echo "========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error: This script must be run as root (use sudo)${NC}"
    exit 1
fi

# Check if modules are built
if [ ! -f "$SCRIPT_DIR/drivers/keyboard_driver.ko" ] || [ ! -f "$SCRIPT_DIR/drivers/mouse_driver.ko" ]; then
    echo -e "${YELLOW}Warning: Kernel modules not found. Building now...${NC}"
    cd "$SCRIPT_DIR"
    make modules
    echo ""
fi

# Unload modules if already loaded
echo "Checking for existing modules..."
if lsmod | grep -q "keyboard_driver"; then
    echo "Unloading existing keyboard_driver..."
    rmmod keyboard_driver || true
fi

if lsmod | grep -q "mouse_driver"; then
    echo "Unloading existing mouse_driver..."
    rmmod mouse_driver || true
fi

echo ""
echo "Loading keyboard driver..."
insmod "$SCRIPT_DIR/drivers/keyboard_driver.ko"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Keyboard driver loaded successfully${NC}"
else
    echo -e "${RED}✗ Failed to load keyboard driver${NC}"
    exit 1
fi

echo "Loading mouse driver..."
insmod "$SCRIPT_DIR/drivers/mouse_driver.ko"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Mouse driver loaded successfully${NC}"
else
    echo -e "${RED}✗ Failed to load mouse driver${NC}"
    exit 1
fi

echo ""
echo "Waiting for devices to initialize..."
sleep 1

# Find the input device numbers
echo ""
echo "========================================="
echo "Device Information"
echo "========================================="

KBDEV=$(grep -l "Virtual PS/2 Keyboard" /sys/class/input/input*/name 2>/dev/null | head -1 | sed 's|/sys/class/input/||;s|/name||')
MOUSEDEV=$(grep -l "Virtual PS/2 Mouse" /sys/class/input/input*/name 2>/dev/null | head -1 | sed 's|/sys/class/input/||;s|/name||')

if [ -n "$KBDEV" ]; then
    KBEVENT=$(basename /sys/class/input/$KBDEV/event* 2>/dev/null | head -1)
    KBSYSFS="/sys/devices/virtual/input/$KBDEV/inject_scancode"
    
    echo -e "${GREEN}Keyboard Device:${NC}"
    echo "  Input Device: $KBDEV"
    echo "  Event Device: /dev/input/$KBEVENT"
    echo "  Sysfs Path:   $KBSYSFS"
    echo ""
else
    echo -e "${RED}Warning: Could not find keyboard device${NC}"
    echo ""
fi

if [ -n "$MOUSEDEV" ]; then
    MOUSEEVENT=$(basename /sys/class/input/$MOUSEDEV/event* 2>/dev/null | head -1)
    MOUSESYSFS="/sys/devices/virtual/input/$MOUSEDEV/inject_packet"
    
    echo -e "${GREEN}Mouse Device:${NC}"
    echo "  Input Device: $MOUSEDEV"
    echo "  Event Device: /dev/input/$MOUSEEVENT"
    echo "  Sysfs Path:   $MOUSESYSFS"
    echo ""
else
    echo -e "${RED}Warning: Could not find mouse device${NC}"
    echo ""
fi

# Show kernel messages
echo "========================================="
echo "Recent Kernel Messages"
echo "========================================="
dmesg | tail -15 | grep -E "virtual_keyboard|virtual_mouse|Virtual PS/2"

echo ""
echo "========================================="
echo "Installation Complete!"
echo "========================================="
echo ""
echo "Next Steps:"
echo ""
echo "1. Test keyboard driver:"
if [ -n "$KBSYSFS" ]; then
    echo "   echo 0x1E | sudo tee $KBSYSFS  # Inject 'A' key press"
    echo "   echo 0x9E | sudo tee $KBSYSFS  # Inject 'A' key release"
fi
echo ""
echo "2. Test mouse driver:"
if [ -n "$MOUSESYSFS" ]; then
    echo "   echo \"0x09 0x10 0xF0\" | sudo tee $MOUSESYSFS  # Move mouse"
fi
echo ""
echo "3. Read events:"
if [ -n "$KBEVENT" ]; then
    echo "   sudo $SCRIPT_DIR/userspace/reader /dev/input/$KBEVENT"
fi
echo ""
echo "4. Run automated tests:"
echo "   sudo bash $SCRIPT_DIR/tests/test_keyboard.sh"
echo "   sudo bash $SCRIPT_DIR/tests/test_mouse.sh"
echo ""
echo "5. Unload modules when done:"
echo "   sudo rmmod mouse_driver keyboard_driver"
echo ""
