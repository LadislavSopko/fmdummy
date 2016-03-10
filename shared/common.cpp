#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "common.h"
#include "sais.h"
#include "../libs/asmlib.h"

namespace fmdummy {

unsigned long long getFileSize(const char *inFileName, int elemSize) {
	FILE *InFile;
	InFile = fopen(inFileName, "rb");
	if (InFile == NULL) {
		cout << "Can't open file " << inFileName << endl;
		exit(1);
	}
	if (fseek(InFile, 0, SEEK_END) != 0) {
		cout << "Something is wrong with file " << inFileName << endl;
		exit(1);
	}
	long long fileSize = ftell(InFile);
	if (fileSize == -1) {
		cout << "Something is wrong with file " << inFileName << endl;
		exit(1);
	}
	fclose(InFile);
	return fileSize / elemSize;
}

FILE *openFile(const char *inFileName, int elemSize, unsigned int &len) {
	FILE *InFile;
	InFile = fopen(inFileName, "rb");
	if (InFile == NULL) {
		cout << "Can't open file " << inFileName << endl;
		exit(1);
	}
	if (fseek(InFile, 0, SEEK_END) != 0) {
		cout << "Something is wrong with file " << inFileName << endl;
		exit(1);
	}
	long long fileSize = ftell(InFile);
	if (fileSize == -1) {
		cout << "Something is wrong with file " << inFileName << endl;
		exit(1);
	}
	if (fileSize == 0) {
		cout << "File " << inFileName << " is empty." << endl;
		exit(1);
	}
	rewind(InFile);
	len = fileSize / elemSize;
	return InFile;
}

unsigned char *readFileChar(const char *inFileName, unsigned int &len, unsigned int addLen) {
	FILE *InFile = openFile(inFileName, 1, len);
	unsigned char *S = new unsigned char[len + addLen];
	if (fread(S, (size_t)1, (size_t)len, InFile) != (size_t)len) {
		cout << "Error reading file " << inFileName << endl;
		exit(1);
	}
	fclose(InFile);
	return S;
}

unsigned int *readFileInt(const char *inFileName, unsigned int &len, unsigned int addLen) {
	FILE *InFile = openFile(inFileName, 4, len);
	unsigned int *S = new unsigned int[len + addLen];
	if (fread(S, (size_t)4, (size_t)len, InFile) != (size_t)len) {
		cout << "Error reading file " << inFileName << endl;
		exit(1);
	}
	fclose(InFile);
	return S;
}

unsigned long long *readFileLong(const char *inFileName, unsigned int &len, unsigned int addLen) {
	FILE *InFile = openFile(inFileName, 8, len);
	unsigned long long *S = new unsigned long long[len + addLen];
	if (fread(S, (size_t)8, (size_t)len, InFile) != (size_t)len) {
		cout << "Error reading file " << inFileName << endl;
		exit(1);
	}
	fclose(InFile);
	return S;
}

bool fileExists(const char *inFileName) {
	FILE *InFile;
	InFile = fopen(inFileName, "rb");
	if (InFile == NULL) {
		return false;
	}
	if (InFile == NULL) {
		return false;
	}
	if (fseek(InFile, 0, SEEK_END) != 0) {
		return false;
	}
	long long fileSize = ftell(InFile);
	if (fileSize == -1) {
		return false;
	}
	if (fileSize == 0) {
		return false;
	}
	fclose(InFile);
	return true;
}

unsigned char *readText(const char *inFileName, unsigned int &textLen, unsigned char eof) {
	unsigned char *S = readFileChar(inFileName, textLen, 1);
	S[textLen] = eof;
	return S;
}

void checkNullChar(unsigned char *text, unsigned int textLen) {
	for (unsigned int i = 0; i < textLen; ++i) {
		if (text[i] == '\0') {
			cout << "Error: text contains at least one 0 character" << endl;
			exit(1);
		}
	}
}

unsigned int *getSA(unsigned char *text, unsigned int textLen, unsigned int &saLen, unsigned int addLen, bool verbose) {
	saLen = textLen + 1;
	if (verbose) cout << "Building SA ... " << flush;
	unsigned int *sa = new unsigned int[saLen + addLen];
	sa[0] = textLen;
	++sa;
	sais(text, (int *)sa, textLen);
	--sa;
	if (verbose) cout << "Done" << endl;
	return sa;
}

unsigned char *getBWT(unsigned char *text, unsigned int textLen, unsigned int *sa, unsigned int saLen, unsigned int &bwtLen, unsigned int addLen, bool verbose) {
	if (verbose) cout << "Building BWT ... " << flush;
	bwtLen = textLen + 1;
	unsigned char *bwt = new unsigned char[bwtLen + 1 + addLen];
	bwt[bwtLen + addLen] = '\0';
	bwt[0] = text[textLen - 1];
	for (unsigned int i = 1; i < saLen; ++i) {
		if (sa[i] == 0) bwt[i] = '\0';
		else bwt[i] = text[sa[i] - 1];
	}
	if (verbose) cout << "Done" << endl;
	return bwt;
}

unsigned char *getBWT(unsigned char *text, unsigned int textLen, unsigned int &bwtLen, unsigned int addLen, bool verbose) {
	unsigned int saLen;
	unsigned int *sa = getSA(text, textLen, saLen, 0, verbose);
	unsigned char *bwt = getBWT(text, textLen, sa, saLen, bwtLen, addLen, verbose);
	delete[] sa;
	return bwt;
}

void fillArrayC(unsigned char *text, unsigned int textLen, unsigned int* C, bool verbose) {
	if (verbose) cout << "Building array C ... " << flush;
	for (int i = 0; i < 257; ++i) C[i] = 0;
	for (unsigned int i = 0; i < textLen; ++i) {
		++C[text[i] + 1];
	}
	C[1] = 1;
	for (int i = 0; i < 256; ++i) {
		C[i + 1] += C[i];
	}
	if (verbose) cout << "Done" << endl;
}

string getStringFromSelectedChars(vector<unsigned char> selectedChars, string separator) {
	if (selectedChars.size() == 0) return "all";
	stringstream ss;
	for (vector<unsigned char>::iterator it = selectedChars.begin(); it != selectedChars.end(); ++it) {
		ss << (unsigned int)(*it);
		if (it + 1 != selectedChars.end()) ss << separator;
	}
	string s = ss.str();
	return s;
}

void binarySearch(unsigned int *sa, unsigned char *text, unsigned int lStart, unsigned int rStart, unsigned char *pattern, int patternLength, unsigned int &beg, unsigned int &end) {
	if (pattern[patternLength - 1] == 255) binarySearchStrncmp(sa, text, lStart, rStart, pattern, patternLength, beg, end);
	else binarySearchAStrcmp(sa, text, lStart, rStart, pattern, patternLength, beg, end);
}

void binarySearchAStrcmp(unsigned int *sa, unsigned char *text, unsigned int lStart, unsigned int rStart, unsigned char *pattern, int patternLength, unsigned int &beg, unsigned int &end) {
	unsigned int l = lStart;
	unsigned int r = rStart;
	unsigned int mid;
	while (l < r) {
		mid = (l + r) / 2;
		if (A_strcmp((const char*)pattern, (const char*)(text + sa[mid])) > 0) {
			l = mid + 1;
		}
		else {
			r = mid;
		}
	}
	beg = l;
	r = rStart;
	++pattern[patternLength - 1];
	while (l < r) {
		mid = (l + r) / 2;
		if (A_strcmp((const char*)pattern, (const char*)(text + sa[mid])) <= 0) {
			r = mid;
		}
		else {
			l = mid + 1;
		}
	}
	--pattern[patternLength - 1];
	end = r;
}

void binarySearchStrncmp(unsigned int *sa, unsigned char *text, unsigned int lStart, unsigned int rStart, unsigned char *pattern, int patternLength, unsigned int &beg, unsigned int &end) {
	unsigned int l = lStart;
	unsigned int r = rStart;
	unsigned int mid;
	while (l < r) {
		mid = (l + r) / 2;
		if (strncmp((const char*)pattern, (const char*)(text + sa[mid]), patternLength) > 0) {
			l = mid + 1;
		}
		else {
			r = mid;
		}
	}
	beg = l;
	r = rStart;
	while (l < r) {
		mid = (l + r) / 2;
		if (strncmp((const char*)pattern, (const char*)(text + sa[mid]), patternLength) < 0) {
			r = mid;
		}
		else {
			l = mid + 1;
		}
	}
	end = r;
}

void fillLUT1(unsigned int lut1[256][2], unsigned char *text, unsigned int *sa, unsigned int saLen) {
	unsigned char lutPattern[2];
	lutPattern[1] = '\0';
	for (int i = 0; i < 256; ++i) {
		lutPattern[0] = (unsigned char)i;
		binarySearch(sa, text, 0, saLen, lutPattern, 1, lut1[i][0], lut1[i][1]);
		++lut1[i][1];
	}
}

void fillLUT2(unsigned int lut2[256][256][2], unsigned char *text, unsigned int *sa, unsigned int saLen) {
	unsigned char lutPattern[3];
	lutPattern[2] = '\0';
	for (int i = 0; i < 256; ++i) {
		lutPattern[0] = (unsigned char)i;
		for (int j = 0; j < 256; ++j) {
			lutPattern[1] = (unsigned char)j;
			binarySearch(sa, text, 0, saLen, lutPattern, 2, lut2[i][j][0], lut2[i][j][1]);
			++lut2[i][j][1];
		}
	}
}

void encode(unsigned char *pattern, unsigned int patternLen, unsigned char *encodedChars, unsigned int *encodedCharsLen, unsigned int maxEncodedCharsLen, unsigned char *encodedPattern, unsigned int &encodedPatternLen) {
	unsigned char* p = pattern;
	encodedPatternLen = 0;
	for (; p < pattern + patternLen; ++p) {
		for (unsigned int i = 0; i < encodedCharsLen[*p]; ++i) encodedPattern[encodedPatternLen++] = encodedChars[(unsigned int)(*p) * maxEncodedCharsLen + i];
	}
	encodedPattern[encodedPatternLen] = '\0';
}

}
