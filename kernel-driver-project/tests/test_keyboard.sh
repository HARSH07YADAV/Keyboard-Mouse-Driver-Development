#!/bin/bash
# test_keyboard.sh - Automated keyboard driver test script
# Educational OS Project

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m' # No Color

echo "========================================="
echo "Virtual Keyboard Driver Test"
echo "========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error: This script must be run as root (use sudo)${NC}"
    exit 1
fi

# Find the sysfs injection path
SYSFS_PATH=$(find /sys/devices/virtual/input -name "inject_scancode" 2>/dev/null | head -1)

if [ -z "$SYSFS_PATH" ]; then
    echo -e "${RED}Error: Cannot find keyboard driver sysfs interface${NC}"
    echo "Make sure the keyboard_driver module is loaded:"
    echo "  sudo insmod drivers/keyboard_driver.ko"
    exit 1
fi

echo -e "${GREEN}Found keyboard driver at: $SYSFS_PATH${NC}"
echo ""

# Function to inject scan code
inject_scancode() {
    local code=$1
    local desc=$2
    echo -e "${BLUE}Injecting:${NC} $desc (scan code $code)"
    echo $code > $SYSFS_PATH
    sleep 0.2
}

# Test sequence
echo "Starting test sequence..."
echo "Watch dmesg or use the event reader to see the results"
echo ""

# Test individual keys
echo -e "${YELLOW}=== Testing letter keys ===${NC}"
inject_scancode 0x1E "Key 'A' press"
inject_scancode 0x9E "Key 'A' release"
sleep 0.3

inject_scancode 0x30 "Key 'B' press"
inject_scancode 0xB0 "Key 'B' release"
sleep 0.3

inject_scancode 0x2E "Key 'C' press"
inject_scancode 0xAE "Key 'C' release"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing number keys ===${NC}"
inject_scancode 0x02 "Key '1' press"
inject_scancode 0x82 "Key '1' release"
sleep 0.3

inject_scancode 0x03 "Key '2' press"
inject_scancode 0x83 "Key '2' release"
sleep 0.3

inject_scancode 0x04 "Key '3' press"
inject_scancode 0x84 "Key '3' release"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing special keys ===${NC}"
inject_scancode 0x39 "SPACE press"
inject_scancode 0xB9 "SPACE release"
sleep 0.3

inject_scancode 0x1C "ENTER press"
inject_scancode 0x9C "ENTER release"
sleep 0.3

inject_scancode 0x0E "BACKSPACE press"
inject_scancode 0x8E "BACKSPACE release"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing modifier keys ===${NC}"
inject_scancode 0x2A "LEFT_SHIFT press"
inject_scancode 0x1E "Key 'A' press (with shift)"
inject_scancode 0x9E "Key 'A' release"
inject_scancode 0xAA "LEFT_SHIFT release"
sleep 0.3

inject_scancode 0x1D "LEFT_CTRL press"
inject_scancode 0x9D "LEFT_CTRL release"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing function keys ===${NC}"
inject_scancode 0x3B "F1 press"
inject_scancode 0xBB "F1 release"
sleep 0.3

inject_scancode 0x3C "F2 press"
inject_scancode 0xBC "F2 release"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing key repeat (hold) ===${NC}"
echo -e "${BLUE}Simulating 'A' key held down...${NC}"
inject_scancode 0x1E "Key 'A' press"
sleep 0.1
inject_scancode 0x1E "Key 'A' repeat"
sleep 0.1
inject_scancode 0x1E "Key 'A' repeat"
sleep 0.1
inject_scancode 0x9E "Key 'A' release"
sleep 0.3

echo ""
echo "========================================="
echo -e "${GREEN}Test Complete!${NC}"
echo "========================================="
echo ""
echo "Check dmesg output:"
echo "  dmesg | tail -30"
echo ""
echo "Or use the event reader:"
echo "  EVENT_DEV=\$(cat /proc/bus/input/devices | grep -A 5 'Virtual PS/2 Keyboard' | grep 'event' | sed 's/.*event\\([0-9]*\\).*/\\1/')"
echo "  sudo ./userspace/reader /dev/input/event\$EVENT_DEV"
echo ""

# Show recent kernel messages
echo "Recent kernel messages:"
echo "---"
dmesg | grep virtual_keyboard | tail -15
