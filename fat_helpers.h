#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fat_fs.h"
#include "fat.h"

uint64_t getFirstSectorOfCluster(FS_Cluster cluster, FS_Instance * fsi);
FS_FATEntry getFATEntryForCluster(FS_Cluster cluster, FS_Instance * fsi);
uint8_t isFATEntryEOF(FS_FATEntry entry, FS_Instance * fsi);
uint8_t isFATEntryBad(FS_FATEntry entry, FS_Instance * fsi);
FS_EntryList * getDirListing(FS_Cluster dir, FS_Instance * fsi);

uint8_t maskAndTest(uint8_t val, uint8_t mask);