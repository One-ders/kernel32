/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2018 Aleph One Ltd.
 *
 * Created by Timothy Manning <timothy@yaffs.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "test_yaffs_mount_EBUSY.h"

static int handle=-1;

int test_yaffs_unmount_EBUSY(void)
{
	int output=0;
	int error_code=0;

	handle=yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE);
	if (handle<0){
		printf("failed to open file\n");
		return -1;
	}

	output=yaffs_unmount(YAFFS_MOUNT_POINT);
	if (output==-1){
		error_code=yaffs_get_error();
		if (abs(error_code)==EBUSY){
			return 1;
		} else {
			print_message("different error than expected\n",2);
			return -1;
		}
	} else {
		print_message("non existant mount point unmounted.(which is a bad thing)\n",2);
		return -1;
	}

}
int test_yaffs_unmount_EBUSY_clean(void)
{
	if (handle>=0){
		return yaffs_close(handle);	
	} else {
		return 1;
	}
}

