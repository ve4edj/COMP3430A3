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

void setFATEntryForCluster(FS_Cluster cluster, FS_FATEntry entry, FS_Instance * fsi) {
	uint32_t sectorNum = getFATSectorNum(cluster, fsi);
	uint32_t entOffset = getFATEntryOffset(cluster, fsi);
	uint32_t bytesToRead = fsi->bootsect->BPB_BytsPerSec;
	if ((fsi->type == FS_FAT12) && (entOffset == (fsi->bootsect->BPB_BytsPerSec - 1)) && !((sectorNum - fsi->bootsect->BPB_RsvdSecCnt) == (fsi->FATsz - 1))) {
		bytesToRead *= 2;
	}
	uint8_t * FATSector = malloc(bytesToRead);
	if (NULL == FATSector)
		return;
	fseek(fsi->disk, (sectorNum * fsi->bootsect->BPB_BytsPerSec), SEEK_SET);
	fread(FATSector, bytesToRead, 1, fsi->disk);
	switch (fsi->type) {
		case FS_FAT12:
			if (cluster % 2) {
				(*((uint16_t *)&FATSector[entOffset])) &= 0x000F;
				entry <<= 4;
			} else {
				(*((uint16_t *)&FATSector[entOffset])) &= 0xF000;
				entry &= 0x0FFF;
			}
			(*((uint16_t *)&FATSector[entOffset])) |= entry;
			break;
		case FS_FAT16:
			(*((uint16_t *)&FATSector[entOffset])) = entry;
			break;
		case FS_FAT32:
			(*((uint32_t *)&FATSector[entOffset])) &= 0xF0000000;
			(*((uint32_t *)&FATSector[entOffset])) |= entry & 0x0FFFFFFF;
			break;
	}
	fseek(fsi->disk, (sectorNum * fsi->bootsect->BPB_BytsPerSec), SEEK_SET);
	fwrite(FATSector, bytesToRead, 1, fsi->disk);
	free(FATSector);
}

FS_FATEntry getEOFMarker(FS_Instance * fsi) {
	switch (fsi->type) {
		case FS_FAT12:
			return 0x00000FF8;
		case FS_FAT16:
			return 0x0000FFF8;
		case FS_FAT32:
			return 0x0FFFFFF8;
	}
	return 0xFFFFFFFF;
}

uint8_t isFATEntryEOF(FS_FATEntry entry, FS_Instance * fsi) {
	return (entry >= getEOFMarker(fsi));
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

uint8_t isValidFilenameChar(char c, uint8_t isLongFilename) {
	if ((0x20 > c) && (0x00 != c))
		return 0;
	if ('A' <= c && 'Z' >= c)
		return 1;
	if ('0' <= c && '9' >= c)
		return 1;
	if (0x7F < c)
		return 1;
	switch (c) {
		case ' ':
		case '.':
		case '$':
		case '%':
		case '-':
		case '_':
		case '@':
		case '~':
		case '`':
		case '!':
		case '(':
		case ')':
		case '{':
		case '}':
		case '^':
		case '#':
		case '&':
		case '\'':
			return 1;
		default:
			if (isLongFilename) {
				if ('a' <= c && 'z' >= c)
					return 1;
				switch (c) {
					case '+':
					case ',':
					case ';':
					case '=':
					case '[':
					case ']':
						return 1;
					default:
						return 0;
				}
			}
			return 0;
	}
}

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

uint8_t getLongNameChecksum(fatEntry * entry) {
	uint8_t checksum = 0;
	for (int i = 0; i < DIR_Name_LENGTH; i++) {
		checksum = (((checksum & 1) ? 0x80 : 0) + (checksum >> 1)) + entry->DIR_Name[i];
	}
	return checksum;
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
				if (0 != ln->LDIR_Type) {
					free(longName);
					longName = NULL;
					break;
				}
				uint8_t startPos = getLongNameStartPos(ln);
				for (int i = 0; i < LDIR_LettersPerEntry; i++) {
					longName[startPos + i] = getLongNameLetterAtPos(i, ln);
					if (0x0000 == longName[startPos + i])
						break;
					if (!isValidFilenameChar(longName[startPos + i] & 0x00FF, 1)) {
						free(longName);
						longName = NULL;
						break;
					}
				}
				if (NULL == longName)
					continue;
			} else {
				uint8_t validEntry = 1;
				for (int j = 0; j < DIR_Name_LENGTH; j++)
					if (!isValidFilenameChar(entry->DIR_Name[j], 0)) {
						validEntry = 0;
					}
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
	} while (specialRootDir ? (dir < (fsi->rootDirPos + fsi->rootDirSectors)) : !isFATEntryEOF(dir, fsi));
	free(entries);
	return listHead;
}

