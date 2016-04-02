#ifndef FAT_FS_H
#define FAT_FS_H

#include <inttypes.h>
#include <stdio.h>
#include "fat.h"

struct FS_Instance_struct {
	FILE * disk;
	fatBS * bootsect;

};

typedef struct FS_Instance_struct FS_Instance;
typedef uint32_t FS_CurrentDir;

FS_Instance * fs_create_instance(char * image_path);
FS_CurrentDir fs_get_root(FS_Instance * fat_fs);

void print_info(FS_Instance * fat_fs);
void print_dir(FS_Instance * fat_fs, FS_CurrentDir current_dir);
FS_CurrentDir change_dir(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path);
void get_file(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path, char * local_path);

// These are for the bonus questions
void put_file(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path, char * local_path);
void make_dir(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path);
void delete_file(FS_Instance * fat_fs, FS_CurrentDir current_dir, char * path);

void fs_cleanup(FS_Instance *fat_fs);

#endif
