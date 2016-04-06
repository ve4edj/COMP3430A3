#include "fat_helpers.h"
#include <string.h>

uint64_t calcFATOffset(FS_Cluster cluster, FS_Instance * fsi) {
	switch (fsi->type) {
		case FS_FAT12:
			return cluster + (cluster / 2);
		case FS_FAT16:
			return cluster * 2;
		case FS_FAT32:
			return cluster * 4;
	}
	return 0xFFFFFFFFFFFFFFFF;
}

uint32_t getFATSectorNum(FS_Cluster cluster, FS_Instance * fsi) {
	return (fsi->bootsect->BPB_RsvdSecCnt + (calcFATOffset(cluster, fsi) / fsi->bootsect->BPB_BytsPerSec));
}

uint32_t getFATEntryOffset(FS_Cluster cluster, FS_Instance * fsi) {
	return (calcFATOffset(cluster, fsi) % fsi->bootsect->BPB_BytsPerSec);
}

uint64_t getFirstSectorOfCluster(FS_Cluster cluster, FS_Instance * fsi) {
	return (((cluster - 2) * fsi->bootsect->BPB_SecPerClus) + fsi->dataSec);
}

FS_FATEntry getFATEntryForCluster(FS_Cluster cluster, FS_Instance * fsi) {
	uint32_t sectorNum = getFATSectorNum(cluster, fsi);
	uint32_t entOffset = getFATEntryOffset(cluster, fsi);
	uint32_t bytesToRead = fsi->bootsect->BPB_BytsPerSec;
	if ((fsi->type == FS_FAT12) && (entOffset == (fsi->bootsect->BPB_BytsPerSec - 1)) && !((sectorNum - fsi->bootsect->BPB_RsvdSecCnt) == (fsi->FATsz - 1))) {
		bytesToRead *= 2;
	}
	uint8_t * FATSector = malloc(bytesToRead);
	if (NULL == FATSector)
		return 0;
	fseek(fsi->disk, (sectorNum * fsi->bootsect->BPB_BytsPerSec), SEEK_SET);
	fread(FATSector, bytesToRead, 1, fsi->disk);
	FS_FATEntry entry = 0xFFFFFFFF;
	switch (fsi->type) {
		case FS_FAT12:
			if (cluster % 2)
				entry = ((*((uint16_t *)&FATSector[entOffset])) >> 4);
			else
				entry = ((*((uint16_t *)&FATSector[entOffset])) & 0x0FFF);
		case FS_FAT16:
			entry = (*((uint16_t *)&FATSector[entOffset]));
		case FS_FAT32:
			entry = ((*((uint32_t *)&FATSector[entOffset])) & 0x0FFFFFFF);
	}
	free(FATSector);
	return entry;
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
	return 0xFF;
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
	return 0xFF;
}

uint8_t maskAndTest(uint8_t val, uint8_t mask) { return (val & mask) == mask; }

uint16_t getLongNameLetterAtPos(int pos, fatLongName * ln) {
	uint16_t letter = 0x0000;
	if (pos < (LDIR_Name1_LENGTH / 2)) {
		letter = ((uint16_t *)&ln->LDIR_Name1[0])[pos];
	} else if (pos < ((LDIR_Name1_LENGTH + LDIR_Name2_LENGTH) / 2)) {
		letter = ((uint16_t *)&ln->LDIR_Name2[0])[pos - LDIR_Name1_LENGTH];
	} else if (pos < ((LDIR_Name1_LENGTH + LDIR_Name2_LENGTH + LDIR_Name3_LENGTH) / 2)) {
		letter = ((uint16_t *)&ln->LDIR_Name3[0])[pos - LDIR_Name1_LENGTH - LDIR_Name2_LENGTH];
	}
	return letter;
}

uint8_t getLongNameLength(fatLongName * ln) {
	if (maskAndTest(ln->LDIR_Ord, LAST_LONG_ENTRY)) {
		uint8_t nameLen = ((ln->LDIR_Ord & ~(LAST_LONG_ENTRY)) - 1) * LDIR_LettersPerEntry;
		for (int i = 0; i < LDIR_LettersPerEntry; i++) {
			if (0x0000 == getLongNameLetterAtPos(i, ln)) {
				return nameLen;
			}
			nameLen++;
		}
		return nameLen;
	}
	return 0;
}

uint8_t getLongNameStartPos(fatLongName * ln) {
	return ((ln->LDIR_Ord & ~(LAST_LONG_ENTRY)) - 1) * LDIR_LettersPerEntry;
}

FS_EntryList * getDirListing(FS_Cluster dir, FS_Instance * fsi) {
	uint32_t bytesPerCluster = fsi->bootsect->BPB_SecPerClus * fsi->bootsect->BPB_BytsPerSec;
	uint32_t entriesPerCluster = bytesPerCluster / sizeof(fatEntry);
	fatEntry * entries = malloc(entriesPerCluster * sizeof(fatEntry));
	if (NULL == entries)
		return NULL;
	uint16_t * longName = NULL;
	FS_EntryList * listHead = NULL;
	do {
		fseek(fsi->disk, getFirstSectorOfCluster(dir, fsi), SEEK_SET);
		fread(entries, sizeof(fatEntry), entriesPerCluster, fsi->disk);
		for (int i = 0; i < entriesPerCluster; i++) {
			fatEntry * entry = &entries[i];
			if (0x00 == entry->DIR_Name[0])
				break;
			if (0xE5 == entry->DIR_Name[0])
				continue;
			if (maskAndTest(entry->DIR_Attr, ATTR_LONG_NAME)) {
				fatLongName * ln = (fatLongName *)entry;
				if (NULL == longName)
					longName = calloc(getLongNameLength(ln) + 1, sizeof(uint16_t));
				if (NULL == longName)
					return NULL;															// should do some cleanup here
																							// check the type and verify the checksum here
				uint8_t startPos = getLongNameStartPos(ln);
				for (int i = 0; i < LDIR_LettersPerEntry; i++) {
					longName[startPos + i] = getLongNameLetterAtPos(i, ln);
				}
			} else {
				FS_EntryList * listEntry = malloc(sizeof(FS_EntryList));
				if (NULL == listEntry)
					return NULL;															// should do some cleanup here
				listEntry->node = malloc(sizeof(FS_Entry));
				if (NULL == listEntry->node)
					return NULL;															// should do some cleanup here
				listEntry->node->entry = malloc(sizeof(fatEntry));
				if (NULL == listEntry->node->entry)
					return NULL;															// should do some cleanup here
				memcpy(listEntry->node->entry, entry, sizeof(fatEntry));
				listEntry->node->filename = longName;
				longName = NULL;
				listEntry->next = listHead;
				listHead = listEntry;
			}
		}
		dir = getFATEntryForCluster(dir, fsi);
	} while (!isFATEntryEOF(dir, fsi));
	free(entries);
	return listHead;
}

// code for following a cluster chain, may be useful later

// uint32_t clusterToRead = (uint32_t)dir;
// uint32_t clusterOffset = entry * sizeof(fatEntry);
// while (clusterOffset >= fsi->bootsect->BPB_BytsPerSec) {
// 	clusterToRead = getFATEntryForCluster(clusterToRead, fsi);
// 	clusterOffset -= fsi->bootsect->BPB_BytsPerSec;
// }