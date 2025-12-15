#ifndef _SHARED_STORAGE_H_
#define _SHARED_STORAGE_H_

#include <ff.h>

FRESULT mount_shared_storage();
void unmount_shared_storage();

#endif