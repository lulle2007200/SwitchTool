#include "diskinfo.h"
#include <ShlObj.h>
#include <SetupAPI.h>
#include <ntddstor.h>
#include <initguid.h>
#include <devpkey.h>
#include <devpropdef.h>
#include <Shlwapi.h>
#include <PathCch.h>

int diskInfoCompareDevNumber(llist_node_t *diskInfo1, llist_node_t *diskInfo2){
	if(((disk_info_t*)diskInfo1)->diskNumber > ((disk_info_t*)diskInfo2)->diskNumber){
		return(1);
	}
	if(((disk_info_t*)diskInfo1)->diskNumber < ((disk_info_t*)diskInfo2)->diskNumber){
		return(-1);
	}
	return(0);
}

void DiskInfoDelete(llist_node_t *diskInfo){
	free(((disk_info_t*)diskInfo)->devPath);
	free(((disk_info_t*)diskInfo)->manufacturer);
	free(((disk_info_t*)diskInfo)->friendlyName);
	free(diskInfo);
}

BOOL GetDeviceProperty(HDEVINFO diskClassDevs,
                       SP_DEVINFO_DATA *devInfoData,
                       const DEVPROPKEY *devProperty,
                       LPVOID *devPropertyBuffer){
	BOOL result;
	DWORD requiredSize;
	DEVPROPTYPE propertyType;
	result = SetupDiGetDevicePropertyW(diskClassDevs,
	                                   devInfoData,
	                                   devProperty,
	                                   &propertyType,
	                                   NULL,
	                                   0,
	                                   &requiredSize,
	                                   0);
	if(result == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER){
		LogError("SetupDiGetDevicePropertyW");
		return(FALSE);
	}
	*devPropertyBuffer = malloc(requiredSize);
	if(*devPropertyBuffer == NULL){
		LogFatal("malloc failed");
		return(FALSE);
	}
	result = SetupDiGetDevicePropertyW(diskClassDevs,
	                                   devInfoData,
	                                   devProperty,
	                                   &propertyType,
	                                   (PBYTE)*devPropertyBuffer,
	                                   requiredSize,
	                                   NULL,
	                                   0);
	if(result == FALSE){
		LogError("SetupDiGetDevicePropertyW failed");
		free(*devPropertyBuffer);
		return(FALSE);
	}
	return(TRUE);
}

BOOL EnumerateDisks(llist_t *diskList){
	HDEVINFO diskClassDevs;
	diskClassDevs = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK,
	                                    NULL, NULL,
	                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if(diskClassDevs == INVALID_HANDLE_VALUE){
		LogError("SetupDiGetClassDevs failed");
		return(FALSE);
	}
	SP_DEVICE_INTERFACE_DATA devInterfaceData;
	devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	DWORD devIndex = 0;
	while(SetupDiEnumDeviceInterfaces(diskClassDevs,
	                                  NULL,
	                                  &GUID_DEVINTERFACE_DISK,
	                                  devIndex,
	                                  &devInterfaceData)){
		devIndex++;
		SP_DEVICE_INTERFACE_DETAIL_DATA *devInterfaceDetailData;
		DWORD requiredSize;
		BOOL result = SetupDiGetDeviceInterfaceDetail(diskClassDevs,
		                                              &devInterfaceData,
		                                              NULL,
		                                              0,
		                                              &requiredSize,
		                                              NULL);
		if(result == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER){
			LogError("SetupDiGetDeviceInterfaceDetail failed");
			goto error_cleanup_continue;
		}
		devInterfaceDetailData = malloc(requiredSize);
		if(devInterfaceDetailData == NULL){
			LogFatal("Failed to alloc memory");
			goto error_cleanup_continue;
			//TODO: exit
		}
		devInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		SP_DEVINFO_DATA devInfoData;
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		result = SetupDiGetDeviceInterfaceDetail(diskClassDevs,
		                                         &devInterfaceData,
		                                         devInterfaceDetailData,
		                                         requiredSize,
		                                         NULL,
		                                         &devInfoData);
		if(result == FALSE){
			LogError("SetupDiGetDeviceInterfaceDetail failed, %i, %i", GetLastError(), requiredSize);
			goto error_cleanup_continue;
		}
		DWORD devPathLength;
		devPathLength = requiredSize - offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA,
		                                        DevicePath);
		disk_info_t *diskInfo;
		diskInfo = (disk_info_t*)malloc(sizeof(disk_info_t));
		diskInfo->devPath = malloc(devPathLength);
		if(diskInfo->devPath == NULL){
			LogFatal("Failed to alloc memory");
			goto error_cleanup_continue;
		}
		memcpy(diskInfo->devPath, devInterfaceDetailData->DevicePath, devPathLength);
		HANDLE devHandle;
		devHandle = CreateFile(devInterfaceDetailData->DevicePath,
		                       GENERIC_READ,
		                       FILE_SHARE_READ | FILE_SHARE_WRITE,
		                       NULL,
		                       OPEN_EXISTING,
		                       FILE_ATTRIBUTE_NORMAL,
		                       NULL);
		if(devHandle == INVALID_HANDLE_VALUE){
			LogError("CreateFile failed");
			goto error_cleanup_continue;
		}
		DWORD bytesReturned;
		STORAGE_DEVICE_NUMBER diskNumber;
		result = DeviceIoControl(devHandle,
		                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
		                         NULL,
		                         0,
		                         &diskNumber,
		                         sizeof(STORAGE_DEVICE_NUMBER),
		                         &bytesReturned,
		                         NULL);
		if(result == FALSE){
			goto error_cleanup_continue;
		}
		diskInfo->diskNumber = diskNumber.DeviceNumber;
		STORAGE_READ_CAPACITY diskCapacityInfo;
		diskCapacityInfo.Version = sizeof(STORAGE_READ_CAPACITY);
		result = DeviceIoControl(devHandle,
		                         IOCTL_STORAGE_READ_CAPACITY,
		                         NULL,
		                         0,
		                         &diskCapacityInfo,
		                         sizeof(STORAGE_READ_CAPACITY),
		                         &bytesReturned,
		                         NULL);
		if(result == FALSE){
			LogError("DeviceIoControl failed");
			goto error_cleanup_continue;
		}
		diskInfo->size = diskCapacityInfo.DiskLength.QuadPart;
		result = GetDeviceProperty(diskClassDevs,
		                           &devInfoData,
		                           &DEVPKEY_Device_FriendlyName,
		                           (LPVOID)&(diskInfo->friendlyName));
		if(result == FALSE){
			LogError("GetDeviceProperty failed");
			goto error_cleanup_continue;

		}
		result = GetDeviceProperty(diskClassDevs,
		                           &devInfoData,
		                           &DEVPKEY_Device_Manufacturer,
		                           (LPVOID)&(diskInfo->manufacturer));
		if(result == FALSE){
			LogError("GetDeviceProperty failed");
			goto error_cleanup_continue;
		}

		llistInsertInOrder(diskList, (llist_node_t*)diskInfo, diskInfoCompareDevNumber);

		goto cleanup_continue;

		error_cleanup_continue:
		DiskInfoDelete((llist_node_t*)diskInfo);
		cleanup_continue:
		free(devInterfaceDetailData);
		if(devHandle != INVALID_HANDLE_VALUE){
			CloseHandle(devHandle);
		}
	}
	SetupDiDestroyDeviceInfoList(diskClassDevs);
	return(TRUE);
}

