#include "fat_helpers.h"

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
			break;
		case FS_FAT16:
			entry = (*((uint16_t *)&FATSector[entOffset]));
			break;
		case FS_FAT32:
			entry = ((*((uint32_t *)&FATSector[entOffset])) & 0x0FFFFFFF);
			break;
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
		letter = ((uint16_t *)&ln->LDIR_Name2[0])[pos - (LDIR_Name1_LENGTH / 2)];
	} else if (pos < ((LDIR_Name1_LENGTH + LDIR_Name2_LENGTH + LDIR_Name3_LENGTH) / 2)) {
		letter = ((uint16_t *)&ln->LDIR_Name3[0])[pos - (LDIR_Name1_LENGTH + LDIR_Name2_LENGTH) / 2];
	}
	return letter;
}

uint8_t getLongNameLength(fatLongName * ln) {
	if (maskAndTest(ln->LDIR_Ord, LAST_LONG_ENTRY)) {
		uint8_t nameLen = ((ln->LDIR_Ord & ~(LAST_LONG_ENTRY)) - 1) * LDIR_LettersPerEntry;
		for (int i = 0; i < LDIR_LettersPerEntry; i++) {
			if (0x0000 == getLongNameLetterAtPos(i, ln))
				break;
			nameLen++;
		}
		return nameLen;
	}
	return 0;
}

uint8_t getLongNameStartPos(fatLongName * ln) {
	return ((ln->LDIR_Ord & ~(LAST_LONG_ENTRY)) - 1) * LDIR_LettersPerEntry;
}

uint8_t isSpecialRootDir(FS_Cluster dir, FS_Instance * fsi) {
	return ((fsi->type == FS_FAT12) || (fsi->type == FS_FAT16)) && (dir == 0x00000000);
}

FS_EntryList * getDirListing(FS_Cluster dir, FS_Instance * fsi) {
	uint8_t specialRootDir = isSpecialRootDir(dir, fsi);
	uint32_t bytesPerCluster = ((!specialRootDir) ? fsi->bootsect->BPB_SecPerClus : 1) * fsi->bootsect->BPB_BytsPerSec;
	uint32_t entriesPerCluster = bytesPerCluster / sizeof(fatEntry);
	fatEntry * entries = malloc(entriesPerCluster * sizeof(fatEntry));
	if (NULL == entries)
		return NULL;
	uint16_t * longName = NULL;
	FS_EntryList * listHead = NULL;
	FS_EntryList * listTail = NULL;
	if (specialRootDir)
		dir = fsi->rootDirPos;
	do {
		uint64_t seekTo = dir;
		if (!specialRootDir)
			seekTo = getFirstSectorOfCluster(dir, fsi);
		fseek(fsi->disk, (seekTo * fsi->bootsect->BPB_BytsPerSec), SEEK_SET);
		fread(entries, sizeof(fatEntry), entriesPerCluster, fsi->disk);
		for (int i = 0; i < entriesPerCluster; i++) {
			fatEntry * entry = &(entries[i]);
			if (0x00 == entry->DIR_Name[0])
				break;
			if (0xE5 == entry->DIR_Name[0])
				continue;
			if (0x05 == entry->DIR_Name[0])
				entry->DIR_Name[0] = 0xE5;
			if (maskAndTest(entry->DIR_Attr, ATTR_LONG_NAME)) {
				fatLongName * ln = (fatLongName *)entry;
				if (NULL == longName)
					longName = calloc(getLongNameLength(ln) + 1, sizeof(uint16_t));
				if (NULL == longName)
					return NULL;															// should do some cleanup here
																							// check the type and verify the checksum here
				uint8_t startPos = getLongNameStartPos(ln);
				for (int i = 0; i < LDIR_LettersPerEntry; i++) {
					longName[startPos + i] = getLongNameLetterAtPos(i, ln);					// validate the characters
				}
			} else {
				uint8_t validEntry = 1;
				for (int j = 0; j < DIR_Name_LENGTH; j++)									// check for the other invalid characters too
					if (0x20 > entry->DIR_Name[j])
						validEntry = 0;
				if (!validEntry)
					continue;
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
				listEntry->next = NULL;
				if (NULL != listTail)
					listTail->next = listEntry;
				listTail = listEntry;
				if (NULL == listHead)
					listHead = listEntry;
			}
		}
		if (!specialRootDir)
			dir = getFATEntryForCluster(dir, fsi);
		else
			dir++;
	} while (specialRootDir ? (dir < (fs_get_root(fsi) + fsi->rootDirSectors)) : !isFATEntryEOF(dir, fsi));
	free(entries);
	return listHead;
}