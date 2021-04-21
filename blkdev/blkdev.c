#include "..\log.h"
#include "..\llist.h"

#include <stddef.h>
#include <string.h>
#include <tchar.h>
#include <Windows.h>
#include <SetupAPI.h>
#include <initguid.h>
#include <devpkey.h>
#include <devpropdef.h>
#include <stdlib.h>
#include <cfgmgr32.h>
#include "..\leak_detector_c.h"

#define START_ERR_CHK() \
	DWORD error = ERROR_SUCCESS

#define RESET_ERR_CHK() \
	error = ERROR_SUCCESS

#define _ERR_CHK_EX(cond, code, label, msg, ...) \
	if(! (cond)){ \
		error = code; \
		LogError(TEXT(msg), __VA_ARGS__); \
		goto error_exit_##label; \
	}

#define _ERR_CHK(cond, code, msg, ...) \
	_ERR_CHK_EX(cond, code, default, msg, __VA_ARGS__)

#define ERR_CHK_WINAPI(cond, api) \
	ERR_CHK_WINAPI_EX(cond, default, api)

#define ERR_CHK_WINAPI_EX(cond, label, api) \
	do{ \
		DWORD errChkLastError = GetLastError(); \
		_ERR_CHK_EX(cond, errChkLastError, label, api "(%i)", errChkLastError); \
	}while(0)

#define ERR_CHK(cond, msg, ...) \
	ERR_CHK_EX(cond, default, msg, __VA_ARGS__)

#define ERR_CHK_EX(cond, label, msg, ...) \
	_ERR_CHK_EX(cond, 1, label, msg, __VA_ARGS__)

#define safe_malloc(size, ptr) \
	*ptr = malloc(size); \
	ERR_CHK(*ptr != NULL, \
		    "malloc")

#define safe_malloc_ex(size, ptr, label) \
	*ptr = malloc(size); \
	ERR_CHK_EX(*ptr != NULL, \
	           label, \
		       "malloc")

#define safe_calloc(count, size, ptr) \
	*ptr = calloc(count, size); \
	ERR_CHK(*ptr != NULL, \
	        "malloc")

#define safe_calloc_ex(count, size, ptr, label) \
	*ptr = calloc(count, size); \
	ERR_CHK_EX(*ptr != NULL, \
	           label, \
	           "malloc")

#define END_ERR_CHK_EX(label) \
	error_exit_##label:

#define END_ERR_CHK() \
	END_ERR_CHK_EX(default)

typedef enum blkdev_partition_style_e{
	BLKDEV_PARTITION_TYPE_RAW,
	BLKDEV_PARTITION_TYPE_MBR,
	BLKDEV_PARTITION_TYPE_GPT,
	BLKDEV_PARTITION_TYPE_HYBRID
}blkdev_partition_style_t;

typedef enum blkdev_disk_type_e{
	BLKDEV_TYPE_UNK,
	BLKDEV_TYPE_HDD,
	BLKDEV_TYPE_SSD
}blkdev_disk_type;

typedef enum blkdev_bus_type_e {
	BLKDEV_BUS_UNK = BusTypeUnknown,
	BLKDEV_BUS_SCSI = BusTypeScsi,
	BLKDEV_BUS_ATAPI = BusTypeAtapi,
	BLKDEV_BUS_ATA = BusTypeAta,
	BLKDEV_BUS_1394 = BusType1394,
	BLKDEV_BUS_SSA = BusTypeSsa,
	BLKDEV_BUS_FIBRE = BusTypeFibre,
	BLKDEV_BUS_USB = BusTypeUsb,
	BLKDEV_BUS_RAID = BusTypeRAID,
	BLKDEV_BUS_ISCSI = BusTypeiScsi,
	BLKDEV_BUS_SAS = BusTypeSas,
	BLKDEV_BUS_SATA = BusTypeSata,
	BLKDEV_BUS_SD = BusTypeSd,
	BLKDEV_BUS_MMC = BusTypeMmc,
	BLKDEV_BUS_VIRTUAL = BusTypeVirtual,
	BLKDEV_BUS_FILE_VIRTUAL = BusTypeFileBackedVirtual,
	BLKDEV_BUS_SPACES = BusTypeSpaces,
	BLKDEV_BUS_NVME = BusTypeNvme,
	BLKDEV_BUS_SCM = BusTypeSCM,
	BLKDEV_BUS_UFS = BusTypeUfs,
	BLKDEV_BUS_MAX = BusTypeMax,
	BLKDEV_BUS_RESERVED = BusTypeMaxReserved
}blkdev_bus_type;

