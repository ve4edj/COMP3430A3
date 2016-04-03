#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "fat_fs.h"
#include "fat.h"

uint64_t getFirstSectorOfCluster(uint64_t cluster, FS_Instance * fsi);
FS_FATEntry getFATEntryForCluster(uint64_t cluster, FS_Instance * fsi);
uint8_t isFATEntryEOF(FS_FATEntry entry, FS_Instance * fsi);
uint8_t isFATEntryBad(FS_FATEntry entry, FS_Instance * fsi);