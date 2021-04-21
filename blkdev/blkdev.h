#ifndef BLKDEV_H
#define BLKDEV_H

#include <Windows.h>

typedef enum blkdev_disk_type_e{
	BLKDEV_TYPE_UNK,
	BLKDEV_TYPE_HDD,
	BLKDEV_TYPE_SSD
}blkdev_disk_type;

typedef enum blkdev_part_type_e{
	BLKDEV_PART_RAW,
	BLKDEV_PART_MBR,
	BLKDEV_PART_GPT
}blkdev_part_type_t;

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

typedef struct blkdev_entry_s{
	LONGLONG firstSector;
	LONGLONG lastSector;
	LONGLONG size;
	blkdev_part_type_t type;
}blkdev_entry_t;

typedef struct blkdev_disk_info_s{
	LPTSTR diskFullPath;
	LPTSTR diskShortPath;
	LPTSTR diskVendor;
	LPTSTR diskProductId;
	LPTSTR diskSerial;
	LPTSTR diskRevision;
	LPTSTR diskManufacturer;
	LPTSTR diskFriendlyName;
	BOOL diskIsRemovable;
	BOOL diskIsWritable;
	blkdev_disk_type diskType;
	blkdev_bus_type diskBusType;
	DWORD diskNumber;
	LONGLONG diskSize;
}blkdev_disk_info_t;




typedef void* blkdev_handle_t;

DWORD BlkdevRead(blkdev_handle_t device);

#endif