typedef struct blkdev_disk_mbr_data_s{
	DWORD diskSignature;
}blkdev_disk_mbr_data_t;

typedef struct blkdev_disk_gpt_data_s{
	GUID diskGuid;
}blkdev_disk_gpt_data_t;

typedef struct blkdev_disk_info_s{
	LPTSTR diskFullPath;
	LPTSTR diskShortPath;
	LPTSTR diskVendor;
	LPTSTR diskProductId;
	LPTSTR diskSerial;
	LPTSTR diskRevision;
	LPTSTR diskManufacturer;
	LPTSTR diskFriendlyName;
	DWORD diskPhysicalSectorSize;
	DWORD diskLogicalSectorSize;
	DWORD diskNumber;
	BOOL diskIsRemovable;
	BOOL diskIsWritable;
	blkdev_disk_type diskType;
	blkdev_bus_type diskBusType;
	union{
		blkdev_disk_mbr_data_t diskMbrData;
		blkdev_disk_gpt_data_t diskGptData;
	}partTableData;
}blkdev_disk_info_t;

typedef struct blkdev_disk_partition_gpt_data_s{
	GUID partGuid;
	GUID partUniqueGuid;
	WCHAR partName[72];
	LONGLONG partAttributes;
}blkdev_disk_partition_gpt_data_t;

typedef struct blkdev_disk_partition_mbr_data_s{
	BOOL partIsBootable;
	CHAR partType;
}blkdev_disk_partition_mbr_data_t;

typedef struct blkdev_disk_partition_s{
	llist_node_t *next;
	llist_node_t *prev;

	LONGLONG partFirstSector;
	LONGLONG partLastSector;
	LONGLONG partSize;
	union{
		blkdev_disk_partition_mbr_data_t partMbrData;
		blkdev_disk_partition_gpt_data_t partGptData;
	}partTableData;
	blkdev_partition_style_t partStyle;
}blkdev_disk_partition_t;

typedef struct blkdev_disk_info_node_s{
	llist_node_t *next;
	llist_node_t *prev;

	blkdev_disk_info_t diskInfo;
	llist_t diskPartitions;
}blkdev_disk_info_node_t;

typedef void* blkdev_set_handle_t;

static const LPTSTR blkdevPartTypeStrings[] = {
	[BLKDEV_PARTITION_TYPE_RAW] = TEXT("Raw"),
	[BLKDEV_PARTITION_TYPE_GPT] = TEXT("GPT"),
	[BLKDEV_PARTITION_TYPE_MBR] = TEXT("MBR"),
	[BLKDEV_PARTITION_TYPE_HYBRID] = TEXT("Hybrid"),
};

static const LPTSTR blkdevDiskTypeStrings[] = {
	[BLKDEV_TYPE_UNK] = TEXT("Unknown"),
	[BLKDEV_TYPE_SSD] = TEXT("SSD"),
	[BLKDEV_TYPE_HDD] = TEXT("HDD")
};

