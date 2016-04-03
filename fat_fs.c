#include "fat_fs.h"
#include <stdlib.h>
#include <stdio.h>

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
	fsi->dataSec = fsi->numSectors - (fsi->bootsect->BPB_RsvdSecCnt + (fsi->bootsect->BPB_NumFATs * fsi->FATsz) + fsi->rootDirSectors);
	fsi->countOfClusters = fsi->dataSec / fsi->bootsect->BPB_SecPerClus;

	if (fsi->countOfClusters < 4085) {
		fsi->type = FS_FAT12;
	} else if (fsi->countOfClusters < 65525) {
		fsi->type = FS_FAT16;
	} else {
		fsi->type = FS_FAT32;
	}

	return fsi;
}

FS_CurrentDir fs_get_root(FS_Instance * fat_fs) {

}

void loopPrintChar(uint8_t * str, int len) {
	for (int i = 0; i < len; i++) {
		printf("%c", str[i]);
	}
}

void print_info(FS_Instance * fat_fs) {
	fatBS * bs = fat_fs->bootsect;
	printf("\n");
	printf("Disk information:\n-----------------\n");
	printf("OEM Name: ");
	loopPrintChar(&(bs->BS_OEMName[0]), BS_OEMName_LENGTH);
	printf("\n");
	printf("Volume Label: ");
	if (fat_fs->type == FS_FAT32)
		loopPrintChar(&(fat_fs->bootsect32->BS_VolLab[0]), BS_VolLab_LENGTH);
	else
		loopPrintChar(&(fat_fs->bootsect16->BS_VolLab[0]), BS_VolLab_LENGTH);
	printf("\n");
	printf("File System Type (read): ");
	if (fat_fs->type == FS_FAT32)
		loopPrintChar(&(fat_fs->bootsect32->BS_FilSysType[0]), BS_FilSysType_LENGTH);
	else
		loopPrintChar(&(fat_fs->bootsect16->BS_FilSysType[0]), BS_FilSysType_LENGTH);
	printf("\n");
	printf("Media Type: 0x%2X (%sremovable)\n", bs->BPB_Media, (bs->BPB_Media == 0xF8) ? "non-" : "");
	const char * units[] = {"B", "kB", "MB", "GB", "TB"};
	int currUnit = 0;
	long double scaledSz = fat_fs->totalSize;
	while (((long long)(scaledSz / 1024)) > 0) {
		scaledSz /= 1024;
		currUnit++;
	}
	printf("Size: %llu bytes (%.3Lf %s)\n", fat_fs->totalSize, scaledSz, units[currUnit]);
	printf("\n");
	printf("Disk geometry:\n--------------\n");
	printf("Bytes Per Sector: %d\n", bs->BPB_BytsPerSec);
	printf("Sectors Per Cluster: %d\n", bs->BPB_SecPerClus);
	printf("Total Sectors: %d\n", fat_fs->numSectors);
	printf("Physical - Sectors per Track: %d\n", bs->BPB_SecPerTrk);
	printf("Physical - Heads: %d\n", bs->BPB_NumHeads);
	printf("\n");
	printf("File system info:\n-----------------\n");
	printf("Volume ID: ");
	if (fat_fs->type == FS_FAT32)
		printf("%d", fat_fs->bootsect32->BS_VolID);
	else
		printf("%d", fat_fs->bootsect16->BS_VolID);
	printf("\n");
	printf("File System Type (computed): %s\n", typeNames[fat_fs->type]);
	printf("FAT Size (sectors): %d\n", fat_fs->FATsz);
	// read the FAT to figure out how much free space there is
	printf("Free space: %d bytes\n", 0);
	printf("\n");
}

void print_dir(FS_Instance * fat_fs, FS_CurrentDir current_dir) {

}

FS_CurrentDir change_dir(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path) {

}

void get_file(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path, char * local_path) {

}

void put_file(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path, char * local_path) {

}

void make_dir(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path) {

}

void delete_file(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path) {

}

void fs_cleanup(FS_Instance * fat_fs) {
	if (NULL != fat_fs) {
		if (NULL != fat_fs->disk) {
			fclose(fat_fs->disk);
		}
		if (NULL != fat_fs->bootsect) {
			free(fat_fs->bootsect);
		}
		if (NULL != fat_fs->bootsect16) {
			free(fat_fs->bootsect16);
		}
		if (NULL != fat_fs->bootsect32) {
			free(fat_fs->bootsect32);
		}
		free(fat_fs);
	}
}
