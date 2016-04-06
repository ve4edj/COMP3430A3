#include "fat_fs.h"
#include "fat_helpers.h"

const char * typeNames[] = {"FAT12", "FAT16", "FAT32"};

FS_Instance * fs_create_instance(char * image_path) {
	FS_Instance * fsi = malloc(sizeof(FS_Instance));
	if (NULL == fsi) {
		return NULL;
	}
	fsi->disk = fopen(image_path, "r+");
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
			return (fsi->bootsect->BPB_RsvdSecCnt + (fsi->bootsect->BPB_NumFATs * fsi->FATsz));
			break;
		case FS_FAT32:
			return fsi->bootsect32->BPB_RootClus;
			break;
	}
	return 0x00000000;
}

void loopPrintChar(uint8_t * str, int len) { for (int i = 0; i < len; printf("%c", str[i++])); }

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
	const char * units[] = {"B", "kB", "MB", "GB", "TB"};
	int currUnit = 0;
	long double scaledSz = fsi->totalSize;
	while (((long long)(scaledSz / 1024)) > 0) {
		scaledSz /= 1024;
		currUnit++;
	}
	printf("Size: %llu bytes (%.3Lf %s)\n", fsi->totalSize, scaledSz, units[currUnit]);
	printf("\n");
	printf("Disk geometry:\n--------------\n");
	printf("Bytes Per Sector: %d\n", bs->BPB_BytsPerSec);
	printf("Sectors Per Cluster: %d\n", bs->BPB_SecPerClus);
	printf("Total Sectors: %d\n", fsi->numSectors);
	printf("Physical - Sectors per Track: %d\n", bs->BPB_SecPerTrk);
	printf("Physical - Heads: %d\n", bs->BPB_NumHeads);
	printf("\n");
	printf("File system info:\n-----------------\n");
	printf("Volume ID: ");
	if (fsi->type == FS_FAT32)
		printf("%d", fsi->bootsect32->BS_VolID);
	else
		printf("%d", fsi->bootsect16->BS_VolID);
	printf("\n");
	printf("File System Type (computed): %s\n", typeNames[fsi->type]);
	printf("FAT Size (sectors): %d\n", fsi->FATsz);
	printf("Number of FATs: %d\n", fsi->bootsect->BPB_NumFATs);
	printf("Reserved sectors: %d\n", fsi->bootsect->BPB_RsvdSecCnt);
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

void print_dir(FS_Instance * fsi, FS_Directory current_dir) {
	FS_EntryList * el = getDirListing((FS_Cluster)current_dir, fsi);
	while (NULL != el) {
		FS_Entry * ent = el->node;
		loopPrintChar(ent->entry->DIR_Name, DIR_Name_LENGTH);
		if (ent->filename) {
			printf(" ( ");
			int idx = 0;
			while (0x0000 != ent->filename[idx]) {
				printf("%c", ent->filename[idx++] >> 8);
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
}

FS_Directory change_dir(FS_Instance * fsi, FS_Directory current_dir, char * path) {

}

void get_file(FS_Instance * fsi, FS_Directory current_dir, char * path, char * local_path) {

}

void put_file(FS_Instance * fsi, FS_Directory current_dir, char * path, char * local_path) {

}

FS_Directory make_dir(FS_Instance * fsi, FS_Directory current_dir, char * path) {

}

FS_Directory delete_file(FS_Instance * fsi, FS_Directory current_dir, char * path) {

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
