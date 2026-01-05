#ifndef _SHARED_STORAGE_H
#define _SHARED_STORAGE_H

#include <ff.h>

FRESULT mount_shared_storage();
void unmount_shared_storage();

#endif