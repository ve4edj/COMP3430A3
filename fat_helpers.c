#include "fat_fs.h"
#include <stdlib.h>
#include <stdio.h>

uint64_t calcFATOffset(uint64_t cluster, FS_Instance * fsi) {
	switch (fsi->type) {
		case FS_FAT12:
			return cluster + (cluster / 2);
		case FS_FAT16:
			return cluster * 2;
		case FS_FAT32:
			return cluster * 4;
	}
}

uint32_t getFATSectorNum(uint64_t cluster, FS_Instance * fsi) {
	return (fsi->bootsect->BPB_RsvdSecCnt + (calcFATOffset(cluster, fsi) / fsi->bootsect->BPB_BytsPerSec));
}

uint32_t getFATEntryOffset(uint64_t cluster, FS_Instance * fsi) {
	return (calcFATOffset(cluster, fsi) % fsi->bootsect->BPB_BytsPerSec);
}

uint64_t getFirstSectorOfCluster(uint64_t cluster, FS_Instance * fsi) {
	return (((cluster - 2) * fsi->bootsect->BPB_SecPerClus) + fsi->dataSec);
}

FS_FATEntry getFATEntryForCluster(uint64_t cluster, FS_Instance * fsi) {
	uint32_t sectorNum = getFATSectorNum(cluster, fsi);
	uint32_t entOffset = getFATEntryOffset(cluster, fsi);
	uint32_t bytesToRead = fsi->bootsect->BPB_BytsPerSec;
	if ((fsi->type == FS_FAT12) && (entOffset == (fsi->bootsect->BPB_BytsPerSec - 1)) && !((sectorNum - fsi->bootsect->BPB_RsvdSecCnt) == (fsi->FATsz - 1))) {
		bytesToRead *= 2;
	}
	uint8_t * FATSector = malloc(bytesToRead);
	fseek(fsi->disk, (sectorNum * fsi->bootsect->BPB_BytsPerSec), SEEK_SET);
	fread(FATSector, bytesToRead, 1, fsi->disk);
	switch (fsi->type) {
		case FS_FAT12:
			if (cluster % 2)
				return ((*((uint16_t *)&FATSector[entOffset])) >> 4);
			else
				return ((*((uint16_t *)&FATSector[entOffset])) & 0x0FFF);
		case FS_FAT16:
			return (*((uint16_t *)&FATSector[entOffset]));
		case FS_FAT32:
			return ((*((uint32_t *)&FATSector[entOffset])) & 0x0FFFFFFF);
	}
}

uint8_t isFATEntryEOF(FS_FATEntry entry, FS_Instance * fsi) {
	switch (fsi->type) {
		case FS_FAT12:
			return (entry >= 0x0FF8);
		case FS_FAT16:
			return (entry >= 0xFFF8);
		case FS_FAT32:
			return (entry >= 0x0FFFFFF8);
	}
}

uint8_t isFATEntryBad(FS_FATEntry entry, FS_Instance * fsi) {
	switch (fsi->type) {
		case FS_FAT12:
			return (entry == 0x0FF7);
		case FS_FAT16:
			return (entry == 0xFFF7);
		case FS_FAT32:
			return (entry == 0x0FFFFFF7);
	}
}