/*
 * keyboard_driver.c - Virtual PS/2 Keyboard Driver
 * 
 * Educational Linux kernel module demonstrating:
 * - Input subsystem integration
 * - Scan code to keycode translation
 * - IRQ simulation with tasklets
 * - Sysfs interface for testing
 * - Proper locking and buffering
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

#define DRIVER_NAME "virtual_keyboard"
#define BUFFER_SIZE 128

/* Driver data structure */
struct vkbd_device {
    struct input_dev *input;
    struct tasklet_struct tasklet;
    spinlock_t buffer_lock;
    unsigned char buffer[BUFFER_SIZE];
    unsigned int head;
    unsigned int tail;
    bool shift_pressed;
};

static struct vkbd_device *vkbd_dev;

/*
 * Scan Code to Linux Keycode Translation Table
 * PS/2 Set 1 scan codes (make codes, release = make | 0x80)
 */
static const unsigned short scancode_to_keycode[128] = {
    [0x01] = KEY_ESC,
    [0x02] = KEY_1,
    [0x03] = KEY_2,
    [0x04] = KEY_3,
    [0x05] = KEY_4,
    [0x06] = KEY_5,
    [0x07] = KEY_6,
    [0x08] = KEY_7,
    [0x09] = KEY_8,
    [0x0A] = KEY_9,
    [0x0B] = KEY_0,
    [0x0C] = KEY_MINUS,
    [0x0D] = KEY_EQUAL,
    [0x0E] = KEY_BACKSPACE,
    [0x0F] = KEY_TAB,
    [0x10] = KEY_Q,
    [0x11] = KEY_W,
    [0x12] = KEY_E,
    [0x13] = KEY_R,
    [0x14] = KEY_T,
    [0x15] = KEY_Y,
    [0x16] = KEY_U,
    [0x17] = KEY_I,
    [0x18] = KEY_O,
    [0x19] = KEY_P,
    [0x1A] = KEY_LEFTBRACE,
    [0x1B] = KEY_RIGHTBRACE,
    [0x1C] = KEY_ENTER,
    [0x1D] = KEY_LEFTCTRL,
    [0x1E] = KEY_A,
    [0x1F] = KEY_S,
    [0x20] = KEY_D,
    [0x21] = KEY_F,
    [0x22] = KEY_G,
    [0x23] = KEY_H,
    [0x24] = KEY_J,
    [0x25] = KEY_K,
    [0x26] = KEY_L,
    [0x27] = KEY_SEMICOLON,
    [0x28] = KEY_APOSTROPHE,
    [0x29] = KEY_GRAVE,
    [0x2A] = KEY_LEFTSHIFT,
    [0x2B] = KEY_BACKSLASH,
    [0x2C] = KEY_Z,
    [0x2D] = KEY_X,
    [0x2E] = KEY_C,
    [0x2F] = KEY_V,
    [0x30] = KEY_B,
    [0x31] = KEY_N,
    [0x32] = KEY_M,
    [0x33] = KEY_COMMA,
    [0x34] = KEY_DOT,
    [0x35] = KEY_SLASH,
    [0x36] = KEY_RIGHTSHIFT,
    [0x37] = KEY_KPASTERISK,
    [0x38] = KEY_LEFTALT,
    [0x39] = KEY_SPACE,
    [0x3A] = KEY_CAPSLOCK,
    [0x3B] = KEY_F1,
    [0x3C] = KEY_F2,
    [0x3D] = KEY_F3,
    [0x3E] = KEY_F4,
    [0x3F] = KEY_F5,
    [0x40] = KEY_F6,
    [0x41] = KEY_F7,
    [0x42] = KEY_F8,
    [0x43] = KEY_F9,
    [0x44] = KEY_F10,
};

/*
 * Buffer Management Functions
 * Circular buffer for scan codes with proper locking
 */
static bool buffer_empty(struct vkbd_device *dev)
{
    return dev->head == dev->tail;
}

static bool buffer_full(struct vkbd_device *dev)
{
    return ((dev->head + 1) % BUFFER_SIZE) == dev->tail;
}

