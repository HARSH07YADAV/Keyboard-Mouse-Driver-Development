#!/bin/bash
# test_mouse.sh - Automated mouse driver test script
# Educational OS Project

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m' # No Color

echo "========================================="
echo "Virtual Mouse Driver Test"
echo "========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error: This script must be run as root (use sudo)${NC}"
    exit 1
fi

# Find the sysfs injection path
SYSFS_PATH=$(find /sys/devices/virtual/input -name "inject_packet" 2>/dev/null | head -1)

if [ -z "$SYSFS_PATH" ]; then
    echo -e "${RED}Error: Cannot find mouse driver sysfs interface${NC}"
    echo "Make sure the mouse_driver module is loaded:"
    echo "  sudo insmod drivers/mouse_driver.ko"
    exit 1
fi

echo -e "${GREEN}Found mouse driver at: $SYSFS_PATH${NC}"
echo ""

# Function to inject mouse packet
# PS/2 packet format: status_byte dx dy
# Status byte: [Y_overflow | X_overflow | Y_sign | X_sign | 1 | Middle | Right | Left]
inject_packet() {
    local status=$1
    local dx=$2
    local dy=$3
    local desc=$4
    echo -e "${BLUE}Injecting:${NC} $desc"
    echo "$status $dx $dy" > $SYSFS_PATH
    sleep 0.2
}

# Test sequence
echo "Starting test sequence..."
echo "Watch dmesg or use the event reader to see the results"
echo ""

# Test button clicks
echo -e "${YELLOW}=== Testing mouse buttons ===${NC}"
inject_packet "0x09" "0x00" "0x00" "Left button press (no movement)"
inject_packet "0x08" "0x00" "0x00" "Left button release"
sleep 0.3

inject_packet "0x0A" "0x00" "0x00" "Right button press"
inject_packet "0x08" "0x00" "0x00" "Right button release"
sleep 0.3

inject_packet "0x0C" "0x00" "0x00" "Middle button press"
inject_packet "0x08" "0x00" "0x00" "Middle button release"
sleep 0.3

inject_packet "0x0B" "0x00" "0x00" "Left+Right buttons press"
inject_packet "0x08" "0x00" "0x00" "All buttons release"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing mouse movement ===${NC}"
inject_packet "0x08" "0x0A" "0x00" "Move right by 10"
sleep 0.2

inject_packet "0x08" "0xF6" "0x00" "Move left by 10 (-10 = 0xF6)"
sleep 0.2

inject_packet "0x08" "0x00" "0x0A" "Move down by 10"
sleep 0.2

inject_packet "0x08" "0x00" "0xF6" "Move up by 10 (-10 = 0xF6)"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing diagonal movement ===${NC}"
inject_packet "0x08" "0x14" "0x14" "Move right-down (20, 20)"
sleep 0.2

inject_packet "0x08" "0xEC" "0xEC" "Move left-up (-20, -20)"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing movement with buttons ===${NC}"
inject_packet "0x09" "0x05" "0x00" "Left button drag right"
inject_packet "0x09" "0x05" "0x00" "Continue drag"
inject_packet "0x08" "0x00" "0x00" "Release button"
sleep 0.3

inject_packet "0x0A" "0x00" "0x0A" "Right button drag down"
inject_packet "0x0A" "0x00" "0x0A" "Continue drag"
inject_packet "0x08" "0x00" "0x00" "Release button"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing circular motion ===${NC}"
echo -e "${BLUE}Simulating circular mouse movement...${NC}"
# Approximate circle with 8 segments
inject_packet "0x08" "0x14" "0x00" "E  (20, 0)"
sleep 0.1
inject_packet "0x08" "0x0E" "0x0E" "SE (14, 14)"
sleep 0.1
inject_packet "0x08" "0x00" "0x14" "S  (0, 20)"
sleep 0.1
inject_packet "0x08" "0xF2" "0x0E" "SW (-14, 14)"
sleep 0.1
inject_packet "0x08" "0xEC" "0x00" "W  (-20, 0)"
sleep 0.1
inject_packet "0x08" "0xF2" "0xF2" "NW (-14, -14)"
sleep 0.1
inject_packet "0x08" "0x00" "0xEC" "N  (0, -20)"
sleep 0.1
inject_packet "0x08" "0x0E" "0xF2" "NE (14, -14)"
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing rapid movements ===${NC}"
echo -e "${BLUE}Simulating rapid mouse jitter...${NC}"
for i in {1..5}; do
    inject_packet "0x08" "0x02" "0x02" "Small move"
    inject_packet "0x08" "0xFE" "0xFE" "Small move back"
done
sleep 0.3

echo ""
echo -e "${YELLOW}=== Testing large movements ===${NC}"
inject_packet "0x08" "0x7F" "0x00" "Large move right (127)"
sleep 0.2
inject_packet "0x08" "0x81" "0x00" "Large move left (-127)"
sleep 0.2
inject_packet "0x08" "0x00" "0x7F" "Large move down (127)"
sleep 0.2
inject_packet "0x08" "0x00" "0x81" "Large move up (-127)"
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
echo "  EVENT_DEV=\$(cat /proc/bus/input/devices | grep -A 5 'Virtual PS/2 Mouse' | grep 'event' | sed 's/.*event\\([0-9]*\\).*/\\1/')"
echo "  sudo ./userspace/reader /dev/input/event\$EVENT_DEV"
echo ""

# Show recent kernel messages
echo "Recent kernel messages:"
echo "---"
dmesg | grep virtual_mouse | tail -15