void freeFSEntryListItem(FS_EntryList * toFree) {
	free(toFree->node->filename);
	free(toFree->node->entry);
	free(toFree->node);
	free(toFree);
}

FS_Cluster getNextFreeCluster(FS_Instance * fsi) {
	for (FS_Cluster i = 0; i < fsi->countOfClusters; i++) {
		if (getFATEntryForCluster(i+2, fsi) == 0) {
			return i+2;
		}
	}
	return 0x00000001;
}

uint8_t getNumberOfLongEntriesForFilename(char * filename) {
	uint8_t validShortName = 1, isExtensionPart = 0;
	for (int i = 0; i < strlen(filename); i++) {
		validShortName &= isValidFilenameChar(filename[i], 0);
		if (isExtensionPart > 3)
			validShortName = 0;
		if (isExtensionPart)
			isExtensionPart++;
		if ('.' == filename[i]) {
			if (isExtensionPart)
				validShortName = 0;
			else
				isExtensionPart = 1;
		}
		if ((i == 8) && !isExtensionPart)
			validShortName = 0;
		if (i > DIR_Name_LENGTH)
			validShortName = 0;
		if (!validShortName)
			break;
	}
	if (validShortName)
		return 0;
	return ((strlen(filename) - 1) / LDIR_LettersPerEntry) + 1;
}

void setNumericTail(fatEntry * entry, uint32_t tailVal) {
	uint8_t tailPos = 7;
	while (tailVal) {
		entry->DIR_Name[tailPos--] = (tailVal % 10) + '0';
		tailVal /= 10;
	}
	if (7 > tailPos)
		entry->DIR_Name[tailPos] = '~';
}

fs_result fillShortNameFromLongName(FS_Cluster dir, fatEntry * entry, char * filename, FS_Instance * fsi) {
	uint32_t currTail = 0;
	char * name = strdup(filename);
	if (NULL == name)
		return ERR_MALLOCFAILED;
	char * extension = strrchr(name, '.');
	if (NULL != extension) {
		*extension = '\0';
		extension++;
	}
	uint8_t wasLossy = 0;
	uint8_t j = 0;
	for (uint8_t i = 0; i < strlen(name); i++) {
		if (8 == j) {
			wasLossy = 1;
			break;
		}
		char c = toupper(name[i]);
		if (' ' == c)
			continue;
		if ('.' == c)
			continue;
		if (!isValidFilenameChar(c, 0))
			entry->DIR_Name[j++] = '_';
		else
			entry->DIR_Name[j++] = c;
	}
	while (8 > j) { entry->DIR_Name[j++] = ' '; }
	if (wasLossy) {
		setNumericTail(entry, ++currTail);
	}
	if (NULL != extension) {
		for (uint8_t i = 0; i < strlen(extension); i++) {
			if (DIR_Name_LENGTH == j)
				break;
			char c = toupper(extension[i]);
			if (' ' == c)
				continue;
			if ('.' == c)
				continue;
			if (!isValidFilenameChar(c, 0))
				entry->DIR_Name[j++] = '_';
			else
				entry->DIR_Name[j++] = c;
		}
	}
	while (DIR_Name_LENGTH > j) { entry->DIR_Name[j++] = ' '; }
	free(name);

	FS_EntryList * el = getDirListing((FS_Cluster)dir, fsi);
	FS_EntryList * curr = el;
	uint8_t found = 0;
	while (NULL != curr) {
		// loop through the directory looking for the short name or long name
		// if the long name is found, set found and break
		// if the short name is found and neither have a long name, set found and break
		// if the short name is found but the long name is different or the found entry doesn't have a long name, update the numeric tail and restart looking
		curr = curr->next;
	}
	while (NULL != el) {
		FS_EntryList * toFree = el;
		el = el->next;
		freeFSEntryListItem(toFree);
	}
	if (found) {
		return ERR_FILENAMEEXISTS;
	}
	return ERR_SUCCESS;
}

void getLongNameSection(fatEntry * entry, fatLongName * ln, uint8_t section, uint8_t entries, char * filename) {
	ln->LDIR_Ord = entries - section;
	if (0 == section)
		ln->LDIR_Ord |= LAST_LONG_ENTRY;
	ln->LDIR_Attr = ATTR_LONG_NAME;
	ln->LDIR_Type = 0;
	ln->LDIR_Chksum = getLongNameChecksum(entry);
	ln->LDIR_Zero = 0;
	//ln->LDIR_Name1[LDIR_Name1_LENGTH] = ;
	//ln->LDIR_Name2[LDIR_Name2_LENGTH] = ;
	//ln->LDIR_Name3[LDIR_Name3_LENGTH] = ;
}

