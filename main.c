#include "log.h"
#include "llist.h"
#include "diskinfo.h"
#include "utils.h"

#include <Shlwapi.h>


int main(int argc, char *argv[]){
	LogSetQuiet(false);

	if(IsElevated() == FALSE){
		LogError("This application requires admin privileges.");
		return(1);
	}

	llist_t diskList;
	llistInit(&diskList);
	GetValidStorageDevices(&diskList);

	llist_node_t *current;
	LPSTR result;
	llistForEach(&diskList, current){
		result = StrStr(((disk_info_t*)current)->friendlyName, L"hekate");
		if(result != NULL){
			//LogInfo("%S", ((disk_info_t*)current)->friendlyName);
		}
	}


	llistForEach(&diskList, current){
		LogInfo("diskNumber: %i", ((disk_info_t*)current)->diskNumber);
		LogInfo("diskPath: %S", ((disk_info_t*)current)->devPath);
		LogInfo("friendlyName: %S", ((disk_info_t*)current)->friendlyName);
		LogInfo("manufacturer: %S", ((disk_info_t*)current)->manufacturer);
		LogInfo("size: %lld", ((disk_info_t*)current)->size);
		LogInfo("isReadOnly: %i\n", ((disk_info_t*)current)->isReadOnly);
	}

    return(0);
}