BOOL GetSystemDriveNumbers(DWORD *numberSystemDrives, DWORD **systemDrives){
	BOOL ret = FALSE;
	HRESULT result;
	LPWSTR systemPath;
	result = SHGetKnownFolderPath(&FOLDERID_Windows, KF_FLAG_DEFAULT, NULL, &systemPath);
	PathStripToRootW(systemPath);
	WCHAR volumePath[MAX_PATH];
	result = GetVolumeNameForVolumeMountPointW(systemPath, volumePath, MAX_PATH);
	if(result == FALSE){
		LogError("GetVolumeNameForVolumeMountPointW failed");
		goto GetSystemDriveNumbers_cleanup;
	}
	volumePath[lstrlenW(volumePath)-1] = 0;
	HANDLE devHandle;
	devHandle = CreateFileW(volumePath,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
	if(devHandle == INVALID_HANDLE_VALUE){
		LogError("CreateFile failed");
		goto GetSystemDriveNumbers_cleanup;
	}
	VOLUME_DISK_EXTENTS *volumeInfo = NULL;
	DWORD volumeInfoSize;
	volumeInfoSize = sizeof(VOLUME_DISK_EXTENTS);
	DWORD bytesReturned;
	DWORD tries = 0;
	do{
		free(volumeInfo);
		volumeInfo = malloc(volumeInfoSize);
		if(volumeInfo == NULL){
			LogFatal("malloc failed");
			goto GetSystemDriveNumbers_cleanup;
		}
		result = DeviceIoControl(devHandle,
		                         IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		                         NULL,
		                         0,
		                         volumeInfo,
		                         volumeInfoSize,
		                         &bytesReturned,
		                         NULL);
		volumeInfoSize += sizeof(DISK_EXTENT);
		tries++;
	}while(result != TRUE &&
	      (GetLastError() == ERROR_INSUFFICIENT_BUFFER || GetLastError() == ERROR_MORE_DATA) &&
	      tries <= 5);
	CloseHandle(devHandle);
	if(result == FALSE){
		LogError("DeviceIoControl failed");
		goto GetSystemDriveNumbers_cleanup;
	}
	*systemDrives = malloc(volumeInfo->NumberOfDiskExtents * sizeof(DWORD));
	if(*systemDrives == NULL){
		LogFatal("malloc failed");
		goto GetSystemDriveNumbers_cleanup;
	}
	*numberSystemDrives = volumeInfo->NumberOfDiskExtents;
	for(DWORD i = 0; i < volumeInfo->NumberOfDiskExtents; i++){
		(*systemDrives)[i] = volumeInfo->Extents[i].DiskNumber;
	}
	ret = TRUE;
	GetSystemDriveNumbers_cleanup:
	free(volumeInfo);
	return(ret);
}

BOOL GetValidStorageDevices(llist_t *devList){
	BOOL result;
	result = EnumerateDisks(devList);
	if(result == FALSE){
		LogError("EnumerateDisks failed");
		return(FALSE);
	}
	DWORD numberSystemDrives;
	DWORD *systemDrives;
	result = GetSystemDriveNumbers(&numberSystemDrives, &systemDrives);
	if(result == FALSE){
		LogError("GetSystemDriveNumbers failed");
		return(FALSE);
	}
	llist_node_t *current;
	llistForEach(devList, current){
		for(DWORD i = 0; i < numberSystemDrives; i++){
			if(((disk_info_t*)current)->diskNumber == systemDrives[i]){
				llistDeleteNode(devList, current, DiskInfoDelete);
			}
		}
	}

	return(TRUE);
}