static const LPTSTR blkdevBusTypeStrings[] = {
	[BLKDEV_BUS_UNK] = TEXT("Unknown"),
	[BLKDEV_BUS_SCSI] = TEXT("SCSI"),
	[BLKDEV_BUS_ATAPI] = TEXT("ATAPI"),
	[BLKDEV_BUS_ATA] = TEXT("ATA"),
	[BLKDEV_BUS_1394] = TEXT("FireWire"),
	[BLKDEV_BUS_SSA] = TEXT("SSA"),
	[BLKDEV_BUS_FIBRE] = TEXT("Fibre channel"),
	[BLKDEV_BUS_USB] = TEXT("USB"),
	[BLKDEV_BUS_RAID] = TEXT("RAID"),
	[BLKDEV_BUS_ISCSI] = TEXT("iSCSI"),
	[BLKDEV_BUS_SAS] = TEXT("SAS"),
	[BLKDEV_BUS_SATA] = TEXT("SATA"),
	[BLKDEV_BUS_SD] = TEXT("SD"),
	[BLKDEV_BUS_MMC] = TEXT("MMC"),
	[BLKDEV_BUS_VIRTUAL] = TEXT("Virtual"),
	[BLKDEV_BUS_FILE_VIRTUAL] = TEXT("File based virtual"),
	[BLKDEV_BUS_SPACES] = TEXT("Storage spaces"),
	[BLKDEV_BUS_NVME] = TEXT("NVMe"),
	[BLKDEV_BUS_SCM] = TEXT("SCM"),
	[BLKDEV_BUS_UFS] = TEXT("UFS"),
	[BLKDEV_BUS_MAX] = TEXT("MAX"),
	[BLKDEV_BUS_RESERVED] = TEXT("Reserved")
};

LPTSTR BlkdevGetBusTypeString(blkdev_bus_type busType){
	return(blkdevBusTypeStrings[busType]);
}

LPTSTR BlkdevGetDiskTypeString(blkdev_disk_type diskType){
	return(blkdevDiskTypeStrings[diskType]);
}

LPTSTR BlkdevGetPartTypeString(blkdev_partition_style_t partType){
	return(blkdevPartTypeStrings[partType]);
}

LPTSTR BlkdevGetDevicePropertyString(HDEVINFO diskClassDevs,
                                     SP_DEVINFO_DATA *devInfoData,
                                     const DEVPROPKEY *devProperty){
	START_ERR_CHK();
	DWORD result;
	DEVPROPTYPE propertyType;
	DWORD requiredSize;
	result = SetupDiGetDevicePropertyW(diskClassDevs,
	                                   devInfoData,
	                                   devProperty,
	                                   &propertyType,
	                                   NULL,
	                                   0,
	                                   &requiredSize,
	                                   0);
	ERR_CHK_WINAPI(result == TRUE || GetLastError() == ERROR_INSUFFICIENT_BUFFER,
	               "SetupDiGetDeviceProperty");

	LPWSTR stringDest;
	safe_malloc(requiredSize, &stringDest);

	result = SetupDiGetDevicePropertyW(diskClassDevs,
	                                   devInfoData,
	                                   devProperty,
	                                   &propertyType,
	                                   (PBYTE)stringDest,
	                                   requiredSize,
	                                   NULL,
	                                   0);
	ERR_CHK_WINAPI(result == TRUE,
	               "SetupDiGetDeviceProperty");
	#ifdef UNICODE
	END_ERR_CHK();
	return(stringDest);
	#else
	LPSTR buffer;
	size_t bufferLength;
	result = wcstombs_s(&bufferLength,
                        NULL,
                        0,
                        stringDest,
                        0);
	ERR_CHK(result == 0,
	        "wcstombs_s");
	safe_malloc(bufferLength, &buffer);
	wcstombs_s(NULL,
	           buffer,
	           bufferLength,
	           stringDest,
	           bufferLength - sizeof(CHAR));
	END_ERR_CHK();
	free(stringDest);
	return(buffer);
	#endif
}

LPTSTR BlkdevGetStringFromStorageDescriptor(STORAGE_DEVICE_DESCRIPTOR *descriptor,
                                            DWORD stringOffset){
	START_ERR_CHK();
	LPSTR string;
	if(stringOffset == 0){
		string = "";
	}else{
		string = (LPSTR)((CHAR*)descriptor + stringOffset);
	}
	DWORD stringLength = strlen(string) + 1;
	DWORD requiredSize = stringLength * sizeof(TCHAR);
	LPTSTR stringDest;
	safe_malloc(sizeof(TCHAR) * stringLength, &stringDest);
	#ifdef UNICODE
	CHAR *currentSrc = (CHAR*)string;
	TCHAR *currentDest = (TCHAR*)stringDest;
	for(DWORD i = 0; i < stringLength; i++){
		currentDest[i] = currentSrc[i];
	}
	#else
	memcpy_s(stringDest,
	         requiredSize,
	         string,
	         requiredSize);
	#endif

	END_ERR_CHK();
	return(stringDest);
}

