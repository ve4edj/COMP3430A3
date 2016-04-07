#include "fat_fs.h"
#include "fat_helpers.h"

const char * typeNames[] = {"FAT12", "FAT16", "FAT32"};

FS_Instance * fs_create_instance(char * imagePath) {
	FS_Instance * fsi = malloc(sizeof(FS_Instance));
	if (NULL == fsi) {
		return NULL;
	}
	fsi->disk = fopen(imagePath, "r+");
	if (NULL == fsi->disk) {
		fs_cleanup(fsi);
		return NULL;
	}
	fsi->bootsect = malloc(sizeof(fatBS));
	if (NULL == fsi->bootsect) {
		fs_cleanup(fsi);
		return NULL;
	}
	fread(fsi->bootsect, sizeof(fatBS), 1, fsi->disk);
	if (0 == fsi->bootsect->BPB_RootEntCnt) {
		fsi->bootsect16 = NULL;
		fsi->bootsect32 = malloc(sizeof(fatBS32));
		if (NULL == fsi->bootsect32) {
			fs_cleanup(fsi);
			return NULL;
		}
		fsi->type = FS_FAT32;
		fread(fsi->bootsect32, sizeof(fatBS32), 1, fsi->disk);
		fsi->FATsz = fsi->bootsect32->BPB_FATSz32;
	} else {
		fsi->bootsect32 = NULL;
		fsi->bootsect16 = malloc(sizeof(fatBS16));
		if (NULL == fsi->bootsect16) {
			fs_cleanup(fsi);
			return NULL;
		}
		fsi->type = FS_FAT16;
		fread(fsi->bootsect16, sizeof(fatBS16), 1, fsi->disk);
	}
	if (0 != fsi->bootsect->BPB_FATSz16)
		fsi->FATsz = fsi->bootsect->BPB_FATSz16;

	fsi->numSectors = fsi->bootsect->BPB_TotSec32;
	if (fsi->numSectors == 0)
		fsi->numSectors = fsi->bootsect->BPB_TotSec16;
	fsi->totalSize = fsi->numSectors * fsi->bootsect->BPB_BytsPerSec;

	fsi->rootDirSectors = ((fsi->bootsect->BPB_RootEntCnt * 32) + (fsi->bootsect->BPB_BytsPerSec - 1)) / fsi->bootsect->BPB_BytsPerSec;
	fsi->dataSec = (fsi->bootsect->BPB_RsvdSecCnt + (fsi->bootsect->BPB_NumFATs * fsi->FATsz) + fsi->rootDirSectors);
	fsi->countOfClusters = (fsi->numSectors - fsi->dataSec) / fsi->bootsect->BPB_SecPerClus;
	fsi->rootDirPos = fsi->bootsect->BPB_RsvdSecCnt + (fsi->bootsect->BPB_NumFATs * fsi->FATsz);

	if (fsi->countOfClusters < 4085) {
		fsi->type = FS_FAT12;
	} else if (fsi->countOfClusters < 65525) {
		fsi->type = FS_FAT16;
	} else {
		fsi->type = FS_FAT32;
	}

	return fsi;
}

FS_Directory fs_get_root(FS_Instance * fsi) {
	switch (fsi->type) {
		case FS_FAT12:
		case FS_FAT16:
			return 0x00000000;
			break;
		case FS_FAT32:
			return fsi->bootsect32->BPB_RootClus;
			break;
	}
	return 0x00000000;
}

void loopPrintChar(uint8_t * str, int len) { for (int i = 0; i < len; printf("%c", str[i++])); }

const char * units[] = {"B", "kB", "MB", "GB", "TB"};
uint8_t scaleFileSize(long double * scaledSz) {
	int currUnit = 0;
	while (((long long)(*scaledSz / 1024)) > 0) {
		*scaledSz /= 1024;
		currUnit++;
	}
	return currUnit;
}

