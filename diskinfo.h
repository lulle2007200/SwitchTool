#ifndef DISKINFO_H
#define DISKINFO_H

#include <Windows.h>
#include "log.h"
#include "llist.h"

typedef struct disk_info_s{
	llist_node_t 	*next;
	llist_node_t 	*prev;
	LPTSTR			manufacturer;
	LPTSTR			friendlyName;
	LPTSTR			devPath;
	DWORD			diskNumber;
	LONGLONG		size;
	BOOL			isReadOnly;
}disk_info_t;

BOOL GetValidStorageDevices(llist_t *devList);

void DiskInfoDelete(llist_node_t *diskInfo);

#endif