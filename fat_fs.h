#ifndef FAT_FS_H
#define FAT_FS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "fat.h"

extern const char * typeNames[];

typedef enum {
	FS_FAT12 = 0,
	FS_FAT16 = 1,
	FS_FAT32 = 2
} fs_type;

typedef enum {
	ERR_SUCCESS,
	ERR_NOFREESPACE,
	ERR_FILENAMEEXISTS,
	ERR_FILENOTFOUND,
	ERR_FOPENFAILEDREAD,
	ERR_FOPENFAILEDWRITE,
	ERR_DELETESPECIALDIR,
	ERR_MALLOCFAILED,
	ERR_ROOTDIRFULL
} fs_result;

typedef uint32_t FS_Directory;
typedef uint32_t FS_FATEntry;
typedef uint32_t FS_Cluster;

struct FS_Instance_struct {
	FILE * disk;
	fs_type type;
	fatBS * bootsect;
	fatBS16 * bootsect16;
	fatBS32 * bootsect32;
	fat32FSInfo * fsInfo;
	uint32_t FATsz;
	uint32_t numSectors;
	uint64_t totalSize;
	uint64_t rootDirSectors;
	uint64_t dataSec;
	uint64_t countOfClusters;
	FS_Directory rootDirPos;
};

struct FS_Entry_struct {
	uint16_t * filename;
	fatEntry * entry;
};

struct FS_EntryList_struct {
	struct FS_Entry_struct * node;
	struct FS_EntryList_struct * next;
};

typedef struct FS_Instance_struct FS_Instance;
typedef struct FS_Entry_struct FS_Entry;
typedef struct FS_EntryList_struct FS_EntryList;

FS_Instance * fs_create_instance(char * imagePath);
FS_Directory fs_get_root(FS_Instance * fsi);

void print_info(FS_Instance * fsi);
void print_dir(FS_Instance * fsi, FS_Directory currDir);
FS_Directory change_dir(FS_Instance * fsi, FS_Directory currDir, char * path);
fs_result get_file(FS_Instance * fsi, FS_Directory currDir, char * path, char * localPath);
fs_result put_file(FS_Instance * fsi, FS_Directory currDir, char * path, char * localPath);
fs_result make_dir(FS_Instance * fsi, FS_Directory currDir, char * path);
FS_Directory delete_file(FS_Instance * fsi, FS_Directory currDir, char * path);

void fs_cleanup(FS_Instance * fsi);

#endif
