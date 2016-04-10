#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define DIR_Name_LENGTH 11
#define LDIR_Name1_LENGTH 10
#define LDIR_Name2_LENGTH 12
#define LDIR_Name3_LENGTH 4
#define LDIR_LettersPerEntry ((LDIR_Name1_LENGTH + LDIR_Name2_LENGTH + LDIR_Name3_LENGTH) / 2)

uint8_t isValidFilenameChar(char c, uint8_t isLongFilename) {
	if (0x20 > c && 0x00 != c)
		return 0;
	if ('A' <= c && 'Z' >= c)
		return 1;
	if ('0' <= c && '9' >= c)
		return 1;
	if (0x7F < c)
		return 1;
	switch (c) {
		case ' ':
		case '.':
		case '$':
		case '%':
		case '-':
		case '_':
		case '@':
		case '~':
		case '`':
		case '!':
		case '(':
		case ')':
		case '{':
		case '}':
		case '^':
		case '#':
		case '&':
		case '\'':
			return 1;
		default:
			if (isLongFilename) {
				if ('a' <= c && 'z' >= c)
					return 1;
				switch (c) {
					case '+':
					case ',':
					case ';':
					case '=':
					case '[':
					case ']':
					case '\0':
						return 1;
					default:
						return 0;
				}
			}
			return 0;
	}
}

uint8_t getNumberOfLongEntriesForFilename(char * filename) {
	uint8_t validShortName = 1, isExtensionPart = 0;
	for (int i = 0; i < strlen(filename); i++) {
		validShortName &= isValidFilenameChar(filename[i], 0);
		if (isExtensionPart > 3)
			validShortName = 0;
		if (isExtensionPart)
			isExtensionPart++;
		if ('.' == filename[i]) {
			if (isExtensionPart)
				validShortName = 0;
			else
				isExtensionPart = 1;
		}
		if ((i == 8) && !isExtensionPart)
			validShortName = 0;
		if (i > DIR_Name_LENGTH)
			validShortName = 0;
		if (!validShortName)
			break;
	}
	if (validShortName)
		return 0;
	return ((strlen(filename) - 1) / LDIR_LettersPerEntry) + 1;
}

void setNumericTail(char * shortName, uint32_t tailVal) {
	uint8_t tailPos = 7;
	while (tailVal) {
		shortName[tailPos--] = (tailVal % 10) + '0';
		tailVal /= 10;
	}
	if (7 > tailPos)
		shortName[tailPos] = '~';
}

char * genShortName(char * longName, uint32_t currTail) {
	char * name = strdup(longName);
	char * shortName = malloc(sizeof(char) * (DIR_Name_LENGTH + 1));
	shortName[DIR_Name_LENGTH] = '\0';

	char * extension = strrchr(name, '.');
	if (NULL != extension) {
		*extension = '\0';
		extension++;
	}
	uint8_t wasLossy = 0;
	uint8_t j = 0;
	for (uint8_t i = 0; i < strlen(name); i++) {
		if (8 == j) {
			wasLossy = 1;
			break;
		}
		char c = toupper(name[i]);
		if (' ' == c)
			continue;
		if ('.' == c)
			continue;
		if (!isValidFilenameChar(c, 0))
			shortName[j++] = '_';
		else
			shortName[j++] = c;
	}
	while (8 > j) { shortName[j++] = ' '; }
	if (wasLossy) {
		setNumericTail(shortName, ++currTail);
	}
	if (NULL != extension) {
		for (uint8_t i = 0; i < strlen(extension); i++) {
			if (DIR_Name_LENGTH == j)
				break;
			char c = toupper(extension[i]);
			if (' ' == c)
				continue;
			if ('.' == c)
				continue;
			if (!isValidFilenameChar(c, 0))
				shortName[j++] = '_';
			else
				shortName[j++] = c;
		}
	}
	while (DIR_Name_LENGTH > j) { shortName[j++] = ' '; }
	free(name);

	return shortName;
}

void testFilename(char * filename, uint32_t tail) {
	char * sn = genShortName(filename, tail);
	printf("Filename %s, LFN entires %d, String len: %d, Short Name: %s\n", filename, getNumberOfLongEntriesForFilename(filename), strlen(filename), sn);
	free(sn);
}

int main(int argc, char ** argv) {
	testFilename("TEST.TXT", 0);
	testFilename("12345678.910", 0);
	testFilename("1234567.890", 0);
	testFilename("Hello.c", 0);
	testFilename("VERYLONGF.ILE", 0);
	testFilename("this2longnames", 986);
	testFilename("verylongfile.name", 1233);
	testFilename("TEST.1234", 0);
	testFilename("TEXT.TMP.TXT", 0);
	testFilename("TEXTTMP.TXT", 0);
	testFilename("foobarfoobarfoo.bar", 0);
}