void BlkdevDestroyPartInfoNodeCallback(llist_node_t *node){
	free(node);
}

void BlkdevDestroyDiskInfoNode(blkdev_disk_info_node_t *node){
	free(node->diskInfo.diskFullPath);
	free(node->diskInfo.diskShortPath);
	free(node->diskInfo.diskVendor);
	free(node->diskInfo.diskProductId);
	free(node->diskInfo.diskSerial);
	free(node->diskInfo.diskRevision);
	free(node->diskInfo.diskManufacturer);
	free(node->diskInfo.diskFriendlyName);
	llistDeleteAll(&(node->diskPartitions), BlkdevDestroyPartInfoNodeCallback);
}

void BlkdevDestroyDiskInfoNodeCallback(llist_node_t *node){
	blkdev_disk_info_node_t *diskInfo = (blkdev_disk_info_node_t*)node;
	BlkdevDestroyDiskInfoNode(diskInfo);
	free(diskInfo);
}


void BlkdevDestroyDevList(blkdev_set_handle_t diskList){
	if(diskList == NULL){
		return;
	}
	llistDeleteAll(diskList, BlkdevDestroyDiskInfoNodeCallback);
}

blkdev_disk_info_node_t* BlkdevGetDevInfo1(HDEVINFO diskClassDevs,
                                           SP_DEVICE_INTERFACE_DATA *devInterfaceData){
	START_ERR_CHK();
	blkdev_disk_info_node_t *newDiskNode;
	safe_calloc(1, sizeof(blkdev_disk_info_node_t), &newDiskNode);
	llistInit(&(newDiskNode->diskPartitions));

	DWORD result;
	DWORD requiredSize;

	SP_DEVICE_INTERFACE_DETAIL_DATA *devInterfaceDetailData = NULL;
	STORAGE_DEVICE_DESCRIPTOR *diskDescriptor = NULL;
	blkdev_disk_partition_t *rawPartition = NULL;


	result = SetupDiGetDeviceInterfaceDetail(diskClassDevs,
	                                         devInterfaceData,
	                                         NULL,
	                                         0,
	                                         &requiredSize,
	                                         0);
	ERR_CHK_WINAPI(result == FALSE &&
	               GetLastError() == ERROR_INSUFFICIENT_BUFFER,
	               "SetupDiGetDeviceInterfaceDetail");

	safe_malloc(requiredSize, &devInterfaceDetailData);
	devInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	result = SetupDiGetDeviceInterfaceDetail(diskClassDevs,
	                                         devInterfaceData,
	                                         devInterfaceDetailData,
	                                         requiredSize,
	                                         NULL,
	                                         &devInfoData);
	ERR_CHK_WINAPI(result == TRUE, "SetupDiGetDeviceInterfaceDetail");

	requiredSize -= offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA, DevicePath);
	safe_malloc(requiredSize, &(newDiskNode->diskInfo.diskFullPath));
	result = memcpy_s(newDiskNode->diskInfo.diskFullPath,
	                  requiredSize,
	                  devInterfaceDetailData->DevicePath,
	                  requiredSize);
	ERR_CHK(result == 0,
	        "memcpy_s");

	HANDLE devHandle;
	devHandle = CreateFile(devInterfaceDetailData->DevicePath,
	                       GENERIC_READ,
	                       FILE_SHARE_READ | FILE_SHARE_WRITE,
	                       NULL,
	                       OPEN_EXISTING,
	                       0,
	                       NULL);
	ERR_CHK_WINAPI(devHandle != INVALID_HANDLE_VALUE, "CreateFile");

	STORAGE_DEVICE_NUMBER diskNumber;
	result = DeviceIoControl(devHandle,
	                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
	                         NULL,
	                         0,
	                         &diskNumber,
	                         sizeof(diskNumber),
	                         &requiredSize,
	                         NULL);
	ERR_CHK_WINAPI(result == TRUE, "DeviceIoControl");
	newDiskNode->diskInfo.diskNumber = diskNumber.DeviceNumber;

	requiredSize = MAX_PATH * sizeof(TCHAR);
	safe_malloc(requiredSize, &(newDiskNode->diskInfo.diskShortPath));
	result = _sntprintf_s(newDiskNode->diskInfo.diskShortPath,
	                      requiredSize,
	                      MAX_PATH,
	                      TEXT("\\\\.\\PhysicalDrive%lu"),
	                      diskNumber.DeviceNumber);
	ERR_CHK(result != -1, "_sntprintf_s");

	STORAGE_DESCRIPTOR_HEADER diskDescriptorSizeInfo;
	diskDescriptorSizeInfo.Version = sizeof(STORAGE_DESCRIPTOR_HEADER);

	STORAGE_PROPERTY_QUERY diskProperty;
	diskProperty.QueryType = PropertyStandardQuery;
	diskProperty.PropertyId = StorageDeviceProperty;
	result = DeviceIoControl(devHandle,
	                         IOCTL_STORAGE_QUERY_PROPERTY,
	                         &diskProperty,
	                         sizeof(diskProperty),
	                         &diskDescriptorSizeInfo,
	                         sizeof(diskDescriptorSizeInfo),
	                         &requiredSize,
	                         NULL);
	ERR_CHK_WINAPI(result == TRUE, "DeviceIoControl");

	safe_malloc(diskDescriptorSizeInfo.Size, &diskDescriptor);
	diskDescriptor->Version = diskDescriptorSizeInfo.Size;
	result = DeviceIoControl(devHandle,
	                         IOCTL_STORAGE_QUERY_PROPERTY,
	                         &diskProperty,
	                         sizeof(diskProperty),
	                         diskDescriptor,
	                         diskDescriptorSizeInfo.Size,
	                         &requiredSize,
	                         NULL);
	ERR_CHK_WINAPI(result == TRUE, "DeviceIoControl");

	newDiskNode->diskInfo.diskIsRemovable = diskDescriptor->RemovableMedia;
	newDiskNode->diskInfo.diskBusType = (blkdev_bus_type)diskDescriptor->BusType;

	LPTSTR descriptorString = NULL;
	descriptorString = BlkdevGetStringFromStorageDescriptor(diskDescriptor,
		                                                    diskDescriptor->ProductIdOffset);
	ERR_CHK(descriptorString != NULL, "BlkdevGetStringFromStorageDescriptor");
	newDiskNode->diskInfo.diskProductId = descriptorString;

	descriptorString = BlkdevGetStringFromStorageDescriptor(diskDescriptor,
		                                                    diskDescriptor->VendorIdOffset);
	ERR_CHK(descriptorString != NULL, "BlkdevGetStringFromStorageDescriptor");
	newDiskNode->diskInfo.diskVendor = descriptorString;

	descriptorString = BlkdevGetStringFromStorageDescriptor(diskDescriptor,
		                                                    diskDescriptor->ProductRevisionOffset);
	ERR_CHK(descriptorString != NULL, "BlkdevGetStringFromStorageDescriptor");
	newDiskNode->diskInfo.diskRevision = descriptorString;

	descriptorString = BlkdevGetStringFromStorageDescriptor(diskDescriptor,
		                                                    diskDescriptor->SerialNumberOffset);
	ERR_CHK(descriptorString != NULL, "BlkdevGetStringFromStorageDescriptor");
	newDiskNode->diskInfo.diskSerial = descriptorString;

	DEVICE_TRIM_DESCRIPTOR diskTrimDescriptor;
	diskTrimDescriptor.Version = sizeof(diskTrimDescriptor);

	diskProperty.QueryType = PropertyStandardQuery;
	diskProperty.PropertyId = StorageDeviceTrimProperty;
	result = DeviceIoControl(devHandle,
	                         IOCTL_STORAGE_QUERY_PROPERTY,
	                         &diskProperty,
	                         sizeof(diskProperty),
	                         &diskTrimDescriptor,
	                         sizeof(diskTrimDescriptor),
	                         &requiredSize,
	                         NULL);
	if(result == FALSE){
		newDiskNode->diskInfo.diskType = BLKDEV_TYPE_UNK;
	}else{
		if(diskTrimDescriptor.TrimEnabled == TRUE){
			newDiskNode->diskInfo.diskType = BLKDEV_TYPE_SSD;
		}else{
			newDiskNode->diskInfo.diskType = BLKDEV_TYPE_HDD;
		}
	}

	result = DeviceIoControl(devHandle,
	                         IOCTL_DISK_IS_WRITABLE,
	                         NULL,
	                         0,
	                         NULL,
	                         0,
	                         &requiredSize,
	                         NULL);
	DWORD lastError = GetLastError();
	ERR_CHK_WINAPI(result == TRUE || lastError == ERROR_WRITE_PROTECT,
	               "DeviceIoControl");
	if(lastError == ERROR_WRITE_PROTECT){
		newDiskNode->diskInfo.diskIsWritable = FALSE;
	}else{
		newDiskNode->diskInfo.diskIsWritable = TRUE;
	}

	descriptorString = BlkdevGetDevicePropertyString(diskClassDevs,
	                                                 &devInfoData,
	                                                 &DEVPKEY_Device_FriendlyName);
	ERR_CHK(descriptorString != NULL, "BlkdevGetDevicePropertyString");
	newDiskNode->diskInfo.diskFriendlyName = descriptorString;

	descriptorString = BlkdevGetDevicePropertyString(diskClassDevs,
	                                                 &devInfoData,
	                                                 &DEVPKEY_Device_Manufacturer);
	ERR_CHK(descriptorString != NULL, "BlkdevGetDevicePropertyString");
	newDiskNode->diskInfo.diskManufacturer = descriptorString;

	STORAGE_READ_CAPACITY diskCapacity;
	result = DeviceIoControl(devHandle,
	                         IOCTL_STORAGE_READ_CAPACITY,
	                         NULL,
	                         0,
	                         &diskCapacity,
	                         sizeof(diskCapacity),
	                         &requiredSize,
	                         NULL);
	ERR_CHK_WINAPI(result == TRUE, "DeviceIoControl");

	diskProperty.QueryType = PropertyStandardQuery;
	diskProperty.PropertyId = StorageAccessAlignmentProperty;

	STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR diskAlignment;
	diskAlignment.Version = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
	result = DeviceIoControl(devHandle,
	                         IOCTL_STORAGE_QUERY_PROPERTY,
	                         &diskProperty,
	                         sizeof(diskProperty),
	                         &diskAlignment,
	                         sizeof(diskAlignment),
	                         &requiredSize,
	                         NULL);
	//ERR_CHK_WINAPI(result == TRUE,
	//               "DeviceIoControl");

	newDiskNode->diskInfo.diskPhysicalSectorSize = diskAlignment.BytesPerPhysicalSector;
	newDiskNode->diskInfo.diskLogicalSectorSize = diskAlignment.BytesPerLogicalSector;

	//--------------------------------------------------------------------------

	safe_malloc(sizeof(blkdev_disk_partition_t), &rawPartition);

	llistInsertHead(&(newDiskNode->diskPartitions), (llist_node_t*)rawPartition);

	rawPartition->partFirstSector = 0;
	rawPartition->partSize = diskCapacity.DiskLength.QuadPart;
	rawPartition->partLastSector = diskCapacity.DiskLength.QuadPart / 512;
	rawPartition->partStyle = BLKDEV_PARTITION_TYPE_RAW;

	END_ERR_CHK();
	if(error != ERROR_SUCCESS){
		BlkdevDestroyDiskInfoNode(newDiskNode);
		free(newDiskNode);
		newDiskNode = NULL;
	}
	free(devInterfaceDetailData);
	free(diskDescriptor);
	return(newDiskNode);
}

