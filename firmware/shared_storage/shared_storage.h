#ifndef _SHARED_STORAGE_H_
#define _SHARED_STORAGE_H_

#include <ff.h>

void init_shared_storage();
void do_shared_storage();
FRESULT mount_shared_storage();
void unmount_shared_storage();

#endif