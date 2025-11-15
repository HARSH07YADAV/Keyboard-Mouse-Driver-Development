/*
 * mouse_driver.c - Virtual PS/2 Mouse Driver
 * 
 * Educational Linux kernel module demonstrating:
 * - Input subsystem integration for mouse events
 * - PS/2 3-byte packet parsing
 * - Relative motion and button tracking
 * - IRQ simulation with tasklets
 * - Sysfs interface for testing
 * - Proper locking and buffering
 *
 * PS/2 Mouse Packet Format (3 bytes):
 * Byte 0: [Y_overflow | X_overflow | Y_sign | X_sign | 1 | Middle | Right | Left]
 * Byte 1: X movement (8-bit signed)
 * Byte 2: Y movement (8-bit signed)
 *
 * License: MIT
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sysfs.h>

#define DRIVER_NAME "virtual_mouse"
#define BUFFER_SIZE 256  /* Must be multiple of 3 for packet alignment */
#define PACKET_SIZE 3

/* Driver data structure */
struct vmouse_device {
    struct input_dev *input;
    struct tasklet_struct tasklet;
    spinlock_t buffer_lock;
    unsigned char buffer[BUFFER_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned char packet[PACKET_SIZE];
    unsigned int packet_idx;
};

static struct vmouse_device *vmouse_dev;

/*
 * PS/2 Packet Bit Definitions
 */
#define PS2_LEFT_BTN    (1 << 0)
#define PS2_RIGHT_BTN   (1 << 1)
#define PS2_MIDDLE_BTN  (1 << 2)
#define PS2_ALWAYS_ONE  (1 << 3)
#define PS2_X_SIGN      (1 << 4)
#define PS2_Y_SIGN      (1 << 5)
#define PS2_X_OVERFLOW  (1 << 6)
#define PS2_Y_OVERFLOW  (1 << 7)

/*
 * Buffer Management Functions
 * Circular buffer for packet bytes with proper locking
 */
static bool buffer_empty(struct vmouse_device *dev)
{
    return dev->head == dev->tail;
}

static bool buffer_full(struct vmouse_device *dev)
{
    return ((dev->head + 1) % BUFFER_SIZE) == dev->tail;
}

static void buffer_push(struct vmouse_device *dev, unsigned char byte)
{
    unsigned long flags;
    
    spin_lock_irqsave(&dev->buffer_lock, flags);
    
    if (!buffer_full(dev)) {
        dev->buffer[dev->head] = byte;
        dev->head = (dev->head + 1) % BUFFER_SIZE;
    } else {
        pr_warn("%s: Buffer overflow, dropping byte 0x%02x\n",
                DRIVER_NAME, byte);
    }
    
    spin_unlock_irqrestore(&dev->buffer_lock, flags);
}

static int buffer_pop(struct vmouse_device *dev, unsigned char *byte)
{
    unsigned long flags;
    int ret = 0;
    
    spin_lock_irqsave(&dev->buffer_lock, flags);
    
    if (!buffer_empty(dev)) {
        *byte = dev->buffer[dev->tail];
        dev->tail = (dev->tail + 1) % BUFFER_SIZE;
        ret = 1;
    }
    
    spin_unlock_irqrestore(&dev->buffer_lock, flags);
    
    return ret;
}

/*
 * Parse and Process PS/2 Mouse Packet
 * Returns true if packet is valid
 */
static bool process_packet(struct vmouse_device *dev)
{
    unsigned char status = dev->packet[0];
    signed char dx_raw = (signed char)dev->packet[1];
    signed char dy_raw = (signed char)dev->packet[2];
    int dx, dy;
    bool left, right, middle;
    
    /* Validate packet - bit 3 should always be 1 */
    if (!(status & PS2_ALWAYS_ONE)) {
        pr_debug("%s: Invalid packet - bit 3 not set (0x%02x 0x%02x 0x%02x)\n",
                 DRIVER_NAME, dev->packet[0], dev->packet[1], dev->packet[2]);
        return false;
    }
    
    /* Check for overflow - just log and clamp if needed */
    if (status & PS2_X_OVERFLOW) {
        pr_debug("%s: X overflow detected\n", DRIVER_NAME);
    }
    if (status & PS2_Y_OVERFLOW) {
        pr_debug("%s: Y overflow detected\n", DRIVER_NAME);
    }
    
    /* Extract button states */
    left = (status & PS2_LEFT_BTN) != 0;
    right = (status & PS2_RIGHT_BTN) != 0;
    middle = (status & PS2_MIDDLE_BTN) != 0;
    
    /* Calculate relative movement (already signed) */
    dx = dx_raw;
    dy = dy_raw;
    
    /* PS/2 Y axis is inverted compared to Linux convention */
    dy = -dy;
    
    pr_debug("%s: Packet: buttons[L:%d R:%d M:%d] dx:%d dy:%d\n",
             DRIVER_NAME, left, right, middle, dx, dy);
    
    /* Report button events */
    input_report_key(dev->input, BTN_LEFT, left);
    input_report_key(dev->input, BTN_RIGHT, right);
    input_report_key(dev->input, BTN_MIDDLE, middle);
    
    /* Report relative motion */
    if (dx != 0)
        input_report_rel(dev->input, REL_X, dx);
    if (dy != 0)
        input_report_rel(dev->input, REL_Y, dy);
    
    /* Sync to indicate complete event */
    input_sync(dev->input);
    
    return true;
}

/*
 * Tasklet Bottom-Half Handler
 * Assembles 3-byte packets and processes them
 */
static void vmouse_tasklet_handler(unsigned long data)
{
    struct vmouse_device *dev = (struct vmouse_device *)data;
    unsigned char byte;
    
    while (buffer_pop(dev, &byte)) {
        dev->packet[dev->packet_idx++] = byte;
        
        /* Wait until we have a complete 3-byte packet */
        if (dev->packet_idx >= PACKET_SIZE) {
            process_packet(dev);
            dev->packet_idx = 0;  /* Reset for next packet */
        }
    }
}

/*
 * Simulated IRQ Handler (Top Half)
 * In real driver, this would be called by hardware interrupt
 * Here, triggered by sysfs injection
 */
static void vmouse_simulate_irq(unsigned char byte)
{
    /* Buffer the byte */
    buffer_push(vmouse_dev, byte);
    
    /* Schedule tasklet for bottom-half processing */
    tasklet_schedule(&vmouse_dev->tasklet);
}

/*
 * Sysfs Interface for Testing
 * Allows injection of complete packet: echo "0x09 0x10 0xF0" > inject_packet
 */
static ssize_t inject_packet_store(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf, size_t count)
{
    unsigned long bytes[3];
    int i, n;
    const char *p = buf;
    char *endp;
    
    /* Parse up to 3 space-separated hex values */
    for (i = 0; i < 3; i++) {
        /* Skip whitespace */
        while (*p && (*p == ' ' || *p == '\t' || *p == '\n'))
            p++;
        
        if (!*p)
            break;
        
        bytes[i] = simple_strtoul(p, &endp, 0);
        if (endp == p) {
            pr_warn("%s: Invalid packet format\n", DRIVER_NAME);
            return -EINVAL;
        }
        
        if (bytes[i] > 0xFF) {
            pr_warn("%s: Invalid byte value 0x%lx (must be 0-255)\n",
                    DRIVER_NAME, bytes[i]);
            return -EINVAL;
        }
        
        p = endp;
    }
    
    n = i;
    
    if (n != 3) {
        pr_warn("%s: Expected 3 bytes, got %d\n", DRIVER_NAME, n);
        return -EINVAL;
    }
    
    pr_info("%s: Injecting packet: 0x%02lx 0x%02lx 0x%02lx\n",
            DRIVER_NAME, bytes[0], bytes[1], bytes[2]);
    
    /* Inject the packet bytes */
    for (i = 0; i < 3; i++) {
        vmouse_simulate_irq((unsigned char)bytes[i]);
    }
    
    return count;
}

static DEVICE_ATTR_WO(inject_packet);

static struct attribute *vmouse_attrs[] = {
    &dev_attr_inject_packet.attr,
    NULL,
};

static const struct attribute_group vmouse_attr_group = {
    .attrs = vmouse_attrs,
};

/*
 * Module Initialization
 */
static int __init vmouse_init(void)
{
    int ret;
    
    pr_info("%s: Initializing virtual mouse driver\n", DRIVER_NAME);
    
    /* Allocate driver data structure */
    vmouse_dev = kzalloc(sizeof(struct vmouse_device), GFP_KERNEL);
    if (!vmouse_dev)
        return -ENOMEM;
    
    /* Initialize buffer and lock */
    spin_lock_init(&vmouse_dev->buffer_lock);
    vmouse_dev->head = 0;
    vmouse_dev->tail = 0;
    vmouse_dev->packet_idx = 0;
    
    /* Initialize tasklet for bottom-half processing */
    tasklet_init(&vmouse_dev->tasklet, vmouse_tasklet_handler,
                 (unsigned long)vmouse_dev);
    
    /* Allocate input device */
    vmouse_dev->input = input_allocate_device();
    if (!vmouse_dev->input) {
        pr_err("%s: Failed to allocate input device\n", DRIVER_NAME);
        ret = -ENOMEM;
        goto err_free_dev;
    }
    
    /* Setup input device properties */
    vmouse_dev->input->name = "Virtual PS/2 Mouse";
    vmouse_dev->input->phys = "virtual/input1";
    vmouse_dev->input->id.bustype = BUS_HOST;
    vmouse_dev->input->id.vendor = 0x0001;
    vmouse_dev->input->id.product = 0x0002;
    vmouse_dev->input->id.version = 0x0100;
    
    /* Set event types: relative positioning and buttons */
    vmouse_dev->input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
    
    /* Set which buttons we support */
    set_bit(BTN_LEFT, vmouse_dev->input->keybit);
    set_bit(BTN_RIGHT, vmouse_dev->input->keybit);
    set_bit(BTN_MIDDLE, vmouse_dev->input->keybit);
    
    /* Set relative axes */
    set_bit(REL_X, vmouse_dev->input->relbit);
    set_bit(REL_Y, vmouse_dev->input->relbit);
    
    /* Register input device with the input subsystem */
    ret = input_register_device(vmouse_dev->input);
    if (ret) {
        pr_err("%s: Failed to register input device\n", DRIVER_NAME);
        goto err_free_input;
    }
    
    /* Create sysfs interface for packet injection */
    ret = sysfs_create_group(&vmouse_dev->input->dev.kobj, &vmouse_attr_group);
    if (ret) {
        pr_err("%s: Failed to create sysfs group\n", DRIVER_NAME);
        goto err_unregister_input;
    }
    
    pr_info("%s: Successfully registered as %s\n", DRIVER_NAME,
            dev_name(&vmouse_dev->input->dev));
    pr_info("%s: Inject packets via: /sys/devices/virtual/input/%s/inject_packet\n",
            DRIVER_NAME, dev_name(&vmouse_dev->input->dev));
    pr_info("%s: Packet format: 'status_byte dx dy' (3 bytes, space-separated hex)\n",
            DRIVER_NAME);
    
    return 0;

err_unregister_input:
    input_unregister_device(vmouse_dev->input);
    vmouse_dev->input = NULL;  /* input_unregister_device frees it */
err_free_input:
    if (vmouse_dev->input)
        input_free_device(vmouse_dev->input);
err_free_dev:
    tasklet_kill(&vmouse_dev->tasklet);
    kfree(vmouse_dev);
    return ret;
}

/*
 * Module Cleanup
 */
static void __exit vmouse_exit(void)
{
    pr_info("%s: Cleaning up virtual mouse driver\n", DRIVER_NAME);
    
    /* Remove sysfs interface */
    sysfs_remove_group(&vmouse_dev->input->dev.kobj, &vmouse_attr_group);
    
    /* Kill tasklet */
    tasklet_kill(&vmouse_dev->tasklet);
    
    /* Unregister input device */
    input_unregister_device(vmouse_dev->input);
    
    /* Free driver data */
    kfree(vmouse_dev);
    
    pr_info("%s: Driver unloaded\n", DRIVER_NAME);
}

module_init(vmouse_init);
module_exit(vmouse_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Course Project");
MODULE_DESCRIPTION("Virtual PS/2 Mouse Driver for Educational Purposes");
MODULE_VERSION("1.0");
