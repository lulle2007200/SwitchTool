#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <Shlwapi.h>

#include "log.h"
#include "llist.h"
#include "diskinfo.h"
#include "utils.h"
#include "messages.h"

typedef DWORD(*menu_entry_callback)(void *data);

typedef struct menu_entry_s{
	llist_node_t *next;
	llist_node_t *prev;
	LPTSTR menuEntryString;
	menu_entry_callback menuEntryCallback;
	void *data;
}menu_entry_t;

typedef enum menu_common_entries_e {MENU_QUIT, MENU_CONTINUE, MENU_BACK, MENU_REFRESH, MENU_SHOW_ALL} menu_common_entries_t;

typedef enum menu_entry_type_e {MENU_COMMON_ENTRY, MENU_DISK_ENTRY} menu_entry_type_t;

DWORD MenuEntryQuitCallback(void *data);
DWORD MenuEntryRefreshCallback(void *data);

static menu_entry_t gMenuEntries[] = {
	[MENU_QUIT] 	= {.menuEntryString = MSG_QUIT, .menuEntryCallback = MenuEntryQuitCallback},
	[MENU_REFRESH] 	= {.menuEntryString = MSG_REFRESH_DEVICE_LIST, .menuEntryCallback = MenuEntryRefreshCallback},
};

DWORD MenuEntryBack(void *data){
	return(MENU_BACK);
}

DWORD MenuDiskCallback(void *data){
	return(MENU_QUIT);
}




DWORD MenuEntryRefreshCallback(void *data){
	return(MENU_REFRESH);
}

DWORD MenuEntryQuitCallback(void *data){
	return(MENU_QUIT);
}

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

void FlushInput(){
	//TODO: fix this shit. flush before reading input to ignore input from earlier
	while(_gettchar() != TEXT('\n')){};
}

DWORD UserSelectOption(DWORD min, DWORD max){
	DWORD input;
	while(TRUE){
		_tprintf(MSG_SELECT_OPTION);
		if(_tscanf_s(TEXT("%i"), &input) == 1){
			if(input >= min && input <= max){
				break;
			}
		}
		_tprintf(MSG_INVALID_OPTION TEXT("\n"));
		FlushInput();
	}
	return(input);
}

DWORD DisplayMenu(llist_t *menuList, LPTSTR menuTitle){
	llist_node_t *current;
	DWORD i = 1;
	_tprintf(TEXT("%s\n"), menuTitle);
	llistForEach(menuList, current){
		_tprintf(TEXT("[%i] %s\n"), i, ((menu_entry_t*)current)->menuEntryString);
		i++;
	}
	DWORD userSelection = UserSelectOption(1, i - 1);
	menu_entry_t *selectedEntry = (menu_entry_t*)llistGetIdx(menuList, userSelection - 1);
	_tprintf(MSG_H_SEPERATOR);
	DWORD result = selectedEntry->menuEntryCallback(selectedEntry->data);
	return(result);
}

DWORD MenuShowAllCallback(void *data){
	llist_t *diskList = (llist_t*)data;
	llist_node_t *current;
	llist_t menuList;
	llistInit(&menuList);
	llistInsertHead(&menuList, (llist_node_t*)&gMenuEntries[MENU_QUIT]);
	llistInsertHead(&menuList, (llist_node_t*)&gMenuEntries[MENU_REFRESH]);
	llistForEach(diskList, current){
		menu_entry_t *newEntry;
		newEntry = malloc(sizeof(menu_entry_t));
		newEntry->menuEntryCallback = MenuDiskCallback;
		newEntry->menuEntryString = ((disk_info_t*)current)->friendlyName;
		newEntry->data = current;
		llistInsertHead(&menuList, (llist_node_t*)newEntry);
	}
	DWORD result = DisplayMenu(&menuList, MSG_SELECT_DEVICE);
	return(MENU_SHOW_ALL);
}

int main(int argc, char *argv[]){
	DWORD result = -1;
	LogSetQuiet(false);

	_tprintf(MSG_MORE_INFO);
	_tprintf(MSG_H_SEPERATOR);

	if(IsElevated() == FALSE){
		LogError(MSG_ERR_NOT_ELEVATED);
		return(1);
	}

	do{
		llist_t diskList;
		llistInit(&diskList);
		GetValidStorageDevices(&diskList);

		if(llistIsEmpty(&diskList)){
			llist_t menuList;
			llistInit(&menuList);
			llistInsertHead(&menuList, (llist_node_t*)&gMenuEntries[MENU_QUIT]);
			llistInsertHead(&menuList, (llist_node_t*)&gMenuEntries[MENU_REFRESH]);
			result = DisplayMenu(&menuList, MSG_NO_DEVICES);
			switch(result){
				case MENU_REFRESH:
					continue;
				case MENU_QUIT:
					return(0);
			}
		}

		llist_t hekateDisks;
		GetHekateUmsDisks(&diskList, &hekateDisks);
		if(llistIsEmpty(&hekateDisks) == FALSE){
			llist_t menuList;
			llistInit(&menuList);
			llistInsertHead(&menuList, (llist_node_t*)&gMenuEntries[MENU_QUIT]);
			llistInsertHead(&menuList, (llist_node_t*)&gMenuEntries[MENU_REFRESH]);
			menu_entry_t menuShowAll;
			menuShowAll.menuEntryCallback = MenuShowAllCallback;
			menuShowAll.menuEntryString = MSG_SHOW_ALL;
			menuShowAll.data = &diskList;
			llistInsertHead(&menuList, (llist_node_t*)&menuShowAll);
			result = DisplayMenu(&menuList, MSG_NO_HEKATE_DEVICES);
			switch(result){
				case MENU_REFRESH:
					continue;
				case MENU_QUIT:
					return(0);
			}
		}

	}while(result != MENU_QUIT);
	return(1);

	// llist_t hekateUmsDiskList;
	// llistInit(&hekateUmsDiskList);
	// GetHekateUmsDisks(&diskList, &hekateUmsDiskList);

	// if(llistIsEmpty(&hekateUmsDiskList)){

	// }else{
	// 	disk_info_t *selectedDisk;
	// 	if(llistGetLength(&hekateUmsDiskList) == 1){
	// 		//selectedDisk = (disk_info_t*)llistGetHead(&hekateUmsDiskList);
	// 	}else{
	// 		//selectedDisk = SelectHekateDisk(&hekateUmsDiskList);
	// 	}
	// }

    return(1);
}