void print_info(FS_Instance * fsi) {
	fatBS * bs = fsi->bootsect;
	printf("\n");
	printf("Disk information:\n-----------------\n");
	printf("OEM Name: ");
	loopPrintChar(&(bs->BS_OEMName[0]), BS_OEMName_LENGTH);
	printf("\n");
	printf("Volume Label: ");
	if (fsi->type == FS_FAT32)
		loopPrintChar(&(fsi->bootsect32->BS_VolLab[0]), BS_VolLab_LENGTH);
	else
		loopPrintChar(&(fsi->bootsect16->BS_VolLab[0]), BS_VolLab_LENGTH);
	printf("\n");
	printf("File System Type (read): ");
	if (fsi->type == FS_FAT32)
		loopPrintChar(&(fsi->bootsect32->BS_FilSysType[0]), BS_FilSysType_LENGTH);
	else
		loopPrintChar(&(fsi->bootsect16->BS_FilSysType[0]), BS_FilSysType_LENGTH);
	printf("\n");
	printf("Media Type: 0x%2X (%sremovable)\n", bs->BPB_Media, (bs->BPB_Media == 0xF8) ? "non-" : "");
	long double scaledSz = fsi->totalSize;
	int theUnit = scaleFileSize(&scaledSz);
	printf("Size: %llu bytes (%.3Lf %s)\n", fsi->totalSize, scaledSz, units[theUnit]);
	printf("\n");
	printf("Disk geometry:\n--------------\n");
	printf("Bytes Per Sector: %u\n", bs->BPB_BytsPerSec);
	printf("Sectors Per Cluster: %u\n", bs->BPB_SecPerClus);
	printf("Total Sectors: %u\n", fsi->numSectors);
	printf("Physical - Sectors per Track: %u\n", bs->BPB_SecPerTrk);
	printf("Physical - Heads: %u\n", bs->BPB_NumHeads);
	printf("\n");
	printf("File system info:\n-----------------\n");
	printf("Volume ID: ");
	if (fsi->type == FS_FAT32)
		printf("%lu", fsi->bootsect32->BS_VolID);
	else
		printf("%lu", fsi->bootsect16->BS_VolID);
	printf("\n");
	printf("File System Type (computed): %s\n", typeNames[fsi->type]);
	printf("FAT Size (sectors): %u\n", fsi->FATsz);
	printf("Number of FATs: %u\n", fsi->bootsect->BPB_NumFATs);
	printf("Reserved sectors: %u\n", fsi->bootsect->BPB_RsvdSecCnt);
	printf("Root directory sectors: %llu\n", fsi->rootDirSectors);
	printf("Data clusters: %llu\n", fsi->countOfClusters);
	uint32_t freeClusters = 0;
	for (int i = 0; i < fsi->countOfClusters; i++) {
		if (getFATEntryForCluster(i+2, fsi) == 0) {
			freeClusters++;
		}
	}
	printf("Free space: %d bytes\n", freeClusters * fsi->bootsect->BPB_SecPerClus * fsi->bootsect->BPB_BytsPerSec);
	printf("\n");
}

char * getFilenameForEntry(fatEntry * ent) {
	char * filename = malloc(sizeof(char) * (DIR_Name_LENGTH + 2));
	if (NULL != filename) {
		int i = 0, j = 0;
		while (j < DIR_Name_LENGTH) {
			if (0x20 <= ent->DIR_Name[j]) {
				uint8_t isPadding = 1;
				for (int k = j; k < ((j < 8) ? 8 : 11); k++) {
					if (' ' != ent->DIR_Name[k]) {
						isPadding = 0;
						break;
					}
				}
				if (!isPadding)
					filename[i++] = ent->DIR_Name[j];
			}
			if ((7 == j) && (' ' != ent->DIR_Name[8]))
				filename[i++] = '.';
			j++;
		}
		filename[i] = '\0';
	}
	return filename;
}

