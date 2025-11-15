/*
 * reader.c - User-space Input Event Reader
 * 
 * Reads events from Linux input devices (/dev/input/eventX)
 * and displays them in human-readable format.
 *
 * Supports keyboard and mouse events from our virtual drivers.
 *
 * Usage: ./reader /dev/input/eventX
 *
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

/* Color codes for pretty output */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_CYAN    "\033[1;36m"

/*
 * Convert Linux keycode to readable string
 */
const char* keycode_to_string(unsigned int code)
{
    static char buffer[32];
    
    /* Common keys */
    switch (code) {
        case KEY_ESC: return "ESC";
        case KEY_1: return "1";
        case KEY_2: return "2";
        case KEY_3: return "3";
        case KEY_4: return "4";
        case KEY_5: return "5";
        case KEY_6: return "6";
        case KEY_7: return "7";
        case KEY_8: return "8";
        case KEY_9: return "9";
        case KEY_0: return "0";
        case KEY_MINUS: return "MINUS";
        case KEY_EQUAL: return "EQUAL";
        case KEY_BACKSPACE: return "BACKSPACE";
        case KEY_TAB: return "TAB";
        case KEY_Q: return "Q";
        case KEY_W: return "W";
        case KEY_E: return "E";
        case KEY_R: return "R";
        case KEY_T: return "T";
        case KEY_Y: return "Y";
        case KEY_U: return "U";
        case KEY_I: return "I";
        case KEY_O: return "O";
        case KEY_P: return "P";
        case KEY_LEFTBRACE: return "LEFT_BRACE";
        case KEY_RIGHTBRACE: return "RIGHT_BRACE";
        case KEY_ENTER: return "ENTER";
        case KEY_LEFTCTRL: return "LEFT_CTRL";
        case KEY_A: return "A";
        case KEY_S: return "S";
        case KEY_D: return "D";
        case KEY_F: return "F";
        case KEY_G: return "G";
        case KEY_H: return "H";
        case KEY_J: return "J";
        case KEY_K: return "K";
        case KEY_L: return "L";
        case KEY_SEMICOLON: return "SEMICOLON";
        case KEY_APOSTROPHE: return "APOSTROPHE";
        case KEY_GRAVE: return "GRAVE";
        case KEY_LEFTSHIFT: return "LEFT_SHIFT";
        case KEY_BACKSLASH: return "BACKSLASH";
        case KEY_Z: return "Z";
        case KEY_X: return "X";
        case KEY_C: return "C";
        case KEY_V: return "V";
        case KEY_B: return "B";
        case KEY_N: return "N";
        case KEY_M: return "M";
        case KEY_COMMA: return "COMMA";
        case KEY_DOT: return "DOT";
        case KEY_SLASH: return "SLASH";
        case KEY_RIGHTSHIFT: return "RIGHT_SHIFT";
        case KEY_KPASTERISK: return "KEYPAD_*";
        case KEY_LEFTALT: return "LEFT_ALT";
        case KEY_SPACE: return "SPACE";
        case KEY_CAPSLOCK: return "CAPS_LOCK";
        case KEY_F1: return "F1";
        case KEY_F2: return "F2";
        case KEY_F3: return "F3";
        case KEY_F4: return "F4";
        case KEY_F5: return "F5";
        case KEY_F6: return "F6";
        case KEY_F7: return "F7";
        case KEY_F8: return "F8";
        case KEY_F9: return "F9";
        case KEY_F10: return "F10";
        case BTN_LEFT: return "MOUSE_LEFT";
        case BTN_RIGHT: return "MOUSE_RIGHT";
        case BTN_MIDDLE: return "MOUSE_MIDDLE";
        default:
            snprintf(buffer, sizeof(buffer), "KEY_%u", code);
            return buffer;
    }
}

/*
 * Get current timestamp string
 */
void get_timestamp(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, len, "%H:%M:%S", tm_info);
}

/*
 * Print event in human-readable format
 */
