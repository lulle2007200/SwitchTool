#include "log.h"
#include "llist.h"
#include "diskinfo.h"
#include "utils.h"


int main(int argc, char *argv[]){
	LogSetQuiet(false);

	if(IsElevated() == FALSE){
		LogError("This application requires admin previleges.");
		return(1);
	}

	llist_t diskList;
	llistInit(&diskList);
	GetValidStorageDevices(&diskList);

	llist_node_t *current;
	llistForEach(&diskList, current){
		LogInfo("diskNumber: %i", ((disk_info_t*)current)->diskNumber);
		LogInfo("diskPath: %S", ((disk_info_t*)current)->devPath);
		LogInfo("friendlyName: %S", ((disk_info_t*)current)->friendlyName);
		LogInfo("manufacturer: %S", ((disk_info_t*)current)->manufacturer);
		LogInfo("size: %lld", ((disk_info_t*)current)->size);
	}

    return(0);
}