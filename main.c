#include <Windows.h>
#include <string.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <vcruntime.h>

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

typedef struct menu_restore_info_s{
	LPTSTR defaultFilename;
	disk_info_t *diskInfo;
}menu_restore_info_t;

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
	disk_info_t *diskInfo = (disk_info_t*)data;
	LogInfo(TEXT("current drive path: %s"), diskInfo->devPath);
	LogInfo(TEXT("current drive manufactuer: %s"), diskInfo->manufacturer);
	LogInfo(TEXT("current drive name: %s"), diskInfo->friendlyName);
	LogInfo(TEXT("current drive isReadOnly: %i"), diskInfo->isReadOnly);
	LogInfo(TEXT("current drive size: %llu"), diskInfo->size);
	return(MENU_QUIT);
}

DWORD TestCallback(void *data);



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
		newEntry->menuEntryCallback = TestCallback;
		//TODO: more info in menu entry string (size, serial etc.)
		newEntry->menuEntryString = ((disk_info_t*)current)->friendlyName;
		newEntry->data = current;
		llistInsertHead(&menuList, (llist_node_t*)newEntry);
	}
	DWORD result = DisplayMenu(&menuList, MSG_SELECT_DEVICE);

	return(result);
}

DWORD MenuRestoreFromFileCallback(void *data){

}

typedef enum hekate_disk_type_e {
	HEKATE_TYPE_SD_RAW = 0,
	HEKATE_TYPE_EMMC_GPP,
	HEKATE_TYPE_EMMC_BOOT0,
	HEKATE_TYPE_EMMC_BOOT1,
	HEKATE_TYPE_EMUMMC_GPP,
	HEKATE_TYPE_EMUMMC_BOOT0,
	HEKATE_TYPE_EMUMMC_BOOT1,
	HEKATE_TYPE_COUNT,
	HEKATE_TYPE_UNK
}hekate_disk_type_t;

static const LPTSTR gHekateDiskTypeNames[] = {
	[HEKATE_TYPE_SD_RAW] = TEXT("hekate SD RAW USB Device"),
	[HEKATE_TYPE_EMMC_GPP] = TEXT("hekate eMMC GPP USB Device"),
	[HEKATE_TYPE_EMMC_BOOT0] = TEXT("hekate eMMC BOOT0 USB Device"),
	[HEKATE_TYPE_EMMC_BOOT1] = TEXT("hekate eMMC BOOT1 USB Device"),
	[HEKATE_TYPE_EMUMMC_GPP] = TEXT("hekate SD GPP USB Device"),
	[HEKATE_TYPE_EMUMMC_BOOT0] = TEXT("hekate SD BOOT0 USB Device"),
	[HEKATE_TYPE_EMUMMC_BOOT1] = TEXT("hekate SD BOOT1 USB Device")
};

static const LPTSTR gHekateDefaultBackupFileNames[] = {
	[HEKATE_TYPE_SD_RAW] = TEXT("SD_Raw.img"),
	[HEKATE_TYPE_EMMC_GPP] = TEXT("GPP.img"),
	[HEKATE_TYPE_EMMC_BOOT0] = TEXT("BOOT0.img"),
	[HEKATE_TYPE_EMMC_BOOT1] = TEXT("BOOT1.img"),
	[HEKATE_TYPE_EMUMMC_GPP] = TEXT("GPP.img"),
	[HEKATE_TYPE_EMUMMC_BOOT0] = TEXT("BOOT0.img"),
	[HEKATE_TYPE_EMUMMC_BOOT1] = TEXT("BOOT1.img")
};

hekate_disk_type_t GetHekateDiskType(LPTSTR diskName){
	hekate_disk_type_t result = HEKATE_TYPE_UNK;
	for(DWORD i = 0; i < (DWORD)HEKATE_TYPE_COUNT; i++){
		if(_tcsstr(diskName, gHekateDiskTypeNames[i]) != NULL){
			result = (hekate_disk_type_t)i;
			break;
		}
	}
	return(result);
}

typedef struct backup_info_s{
	LPTSTR defaultFileName;
	disk_info_t *diskInfo;
}backup_info_t;