void* BlkdevGetDevList(){
	START_ERR_CHK();
	HDEVINFO diskClassDevs;
	diskClassDevs = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK,
	                                    NULL,
	                                    NULL,
	                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	ERR_CHK_WINAPI(diskClassDevs != INVALID_HANDLE_VALUE,
	               "SetupDiGetClassDevs");

	llist_t *diskList;
	safe_malloc(sizeof(llist_t), &diskList);
	llistInit(diskList);

	DWORD devIndex = 0;
	SP_DEVICE_INTERFACE_DATA devInterfaceData;
	devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	while(SetupDiEnumDeviceInterfaces(diskClassDevs,
	                                  NULL,
	                                  &GUID_DEVINTERFACE_DISK,
	                                  devIndex,
	                                  &devInterfaceData) == TRUE){
		devIndex++;
		blkdev_disk_info_node_t *newDiskNode;
		newDiskNode = BlkdevGetDevInfo1(diskClassDevs, &devInterfaceData);
		ERR_CHK(newDiskNode != NULL,
		        "BlkdevGetDevInfo");
		llistInsertTail(diskList, (llist_node_t*)newDiskNode);
	}


	END_ERR_CHK();
	if(diskClassDevs != INVALID_HANDLE_VALUE){
		SetupDiDestroyDeviceInfoList(diskClassDevs);
	}
	if(error != ERROR_SUCCESS){
		BlkdevDestroyDevList(diskList);
		free(diskList);
		diskList = NULL;
	}
	return(diskList);
}

