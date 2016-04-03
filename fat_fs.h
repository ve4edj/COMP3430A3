#ifndef FAT_FS_H
#define FAT_FS_H

#include <inttypes.h>
#include <stdio.h>
#include "fat.h"

extern const char * typeNames[];

typedef enum {
	FS_FAT12 = 0,
	FS_FAT16 = 1,
	FS_FAT32 = 2
} fs_type;

struct FS_Instance_struct {
	FILE * disk;
	fs_type type;
	fatBS * bootsect;
	fatBS16 * bootsect16;
	fatBS32 * bootsect32;
	uint32_t FATsz;
	uint32_t numSectors;
	uint64_t totalSize;
	uint64_t rootDirSectors;
	uint64_t dataSec;
	uint64_t countOfClusters;
};

typedef struct FS_Instance_struct FS_Instance;
typedef uint32_t FS_CurrentDir;

FS_Instance * fs_create_instance(char * image_path);
FS_CurrentDir fs_get_root(FS_Instance * fsi);

void print_info(FS_Instance * fsi);
void print_dir(FS_Instance * fsi, FS_CurrentDir current_dir);
FS_CurrentDir change_dir(FS_Instance * fsi, FS_CurrentDir current_dir, char * path);
void get_file(FS_Instance * fsi, FS_CurrentDir current_dir, char * path, char * local_path);
void put_file(FS_Instance * fsi, FS_CurrentDir current_dir, char * path, char * local_path);
FS_CurrentDir make_dir(FS_Instance * fsi, FS_CurrentDir current_dir, char * path);
FS_CurrentDir delete_file(FS_Instance * fsi, FS_CurrentDir current_dir, char * path);

void fs_cleanup(FS_Instance * fsi);

#endif
