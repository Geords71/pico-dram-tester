
// TinyUSB
#include <tusb.h>
#include <bsp/board.h>
#include <ff.h>

#define SS_MOUNT_POINT "/"
static FATFS filesystem;

int f_getc(FIL* fp) {
    unsigned char c;
    UINT br;  // number of bytes actually read
    FRESULT res = f_read(fp, &c, 1, &br);

    if (res != FR_OK || br == 0) {
        return EOF;   // error or end of file
    }
    return c;         // return the character as int
}

TCHAR* f_gets(TCHAR* buf, int size, FIL* fp) {
    int i = 0;
    int c;

    if (size <= 0 || buf == NULL || fp == NULL) {
        return NULL; // invalid arguments
    }

    while (i < size - 1) {          // leave room for '\0'
        c = f_getc(fp);              // read next character
        if (c == EOF) {
            if (i == 0) return NULL; // nothing read
            break;                   // stop, return what we have
        }
        buf[i++] = (char)c;
        if (c == '\n') break;       // stop at newline
    }

    buf[i] = '\0';                  // null-terminate
    return buf;
}

FRESULT mount_shared_storage() {
    int result = f_mount(&filesystem, SS_MOUNT_POINT, 1);
    if (result != FR_OK)
    {
        printf("f_mount fail rc=%d\n", result);
    } 
    return result;
}

void unmount_shared_storage() {
    f_unmount(SS_MOUNT_POINT);
}

void init_shared_storage () {
    // Get TinyUSB going.
    board_init();
    tusb_init();
}

void do_shared_storage() {
    // Perform routine handling of tusb device events.
    tud_task();
}
