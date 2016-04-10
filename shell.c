#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fat_fs.h"

#define BUF_SIZE 256
#define CMD_INFO "INFO"
#define CMD_DIR "DIR"
#define CMD_CD "CD"
#define CMD_GET "GET"
#define CMD_EXIT "EXIT"
#define CMD_PUT "PUT"
#define CMD_MD "MD"
#define CMD_DEL "DEL"

void printError(fs_result result, char * arg) {
	switch (result) {
		case ERR_SUCCESS:
			break;
		case ERR_NOFREESPACE:
			printf("Error: Couldn't create file/directory %s, insufficient space on disk\n", arg);
			break;
		case ERR_FILENAMEEXISTS:
			printf("Error: Couldn't create file/directory %s, duplicate filename\n", arg);
			break;
		case ERR_FILENOTFOUND:
			printf("Error: File %s not found on disk\n", arg);
			break;
		case ERR_FOPENFAILEDREAD:
			printf("Error: Couldn't open local file for reading\n");
			break;
		case ERR_FOPENFAILEDWRITE:
			printf("Error: Couldn't open local file for writing\n");
			break;
		case ERR_DELETESPECIALDIR:
			printf("Error: Cannot delete '.' or '..' entries in a directory\n");
			break;
		case ERR_MALLOCFAILED:
			printf("Error: Failed to allocate sufficient scratchpad RAM\n");
			break;
		case ERR_ROOTDIRFULL:
			printf("Error: Insufficient free entries available in the root directory\n");
			break;
	}
}

int main(int argc, char *argv[]) {
	int done = 0, valid_cmd;
	FS_Instance *fat_fs;
	FS_Directory current_dir;
	char buffer[BUF_SIZE];
	char *arg1, *arg2;

	if (2 != argc) {
		fprintf(stderr, "Usage: %s fatimage\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	fat_fs = fs_create_instance(argv[1]);
	if (NULL == fat_fs) {
		fprintf(stderr, "Invalid FAT image %s.\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	current_dir = fs_get_root(fat_fs);
	printf("\nWelcome to FATshell!\n%s image %s was loaded successfully!\n\n", typeNames[fat_fs->type], argv[1]);
	printf("+-------------------------------------------+\n");
	printf("|                 Commands:                 |\n");
	printf("+-------------------------------------------+\n");
	printf("| EXIT: quit FATshell                       |\n");
	printf("| INFO: display filesystem information      |\n");
	printf("| DIR:  list contents of current directory  |\n");
	printf("| CD:   change directory (multiple levels   |\n");
	printf("|          supported, e.g. '../..')         |\n");
	printf("| GET:  retrieve a file from the image      |\n");
	printf("+-------------------------------------------+\n");
	printf("|                 Features:                 |\n");
	printf("+-------------------------------------------+\n");
	printf("|       === FAT32 formatted disks ===       |\n");
	printf("|       === Long filename entries ===       |\n");
	printf("+-------------------------------------------+\n");
	printf("|                   Note:                   |\n");
	printf("+-------------------------------------------+\n");
	printf("|  The GET and CD commands require that the |\n");
	printf("|  short name of the item be passed as the  |\n");
	printf("|  argument and are case-sensitive.         |\n");
	printf("+-------------------------------------------+\n");

	while (!done) {
		printf("> ");

		if (NULL == fgets(buffer, BUF_SIZE, stdin)) {
			done = 1;
		} else {
			valid_cmd = 1;
			buffer[strlen(buffer)-1] = '\0'; /* cut new line */
			arg1 = strchr(buffer, ' ');

			if (strncasecmp(buffer, CMD_EXIT, strlen(CMD_EXIT)) == 0)
				done = 1;
			else if (strncasecmp(buffer, CMD_INFO, strlen(CMD_INFO)) == 0)
				print_info(fat_fs);
			else if (strncasecmp(buffer, CMD_DIR, strlen(CMD_DIR)) == 0)
				print_dir(fat_fs, current_dir);
			else if (NULL != arg1) {
				arg2 = strchr(arg1+1, ' ');
				if (strncasecmp(buffer, CMD_CD, strlen(CMD_CD)) == 0) {
					FS_Directory temp_dir = change_dir(fat_fs, current_dir, arg1+1);
					if (temp_dir == 0x00000001)
						printf("Directory '%s' not found\n", arg1+1);
					else
						current_dir = temp_dir;
				}
				else if (strncasecmp(buffer, CMD_MD, strlen(CMD_MD)) == 0) {
					fs_result result = make_dir(fat_fs, current_dir, arg1+1);
					printError(result, arg1+1);
				}
				else if (strncasecmp(buffer, CMD_DEL, strlen(CMD_DEL)) == 0)
					current_dir = delete_file(fat_fs, current_dir, arg1+1);
				else if (NULL != arg2) {
					*arg2 = '\0';
					if (strncasecmp(buffer, CMD_GET, strlen(CMD_GET)) == 0) {
						fs_result result = get_file(fat_fs, current_dir, arg1+1, arg2+1);
						printError(result, arg1+1);
					} else if (strncasecmp(buffer, CMD_PUT, strlen(CMD_PUT)) == 0) {
						fs_result result = put_file(fat_fs, current_dir, arg1+1, arg2+1);
						printError(result, arg1+1);
					} else {
						valid_cmd = 0;
					}
				} else {
					valid_cmd = 0;
				}
			} else {
				valid_cmd = 0;
			}
			if (!valid_cmd && '\0' != buffer[0]) {
				printf("\nUnknown command %s.\n", buffer);
			}
		}
	}

	printf("\nExiting...\n");
	fs_cleanup(fat_fs);
	return EXIT_SUCCESS;
}