fs_result getNContiguousDirEntries(FH_DirEntryPos * dirEntry, FS_Cluster dir, uint8_t numEntries, FS_Instance * fsi) {
	uint8_t found = 0;
	uint8_t freeEntriesFound = 0;
	FS_Cluster lastDir;
	uint8_t specialRootDir = isSpecialRootDir(dir, fsi);
	uint32_t bytesPerCluster = ((!specialRootDir) ? fsi->bootsect->BPB_SecPerClus : 1) * fsi->bootsect->BPB_BytsPerSec;
	uint32_t entriesPerCluster = bytesPerCluster / sizeof(fatEntry);
	fatEntry * entries = malloc(entriesPerCluster * sizeof(fatEntry));
	if (NULL == entries)
		return ERR_MALLOCFAILED;
	if (specialRootDir)
		dir = fsi->rootDirPos;
	do {
		freeEntriesFound = 0;
		uint64_t seekTo = dir;
		if (!specialRootDir)
			seekTo = getFirstSectorOfCluster(dir, fsi);
		fseek(fsi->disk, (seekTo * fsi->bootsect->BPB_BytsPerSec), SEEK_SET);
		fread(entries, sizeof(fatEntry), entriesPerCluster, fsi->disk);
		for (int i = 0; i < entriesPerCluster; i++) {
			fatEntry * entry = &(entries[i]);
			if (0 == freeEntriesFound) {
				dirEntry->cluster = dir;
				dirEntry->index = i;
			}
			if (0x00 == entry->DIR_Name[0])
				freeEntriesFound += entriesPerCluster - i;
			else if (0xE5 == entry->DIR_Name[0])
				freeEntriesFound++;
			else
				freeEntriesFound = 0;
			if (freeEntriesFound >= numEntries) {
				found = 1;
				break;
			}
		}
		if (found)
			break;
		lastDir = dir;
		if (!specialRootDir)
			dir = getFATEntryForCluster(dir, fsi);
		else
			dir++;
	} while (specialRootDir ? (dir < (fsi->rootDirPos + fsi->rootDirSectors)) : !isFATEntryEOF(dir, fsi));
	free(entries);
	if (found)
		return ERR_SUCCESS;
	if (isSpecialRootDir)
		return ERR_ROOTDIRFULL;
	FS_Cluster nextCluster = getNextFreeCluster(fsi);
	if (1 == nextCluster)
		return ERR_NOFREESPACE;
	setFATEntryForCluster(lastDir, nextCluster, fsi);
	setFATEntryForCluster(nextCluster, getEOFMarker(fsi), fsi);
	dirEntry->cluster = nextCluster;
	dirEntry->index = 0;
	return ERR_SUCCESS;
}

fs_result addDirListing(FS_Cluster dir, char * filename, fatEntry * entry, FS_Instance * fsi) {
	uint8_t LFNentries = getNumberOfLongEntriesForFilename(filename);
	fs_result result = fillShortNameFromLongName(dir, entry, filename, fsi);
	if (ERR_SUCCESS != result)
		return result;
	FH_DirEntryPos * entryPos = malloc(sizeof(FH_DirEntryPos));
	if (NULL == entryPos)
		return ERR_MALLOCFAILED;
	result = getNContiguousDirEntries(entryPos, dir, LFNentries + 1, fsi);
	if (ERR_SUCCESS != result) {
		free(entryPos);
		return result;
	}
	uint64_t seekTo = entryPos->cluster;
	if (!isSpecialRootDir(dir, fsi))
		seekTo = getFirstSectorOfCluster(entryPos->cluster, fsi);
	fseek(fsi->disk, (seekTo * fsi->bootsect->BPB_BytsPerSec) + (entryPos->index * sizeof(fatEntry)), SEEK_SET);
	free(entryPos);
	fatLongName * ln = malloc(sizeof(fatLongName));
	if (NULL == ln)
		return ERR_MALLOCFAILED;
	for (uint8_t i = 0; i < LFNentries; i++) {
		getLongNameSection(entry, ln, i, LFNentries, filename);
		fwrite(ln, sizeof(fatLongName), 1, fsi->disk);
	}
	fwrite(entry, sizeof(fatEntry), 1, fsi->disk);
	return ERR_SUCCESS;
}

void zeroCluster(FS_Cluster cluster, FS_Instance * fsi) {
	uint8_t zero = 0;
	fseek(fsi->disk, (getFirstSectorOfCluster(cluster, fsi) * fsi->bootsect->BPB_BytsPerSec), SEEK_SET);
	for (int i = 0; i < (fsi->bootsect->BPB_SecPerClus * fsi->bootsect->BPB_BytsPerSec); i++) {
		fwrite(&zero, sizeof(uint8_t), 1, fsi->disk);
	}
}