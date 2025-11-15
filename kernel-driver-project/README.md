# Linux Keyboard & Mouse Driver Project

**Educational Operating Systems Project - Virtual Input Device Drivers**

## Project Overview

This project implements two Linux kernel modules that demonstrate device driver development:
- **keyboard_driver.ko**: PS/2-style keyboard driver with scan code translation
- **mouse_driver.ko**: PS/2-style mouse driver with motion and button tracking

Both drivers integrate with the Linux input subsystem, making them compatible with standard user-space tools like `evtest` and graphical environments.

## Architecture

```
Hardware Layer (Simulated)
        |
        v
   IRQ Handler / Work Queue
        |
        v
Scan Code/Packet Parser
        |
        v
  Linux Input Subsystem
        |
        v
   /dev/input/eventX
        |
        v
  User-space Applications
```

### Data Flow

1. **Hardware Simulation**: Since we may not have direct PS/2 hardware, drivers expose sysfs interfaces to inject simulated scan codes/packets
2. **IRQ Handler**: Processes input data, handles buffering with proper locking (spinlock for IRQ context)
3. **Translation Layer**: 
   - Keyboard: Scan code → Linux keycode → input event
   - Mouse: PS/2 3-byte packets → relative motion + button events
4. **Input Subsystem**: Linux `input_dev` framework reports events to user space
5. **User Applications**: Standard tools (`evtest`, X11, Wayland) consume events via `/dev/input/eventX`

## Requirements

- Linux kernel 5.x or 6.x (tested on 5.15+, 6.x)
- Kernel headers installed: `sudo apt install linux-headers-$(uname -r)`
- GCC and Make
- Root/sudo access for module insertion
- Optional: QEMU for virtual testing

## Build Instructions

### Building the Kernel Modules

```bash
# Build both drivers
make

# Build individual modules
make keyboard
make mouse

# Clean build artifacts
make clean
```

The Makefile uses the kernel build system (`kbuild`) and compiles against your running kernel.

### Building User-space Tools

```bash
# Build the event reader utility
make userspace

# Or manually:
gcc -o userspace/reader userspace/reader.c
```

## Installation and Usage

### Loading the Modules

```bash
# Load keyboard driver
sudo insmod drivers/keyboard_driver.ko

# Load mouse driver
sudo insmod drivers/mouse_driver.ko

# Verify loaded
lsmod | grep -E 'keyboard_driver|mouse_driver'

# Check kernel messages
dmesg | tail -20
```

### Using the Install Script

```bash
# Automated installation
sudo bash install.sh

# This will:
# - Insert both modules
# - Find the input event devices
# - Create convenient symlinks
# - Display usage instructions
```

### Finding Your Device

```bash
# List input devices
cat /proc/bus/input/devices | grep -A 5 "Virtual"

# Find event number
ls -l /dev/input/by-id/ | grep Virtual

# Or check dmesg output
dmesg | grep "registered as"
```

### Reading Events

```bash
# Use the provided reader tool
sudo ./userspace/reader /dev/input/event<N>

# Use evtest (if installed)
sudo evtest /dev/input/event<N>

# Simple raw view (binary output)
sudo hexdump -C /dev/input/event<N>
```

## Testing

### Simulating Keyboard Input

The keyboard driver exposes a sysfs interface to inject scan codes:

```bash
# Find the sysfs path
KBDSYSFS=/sys/devices/virtual/input/input*/inject_scancode

# Inject scan code for 'A' key press (0x1E)
echo 0x1E | sudo tee $KBDSYSFS

# Inject release (scan code | 0x80)
echo 0x9E | sudo tee $KBDSYSFS
```

### Simulating Mouse Input

```bash
# Find the sysfs path
MOUSESYSFS=/sys/devices/virtual/input/input*/inject_packet

# Inject mouse packet: buttons dx dy (3 bytes, space-separated)
# Example: left button pressed, move right 10, down 5
echo "0x01 0x0A 0x05" | sudo tee $MOUSESYSFS

# No buttons, move left 5, up 3
echo "0x00 0xFB 0xFD" | sudo tee $MOUSESYSFS
```

### Running Test Scripts

```bash
# Automated keyboard test
sudo bash tests/test_keyboard.sh

# Automated mouse test
sudo bash tests/test_mouse.sh

# Watch events while testing
sudo ./userspace/reader /dev/input/event<N> &
sudo bash tests/test_keyboard.sh
```

## QEMU Testing

### Preparing a QEMU Environment

```bash
# Install QEMU
sudo apt install qemu-system-x86 qemu-utils

# Download a minimal Linux image (example: Debian cloud image)
wget https://cloud.debian.org/images/cloud/bullseye/latest/debian-11-generic-amd64.qcow2

# Or build your own minimal kernel + initramfs
```

### Running in QEMU

