#include "diskinfo.h"
#include "llist.h"
#include <ShlObj.h>
#include <SetupAPI.h>
#include <ntddstor.h>
#include <initguid.h>
#include <devpkey.h>
#include <devpropdef.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <stdlib.h>
#include <vcruntime.h>

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
		LogError(TEXT("SetupDiGetDevicePropertyW failed"));
		goto GetDeviceProperty_cleanup;
	}
	*devPropertyBuffer = malloc(requiredSize);
	if(*devPropertyBuffer == NULL){
		LogFatal(TEXT("malloc failed"));
		goto GetDeviceProperty_cleanup;
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
		LogError(TEXT("SetupDiGetDevicePropertyW failed"));
		goto GetDeviceProperty_cleanup;
	}
	#ifndef UNICODE
	size_t bufferLength;
	LPTSTR buffer;
	if(wcstombs_s(&bufferLength, NULL, 0, *devPropertyBuffer, 0) != 0){
		LogError(TEXT("wcstombs_s failed"));
		goto GetDeviceProperty_cleanup;
	}
	buffer = malloc(requiredSize);
	if(buffer == NULL){
		LogFatal(TEXT("malloc failed"));
		goto GetDeviceProperty_cleanup;
	}
	wcstombs_s(NULL, buffer, bufferLength, *devPropertyBuffer, bufferLength);

	free(*devPropertyBuffer);
	*devPropertyBuffer = buffer;
	#endif

	return(TRUE);
	GetDeviceProperty_cleanup:
	free(*devPropertyBuffer);
	return(FALSE);
}

BOOL EnumerateDisks(llist_t *diskList){
	HDEVINFO diskClassDevs;
	diskClassDevs = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK,
	                                    NULL, NULL,
	                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if(diskClassDevs == INVALID_HANDLE_VALUE){
		LogError(TEXT("SetupDiGetClassDevs failed"));
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
			LogError(TEXT("SetupDiGetDeviceInterfaceDetail failed"));
			goto error_cleanup_continue;
		}
		devInterfaceDetailData = malloc(requiredSize);
		if(devInterfaceDetailData == NULL){
			LogFatal(TEXT("Failed to alloc memory"));
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
			LogError(TEXT("SetupDiGetDeviceInterfaceDetail failed"));
			goto error_cleanup_continue;
		}
		DWORD devPathLength;
		devPathLength = requiredSize - offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA,
		                                        DevicePath);
		disk_info_t *diskInfo;
		diskInfo = (disk_info_t*)malloc(sizeof(disk_info_t));
		diskInfo->devPath = malloc(devPathLength);
		if(diskInfo->devPath == NULL){
			LogFatal(TEXT("Failed to alloc memory"));
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
			LogError(TEXT("CreateFile failed"));
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
			LogError(TEXT("DeviceIoControl failed"));
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
			LogError(TEXT("DeviceIoControl failed"));
			goto error_cleanup_continue;
		}
		DeviceIoControl(devHandle,
		                IOCTL_DISK_IS_WRITABLE,
		                NULL,
		                0,
		                NULL,
		                0,
		                &bytesReturned,
		                NULL);
		DWORD lastError = GetLastError();
		if(lastError != ERROR_SUCCESS && lastError != ERROR_WRITE_PROTECT){
			LogError(TEXT("DeviceIoControl failed"));
			goto error_cleanup_continue;
		}
		if(lastError == ERROR_WRITE_PROTECT){
			diskInfo->isReadOnly = TRUE;
		}else{
			diskInfo->isReadOnly = FALSE;
		}
		diskInfo->size = diskCapacityInfo.DiskLength.QuadPart;
		result = GetDeviceProperty(diskClassDevs,
		                           &devInfoData,
		                           &DEVPKEY_Device_FriendlyName,
		                           (LPVOID)&(diskInfo->friendlyName));
		if(result == FALSE){
			LogError(TEXT("GetDeviceProperty failed"));
			goto error_cleanup_continue;

		}
		result = GetDeviceProperty(diskClassDevs,
		                           &devInfoData,
		                           &DEVPKEY_Device_Manufacturer,
		                           (LPVOID)&(diskInfo->manufacturer));
		if(result == FALSE){
			LogError(TEXT("GetDeviceProperty failed"));
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
		LogError(TEXT("GetVolumeNameForVolumeMountPointW failed"));
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
		LogError(TEXT("CreateFile failed"));
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
			LogFatal(TEXT("malloc failed"));
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
	if(result == FALSE){
		LogError(TEXT("DeviceIoControl failed"));
		goto GetSystemDriveNumbers_cleanup;
	}
	*systemDrives = malloc(volumeInfo->NumberOfDiskExtents * sizeof(DWORD));
	if(*systemDrives == NULL){
		LogFatal(TEXT("malloc failed"));
		goto GetSystemDriveNumbers_cleanup;
	}
	*numberSystemDrives = volumeInfo->NumberOfDiskExtents;
	for(DWORD i = 0; i < volumeInfo->NumberOfDiskExtents; i++){
		(*systemDrives)[i] = volumeInfo->Extents[i].DiskNumber;
	}
	ret = TRUE;
	GetSystemDriveNumbers_cleanup:
	if(devHandle != INVALID_HANDLE_VALUE){
		CloseHandle(devHandle);
	}
	free(volumeInfo);
	return(ret);
}

BOOL GetValidStorageDevices(llist_t *devList){
	BOOL result;
	BOOL ret = FALSE;
	result = EnumerateDisks(devList);
	if(result == FALSE){
		LogError(TEXT("EnumerateDisks failed"));
		goto GetValidStorageDevices_error;
	}
	DWORD numberSystemDrives;
	DWORD *systemDrives;
	result = GetSystemDriveNumbers(&numberSystemDrives, &systemDrives);
	if(result == FALSE){
		LogError(TEXT("GetSystemDriveNumbers failed"));
		goto GetValidStorageDevices_error;
	}
	llist_node_t *current;
	llistForEach(devList, current){
		for(DWORD i = 0; i < numberSystemDrives; i++){
			if(((disk_info_t*)current)->diskNumber == systemDrives[i]){
				llistDeleteNode(devList, current, DiskInfoDelete);
			}
		}
	}
	free(systemDrives);
	return(TRUE);
	GetValidStorageDevices_error:
	llistDeleteAll(devList, DiskInfoDelete);
	return(FALSE);
}