void BlkdevPrintDevList(llist_t *diskList){
	llist_node_t *current;
	llistForEach(diskList, current){
		blkdev_disk_info_node_t *currentNode = (blkdev_disk_info_node_t*)current;
		LogInfo(TEXT("FullPath: %s"), currentNode->diskInfo.diskFullPath);
		LogInfo(TEXT("ShortPath: %s"), currentNode->diskInfo.diskShortPath);
		LogInfo(TEXT("Vendor: %s"),currentNode->diskInfo.diskVendor);
		LogInfo(TEXT("Manufacturer: %s"),currentNode->diskInfo.diskManufacturer);
		LogInfo(TEXT("ProductId: %s"),currentNode->diskInfo.diskProductId);
		LogInfo(TEXT("Revision: %s"),currentNode->diskInfo.diskRevision);
		LogInfo(TEXT("Serial: %s"),currentNode->diskInfo.diskSerial);
		LogInfo(TEXT("FriendlyName: %s"),currentNode->diskInfo.diskFriendlyName);
		LogInfo(TEXT("Number: %i"),currentNode->diskInfo.diskNumber);
		LogInfo(TEXT("LogicalSectorSize: %i"),currentNode->diskInfo.diskLogicalSectorSize);
		LogInfo(TEXT("PhysicalSectorSize: %i"),currentNode->diskInfo.diskPhysicalSectorSize);
		LogInfo(TEXT("IsRemovable: %i"),currentNode->diskInfo.diskIsRemovable);
		LogInfo(TEXT("IsWritable: %i"),currentNode->diskInfo.diskIsWritable);
		LogInfo(TEXT("Type: %s"),BlkdevGetDiskTypeString(currentNode->diskInfo.diskType));
		LogInfo(TEXT("BusType: %s"),BlkdevGetBusTypeString(currentNode->diskInfo.diskBusType));
		llist_node_t *current1;
		DWORD count = 0;
		llistForEach(&currentNode->diskPartitions, current1){
			blkdev_disk_partition_t *currentPart = (blkdev_disk_partition_t*)current1;
			LogInfo(TEXT("|"));
			LogInfo(TEXT("-> Partition %i"), count);
			LogInfo(TEXT("   FirstSector: %llu"), currentPart->partFirstSector);
			LogInfo(TEXT("   LastSector: %llu"), currentPart->partLastSector);
			LogInfo(TEXT("   Length: %llu"), currentPart->partSize);
			LogInfo(TEXT("   PartitionType: %s"), BlkdevGetPartTypeString(currentPart->partStyle));
		}
		LogInfo(TEXT("--------------------------------------------------------"));
	}
}

int main(int argc, char *argv[]){
	atexit(report_mem_leak);
	LogSetQuiet(false);
	LogInfo(TEXT(""));
	blkdev_set_handle_t test = BlkdevGetDevList();
	if(test != NULL){
		BlkdevPrintDevList(test);
		BlkdevDestroyDevList(test);
		free(test);

	}
	LogInfo(TEXT("End"));
	return(1);
}