static void buffer_push(struct vkbd_device *dev, unsigned char scancode)
{
    unsigned long flags;
    
    spin_lock_irqsave(&dev->buffer_lock, flags);
    
    if (!buffer_full(dev)) {
        dev->buffer[dev->head] = scancode;
        dev->head = (dev->head + 1) % BUFFER_SIZE;
    } else {
        pr_warn("%s: Buffer overflow, dropping scan code 0x%02x\n",
                DRIVER_NAME, scancode);
    }
    
    spin_unlock_irqrestore(&dev->buffer_lock, flags);
}

static int buffer_pop(struct vkbd_device *dev, unsigned char *scancode)
{
    unsigned long flags;
    int ret = 0;
    
    spin_lock_irqsave(&dev->buffer_lock, flags);
    
    if (!buffer_empty(dev)) {
        *scancode = dev->buffer[dev->tail];
        dev->tail = (dev->tail + 1) % BUFFER_SIZE;
        ret = 1;
    }
    
    spin_unlock_irqrestore(&dev->buffer_lock, flags);
    
    return ret;
}

/*
 * Tasklet Bottom-Half Handler
 * Processes buffered scan codes and reports events to input subsystem
 */
static void vkbd_tasklet_handler(unsigned long data)
{
    struct vkbd_device *dev = (struct vkbd_device *)data;
    unsigned char scancode;
    unsigned short keycode;
    bool key_release;
    
    while (buffer_pop(dev, &scancode)) {
        /* Check if this is a key release (bit 7 set) */
        key_release = (scancode & 0x80) != 0;
        scancode &= 0x7F;  /* Clear release bit to get base scan code */
        
        /* Translate scan code to Linux keycode */
        if (scancode >= ARRAY_SIZE(scancode_to_keycode)) {
            pr_debug("%s: Unknown scan code 0x%02x\n", DRIVER_NAME, scancode);
            continue;
        }
        
        keycode = scancode_to_keycode[scancode];
        if (keycode == 0) {
            pr_debug("%s: No mapping for scan code 0x%02x\n", DRIVER_NAME, scancode);
            continue;
        }
        
        /* Track shift key state for demonstration */
        if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
            dev->shift_pressed = !key_release;
        }
        
        /* Report key event to input subsystem */
        input_report_key(dev->input, keycode, !key_release);
        input_sync(dev->input);
        
        pr_debug("%s: Scan code 0x%02x -> keycode %d (%s)\n",
                 DRIVER_NAME, scancode, keycode, 
                 key_release ? "release" : "press");
    }
}

/*
 * Simulated IRQ Handler (Top Half)
 * In real driver, this would be called by hardware interrupt
 * Here, triggered by sysfs injection
 */
static void vkbd_simulate_irq(unsigned char scancode)
{
    /* Buffer the scan code */
    buffer_push(vkbd_dev, scancode);
    
    /* Schedule tasklet for bottom-half processing */
    tasklet_schedule(&vkbd_dev->tasklet);
}

/*
 * Sysfs Interface for Testing
 * Allows user-space to inject scan codes: echo 0x1E > /sys/.../inject_scancode
 */
static ssize_t inject_scancode_store(struct device *dev,
                                      struct device_attribute *attr,
                                      const char *buf, size_t count)
{
    unsigned long scancode;
    int ret;
    
    ret = kstrtoul(buf, 0, &scancode);
    if (ret)
        return ret;
    
    if (scancode > 0xFF) {
        pr_warn("%s: Invalid scan code 0x%lx (must be 0-255)\n",
                DRIVER_NAME, scancode);
        return -EINVAL;
    }
    
    pr_info("%s: Injecting scan code 0x%02lx\n", DRIVER_NAME, scancode);
    vkbd_simulate_irq((unsigned char)scancode);
    
    return count;
}

static DEVICE_ATTR_WO(inject_scancode);

static struct attribute *vkbd_attrs[] = {
    &dev_attr_inject_scancode.attr,
    NULL,
};

