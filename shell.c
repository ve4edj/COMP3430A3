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

int main(int argc, char *argv[]) {
	int done = 0, valid_cmd;
	FS_Instance *fat_fs;
	FS_Directory current_dir;
	char buffer[BUF_SIZE];
	char buffer_raw[BUF_SIZE];
	char *arg1, *arg2;

	if (2 != argc) {
		fprintf(stderr, "Usage: %s fatimage\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	fat_fs = fs_create_instance(argv[1]);
	if (NULL == fat_fs) {
		fprintf(stderr, "Invalid FAT12 image %s.\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	current_dir = fs_get_root(fat_fs);

	while (!done) {
		printf("> ");

		if (NULL == fgets(buffer_raw, BUF_SIZE, stdin)) {
			done = 1;
		} else {
			valid_cmd = 1;
			buffer_raw[strlen(buffer_raw)-1] = '\0'; /* cut new line */
			for (int i=0; i < strlen(buffer_raw)+1; i++)
				buffer[i] = toupper(buffer_raw[i]);
			arg1 = strchr(buffer, ' ');

			if (strncmp(buffer, CMD_EXIT, strlen(CMD_EXIT)) == 0)
				done = 1;
			else if (strncmp(buffer, CMD_INFO, strlen(CMD_INFO)) == 0)
				print_info(fat_fs);
			else if (strncmp(buffer, CMD_DIR, strlen(CMD_DIR)) == 0)
				print_dir(fat_fs, current_dir);
			else if (NULL != arg1) {
				arg2 = strchr(arg1, ' ');
				if (strncmp(buffer, CMD_CD, strlen(CMD_CD)) == 0)
					current_dir = change_dir(fat_fs, current_dir, arg1+1);
				else if (strncmp(buffer, CMD_MD, strlen(CMD_MD)) == 0)
					current_dir = make_dir(fat_fs, current_dir, arg1+1);
				else if (strncmp(buffer, CMD_DEL, strlen(CMD_DEL)) == 0)
					current_dir = delete_file(fat_fs, current_dir, arg1+1);
				else if (NULL != arg2) {
					*arg2 = '\0';
					if (strncmp(buffer, CMD_GET, strlen(CMD_GET)) == 0)
						get_file(fat_fs, current_dir, arg1+1, arg2+1);
					else if (strncmp(buffer, CMD_PUT, strlen(CMD_PUT)) == 0)
						put_file(fat_fs, current_dir, arg1+1, arg2+1);
					else
						valid_cmd = 0;
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