```bash
# Basic QEMU command with keyboard/mouse emulation
qemu-system-x86_64 \
  -kernel /boot/vmlinuz-$(uname -r) \
  -initrd /boot/initrd.img-$(uname -r) \
  -m 2G \
  -smp 2 \
  -append "console=ttyS0 root=/dev/sda1" \
  -drive file=debian-11-generic-amd64.qcow2,format=qcow2 \
  -device usb-ehci,id=usb \
  -device usb-kbd \
  -device usb-mouse \
  -nographic

# Alternative: Use VNC for graphical testing
qemu-system-x86_64 \
  -kernel /boot/vmlinuz-$(uname -r) \
  -m 2G \
  -vnc :1 \
  -hda debian-11-generic-amd64.qcow2
```

### Testing Inside QEMU

1. Boot the VM
2. Copy driver files into VM (use scp or shared folder):
   ```bash
   # On host, create shared folder
   qemu-system-x86_64 ... -virtfs local,path=/home/yadavji/kernel-driver-project,mount_tag=hostshare,security_model=passthrough
   
   # In guest
   mkdir /mnt/host
   mount -t 9p -o trans=virtio hostshare /mnt/host
   ```
3. Build modules inside VM (ensure kernel headers match)
4. Load and test as described above

### Debugging in QEMU

```bash
# Enable debug output
echo 8 | sudo tee /proc/sys/kernel/printk

# Monitor kernel messages
dmesg -w

# Use QEMU monitor
# Press Ctrl+Alt+2 to access monitor, Ctrl+Alt+1 to return
```

## Unloading Modules

```bash
# Remove modules
sudo rmmod mouse_driver
sudo rmmod keyboard_driver

# Verify
lsmod | grep -E 'keyboard_driver|mouse_driver'
```

## Code Structure

```
kernel-driver-project/
├── README.md                    # This file
├── Makefile                     # Build system for modules and userspace
├── install.sh                   # Module installation script
├── drivers/
│   ├── keyboard_driver.c       # Keyboard driver implementation
│   └── mouse_driver.c          # Mouse driver implementation
├── userspace/
│   └── reader.c                # Event reader utility
├── tests/
│   ├── test_keyboard.sh        # Keyboard test script
│   └── test_mouse.sh           # Mouse test script
└── docs/
    ├── lab_report.md           # Simplified explanation
    └── presentation.md         # PPT outline
```

## Design Choices

### Why Linux Input Subsystem?

- **Standard Interface**: User-space applications already support `/dev/input/event*`
- **Feature Rich**: Handles event queuing, timestamps, multi-app access
- **No Character Device Needed**: Avoids writing custom read/write/poll implementations

### Simulated vs Real Hardware

- Real PS/2 ports are rare on modern systems
- Sysfs injection allows testing without hardware
- Design mirrors real driver structure (would need minimal changes for real IRQs)

### Locking Strategy

- **Spinlocks**: Used in IRQ context (tasklet) for buffer access
- **Mutex**: Would use for process context sleepable operations (not needed here)
- Prevents race conditions between injection and event processing

### Scan Code Translation

- Minimal US keyboard layout (letters, digits, common keys)
- Supports make/break codes (press/release via bit 7)
- Basic shift key handling demonstrates stateful processing
- Full layout would need scan code set 2 and more keycodes

## Troubleshooting

### Module Won't Load

**Error**: `insmod: ERROR: could not insert module: Invalid module format`
- **Fix**: Rebuild against correct kernel headers: `make clean && make`

**Error**: `Unknown symbol input_register_device`
- **Fix**: Ensure kernel has INPUT subsystem enabled (it should be)

### Device Not Appearing

- Check `dmesg` for registration messages
- Run `cat /proc/bus/input/devices` to see all input devices
- Verify module loaded: `lsmod | grep keyboard_driver`

### Permission Denied Reading Events

- Input devices require root or user in `input` group
- Run with `sudo` or add user to group: `sudo usermod -a -G input $USER`

### No Events Received

- Verify sysfs paths exist: `find /sys -name inject_scancode`
- Check injection worked: `dmesg | tail` after injection
- Ensure reading correct event device (check dmesg for "registered as eventX")

### QEMU Issues

- **Kernel panic**: Ensure kernel and initrd match
- **Module compile errors in VM**: Install kernel headers in guest
- **No network**: Add `-netdev user,id=net0 -device e1000,netdev=net0` to QEMU command

## Learning Objectives

This project demonstrates:
1. **Kernel Module Basics**: init/exit, module parameters, logging
2. **Input Subsystem**: Registration, event reporting, device lifecycle
3. **Interrupt Handling**: IRQ request/free, bottom-half processing with tasklets
4. **Synchronization**: Spinlocks for shared data in interrupt context
5. **Sysfs Interface**: Kernel-to-userspace communication
6. **Driver Testing**: Strategies for testing without physical hardware

## References

- Linux Device Drivers, 3rd Edition (LDD3)
- Linux Kernel Documentation: Documentation/input/
- Kernel source: drivers/input/keyboard/ and drivers/input/mouse/
- Input event codes: include/uapi/linux/input-event-codes.h

## License

Educational project - MIT License. Use freely for learning.

## Authors

Operating Systems Course Project - 2025