DWORD TestCallback(void *data){
	disk_info_t *diskInfo = data;
	HANDLE diskHandle;
	diskHandle = CreateFile(diskInfo->devPath,
	                        GENERIC_READ | GENERIC_WRITE,
	                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
	                        NULL);
	if(diskHandle == INVALID_HANDLE_VALUE){
		LogInfo(TEXT("InvalidHandle"));
		return(MENU_QUIT);
	}
	UINT8 buffer[512*2*1024] = {0};
	DWORD bytesWritten;
	DWORD count = 0;
	//while(WriteFile(diskHandle, buffer, 512, &bytesWritten, NULL) == TRUE && count < 2048) { count++; LogInfo(TEXT("count: %i, error: %i"), count, GetLastError());}
	//bytesWritten = DeviceIoControl(diskHandle, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &bytesWritten, NULL);
	LogInfo(TEXT("error: %i"), bytesWritten);
	while(WriteFile(diskHandle, buffer, 512*2*1024, &bytesWritten, NULL) == TRUE) { count++; LogInfo(TEXT("count: %i, error: %i"), count, GetLastError());}
	LogInfo(TEXT("count: %i, error: %i"), count, GetLastError());
	return(MENU_QUIT);
}

DWORD MenuBackupToFileCallback(void *data){
	backup_info_t *backupInfo = data;
	HANDLE diskHandle;
	diskHandle = CreateFile(backupInfo->diskInfo->devPath,
	                        GENERIC_READ,
	                        0,
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
	                        NULL);
	if(diskHandle == INVALID_HANDLE_VALUE){
		LogInfo(TEXT("failed to open disk"));
	}

	TCHAR filePath[MAX_PATH];
	PathCombine(filePath, TEXT(".\\"), backupInfo->defaultFileName);
	HANDLE fileHandle = CreateDummyFile(filePath, 1024*1024);//backupInfo->diskInfo->size);
	LogInfo(TEXT("dummyfile %i"), GetLastError());


	return(MENU_QUIT);
}

DWORD MenuHekateDiskCallback(void *data){
	disk_info_t *diskInfo = (disk_info_t*)data;
	hekate_disk_type_t hekateDiskType = GetHekateDiskType(diskInfo->friendlyName);
	llist_t menuList;
	llistInit(&menuList);

	backup_info_t backupInfo;
	backupInfo.diskInfo = diskInfo;
	backupInfo.defaultFileName = gHekateDefaultBackupFileNames[hekateDiskType];
	menu_entry_t menuBackup;
	menuBackup.menuEntryString = MSG_BACKUP_TO_FILE;
	menuBackup.menuEntryCallback = MenuBackupToFileCallback;
	menuBackup.data = &backupInfo;

	llistInsertHead(&menuList, (llist_node_t*)&menuBackup);

	DWORD result = DisplayMenu(&menuList, TEXT("Select action"));
	return(result);
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
		//TODO: delete diskList

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
		llistInit(&hekateDisks);
		GetHekateUmsDisks(&diskList, &hekateDisks);

		llist_t menuList;
		llistInit(&menuList);
		LPTSTR menuTitle = MSG_NO_HEKATE_DEVICES;
		if(llistGetLength(&hekateDisks) == 1){
			TCHAR menuString[200];
			disk_info_t *hekateDiskInfo = (disk_info_t*)llistGetHead(&hekateDisks);
			_stprintf_s(menuString, 200, MSG_SINGLE_HEKATE_DEVICE, hekateDiskInfo->friendlyName);
			menuTitle = menuString;
			menu_entry_t hekateDisk;
			hekateDisk.menuEntryString = MSG_CONTINUE;
			hekateDisk.menuEntryCallback = MenuHekateDiskCallback;
			hekateDisk.data = hekateDiskInfo;
			llistInsertHead(&menuList, (llist_node_t*)&hekateDisk);
		}else if(llistGetLength(&hekateDisks) > 1){
			menuTitle = MSG_MULTIPLE_HEKATE_DEVICES;
			llist_node_t *current;
			llistForEach(&hekateDisks, current){
				menu_entry_t *newEntry;
				newEntry = malloc(sizeof(menu_entry_t));
				newEntry->menuEntryCallback = MenuHekateDiskCallback;
				newEntry->menuEntryString = ((disk_info_t*)current)->friendlyName;
				newEntry->data = current;
				llistInsertHead(&menuList, (llist_node_t*)newEntry);
			}
		}
		menu_entry_t menuShowAll;
		menuShowAll.menuEntryCallback = MenuShowAllCallback;
		menuShowAll.menuEntryString = MSG_SHOW_ALL;
		menuShowAll.data = &diskList;
		if(llistIsEmpty(&diskList) == false){
			llistInsertTail(&menuList, (llist_node_t*)&menuShowAll);
		}
		llistInsertTail(&menuList, (llist_node_t*)&gMenuEntries[MENU_REFRESH]);
		llistInsertTail(&menuList, (llist_node_t*)&gMenuEntries[MENU_QUIT]);
		result = DisplayMenu(&menuList, menuTitle);




	}while(result != MENU_QUIT);
	return(1);

    return(1);
}