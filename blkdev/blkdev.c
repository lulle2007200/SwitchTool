#include "..\log.h"
#include "..\llist.h"

#include <tchar.h>
#include <Windows.h>
#include <SetupAPI.h>
#include <stdlib.h>

#define START_ERR_CHK() \
DWORD error = ERROR_SUCCESS

#define ERR_CHK(cond, msg) \
	if(!(cond)){ \
		error = GetLastError(); \
		LogError(TEXT(msg "(%i)"), error); \
		goto error_exit; \
	}



#define END_ERR_CHK() \
	error_exit:\



typedef void* blkdev_set_handle_t;

blkdev_set_handle_t BlkdevCreateBlkdevSet(){
	START_ERR_CHK();

	//LogInfo(L"");

	HDEVINFO diskClassDevs;
	diskClassDevs = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK,
	                                    NULL,
	                                    NULL,
	                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	ERR_CHK(diskClassDevs != INVALID_HANDLE_VALUE,
	        "SetupDiGetClassDevs");

	DWORD devIndex = 0;
	SP_DEVICE_INTERFACE_DATA devInterfaceData;
	devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	SP_DEVICE_INTERFACE_DETAIL_DATA *devInterfaceDetailData = NULL;
	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	STORAGE_DEVICE_NUMBER diskNumber;
	HANDLE devHandle;
	DWORD requiredSize;
	BOOL result;
	while(SetupDiEnumDeviceInterfaces(diskClassDevs,
	                                  NULL,
	                                  &GUID_DEVINTERFACE_DISK,
	                                  devIndex,
	                                  &devInterfaceData) == TRUE){
		devIndex++;

		result = SetupDiGetDeviceInterfaceDetail(diskClassDevs,
		                                         &devInterfaceData,
		                                         NULL,
		                                         0,
		                                         &requiredSize,
		                                         NULL);
		ERR_CHK(result == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
		        "SetupDiGetDeviceInterfaceDetail");
		devInterfaceDetailData = malloc(requiredSize);
		ERR_CHK(devInterfaceDetailData != NULL,
		        "malloc");
		devInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		result = SetupDiGetDeviceInterfaceDetail(diskClassDevs,
		                                         &devInterfaceData,
		                                         devInterfaceDetailData,
		                                         requiredSize,
		                                         NULL,
		                                         &devInfoData);
		ERR_CHK(result == TRUE,
		        "SetupDiGetDeviceInterfaceDetail");

		devHandle = CreateFile(devInterfaceDetailData->DevicePath,
		                       GENERIC_READ,
		                       FILE_SHARE_READ | FILE_SHARE_WRITE,
		                       NULL,
		                       OPEN_EXISTING,
		                       0,
		                       NULL);
		ERR_CHK(devHandle != INVALID_HANDLE_VALUE,
		        "CreateFile");

		result = DeviceIoControl(devHandle,
		                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
		                         NULL,
		                         0,
		                         &diskNumber,
		                         sizeof(diskNumber),
		                         &requiredSize,
		                         NULL);
		ERR_CHK(result == TRUE,
		        "DeviceIoControl");

		LogInfo(L"%s", devInterfaceDetailData->DevicePath);


		if(devHandle != INVALID_HANDLE_VALUE){
			CloseHandle(devHandle);
		}
		free(devInterfaceDetailData);
		devInterfaceDetailData = NULL;

	}

	END_ERR_CHK();
	return(0);
}

int main(int argc, char *argv[]){
	LogSetQuiet(false);
	BlkdevCreateBlkdevSet();
	return(1);
}