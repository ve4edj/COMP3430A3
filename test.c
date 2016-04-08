#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

void testFilename(char * filename) {
	printf("Filename %s, LFN entires %d, String len: %d\n", filename, getNumberOfLongEntriesForFilename(filename), strlen(filename));
}

int main(int argc, char ** argv) {
	testFilename("TEST.TXT");
	testFilename("12345678.910");
	testFilename("1234567.890");
	testFilename("Hello.c");
	testFilename("VERYLONGF.ILE");
	testFilename("this2longnames");
	testFilename("verylongfile.name");
	testFilename("TEST.1234");
	testFilename("TEXT.TMP.TXT");
	testFilename("TEXTTMP.TXT");
	testFilename("foobarfoobarfoo.bar");
}