static const struct attribute_group vkbd_attr_group = {
    .attrs = vkbd_attrs,
};

/*
 * Module Initialization
 */
static int __init vkbd_init(void)
{
    int ret;
    int i;
    
    pr_info("%s: Initializing virtual keyboard driver\n", DRIVER_NAME);
    
    /* Allocate driver data structure */
    vkbd_dev = kzalloc(sizeof(struct vkbd_device), GFP_KERNEL);
    if (!vkbd_dev)
        return -ENOMEM;
    
    /* Initialize buffer and lock */
    spin_lock_init(&vkbd_dev->buffer_lock);
    vkbd_dev->head = 0;
    vkbd_dev->tail = 0;
    vkbd_dev->shift_pressed = false;
    
    /* Initialize tasklet for bottom-half processing */
    tasklet_init(&vkbd_dev->tasklet, vkbd_tasklet_handler,
                 (unsigned long)vkbd_dev);
    
    /* Allocate input device */
    vkbd_dev->input = input_allocate_device();
    if (!vkbd_dev->input) {
        pr_err("%s: Failed to allocate input device\n", DRIVER_NAME);
        ret = -ENOMEM;
        goto err_free_dev;
    }
    
    /* Setup input device properties */
    vkbd_dev->input->name = "Virtual PS/2 Keyboard";
    vkbd_dev->input->phys = "virtual/input0";
    vkbd_dev->input->id.bustype = BUS_HOST;
    vkbd_dev->input->id.vendor = 0x0001;
    vkbd_dev->input->id.product = 0x0001;
    vkbd_dev->input->id.version = 0x0100;
    
    /* Set event types: we generate key events */
    vkbd_dev->input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
    
    /* Set which keys we can generate */
    for (i = 0; i < ARRAY_SIZE(scancode_to_keycode); i++) {
        if (scancode_to_keycode[i] != 0)
            set_bit(scancode_to_keycode[i], vkbd_dev->input->keybit);
    }
    
    /* Register input device with the input subsystem */
    ret = input_register_device(vkbd_dev->input);
    if (ret) {
        pr_err("%s: Failed to register input device\n", DRIVER_NAME);
        goto err_free_input;
    }
    
    /* Create sysfs interface for scan code injection */
    ret = sysfs_create_group(&vkbd_dev->input->dev.kobj, &vkbd_attr_group);
    if (ret) {
        pr_err("%s: Failed to create sysfs group\n", DRIVER_NAME);
        goto err_unregister_input;
    }
    
    pr_info("%s: Successfully registered as %s\n", DRIVER_NAME,
            dev_name(&vkbd_dev->input->dev));
    pr_info("%s: Inject scan codes via: /sys/devices/virtual/input/%s/inject_scancode\n",
            DRIVER_NAME, dev_name(&vkbd_dev->input->dev));
    
    return 0;

err_unregister_input:
    input_unregister_device(vkbd_dev->input);
    vkbd_dev->input = NULL;  /* input_unregister_device frees it */
err_free_input:
    if (vkbd_dev->input)
        input_free_device(vkbd_dev->input);
err_free_dev:
    tasklet_kill(&vkbd_dev->tasklet);
    kfree(vkbd_dev);
    return ret;
}

/*
 * Module Cleanup
 */
static void __exit vkbd_exit(void)
{
    pr_info("%s: Cleaning up virtual keyboard driver\n", DRIVER_NAME);
    
    /* Remove sysfs interface */
    sysfs_remove_group(&vkbd_dev->input->dev.kobj, &vkbd_attr_group);
    
    /* Kill tasklet */
    tasklet_kill(&vkbd_dev->tasklet);
    
    /* Unregister input device */
    input_unregister_device(vkbd_dev->input);
    
    /* Free driver data */
    kfree(vkbd_dev);
    
    pr_info("%s: Driver unloaded\n", DRIVER_NAME);
}

module_init(vkbd_init);
module_exit(vkbd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Course Project");
MODULE_DESCRIPTION("Virtual PS/2 Keyboard Driver for Educational Purposes");
MODULE_VERSION("1.0");