void print_event(struct input_event *ev)
{
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    /* Event type */
    switch (ev->type) {
        case EV_KEY:
            /* Key or button event */
            if (ev->code >= BTN_MOUSE && ev->code < BTN_JOYSTICK) {
                /* Mouse button */
                printf("%s[%s]%s %sMOUSE_BTN%s %-15s %s%s%s\n",
                       COLOR_CYAN, timestamp, COLOR_RESET,
                       COLOR_YELLOW, COLOR_RESET,
                       keycode_to_string(ev->code),
                       ev->value ? COLOR_GREEN : COLOR_RED,
                       ev->value ? "PRESSED" : "RELEASED",
                       COLOR_RESET);
            } else {
                /* Keyboard key */
                printf("%s[%s]%s %sKEY%s       %-15s %s%s%s\n",
                       COLOR_CYAN, timestamp, COLOR_RESET,
                       COLOR_BLUE, COLOR_RESET,
                       keycode_to_string(ev->code),
                       ev->value ? COLOR_GREEN : COLOR_RED,
                       ev->value ? "PRESSED" : "RELEASED",
                       COLOR_RESET);
            }
            break;
            
        case EV_REL:
            /* Relative movement (mouse) */
            if (ev->code == REL_X) {
                printf("%s[%s]%s %sMOUSE%s     X: %+4d\n",
                       COLOR_CYAN, timestamp, COLOR_RESET,
                       COLOR_YELLOW, COLOR_RESET,
                       ev->value);
            } else if (ev->code == REL_Y) {
                printf("%s[%s]%s %sMOUSE%s     Y: %+4d\n",
                       COLOR_CYAN, timestamp, COLOR_RESET,
                       COLOR_YELLOW, COLOR_RESET,
                       ev->value);
            } else if (ev->code == REL_WHEEL) {
                printf("%s[%s]%s %sMOUSE%s     WHEEL: %+4d\n",
                       COLOR_CYAN, timestamp, COLOR_RESET,
                       COLOR_YELLOW, COLOR_RESET,
                       ev->value);
            } else {
                printf("%s[%s]%s %sREL%s       code=%u value=%d\n",
                       COLOR_CYAN, timestamp, COLOR_RESET,
                       COLOR_YELLOW, COLOR_RESET,
                       ev->code, ev->value);
            }
            break;
            
        case EV_ABS:
            /* Absolute positioning */
            printf("%s[%s]%s %sABS%s       code=%u value=%d\n",
                   COLOR_CYAN, timestamp, COLOR_RESET,
                   COLOR_YELLOW, COLOR_RESET,
                   ev->code, ev->value);
            break;
            
        case EV_SYN:
            /* Synchronization event - indicates end of event group */
            if (ev->code == SYN_REPORT) {
                printf("%s[%s]%s %s--- EVENT COMPLETE ---%s\n",
                       COLOR_CYAN, timestamp, COLOR_RESET,
                       COLOR_RESET, COLOR_RESET);
            }
            break;
            
        case EV_MSC:
            /* Miscellaneous events */
            printf("%s[%s]%s %sMSC%s       code=%u value=%d\n",
                   COLOR_CYAN, timestamp, COLOR_RESET,
                   COLOR_YELLOW, COLOR_RESET,
                   ev->code, ev->value);
            break;
            
        default:
            printf("%s[%s]%s %sUNKNOWN%s   type=%u code=%u value=%d\n",
                   COLOR_CYAN, timestamp, COLOR_RESET,
                   COLOR_YELLOW, COLOR_RESET,
                   ev->type, ev->code, ev->value);
            break;
    }
    
    fflush(stdout);
}

/*
 * Get device name
 */
int get_device_name(int fd, char *name, size_t len)
{
    if (ioctl(fd, EVIOCGNAME(len), name) < 0) {
        return -1;
    }
    return 0;
}

/*
 * Main function
 */
int main(int argc, char *argv[])
{
    int fd;
    struct input_event ev;
    ssize_t bytes;
    char device_name[256] = "Unknown Device";
    
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX\n", argv[0]);
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s /dev/input/event0\n", argv[0]);
        fprintf(stderr, "\nTip: Use 'cat /proc/bus/input/devices' to find devices\n");
        return 1;
    }
    
    /* Open input device */
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: Cannot open %s: %s\n", argv[1], strerror(errno));
        fprintf(stderr, "Try running with sudo: sudo %s %s\n", argv[0], argv[1]);
        return 1;
    }
    
    /* Get device name */
    get_device_name(fd, device_name, sizeof(device_name));
    
    /* Print header */
    printf("\n");
    printf("========================================\n");
    printf("Input Event Reader\n");
    printf("========================================\n");
    printf("Device:  %s\n", argv[1]);
    printf("Name:    %s\n", device_name);
    printf("========================================\n");
    printf("Listening for events... (Press Ctrl+C to exit)\n");
    printf("========================================\n\n");
    
    /* Read events in a loop */
    while (1) {
        bytes = read(fd, &ev, sizeof(ev));
        
        if (bytes < 0) {
            if (errno == EINTR) {
                /* Interrupted by signal, continue */
                continue;
            }
            fprintf(stderr, "\nError reading event: %s\n", strerror(errno));
            break;
        }
        
        if (bytes != sizeof(ev)) {
            fprintf(stderr, "\nError: Short read (got %zd bytes, expected %zu)\n",
                    bytes, sizeof(ev));
            break;
        }
        
        /* Print the event */
        print_event(&ev);
    }
    
    close(fd);
    return 0;
}
