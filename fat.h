#ifndef FAT_H
#define FAT_H

#include <inttypes.h>

/* All values as stored on disk are little-endian */

/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8

/* directory entry constants */
#define DIR_Name_LENGTH 11
#define LDIR_Name1_LENGTH 5
#define LDIR_Name2_LENGTH 6
#define LDIR_Name3_LENGTH 2
#define LDIR_LettersPerEntry (LDIR_Name1_LENGTH + LDIR_Name2_LENGTH + LDIR_Name3_LENGTH)

#pragma pack(push)
#pragma pack(1)
struct fatBS_struct {
	uint8_t BS_jmpBoot[3];
	uint8_t BS_OEMName[BS_OEMName_LENGTH];
	uint16_t BPB_BytsPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;
};

struct fatBS16_struct {
	uint8_t BPB_DrvNum;
	uint8_t BPB_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	uint8_t BS_VolLab[BS_VolLab_LENGTH];
	uint8_t BS_FilSysType[BS_FilSysType_LENGTH];
	uint8_t BS_CodeReserved[448];
	uint8_t BS_SigA;
	uint8_t BS_SigB;
};

struct fatBS32_struct {
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint8_t BPB_FSVerLow;
	uint8_t BPB_FSVerHigh;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	uint8_t BPB_reserved[12];
	uint8_t BPB_DrvNum;
	uint8_t BPB_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	uint8_t BS_VolLab[BS_VolLab_LENGTH];
	uint8_t BS_FilSysType[BS_FilSysType_LENGTH];
	uint8_t BS_CodeReserved[420];
	uint8_t BS_SigA;
	uint8_t BS_SigB;
};

struct fatDate_struct {
	uint16_t day    : 5;
	uint16_t month  : 4;
	uint16_t year   : 7;
};

struct fatTime_struct {
	uint16_t sec  : 5;
	uint16_t min  : 6;
	uint16_t hour : 5;
};

struct fatEntry_struct {
	uint8_t DIR_Name[DIR_Name_LENGTH];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t DIR_CrtTimeTenth;
	struct fatTime_struct DIR_CrtTime;
	struct fatDate_struct DIR_CrtDate;
	struct fatDate_struct DIR_LstAccDate;
	uint16_t DIR_FstClusHI;
	struct fatTime_struct DIR_WrtTime;
	struct fatDate_struct DIR_WrtDate;
	uint16_t DIR_FstClusLO;
	uint32_t DIR_FileSize;
};

struct fatLongName_struct {
	uint8_t LDIR_Ord;
	uint16_t LDIR_Name1[LDIR_Name1_LENGTH];
	uint8_t LDIR_Attr;
	uint8_t LDIR_Type;
	uint8_t LDIR_Chksum;
	uint16_t LDIR_Name2[LDIR_Name2_LENGTH];
	uint16_t LDIR_Zero;
	uint16_t LDIR_Name3[LDIR_Name3_LENGTH];
};
#pragma pack(pop)

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

#define LAST_LONG_ENTRY 0x40

typedef struct fatBS_struct fatBS;
typedef struct fatBS16_struct fatBS16;
typedef struct fatBS32_struct fatBS32;
typedef struct fatDate_struct fatDate;
typedef struct fatTime_struct fatTime;
typedef struct fatEntry_struct fatEntry;
typedef struct fatLongName_struct fatLongName;

#endif
