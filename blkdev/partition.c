#include <Windows.h>

#define BIT(x) (1 << x)

#define BLKDEV_MBR_IS_VALID 0
#define BLKDEV_MBR_INVALID_SIGNATURE BIT(1)
#define BLKDEV_MBR_OVERLAPPING_PARTITIONS BIT(2)

typedef struct __attribute__((packed)) blkdev_mbr_chs_entry_s{
	BYTE mbrChsHead;
	BYTE mbrChsSector;
	BYTE mbrChsCylinder;
}blkdev_mbr_chs_entry_t;

typedef struct __attribute__((packed)) blkdev_mbr_part_entry_s{
	BYTE mbrPartIsBootable;
	blkdev_mbr_chs_entry_t mbrPartChsFirstSector;
	BYTE mbrPartType;
	blkdev_mbr_chs_entry_t mbrPartChsLastSector;
	DWORD mbrPartFirstSector;
	DWORD mbrPartSectorCount;
}blkdev_mbr_part_entry_t;

typedef struct __attribute__((packed)) blkdev_mbr_s{
	BYTE mbrBootloader[446];
	blkdev_mbr_part_entry_t mbrPartitionTable[4];
	BYTE mbrSignature[2];
}blkdev_mbr_t;

BOOL BlkdevMbr

DWORD BlkdevMbrIsValid(blkdev_mbr_t *mbr){
	DWORD result = 0;
	if(mbr->mbrSignature[0] != 0x55 ||
	   mbr->mbrSignature[1] != 0xaa){
		result &= BLKDEV_MBR_INVALID_SIGNATURE;
	}


	return(result);
}
