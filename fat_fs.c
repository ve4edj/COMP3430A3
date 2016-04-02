#include "fat_fs.h"
#include <stdlib.h>
#include <stdio.h>

FS_Instance * fs_create_instance(char * image_path) {
	FS_Instance * fsi = malloc(sizeof(FS_Instance));
	if (NULL == fsi) {
		return NULL;
	}
	fsi->disk = fopen(image_path, "r+");
	if (NULL == fsi->disk) {
		free(fsi);
		return NULL;
	}
	fsi->bootsect = malloc(sizeof(fatBS));
	if (NULL == fsi->bootsect) {
		fclose(fsi->disk);
		free(fsi);
		return NULL;
	}
	fread(fsi->bootsect, sizeof(fatBS), 1, fsi->disk);

	// do moar stuff

	return fsi;
}

FS_CurrentDir fs_get_root(FS_Instance * fat_fs) {

}

void print_info(FS_Instance * fat_fs) {
	printf("Disk information:\n-----------------\n");
	printf("OEM Name: ");
	for (int i = 0; i < BS_OEMName_LENGTH; i++)
		printf("%c", fat_fs->bootsect->BS_OEMName[i]);
	printf("\n");
	printf("Volume Label: ");
	for (int i = 0; i < BS_VolLab_LENGTH; i++)
		printf("%c", fat_fs->bootsect->BS_VolLab[i]);
	printf("\n");

	// print moar stuff
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

}