void print_dir(FS_Instance * fsi, FS_Directory currDir) {
	uint16_t dirCount = 0, fileCount = 0;
	FS_EntryList * el = getDirListing((FS_Cluster)currDir, fsi);
	printf("%12s%25s%7s%20s\n", "Name    ", "Size         ", "Flags ", "Modified Date   ");
	printf("----------------------------------------------------------------\n");
	while (NULL != el) {
		FS_Entry * ent = el->node;
		char * filename = getFilenameForEntry(ent->entry);
		printf("%-12s", filename);
		free(filename);
		if (maskAndTest(ent->entry->DIR_Attr, ATTR_DIRECTORY) || maskAndTest(ent->entry->DIR_Attr, ATTR_VOLUME_ID)) {
			if (maskAndTest(ent->entry->DIR_Attr, ATTR_DIRECTORY) && (('.' != ent->entry->DIR_Name[0]) || (('.' == ent->entry->DIR_Name[0]) && ('.' != ent->entry->DIR_Name[1]) && (' ' != ent->entry->DIR_Name[1]))))
				dirCount++;
			printf("%25s", "");
		} else {
			fileCount++;
			long double scaledSz = ent->entry->DIR_FileSize;
			int theUnit = scaleFileSize(&scaledSz);
			printf("%12u (%7.3Lf %2s)", ent->entry->DIR_FileSize, scaledSz, units[theUnit]);
		}
		printf(" ");
		printf(maskAndTest(ent->entry->DIR_Attr, ATTR_VOLUME_ID) ? "V" : "-");
		printf(maskAndTest(ent->entry->DIR_Attr, ATTR_DIRECTORY) ? "D" : "-");
		printf(maskAndTest(ent->entry->DIR_Attr, ATTR_ARCHIVE)   ? "A" : "-");
		printf(maskAndTest(ent->entry->DIR_Attr, ATTR_SYSTEM)    ? "S" : "-");
		printf(maskAndTest(ent->entry->DIR_Attr, ATTR_HIDDEN)    ? "H" : "-");
		printf(maskAndTest(ent->entry->DIR_Attr, ATTR_READ_ONLY) ? "R" : "-");
		printf(" ");
		fatDate * cDate = &(ent->entry->DIR_WrtDate);
		fatTime * cTime = &(ent->entry->DIR_WrtTime);
		printf("%04d/%02d/%02d %02d:%02d:%02d", cDate->year + 1980, cDate->month, cDate->day, cTime->hour, cTime->min, (cTime->sec * 2) + (ent->entry->DIR_CrtTimeTenth / 2));
		if (ent->filename) {
			printf(" ( ");
			int idx = 0;
			while (0x0000 != ent->filename[idx]) {
				printf("%c", ent->filename[idx++] & 0x00FF);
			}
			printf(" )");
		}
		printf("\n");
		FS_EntryList * toFree = el;
		el = el->next;
		free(toFree->node->filename);
		free(toFree->node->entry);
		free(toFree->node);
		free(toFree);
	}
	printf("\t%d file(s), %d folder(s)\n", fileCount, dirCount);
}

FS_Directory change_dir(FS_Instance * fsi, FS_Directory currDir, char * path) {
	char * pathCopy = strdup(path);
	FS_Directory origDir = currDir;
																										// validate the filename
	char * toke = strtok(pathCopy, "/\\");
	FS_Directory dir = currDir;
	while (NULL != toke) {
		currDir = dir;
		FS_EntryList * el = getDirListing((FS_Cluster)dir, fsi);
		uint8_t found = 0;
		while (NULL != el) {
			FS_Entry * ent = el->node;
			if ((currDir == dir) && (maskAndTest(ent->entry->DIR_Attr, ATTR_DIRECTORY))) {
				char * filename = getFilenameForEntry(ent->entry);
				if (strcmp(toke, filename) == 0) {
					found = 1;
					dir = (ent->entry->DIR_FstClusHI << 8) + ent->entry->DIR_FstClusLO;
				}
				free(filename);
			}
			FS_EntryList * toFree = el;
			el = el->next;
			free(toFree->node->filename);
			free(toFree->node->entry);
			free(toFree->node);
			free(toFree);
		}
		if (!found) {
			dir = 0x00000001;
			break;
		}
		toke = strtok(NULL, "/\\");
	}
	free(pathCopy);
	return dir;
}

void get_file(FS_Instance * fsi, FS_Directory currDir, char * path, char * localPath) {

}

void put_file(FS_Instance * fsi, FS_Directory currDir, char * path, char * localPath) {

}

FS_Directory make_dir(FS_Instance * fsi, FS_Directory currDir, char * path) {

}

FS_Directory delete_file(FS_Instance * fsi, FS_Directory currDir, char * path) {

}

void fs_cleanup(FS_Instance * fsi) {
	if (NULL != fsi) {
		if (NULL != fsi->disk) {
			fclose(fsi->disk);
		}
		if (NULL != fsi->bootsect) {
			free(fsi->bootsect);
		}
		if (NULL != fsi->bootsect16) {
			free(fsi->bootsect16);
		}
		if (NULL != fsi->bootsect32) {
			free(fsi->bootsect32);
		}
		free(fsi);
	}
}
