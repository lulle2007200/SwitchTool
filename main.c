#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <Shlwapi.h>

#include "log.h"
#include "llist.h"
#include "diskinfo.h"
#include "utils.h"
#include "messages.h"

void GetHekateUmsDisks(llist_t *diskList, llist_t *hekateUmsDiskList){
	llist_node_t *current;
	LPTSTR result;
	llistForEach(diskList,current){
		result = _tcsstr(((disk_info_t*)current)->friendlyName, TEXT("hekate"));
		if(result != NULL){
			llist_node_t *temp = current;
			current = current->prev;
			llistRemoveNode(diskList, temp);
			llistInsertHead(hekateUmsDiskList, temp);
		}
	}
}

int main(int argc, char *argv[]){
	LogSetQuiet(false);

	_tprintf(MSG_MORE_INFO);
	_tprintf(MSG_H_SEPERATOR);

	if(IsElevated() == FALSE){
		LogError(MSG_ERR_NOT_ELEVATED);
		return(1);
	}

	llist_t diskList;
	llistInit(&diskList);
	GetValidStorageDevices(&diskList);

	llist_t hekateUmsDiskList;
	llistInit(&hekateUmsDiskList);
	GetHekateUmsDisks(&diskList, &hekateUmsDiskList);


	llist_node_t *current;
	llistForEach(&diskList, current){
		LogInfo(TEXT("diskNumber: %i"), ((disk_info_t*)current)->diskNumber);
		LogInfo(TEXT("diskPath: %s"), ((disk_info_t*)current)->devPath);
		LogInfo(TEXT("friendlyName: %s"), ((disk_info_t*)current)->friendlyName);
		LogInfo(TEXT("manufacturer: %s"), ((disk_info_t*)current)->manufacturer);
		LogInfo(TEXT("size: %lld"), ((disk_info_t*)current)->size);
		LogInfo(TEXT("isReadOnly: %i\n"), ((disk_info_t*)current)->isReadOnly);
	}

	llistDeleteAll(&diskList, DiskInfoDelete);
	llistDeleteAll(&hekateUmsDiskList, DiskInfoDelete);

    return(0);
}