#ifndef FMDUMMY_HPP
#define	FMDUMMY_HPP

#include "shared/common.h"
#include "shared/wt.h"
#include "shared/hash.hpp"
#include "shared/huff.h"
#include <cstdio>
#include <vector>
#include <algorithm>

using namespace std;
using namespace shared;

namespace fmdummy {
    
unsigned char *getBinDenseForChar(unsigned char *bwt, unsigned int bwtLen, int ordChar);
void buildRank_256_counter48(unsigned long long **bwtInLong, unsigned int bwtInLongLen, vector<unsigned char> selectedChars, unsigned long long **bwtWithRanks, unsigned int &bwtWithRanksLen, unsigned long long **alignedBWTWithRanks);
void buildRank_512_counter40(unsigned long long **bwtInLong, unsigned int bwtInLongLen, vector<unsigned char> selectedChars, unsigned long long **bwtWithRanks, unsigned int &bwtWithRanksLen, unsigned long long **alignedBWTWithRanks);
unsigned int count_256_counter48(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal);
void getCountBoundaries_256_counter48(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal, unsigned int &leftBoundary, unsigned int &rightBoundary);
unsigned int count_512_counter40(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal);
void getCountBoundaries_512_counter40(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal, unsigned int &leftBoundary, unsigned int &rightBoundary);
unsigned char *encode125(unsigned char* text, unsigned int textLen, vector<unsigned char> selectedChars, unsigned int &encodedTextLen);
void fill125LUT(vector<unsigned char> selectedChars, unsigned int lut[][125]);
unsigned int count_512_enc125(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned char *bwtWithRanks, unsigned int lut[][125], unsigned int firstVal, unsigned int lastVal);
unsigned int count_1024_enc125(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned char *bwtWithRanks, unsigned int lut[][125], unsigned int firstVal, unsigned int lastVal);
bool sortCharsCount(unsigned int* i, unsigned int* j);

    
enum FMDummy1Type {
        FMD1_256 = 1,
        FMD1_512 = 2
};

template<FMDummy1Type T> class FMDummy1 : public Index {
protected:
	alignas(128) unsigned long long *bwtWithRanks[256];
	alignas(128) unsigned long long *alignedBWTWithRanks[256];
	unsigned int bwtWithRanksLen;
	alignas(128) unsigned int c[257];
	vector<unsigned char> selectedChars;
	bool allChars;
	unsigned int textLen;

	void freeMemory() {
            for (int i = 0; i < 256; ++i) if (this->bwtWithRanks[i] != NULL) delete[] this->bwtWithRanks[i];
        }
        
	void initialize() {
            for (int i = 0; i < 256; ++i) {
                this->bwtWithRanks[i] = NULL;
                this->alignedBWTWithRanks[i] = NULL;
            }
            this->bwtWithRanksLen = 0;
            for (int i = 0; i < 257; ++i) this->c[i] = 0;

            this->textLen = 0;
        }
        
	void setSelectedChars(vector<unsigned char> selectedChars) {
            if (selectedChars.size() > 16) {
                cout << "Error: not valid number of selected chars" << endl;
                exit(1);
            }
            this->selectedChars = selectedChars;
            if (selectedChars.size() == 0) this->allChars = true;
            else this->allChars = false;
        }

public:
	enum IndexType {
		TYPE_256 = 1,
		TYPE_512 = 2
	};
	FMDummy1() {
		this->initialize();
		this->setSelectedChars({});
	}

	FMDummy1(vector<unsigned char> selectedChars) {
		this->initialize();
		this->setSelectedChars(selectedChars);
	}

	~FMDummy1() {
		this->free();
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            if (this->allChars) {
                    if (this->verbose) cout << "Counting char frequencies ... " << flush;
                    unsigned int charsFreq[256];
                    for (unsigned int i = 0; i < 256; ++i) charsFreq[i] = 0;
                    unsigned int selectedCharsLen = 0;
                    for (unsigned int i = 0; i < this->textLen; ++i) {
                            if (charsFreq[(unsigned int)text[i]] == 0) ++selectedCharsLen;
                            ++charsFreq[(unsigned int)text[i]];
                    }
                    if (this->verbose) cout << "Done" << endl;
                    if (selectedCharsLen > 16) {
                            cout << "Error building index: text cannot contain more than 16 unique symbols" << endl;
                            exit(1);
                    }
                    this->selectedChars = {};
                    for (unsigned int i = 0; i < 256; ++i) if (charsFreq[i] > 0) this->selectedChars.push_back(i);
            }
            unsigned int bwtLen;
            unsigned char *bwt = getBWT(textFileName, text, this->textLen, bwtLen, 0, this->verbose);
            if (this->verbose) cout << "Compacting BWT for selected chars ... " << flush;
            ++bwtLen;
            unsigned int bwtDenseLen = (bwtLen / 8);
            if (bwtLen % 8 > 0) ++bwtDenseLen;
            unsigned int bwtDenseInLongLen = bwtDenseLen / sizeof(unsigned long long);
            if (bwtDenseLen % sizeof(unsigned long long) > 0) ++bwtDenseInLongLen;
            unsigned long long *bwtDenseInLong[256];
            for (vector<unsigned char>::iterator it = selectedChars.begin(); it != selectedChars.end(); ++it) {
                    unsigned int selectedChar = (*it);
                    unsigned char *bwtDense = getBinDenseForChar(bwt, bwtLen, selectedChar);
                    bwtDenseInLong[selectedChar] = new unsigned long long[bwtDenseInLongLen + 8];
                    for (unsigned long long j = 0; j < bwtDenseInLongLen; ++j) {
                            bwtDenseInLong[selectedChar][j] = ((unsigned long long)bwtDense[8 * j + 7] << 56) | ((unsigned long long)bwtDense[8 * j + 6] << 48) | ((unsigned long long)bwtDense[8 * j + 5] << 40) | ((unsigned long long)bwtDense[8 * j + 4] << 32) | ((unsigned long long)bwtDense[8 * j + 3] << 24) | ((unsigned long long)bwtDense[8 * j + 2] << 16) | ((unsigned long long)bwtDense[8 * j + 1] << 8) | (unsigned long long)bwtDense[8 * j];
                    }
                    for (unsigned long long j = bwtDenseInLongLen; j < bwtDenseInLongLen + 8; ++j) {
                            bwtDenseInLong[selectedChar][j] = 0ULL;
                    }
                    delete[] bwtDense;
            }
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;

            fillArrayC(text, this->textLen, this->c, this->verbose);

            delete[] text;

            if (this->verbose) cout << "Interweaving BWT with ranks ... " << flush;
            switch(T) {
                case FMDummy1Type::FMD1_512:
                    buildRank_512_counter40(bwtDenseInLong, bwtDenseInLongLen, this->selectedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
                default:
                    buildRank_256_counter48(bwtDenseInLong, bwtDenseInLongLen, this->selectedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
            }
            if (this->verbose) cout << "Done" << endl;

            for (vector<unsigned char>::iterator it = selectedChars.begin(); it != selectedChars.end(); ++it) delete[] bwtDenseInLong[*it];
            if (this->verbose) cout << "Index successfully built" << endl;
        }
        
	void save(const char *fileName) {
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            unsigned int selectedCharsLen = this->selectedChars.size();
            fwrite(&selectedCharsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->selectedChars.size() > 0) {
                    for (vector<unsigned char>::iterator it = selectedChars.begin(); it != selectedChars.end(); ++it) {
                            fwrite(&(*it), (size_t)sizeof(unsigned char), (size_t)1, outFile);
                            fwrite(this->alignedBWTWithRanks[*it], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, outFile);
                    }
            }
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void load(const char *fileName) {
            this->free();
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            unsigned int selectedCharsLen;
            result = fread(&selectedCharsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            this->selectedChars = {};
            if (selectedCharsLen > 0) {
                    for (unsigned int i = 0; i < selectedCharsLen; ++i) {
                            unsigned char c;
                            result = fread(&c, (size_t)sizeof(unsigned char), (size_t)1, inFile);
                            if (result != 1) {
                                    cout << "Error loading index from " << fileName << endl;
                                    exit(1);
                            }
                            this->selectedChars.push_back(c);
                            this->bwtWithRanks[c] = new unsigned long long[this->bwtWithRanksLen + 16];
                            this->alignedBWTWithRanks[c] = this->bwtWithRanks[c];
                            while ((unsigned long long)(this->alignedBWTWithRanks[c]) % 128) ++(this->alignedBWTWithRanks[c]);
                            result = fread(this->alignedBWTWithRanks[c], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, inFile);
                            if (result != this->bwtWithRanksLen) {
                                    cout << "Error loading index from " << fileName << endl;
                                    exit(1);
                            }
                    }
                    this->allChars = false;
            } else this->allChars = true;
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
        
	unsigned int getIndexSize() {
            unsigned int size = sizeof(this->bwtWithRanksLen) + sizeof(this->allChars) + this->selectedChars.size();
            size += (257 * sizeof(unsigned int) + 256 * sizeof(unsigned long long*) + 256 * sizeof(unsigned long long*) + this->selectedChars.size() * sizeof(unsigned char));
            if (this->bwtWithRanksLen > 0) size += (this->selectedChars.size() * (this->bwtWithRanksLen + 16) * sizeof(unsigned long long));
            return size;
        }
        
	unsigned int getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }

	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            switch(T) {
                case FMDummy1Type::FMD1_512:
                    return count_512_counter40(pattern, patternLen - 1, this->c, this->alignedBWTWithRanks, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1]);
                    break;
                default:
                    return count_256_counter48(pattern, patternLen - 1, this->c, this->alignedBWTWithRanks, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1]);
                    break;
            }
        }
};

template<FMDummy1Type T> class FMDummy1Hash : public FMDummy1<T> {
private:
	HTExt<HTType::STANDARD> *ht = NULL;

	void freeMemory() {
            FMDummy1<T>::freeMemory();
            if (this->ht != NULL) this->ht->free();
        }

public:
        FMDummy1Hash(unsigned int k, double loadFactor) {
		this->initialize();
		this->setSelectedChars({});
                this->ht = new HTExt<HTType::STANDARD>(k, loadFactor);
	}
        
	FMDummy1Hash(vector<unsigned char> selectedChars, unsigned int k, double loadFactor) {
		this->initialize();
		this->setSelectedChars(selectedChars);
                this->ht = new HTExt<HTType::STANDARD>(k, loadFactor);
	}

	~FMDummy1Hash() {
		this->free();
                if (this->ht != NULL) delete this->ht;
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            if (this->allChars) {
                    if (this->verbose) cout << "Counting char frequencies ... " << flush;
                    unsigned int charsFreq[256];
                    for (unsigned int i = 0; i < 256; ++i) charsFreq[i] = 0;
                    unsigned int selectedCharsLen = 0;
                    for (unsigned int i = 0; i < this->textLen; ++i) {
                            if (charsFreq[(unsigned int)text[i]] == 0) ++selectedCharsLen;
                            ++charsFreq[(unsigned int)text[i]];
                    }
                    if (this->verbose) cout << "Done" << endl;
                    if (selectedCharsLen > 16) {
                            cout << "Error building index: text cannot contain more than 16 unique symbols" << endl;
                            exit(1);
                    }
                    this->selectedChars = {};
                    for (unsigned int i = 0; i < 256; ++i) if (charsFreq[i] > 0) this->selectedChars.push_back(i);
            }
            unsigned int bwtLen;
            unsigned int saLen;
            unsigned int *sa = getSA(textFileName, text, this->textLen, saLen, 0, this->verbose);
            if (this->verbose) cout << "Building hash table ... " << flush;
            if (this->allChars) this->ht->build(text, this->textLen, sa, saLen);
            else this->ht->build(text, this->textLen, sa, saLen, this->selectedChars);
            if (this->verbose) cout << "Done" << endl;
            unsigned char *bwt = getBWT(text, this->textLen, sa, saLen, bwtLen, 0, this->verbose);
            delete[] sa;
            if (this->verbose) cout << "Compacting BWT for selected chars ... " << flush;
            ++bwtLen;
            unsigned int bwtDenseLen = (bwtLen / 8);
            if (bwtLen % 8 > 0) ++bwtDenseLen;
            unsigned int bwtDenseInLongLen = bwtDenseLen / sizeof(unsigned long long);
            if (bwtDenseLen % sizeof(unsigned long long) > 0) ++bwtDenseInLongLen;
            unsigned long long *bwtDenseInLong[256];
            for (vector<unsigned char>::iterator it = this->selectedChars.begin(); it != this->selectedChars.end(); ++it) {
                    unsigned int selectedChar = (*it);
                    unsigned char *bwtDense = getBinDenseForChar(bwt, bwtLen, selectedChar);
                    bwtDenseInLong[selectedChar] = new unsigned long long[bwtDenseInLongLen + 8];
                    for (unsigned long long j = 0; j < bwtDenseInLongLen; ++j) {
                            bwtDenseInLong[selectedChar][j] = ((unsigned long long)bwtDense[8 * j + 7] << 56) | ((unsigned long long)bwtDense[8 * j + 6] << 48) | ((unsigned long long)bwtDense[8 * j + 5] << 40) | ((unsigned long long)bwtDense[8 * j + 4] << 32) | ((unsigned long long)bwtDense[8 * j + 3] << 24) | ((unsigned long long)bwtDense[8 * j + 2] << 16) | ((unsigned long long)bwtDense[8 * j + 1] << 8) | (unsigned long long)bwtDense[8 * j];
                    }
                    for (unsigned long long j = bwtDenseInLongLen; j < bwtDenseInLongLen + 8; ++j) {
                            bwtDenseInLong[selectedChar][j] = 0ULL;
                    }
                    delete[] bwtDense;
            }
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;

            fillArrayC(text, this->textLen, this->c, this->verbose);

            delete[] text;

            if (this->verbose) cout << "Interweaving BWT with ranks ... " << flush;
            switch(T) {
                case FMDummy1Type::FMD1_512:
                    buildRank_512_counter40(bwtDenseInLong, bwtDenseInLongLen, this->selectedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
                default:
                    buildRank_256_counter48(bwtDenseInLong, bwtDenseInLongLen, this->selectedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
            }
            if (this->verbose) cout << "Done" << endl;

            for (vector<unsigned char>::iterator it = this->selectedChars.begin(); it != this->selectedChars.end(); ++it) delete[] bwtDenseInLong[*it];
            if (this->verbose) cout << "Index successfully built" << endl;
        }
        
	void save(const char *fileName) {
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            unsigned int selectedCharsLen = this->selectedChars.size();
            fwrite(&selectedCharsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->selectedChars.size() > 0) {
                    for (vector<unsigned char>::iterator it = this->selectedChars.begin(); it != this->selectedChars.end(); ++it) {
                            fwrite(&(*it), (size_t)sizeof(unsigned char), (size_t)1, outFile);
                            fwrite(this->alignedBWTWithRanks[*it], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, outFile);
                    }
            }
            this->ht->save(outFile);
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void load(const char *fileName) {
            this->free();
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            unsigned int selectedCharsLen;
            result = fread(&selectedCharsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            this->selectedChars = {};
            if (selectedCharsLen > 0) {
                    for (unsigned int i = 0; i < selectedCharsLen; ++i) {
                            unsigned char c;
                            result = fread(&c, (size_t)sizeof(unsigned char), (size_t)1, inFile);
                            if (result != 1) {
                                    cout << "Error loading index from " << fileName << endl;
                                    exit(1);
                            }
                            this->selectedChars.push_back(c);
                            this->bwtWithRanks[c] = new unsigned long long[this->bwtWithRanksLen + 16];
                            this->alignedBWTWithRanks[c] = this->bwtWithRanks[c];
                            while ((unsigned long long)(this->alignedBWTWithRanks[c]) % 128) ++(this->alignedBWTWithRanks[c]);
                            result = fread(this->alignedBWTWithRanks[c], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, inFile);
                            if (result != this->bwtWithRanksLen) {
                                    cout << "Error loading index from " << fileName << endl;
                                    exit(1);
                            }
                    }
                    this->allChars = false;
            } else this->allChars = true;
            delete this->ht;
            this->ht = new HTExt<HTType::STANDARD>();
            this->ht->load(inFile);
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
        
	unsigned int getIndexSize() {
            return FMDummy1<T>::getIndexSize() + sizeof(this->ht) + this->ht->getHTSize();
        }
        
	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            if (patternLen < this->ht->k) return FMDummy1<T>::count(pattern, patternLen);
            unsigned int leftBoundary, rightBoundary;
            this->ht->getBoundaries(pattern + (patternLen - this->ht->k), leftBoundary, rightBoundary);
            switch(T) {
                case FMDummy1Type::FMD1_512:
                    return count_512_counter40(pattern, patternLen - this->ht->k, this->c, this->alignedBWTWithRanks, leftBoundary + 1, rightBoundary);
                    break;
                default:
                    return count_256_counter48(pattern, patternLen - this->ht->k, this->c, this->alignedBWTWithRanks, leftBoundary + 1, rightBoundary);
                    break;
            }
        }
};

enum FMDummy2Type {
        FMD2_256 = 1,
        FMD2_512 = 2
};

enum FMDummy2BPC {
        BITS_3 = 3,
        BITS_4 = 4
};

enum FMDummy2Schema {
        SCHEMA_SCBO = 1,
        SCHEMA_CB = 2
};


template<FMDummy2Type T, FMDummy2Schema S, FMDummy2BPC BPC> class FMDummy2 : public Index {
protected:
	alignas(128) unsigned long long *bwtWithRanks[256];
	alignas(128) unsigned long long *alignedBWTWithRanks[256];
	unsigned int bwtWithRanksLen;
	unsigned char *encodedChars;
	alignas(128) unsigned int encodedCharsLen[256];
	unsigned int maxEncodedCharsLen;
	alignas(128) unsigned int c[257];
	unsigned int bInC;
	unsigned char *encodedPattern;
	unsigned int maxPatternLen;

	unsigned int textLen;

	void freeMemory() {
            for (int i = 0; i < 256; ++i) if (this->bwtWithRanks[i] != NULL) delete[] this->bwtWithRanks[i];
            if (this->encodedChars != NULL) delete[] this->encodedChars;
            if (this->encodedPattern != NULL) delete[] this->encodedPattern;
        }
        
	void initialize() {
            for (int i = 0; i < 256; ++i) {
                this->bwtWithRanks[i] = NULL;
                this->alignedBWTWithRanks[i] = NULL;
            }
            this->bwtWithRanksLen = 0;
            this->encodedChars = NULL;
            for (int i = 0; i < 256; ++i) this->encodedCharsLen[i] = 0;
            this->maxEncodedCharsLen = 0;
            this->encodedPattern = NULL;
            this->maxPatternLen = 0;
            for (int i = 0; i < 257; ++i) this->c[i] = 0;
            this->bInC = 0;

            this->textLen = 0;
        }
        
	void setMaxEncodedCharsLen() {
            this->maxEncodedCharsLen = 0;
            for (int i = 0; i < 256; ++i) if (this->encodedCharsLen[i] > this->maxEncodedCharsLen) this->maxEncodedCharsLen = this->encodedCharsLen[i];
            this->setEncodedPattern(1000);
        }
        
	void setEncodedPattern(unsigned int maxPatternLen) {
            if (this->encodedPattern != NULL) delete[] this->encodedPattern;
            this->maxPatternLen = maxPatternLen;
            this->encodedPattern = new unsigned char[maxPatternLen * this->maxEncodedCharsLen + 1];
        }
        
	void encodePattern(unsigned char *pattern, unsigned int patternLen, unsigned int &encodedPatternLen, bool &wrongEncoding) {
            if (patternLen > this->maxPatternLen) this->setEncodedPattern(patternLen);
            unsigned char* p = pattern;
            encodedPatternLen = 0;
            for (; p < pattern + patternLen; ++p) {
                if (this->encodedCharsLen[*p] == 0) {
                    wrongEncoding = true;
                    break;
                }
                for (unsigned int i = 0; i < this->encodedCharsLen[*p]; ++i) this->encodedPattern[encodedPatternLen++] = this->encodedChars[(unsigned int)(*p) * this->maxEncodedCharsLen + i];
            }
        }
        
	unsigned char *getEncodedInSCBO(unsigned char *text, unsigned int textLen, unsigned int &encodedTextLen) {
            int max = (int)exp2((double)BPC);

            unsigned int charsCount[256][2];
            for (int i = 0; i < 256; ++i) {
                    charsCount[i][0] = i;
                    charsCount[i][1] = 0;
            }
            for (unsigned int i = 0; i < textLen; ++i) {
                    charsCount[text[i]][1]++;
            }

            unsigned int charsLen = 0;
            for (int i = 0; i < 256; ++i) {
                    if (charsCount[i][1] != 0) ++charsLen;
            }

            vector<unsigned int*> charsCountVector(charsCount, charsCount + 256);
            sort(charsCountVector.begin(), charsCountVector.end(), sortCharsCount);

            unsigned char chars[256];
            int i = 0;
            for (vector<unsigned int*>::iterator it = charsCountVector.begin(); it != charsCountVector.end(); ++it, ++i) {
                    chars[i] = **it;
            }

            unsigned int totalTotal = (unsigned int)(-1);
            unsigned int best[4] = {0, 0, 0, 0};
            unsigned int maxSymbolLen = 0;

            for (int o = 1; o < max - 2; ++o) {
                for (int b = 1; b < max - 2; ++b) {
                    for (int c = 1; c < max - 2; ++c) {
                        for (int s = 1; s < max - 2; ++s) {
                            if (o + b + c + s != max) continue;
                            int sig = charsLen;
                            unsigned int total = 0;
                            unsigned int curr = 0;
                            unsigned int upperBound = o;
                            unsigned int symbolLen = 1;
                            if (sig > 0) {
                                for (unsigned int i = 0; i < upperBound; ++i) {
                                    total += charsCountVector[curr][1] * symbolLen;
                                    ++curr;
                                    if (curr >= charsLen) break;
                                }
                                sig -= upperBound;
                                upperBound = b * s;
                                if (sig > 0) ++symbolLen;
                            }
                            while (sig > 0) {
                                for (unsigned int i = 0; i < upperBound; ++i) {
                                    total += charsCountVector[curr][1] * symbolLen;
                                    ++curr;
                                    if (curr >= charsLen) break;
                                }
                                sig -= upperBound;
                                upperBound *= c;
                                if (sig > 0) ++symbolLen;
                            }
                            if (total < totalTotal) {
                                totalTotal = total;
                                best[0] = o;
                                best[1] = b;
                                best[2] = s;
                                best[3] = c;
                                maxSymbolLen = symbolLen;
                            }
                        }
                    }
                }
            }

            unsigned int o = best[0];
            unsigned int b = best[1];
            unsigned int s = best[2];
            unsigned int c = best[3];

            unsigned int bStart = o;
            unsigned int sStart = bStart + b;
            unsigned int cStart = sStart + s;

            this->encodedChars = new unsigned char[256 * maxSymbolLen];

            for (unsigned int i = 0; i < charsLen; ++i) {
                if (i < o) {
                    this->encodedChars[(unsigned int)chars[i] * maxSymbolLen] = i + 1;
                    this->encodedCharsLen[chars[i]] = 1;
                    continue;
                }
                if (i < o + b * s) {
                    int j = i - o;
                    this->encodedChars[(unsigned int)chars[i] * maxSymbolLen] = bStart + j / s + 1;
                    this->encodedChars[(unsigned int)chars[i] * maxSymbolLen + 1] = sStart + j % s + 1;
                    this->encodedCharsLen[chars[i]] = 2;
                    continue;
                }
                unsigned int temp1 = b * s;
                unsigned int symbolLen = 3;
                unsigned int temp2 = 0;
                while (true) {
                    temp2 = b * s * (unsigned int)pow((double)c, (double)(symbolLen - 2));
                    if (i < o + temp1 + temp2) {
                        int j = i - o - temp1;
                        this->encodedChars[(unsigned int)chars[i] * maxSymbolLen] = bStart + j / (s * (unsigned int)pow((double)c, (double)(symbolLen - 2))) + 1;
                        for (unsigned int k = 1; k < symbolLen - 1; ++k) {
                                this->encodedChars[(unsigned int)chars[i] * maxSymbolLen + k] = cStart + (j / (s * (unsigned int)pow((double)c, (double)(symbolLen - 2 - k)))) % c + 1;
                        }
                        this->encodedChars[(unsigned int)chars[i] * maxSymbolLen + symbolLen - 1] = sStart + j % s + 1;
                        this->encodedCharsLen[chars[i]] = symbolLen;
                        break;
                    }
                    temp1 += temp2;
                    ++symbolLen;
                }
            }

            unsigned char *encodedText = new unsigned char[totalTotal + 1];
            encodedTextLen = 0;

            for (unsigned int i = 0; i < textLen; ++i) {
                    unsigned char ch = text[i];
                    for (unsigned int j = 0; j < this->encodedCharsLen[ch]; ++j) {
                            encodedText[encodedTextLen++] = this->encodedChars[(unsigned int)ch * maxSymbolLen + j];
                    }
            }
            encodedText[encodedTextLen] = '\0';
            return encodedText;
        }
        
	unsigned char *getEncodedInCB(unsigned char *text, unsigned int textLen, unsigned int &encodedTextLen, unsigned int &b) {
            int max = (int)exp2((double)BPC);

            unsigned int charsCount[256][2];
            for (int i = 0; i < 256; ++i) {
                    charsCount[i][0] = i;
                    charsCount[i][1] = 0;
            }
            for (unsigned int i = 0; i < textLen; ++i) {
                    charsCount[text[i]][1]++;
            }

            unsigned int charsLen = 0;
            for (int i = 0; i < 256; ++i) {
                    if (charsCount[i][1] != 0) ++charsLen;
            }

            vector<unsigned int*> charsCountVector(charsCount, charsCount + 256);
            sort(charsCountVector.begin(), charsCountVector.end(), sortCharsCount);

            unsigned char chars[256];
            int i = 0;
            for (vector<unsigned int*>::iterator it = charsCountVector.begin(); it != charsCountVector.end(); ++it, ++i) {
                    chars[i] = **it;
            }

            unsigned int totalTotal = (unsigned int)(-1);
            unsigned int best[2] = {0, 0};
            unsigned int maxSymbolLen = 0;

            for (int b = 1; b < max; ++b) {
                for (int c = 1; c < max; ++c) {
                    if (b + c != max) continue;
                    int sig = charsLen;
                    unsigned int total = 0;
                    unsigned int curr = 0;
                    unsigned int upperBound = b;
                    unsigned int symbolLen = 1;
                    while (sig > 0) {
                        for (unsigned int i = 0; i < upperBound; ++i) {
                            total += charsCountVector[curr][1] * symbolLen;
                            ++curr;
                            if (curr >= charsLen) break;
                        }
                        sig -= upperBound;
                        upperBound *= c;
                        if (sig > 0) ++symbolLen;
                    }
                    if (total < totalTotal) {
                        totalTotal = total;
                        best[0] = b;
                        best[1] = c;
                        maxSymbolLen = symbolLen;
                    }
                }
            }

            b = best[0];
            unsigned int c = best[1];

            unsigned int bStart = 0;
            unsigned int cStart = b;

            this->encodedChars = new unsigned char[256 * maxSymbolLen];

            for (unsigned int i = 0; i < charsLen; ++i) {
                if (i < b) {
                    this->encodedChars[(unsigned int)chars[i] * maxSymbolLen] = i + 1;
                    this->encodedCharsLen[chars[i]] = 1;
                    continue;
                }
                unsigned int temp1 = b;
                unsigned int symbolLen = 2;
                unsigned int temp2 = 0;
                while (true) {
                    temp2 = b * (unsigned int)pow((double)c, (double)(symbolLen - 1));
                    if (i < temp1 + temp2) {
                        int j = i - temp1;
                        this->encodedChars[(unsigned int)chars[i] * maxSymbolLen] = bStart + j / (unsigned int)pow((double)c, (double)(symbolLen - 1)) + 1;
                        for (unsigned int k = 1; k < symbolLen - 1; ++k) {
                                this->encodedChars[(unsigned int)chars[i] * maxSymbolLen + k] = cStart + (j / (unsigned int)pow((double)c, (double)(symbolLen - 1 - k))) % c + 1;
                        }
                        this->encodedChars[(unsigned int)chars[i] * maxSymbolLen + symbolLen - 1] = cStart + j % c + 1;
                        this->encodedCharsLen[chars[i]] = symbolLen;
                        break;
                    }
                    temp1 += temp2;
                    ++symbolLen;
                }
            }
            ++b;

            unsigned char *encodedText = new unsigned char[totalTotal + 1];
            encodedTextLen = 0;

            for (unsigned int i = 0; i < textLen; ++i) {
                unsigned char ch = text[i];
                for (unsigned int j = 0; j < this->encodedCharsLen[ch]; ++j) {
                    encodedText[encodedTextLen++] = this->encodedChars[(unsigned int)ch * maxSymbolLen + j];
                }
            }
            encodedText[encodedTextLen] = '\0';
            return encodedText;
        }

public:
	FMDummy2() {
		this->initialize();
	}

	~FMDummy2() {
		this->free();
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            unsigned int encodedTextLen;
            unsigned char *encodedText = NULL;
            unsigned int b = 0;
            switch (S) {
                case FMDummy2Schema::SCHEMA_SCBO:
                    if (this->verbose) cout << "SCBO text encoding ... " << flush;
                    encodedText = this->getEncodedInSCBO(text, this->textLen, encodedTextLen);
                    if (this->verbose) cout << "Done" << endl;
                    break;
                default:
                    if (this->verbose) cout << "CB text encoding ... " << flush;
                    encodedText = this->getEncodedInCB(text, this->textLen, encodedTextLen, b);
                    if (this->verbose) cout << "Done" << endl;
                    break;
            }
            this->setMaxEncodedCharsLen();
            delete[] text;
            unsigned int bwtLen;
            unsigned int encodedSALen;
            unsigned int *encodedSA = getSA(encodedText, encodedTextLen, encodedSALen, 0, this->verbose);
            unsigned char *bwt = getBWT(encodedText, encodedTextLen, encodedSA, encodedSALen, bwtLen, 0, this->verbose);
            delete[] encodedSA;
            unsigned int encodedCharsLen = (unsigned int)exp2((double)BPC);
            if (this->verbose) cout << "Compacting BWT ... " << flush;
            ++bwtLen;
            unsigned int bwtDenseLen = (bwtLen / 8);
            if (bwtLen % 8 > 0) ++bwtDenseLen;
            unsigned int bwtDenseInLongLen = bwtDenseLen / sizeof(unsigned long long);
            if (bwtDenseLen % sizeof(unsigned long long) > 0) ++bwtDenseInLongLen;
            unsigned long long *bwtDenseInLong[256];
            vector<unsigned char> encodedChars = {};
            for (unsigned int i = 0; i < encodedCharsLen; ++i) {
                    encodedChars.push_back(i + 1);
                    unsigned char *bwtDense = getBinDenseForChar(bwt, bwtLen, encodedChars[i]);
                    bwtDenseInLong[encodedChars[i]] = new unsigned long long[bwtDenseInLongLen + 8];
                    for (unsigned long long j = 0; j < bwtDenseInLongLen; ++j) {
                            bwtDenseInLong[encodedChars[i]][j] = ((unsigned long long)bwtDense[8 * j + 7] << 56) | ((unsigned long long)bwtDense[8 * j + 6] << 48) | ((unsigned long long)bwtDense[8 * j + 5] << 40) | ((unsigned long long)bwtDense[8 * j + 4] << 32) | ((unsigned long long)bwtDense[8 * j + 3] << 24) | ((unsigned long long)bwtDense[8 * j + 2] << 16) | ((unsigned long long)bwtDense[8 * j + 1] << 8) | (unsigned long long)bwtDense[8 * j];
                    }
                    for (unsigned long long j = bwtDenseInLongLen; j < bwtDenseInLongLen + 8; ++j) {
                            bwtDenseInLong[encodedChars[i]][j] = 0ULL;
                    }
                    delete[] bwtDense;
            }
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;

            fillArrayC(encodedText, encodedTextLen, this->c, this->verbose);
            if (S == FMDummy2Schema::SCHEMA_CB) this->bInC = this->c[b];

            if (this->verbose) cout << "Interweaving BWT with ranks ... " << flush;
            switch(T) {
                case FMDummy2Type::FMD2_512:
                    buildRank_512_counter40(bwtDenseInLong, bwtDenseInLongLen, encodedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
                default:
                    buildRank_256_counter48(bwtDenseInLong, bwtDenseInLongLen, encodedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
            }
            if (this->verbose) cout << "Done" << endl;

            for (vector<unsigned char>::iterator it = encodedChars.begin(); it != encodedChars.end(); ++it) delete[] bwtDenseInLong[*it];
            delete[] encodedText;

            if (this->verbose) cout << "Index successfully built" << endl;
        }
        
	void save(const char *fileName) {
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(this->encodedCharsLen, (size_t)sizeof(unsigned int), (size_t)256, outFile);
            fwrite(this->encodedChars, (size_t)sizeof(unsigned char), (size_t)this->maxEncodedCharsLen * 256, outFile);
            unsigned int maxChar = (unsigned int)exp2((double)BPC);
            fwrite(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->bwtWithRanksLen > 0) {
                    for (unsigned int i = 1; i < maxChar + 1; ++i) {
                            fwrite(this->alignedBWTWithRanks[i], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, outFile);
                    }
            }
            if (S == FMDummy2Schema::SCHEMA_CB) fwrite(&this->bInC, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void load(const char *fileName) {
            this->free();
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->encodedCharsLen, (size_t)sizeof(unsigned int), (size_t)256, inFile);
            if (result != 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            this->setMaxEncodedCharsLen();
            this->encodedChars = new unsigned char[this->maxEncodedCharsLen * 256];
            result = fread(this->encodedChars, (size_t)sizeof(unsigned char), (size_t)this->maxEncodedCharsLen * 256, inFile);
            if (result != this->maxEncodedCharsLen * 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            unsigned int maxChar = (unsigned int)exp2((double)BPC);
            result = fread(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->bwtWithRanksLen > 0) {
                    for (int i = 0; i < 256; ++i) this->alignedBWTWithRanks[i] = NULL;
                    for (unsigned int i = 1; i < maxChar + 1; ++i) {
                            this->bwtWithRanks[i] = new unsigned long long[this->bwtWithRanksLen + 16];
                            this->alignedBWTWithRanks[i] = this->bwtWithRanks[i];
                            while ((unsigned long long)(this->alignedBWTWithRanks[i]) % 128) ++(this->alignedBWTWithRanks[i]);
                            result = fread(this->alignedBWTWithRanks[i], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, inFile);
                            if (result != this->bwtWithRanksLen) {
                                    cout << "Error loading index from " << fileName << endl;
                                    exit(1);
                            }
                    }
            }
            if (S == FMDummy2Schema::SCHEMA_CB) {
                    result = fread(&this->bInC, (size_t)sizeof(unsigned int), (size_t)1, inFile);
                    if (result != 1) {
                            cout << "Error loading index from " << fileName << endl;
                            exit(1);
                    }
            }
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
        
	unsigned int getIndexSize() {
            unsigned int size = sizeof(this->maxEncodedCharsLen) + sizeof(this->maxPatternLen) + sizeof(bInC) + sizeof(this->bwtWithRanksLen);
            size += (257 * sizeof(unsigned int) + 256 * sizeof(unsigned long long *) + 256 * sizeof(unsigned long long *) + 256 * sizeof(unsigned int) + (this->maxEncodedCharsLen * this->maxPatternLen + 1) * sizeof(unsigned char) + this->maxEncodedCharsLen * 256 * sizeof(unsigned char));
            if (this->bwtWithRanksLen > 0) size += ((unsigned int)exp2((double)BPC) * (this->bwtWithRanksLen + 16) * sizeof(unsigned long long));
            return size;
        }
        
	unsigned int getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }

	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            bool wrongEncoding = false;
            unsigned int encodedPatternLen;
            this->encodePattern(pattern, patternLen, encodedPatternLen, wrongEncoding);
            if (wrongEncoding) return 0;
            switch(T) {
                case FMDummy2Type::FMD2_512:
                    switch(S) {
                        case FMDummy2Schema::SCHEMA_SCBO:
                            return count_512_counter40(encodedPattern, encodedPatternLen - 1, this->c, this->alignedBWTWithRanks, this->c[encodedPattern[encodedPatternLen - 1]] + 1, this->c[encodedPattern[encodedPatternLen - 1] + 1]);
                            break;
                        default:
                            return count_512_counter40(encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, 1, this->bInC);
                            break;
                    }
                    break;
                default:
                    switch(S) {
                        case FMDummy2Schema::SCHEMA_SCBO:
                            return count_256_counter48(encodedPattern, encodedPatternLen - 1, this->c, this->alignedBWTWithRanks, this->c[encodedPattern[encodedPatternLen - 1]] + 1, this->c[encodedPattern[encodedPatternLen - 1] + 1]);
                            break;
                        default:
                            return count_256_counter48(encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, 1, this->bInC);
                            break;
                    }
                    break; 
            }
        }
};

template<FMDummy2Type T, FMDummy2Schema S, FMDummy2BPC BPC> class FMDummy2Hash : public FMDummy2<T, S, BPC> {
protected:
	HTExt<HTType::STANDARD> *ht = NULL;

	void freeMemory() {
            FMDummy2<T, S, BPC>::freeMemory();
            if (this->ht != NULL) this->ht->free();
        }

public:
	FMDummy2Hash(unsigned int k, double loadFactor) {
		this->initialize();
		this->ht = new HTExt<HTType::STANDARD>(k, loadFactor);
	}

	~FMDummy2Hash() {
		this->free();
                if (this->ht != NULL) delete this->ht;
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            unsigned int saLen;
            unsigned int *sa = getSA(textFileName, text, this->textLen, saLen, 0, this->verbose);
            if (this->verbose) cout << "Building hash table ... " << flush;
            unsigned int uniqueSuffixNum = getUniqueSuffixNum(this->ht->k, text, this->textLen, sa, saLen);
            unsigned long long bucketsNum = (double)uniqueSuffixNum * (1.0 / this->ht->loadFactor);
            unsigned char *cutOutEntries = new unsigned char[bucketsNum * 2];
            this->ht->build(text, this->textLen, sa, saLen, {}, cutOutEntries);
            if (this->verbose) cout << "Done" << endl;
            delete[] sa;
            unsigned int encodedTextLen;
            unsigned char *encodedText = NULL;
            unsigned int b = 0;
            switch (S) {
                case FMDummy2Schema::SCHEMA_SCBO:
                        if (this->verbose) cout << "SCBO text encoding ... " << flush;
                        encodedText = this->getEncodedInSCBO(text, this->textLen, encodedTextLen);
                        if (this->verbose) cout << "Done" << endl;
                        break;
                default:
                        if (this->verbose) cout << "CB text encoding ... " << flush;
                        encodedText = this->getEncodedInCB(text, this->textLen, encodedTextLen, b);
                        if (this->verbose) cout << "Done" << endl;
                        break;
            }
            this->setMaxEncodedCharsLen();
            delete[] text;
            unsigned int bwtLen;
            unsigned int encodedSALen;
            unsigned int *encodedSA = getSA(encodedText, encodedTextLen, encodedSALen, 0, this->verbose);
            unsigned char *bwt = getBWT(encodedText, encodedTextLen, encodedSA, encodedSALen, bwtLen, 0, this->verbose);
            unsigned int encodedCharsLen = (unsigned int)exp2((double)BPC);
            if (this->verbose) cout << "Compacting BWT ... " << flush;
            ++bwtLen;
            unsigned int bwtDenseLen = (bwtLen / 8);
            if (bwtLen % 8 > 0) ++bwtDenseLen;
            unsigned int bwtDenseInLongLen = bwtDenseLen / sizeof(unsigned long long);
            if (bwtDenseLen % sizeof(unsigned long long) > 0) ++bwtDenseInLongLen;
            unsigned long long *bwtDenseInLong[256];
            vector<unsigned char> encodedChars = {};
            for (unsigned int i = 0; i < encodedCharsLen; ++i) {
                    encodedChars.push_back(i + 1);
                    unsigned char *bwtDense = getBinDenseForChar(bwt, bwtLen, encodedChars[i]);
                    bwtDenseInLong[encodedChars[i]] = new unsigned long long[bwtDenseInLongLen + 8];
                    for (unsigned long long j = 0; j < bwtDenseInLongLen; ++j) {
                            bwtDenseInLong[encodedChars[i]][j] = ((unsigned long long)bwtDense[8 * j + 7] << 56) | ((unsigned long long)bwtDense[8 * j + 6] << 48) | ((unsigned long long)bwtDense[8 * j + 5] << 40) | ((unsigned long long)bwtDense[8 * j + 4] << 32) | ((unsigned long long)bwtDense[8 * j + 3] << 24) | ((unsigned long long)bwtDense[8 * j + 2] << 16) | ((unsigned long long)bwtDense[8 * j + 1] << 8) | (unsigned long long)bwtDense[8 * j];
                    }
                    for (unsigned long long j = bwtDenseInLongLen; j < bwtDenseInLongLen + 8; ++j) {
                            bwtDenseInLong[encodedChars[i]][j] = 0ULL;
                    }
                    delete[] bwtDense;
            }
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;

            fillArrayC(encodedText, encodedTextLen, this->c, this->verbose);
            if (S == FMDummy2Schema::SCHEMA_CB) this->bInC = this->c[b];

            if (this->verbose) cout << "Interweaving BWT with ranks ... " << flush;
            switch(T) {
                case FMDummy2Type::FMD2_512:
                    buildRank_512_counter40(bwtDenseInLong, bwtDenseInLongLen, encodedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
                default:
                    buildRank_256_counter48(bwtDenseInLong, bwtDenseInLongLen, encodedChars, this->bwtWithRanks, this->bwtWithRanksLen, this->alignedBWTWithRanks);
                    break;
            }
            if (this->verbose) cout << "Done" << endl;
            unsigned int diff;
            if (this->verbose) cout << "Modifying hash table for encoded text ... " << flush;
            unsigned char *entry = new unsigned char[this->ht->k + 1];
            entry[this->ht->k] = '\0';
            unsigned char *encodedPattern = new unsigned char[this->maxEncodedCharsLen * this->ht->k + 1];
            unsigned int encodedPatternLen;
            for (unsigned int i = 0; i < this->ht->bucketsNum; ++i) {
                    if (this->ht->alignedBoundariesHT[2 * i] != HT<HTType::STANDARD>::emptyValueHT) {
                            entry[0] = cutOutEntries[2 * i];
                            entry[1] = cutOutEntries[2 * i + 1];
                            for (unsigned int j = 0; j < this->ht->prefixLength; ++j) entry[j + 2] = this->ht->alignedEntriesHT[i * this->ht->prefixLength + j];
                            encode(entry, this->ht->k, this->encodedChars, this->encodedCharsLen, this->maxEncodedCharsLen, encodedPattern, encodedPatternLen);
                            diff = this->ht->alignedBoundariesHT[2 * i + 1] - this->ht->alignedBoundariesHT[2 * i];
                            switch (S) {
                                case FMDummy2Schema::SCHEMA_SCBO:
                                        switch(T) {
                                        case FMDummy2Type::FMD2_512:
                                            getCountBoundaries_512_counter40(encodedPattern, encodedPatternLen - 1, this->c, this->alignedBWTWithRanks, this->c[encodedPattern[encodedPatternLen - 1]] + 1, this->c[encodedPattern[encodedPatternLen - 1] + 1], this->ht->alignedBoundariesHT[2 * i], this->ht->alignedBoundariesHT[2 * i + 1]);
                                            break;
                                        default:
                                            getCountBoundaries_256_counter48(encodedPattern, encodedPatternLen - 1, this->c, this->alignedBWTWithRanks, this->c[encodedPattern[encodedPatternLen - 1]] + 1, this->c[encodedPattern[encodedPatternLen - 1] + 1], this->ht->alignedBoundariesHT[2 * i], this->ht->alignedBoundariesHT[2 * i + 1]);
                                            break;
                                        }
                                        break;
                                default:
                                        switch(T) {
                                        case FMDummy2Type::FMD2_512:
                                                getCountBoundaries_512_counter40(encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, 1, this->bInC, this->ht->alignedBoundariesHT[2 * i], this->ht->alignedBoundariesHT[2 * i + 1]);
                                                break;
                                        default:
                                                getCountBoundaries_256_counter48(encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, 1, this->bInC, this->ht->alignedBoundariesHT[2 * i], this->ht->alignedBoundariesHT[2 * i + 1]);
                                                break;
                                        }
                                        break;
                            }
                            this->ht->alignedBoundariesHT[2 * i + 1] = this->ht->alignedBoundariesHT[2 * i] + diff;
                    }
            }
            delete[] encodedPattern;
            delete[] cutOutEntries;
            delete[] entry;
            encodedPattern = new unsigned char[this->maxEncodedCharsLen * 2 + 1];
            unsigned char lutPattern[3];
            lutPattern[2] = '\0';
            for (int i = 0; i < 256; ++i) {
                    lutPattern[0] = (unsigned char)i;
                    for (int j = 0; j < 256; ++j) {
                            diff = this->ht->lut2[i][j][1] - this->ht->lut2[i][j][0];
                            lutPattern[1] = (unsigned char)j;
                            unsigned int encodedPatternLen;
                            encode(lutPattern, 2, this->encodedChars, this->encodedCharsLen, this->maxEncodedCharsLen, encodedPattern, encodedPatternLen);
                            binarySearch(encodedSA, encodedText, 0, encodedSALen, encodedPattern, encodedPatternLen, this->ht->lut2[i][j][0], this->ht->lut2[i][j][1]);
                            this->ht->lut2[i][j][1] = this->ht->lut2[i][j][0] + diff;
                    }
            }
            delete[] encodedPattern;
            if (this->verbose) cout << "Done" << endl;
            delete[] encodedSA;

            for (vector<unsigned char>::iterator it = encodedChars.begin(); it != encodedChars.end(); ++it) delete[] bwtDenseInLong[*it];
            delete[] encodedText;

            if (this->verbose) cout << "Index successfully built" << endl;
        }
        
	void save(const char *fileName) {
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(this->encodedCharsLen, (size_t)sizeof(unsigned int), (size_t)256, outFile);
            fwrite(this->encodedChars, (size_t)sizeof(unsigned char), (size_t)this->maxEncodedCharsLen * 256, outFile);
            unsigned int maxChar = (unsigned int)exp2((double)BPC);
            fwrite(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->bwtWithRanksLen > 0) {
                    for (unsigned int i = 1; i < maxChar + 1; ++i) {
                            fwrite(this->alignedBWTWithRanks[i], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, outFile);
                    }
            }
            if (S == FMDummy2Schema::SCHEMA_CB) fwrite(&this->bInC, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            this->ht->save(outFile);
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void load(const char *fileName) {
            this->free();
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->encodedCharsLen, (size_t)sizeof(unsigned int), (size_t)256, inFile);
            if (result != 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            this->setMaxEncodedCharsLen();
            this->encodedChars = new unsigned char[this->maxEncodedCharsLen * 256];
            result = fread(this->encodedChars, (size_t)sizeof(unsigned char), (size_t)this->maxEncodedCharsLen * 256, inFile);
            if (result != this->maxEncodedCharsLen * 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            unsigned int maxChar = (unsigned int)exp2((double)BPC);
            result = fread(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->bwtWithRanksLen > 0) {
                    for (int i = 0; i < 256; ++i) this->alignedBWTWithRanks[i] = NULL;
                    for (unsigned int i = 1; i < maxChar + 1; ++i) {
                            this->bwtWithRanks[i] = new unsigned long long[this->bwtWithRanksLen + 16];
                            this->alignedBWTWithRanks[i] = this->bwtWithRanks[i];
                            while ((unsigned long long)(this->alignedBWTWithRanks[i]) % 128) ++(this->alignedBWTWithRanks[i]);
                            result = fread(this->alignedBWTWithRanks[i], (size_t)sizeof(unsigned long long), (size_t)this->bwtWithRanksLen, inFile);
                            if (result != this->bwtWithRanksLen) {
                                    cout << "Error loading index from " << fileName << endl;
                                    exit(1);
                            }
                    }
            }
            if (S == FMDummy2Schema::SCHEMA_CB) {
                    result = fread(&this->bInC, (size_t)sizeof(unsigned int), (size_t)1, inFile);
                    if (result != 1) {
                            cout << "Error loading index from " << fileName << endl;
                            exit(1);
                    }
            }
            delete this->ht;
            this->ht = new HTExt<HTType::STANDARD>();
            this->ht->load(inFile);
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
        
	unsigned int getIndexSize() {
            return FMDummy2<T, S, BPC>::getIndexSize() + sizeof(this->ht) + this->ht->getHTSize();
        }

	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            if (patternLen < this->ht->k) return FMDummy2<T, S, BPC>::count(pattern, patternLen);
            unsigned int leftBoundary, rightBoundary;
            this->ht->getBoundaries(pattern + (patternLen - this->ht->k), leftBoundary, rightBoundary);
            bool wrongEncoding = false;
            unsigned int encodedPatternLen;
            this->encodePattern(pattern, patternLen - this->ht->k, encodedPatternLen, wrongEncoding);
            if (wrongEncoding) return 0;
            switch(T) {
                case FMDummy2Type::FMD2_512:
                    switch(S) {
                        case FMDummy2Schema::SCHEMA_SCBO:
                            return count_512_counter40(this->encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, leftBoundary + 1, rightBoundary);
                            break;
                        default:
                            return count_512_counter40(this->encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, leftBoundary + 1, rightBoundary);
                            break;
                    }
                    break;
                default:
                    switch(S) {
                        case FMDummy2Schema::SCHEMA_SCBO:
                            return count_256_counter48(this->encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, leftBoundary + 1, rightBoundary);
                            break;
                        default:
                            return count_256_counter48(this->encodedPattern, encodedPatternLen, this->c, this->alignedBWTWithRanks, leftBoundary + 1, rightBoundary);
                            break;
                    }
                    break; 
            }
        }
};

enum FMDummy3Type {
        FMD3_512 = 1,
        FMD3_1024 = 2
};

template<FMDummy3Type T> class FMDummy3 : public Index {
protected:
	unsigned char *bwtWithRanks;
	unsigned char *alignedBWTWithRanks;
	unsigned int bwtWithRanksLen;
	alignas(128) unsigned int lut[256][125];
	alignas(128) unsigned int c[257];
	unsigned int textLen;
        
	void freeMemory() {
            if (this->bwtWithRanks != NULL) delete[] this->bwtWithRanks;
        }
        
	void initialize() {
            this->bwtWithRanks = NULL;
            this->alignedBWTWithRanks = NULL;
            this->bwtWithRanksLen = 0;
            for (int i = 0; i < 257; ++i) this->c[i] = 0;

            this->textLen = 0;
        }
        
	void buildRank_512_enc125(unsigned char *bwtEnc125, unsigned int bwtLen) {
            unsigned int rank[4] = {0, 0, 0, 0};
            unsigned char *p, signs[4] = { 'A', 'C', 'G', 'T' };

            unsigned int *resRank[4];
            for (int i = 0; i < 4; ++i) {
                    resRank[i] = new unsigned int[(bwtLen * 8) / 384 + 1];
                    resRank[i][0] = 0;
            }
            for (int s = 0; s < 4; ++s) {
                    p = bwtEnc125;
                    for (unsigned int i = 8; p < bwtEnc125 + bwtLen; ++p, i += 8) {
                            rank[s] += this->lut[signs[s]][*p];
                            if (i % 384 == 0) resRank[s][i / 384] = rank[s];
                    }
            }
            this->bwtWithRanksLen = bwtLen + 4 * 4 * ((bwtLen * 8) / 384 + 1);
            this->bwtWithRanks = new unsigned char[this->bwtWithRanksLen + 128];
            this->alignedBWTWithRanks = this->bwtWithRanks;
            while ((unsigned long long)this->alignedBWTWithRanks % 128) ++(this->alignedBWTWithRanks);
            p = bwtEnc125;
            unsigned int counter = 0;
            for (unsigned int i = 0; p < bwtEnc125 + bwtLen; ++p, ++i) {
                    if (i % 48 == 0) {
                            for (int s = 0; s < 4; ++s) {
                                    this->alignedBWTWithRanks[counter++] = (resRank[s][i / 48] & 0x000000FFU);
                                    this->alignedBWTWithRanks[counter++] = ((resRank[s][i / 48] & 0x0000FF00U) >> 8);
                                    this->alignedBWTWithRanks[counter++] = ((resRank[s][i / 48] & 0x00FF0000U) >> 16);
                                    this->alignedBWTWithRanks[counter++] = ((resRank[s][i / 48] & 0xFF000000U) >> 24);
                            }
                    }
                    this->alignedBWTWithRanks[counter++] = *p;
            }
            for (int i = 0; i < 4; ++i) delete[] resRank[i];
        }
        
	void buildRank_1024_enc125(unsigned char *bwtEnc125, unsigned int bwtLen) {
            unsigned int rank[4] = {0, 0, 0, 0};
            unsigned char *p, signs[4] = { 'A', 'C', 'G', 'T' };

            unsigned int *resRank[4];
            for (int i = 0; i < 4; ++i) {
                    resRank[i] = new unsigned int[(bwtLen * 8) / 896 + 1];
                    resRank[i][0] = 0;
            }
            for (int s = 0; s < 4; ++s) {
                    p = bwtEnc125;
                    for (unsigned int i = 8; p < bwtEnc125 + bwtLen; ++p, i += 8) {
                            rank[s] += this->lut[signs[s]][*p];
                            if (i % 896 == 0) resRank[s][i / 896] = rank[s];
                    }
            }
            this->bwtWithRanksLen = bwtLen + 4 * 4 * ((bwtLen * 8) / 896 + 1);
            this->bwtWithRanks = new unsigned char[this->bwtWithRanksLen + 128];
            this->alignedBWTWithRanks = this->bwtWithRanks;
            while ((unsigned long long)this->alignedBWTWithRanks % 128) ++(this->alignedBWTWithRanks);
            p = bwtEnc125;
            unsigned int counter = 0;
            for (unsigned int i = 0; p < bwtEnc125 + bwtLen; ++p, ++i) {
                    if (i % 112 == 0) {
                            for (int s = 0; s < 4; ++s) {
                                    this->alignedBWTWithRanks[counter++] = (resRank[s][i / 112] & 0x000000FFU);
                                    this->alignedBWTWithRanks[counter++] = ((resRank[s][i / 112] & 0x0000FF00U) >> 8);
                                    this->alignedBWTWithRanks[counter++] = ((resRank[s][i / 112] & 0x00FF0000U) >> 16);
                                    this->alignedBWTWithRanks[counter++] = ((resRank[s][i / 112] & 0xFF000000U) >> 24);
                            }
                    }
                    this->alignedBWTWithRanks[counter++] = *p;
            }
            for (int i = 0; i < 4; ++i) delete[] resRank[i];
        }

public:

	FMDummy3() {
		this->initialize();
	}

	~FMDummy3() {
		this->free();
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            if (this->verbose) cout << "Converting text ... " << flush;
            unsigned char *convertedText = new unsigned char[this->textLen];
            for (unsigned int i = 0; i < this->textLen; ++i) {
                    switch (text[i]) {
                    case 'A': case 'C': case 'G': case 'T':
                            convertedText[i] = text[i];
                            break;
                    default:
                            convertedText[i] = 'N';
                    }
            }
            delete[] text;
            if (this->verbose) cout << "Done" << endl;

            unsigned int bwtLen;
            vector<unsigned char> selectedChars = { 'A', 'C', 'G', 'T' };
            unsigned char *bwt = getBWT(convertedText, this->textLen, bwtLen, 0, this->verbose);
            if (this->verbose) cout << "Encoding BWT ... " << flush;
            ++bwtLen;
            unsigned int bwtEnc125Len;
            unsigned char *bwtEnc125 = encode125(bwt, bwtLen, selectedChars, bwtEnc125Len);
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;
            fill125LUT(selectedChars, this->lut);
            fillArrayC(convertedText, this->textLen, this->c, this->verbose);
            delete[] convertedText;
            if (this->verbose) cout << "Interweaving BWT with ranks ... " << flush;
            switch (T) {
                case FMDummy3Type::FMD3_1024:
                        this->buildRank_1024_enc125(bwtEnc125, bwtEnc125Len);
                        break;
                default:
                        this->buildRank_512_enc125(bwtEnc125, bwtEnc125Len);
                        break;
            }
            if (this->verbose) cout << "Done" << endl;
            delete[] bwtEnc125;
            if (this->verbose) cout << "Index successfully built" << endl;
        }
        
	void save(const char *fileName) {
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(this->lut, (size_t)sizeof(unsigned int), (size_t)(256 * 125), outFile);
            fwrite(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->bwtWithRanksLen > 0) fwrite(this->alignedBWTWithRanks, (size_t)sizeof(unsigned char), (size_t)this->bwtWithRanksLen, outFile);
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void load(const char *fileName) {
            this->free();
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->lut, (size_t)sizeof(unsigned int), (size_t)(256 * 125), inFile);
            if (result != (256 * 125)) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->bwtWithRanksLen > 0) {
                    this->bwtWithRanks = new unsigned char[this->bwtWithRanksLen + 128];
                    this->alignedBWTWithRanks = this->bwtWithRanks;
                    while ((unsigned long long)this->alignedBWTWithRanks % 128) ++this->alignedBWTWithRanks;
                    result = fread(this->alignedBWTWithRanks, (size_t)sizeof(unsigned char), (size_t)this->bwtWithRanksLen, inFile);
                    if (result != this->bwtWithRanksLen) {
                            cout << "Error loading index from " << fileName << endl;
                            exit(1);
                    }
            }
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
        
	unsigned int getIndexSize() {
            unsigned int size = sizeof(this->bwtWithRanksLen);
            size += (257 * sizeof(unsigned int) + sizeof(unsigned char*) + 256 * 125 * sizeof(unsigned int));
            if (this->bwtWithRanksLen > 0) size += (this->bwtWithRanksLen + 128) * sizeof(unsigned char);
            return size;
        }
        
	unsigned int getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }

	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            switch(T) {
                case FMDummy3Type::FMD3_1024:
                    return count_1024_enc125(pattern, patternLen - 1, this->c, this->alignedBWTWithRanks, this->lut, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1]);
                    break;
                default:
                    return count_512_enc125(pattern, patternLen - 1, this->c, this->alignedBWTWithRanks, this->lut, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1]);
                    break;
            }
        }
};

template<FMDummy3Type T> class FMDummy3Hash : public FMDummy3<T> {
private:
	HTExt<HTType::STANDARD> *ht = NULL;
        
	void freeMemory() {
            FMDummy3<T>::freeMemory();
            if (this->ht != NULL) this->ht->free();
        }

public:
	FMDummy3Hash(unsigned int k, double loadFactor) {
		this->initialize();
		this->ht = new HTExt<HTType::STANDARD>(k, loadFactor);
	}

	~FMDummy3Hash() {
		this->free();
                if (this->ht != NULL) delete this->ht;
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            if (this->verbose) cout << "Converting text ... " << flush;
            unsigned char *convertedText = new unsigned char[this->textLen];
            for (unsigned int i = 0; i < this->textLen; ++i) {
                    switch (text[i]) {
                    case 'A': case 'C': case 'G': case 'T':
                            convertedText[i] = text[i];
                            break;
                    default:
                            convertedText[i] = 'N';
                    }
            }
            delete[] text;
            if (this->verbose) cout << "Done" << endl;

            unsigned int bwtLen;
            vector<unsigned char> selectedChars = { 'A', 'C', 'G', 'T' };
            unsigned int saLen;
            unsigned int *sa = getSA(convertedText, this->textLen, saLen, 0, this->verbose);
            if (this->verbose) cout << "Building hash table ... " << flush;

            this->ht->build(convertedText, this->textLen, sa, saLen, selectedChars);
            if (this->verbose) cout << "Done" << endl;
            unsigned char *bwt = getBWT(convertedText, this->textLen, sa, saLen, bwtLen, 0, this->verbose);
            delete[] sa;
            if (this->verbose) cout << "Encoding BWT ... " << flush;
            ++bwtLen;
            unsigned int bwtEnc125Len;
            unsigned char *bwtEnc125 = encode125(bwt, bwtLen, selectedChars, bwtEnc125Len);
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;
            fill125LUT(selectedChars, this->lut);
            fillArrayC(convertedText, this->textLen, this->c, this->verbose);
            delete[] convertedText;
            if (this->verbose) cout << "Interweaving BWT with ranks ... " << flush;
            switch (T) {
                case FMDummy3Type::FMD3_1024:
                        this->buildRank_1024_enc125(bwtEnc125, bwtEnc125Len);
                        break;
                default:
                        this->buildRank_512_enc125(bwtEnc125, bwtEnc125Len);
                        break;
            }
            if (this->verbose) cout << "Done" << endl;
            delete[] bwtEnc125;
            if (this->verbose) cout << "Index successfully built" << endl;
        }

	void save(const char *fileName) {
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(this->lut, (size_t)sizeof(unsigned int), (size_t)(256 * 125), outFile);
            fwrite(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->bwtWithRanksLen > 0) fwrite(this->alignedBWTWithRanks, (size_t)sizeof(unsigned char), (size_t)this->bwtWithRanksLen, outFile);
            this->ht->save(outFile);
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }

	void load(const char *fileName) {
            this->free();
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->lut, (size_t)sizeof(unsigned int), (size_t)(256 * 125), inFile);
            if (result != (256 * 125)) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(&this->bwtWithRanksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->bwtWithRanksLen > 0) {
                    this->bwtWithRanks = new unsigned char[this->bwtWithRanksLen + 128];
                    this->alignedBWTWithRanks = this->bwtWithRanks;
                    while ((unsigned long long)this->alignedBWTWithRanks % 128) ++this->alignedBWTWithRanks;
                    result = fread(this->alignedBWTWithRanks, (size_t)sizeof(unsigned char), (size_t)this->bwtWithRanksLen, inFile);
                    if (result != this->bwtWithRanksLen) {
                            cout << "Error loading index from " << fileName << endl;
                            exit(1);
                    }
            }
            delete this->ht;
            this->ht = new HTExt<HTType::STANDARD>();
            this->ht->load(inFile);
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }

	void free() {
            this->freeMemory();
            this->initialize();
        }

	unsigned int getIndexSize() {
            return FMDummy3<T>::getIndexSize() + sizeof(this->ht) + this->ht->getHTSize();
        }

	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            if (patternLen < this->ht->k) return FMDummy3<T>::count(pattern, patternLen);
            unsigned int leftBoundary, rightBoundary;
            this->ht->getBoundaries(pattern + (patternLen - this->ht->k), leftBoundary, rightBoundary);
            switch(T) {
                case FMDummy3Type::FMD3_1024:
                    return count_1024_enc125(pattern, patternLen - this->ht->k, this->c, this->alignedBWTWithRanks, this->lut, leftBoundary + 1, rightBoundary);
                    break;
                default:
                    return count_512_enc125(pattern, patternLen - this->ht->k, this->c, this->alignedBWTWithRanks, this->lut, leftBoundary + 1, rightBoundary);
                    break;
            }
        }
};

enum WTType {
        WT2 = 2,
        WT4 = 4,
        WT8 = 8
};

enum FMDummyHWTType {
        FMDHWT_512 = 8,
        FMDHWT_1024 = 16
};

template<FMDummyHWTType T, WTType W> class FMDummyHWT : public Index {
protected:
	WT *wt;
	alignas(128) unsigned long long code[256];
	alignas(128) unsigned int codeLen[256];
	alignas(128) unsigned int c[257];
	unsigned int textLen;

	void freeMemory() {
            if (this->wt != NULL) delete this->wt;
        }
        
	void initialize() {
            this->wt = NULL;

            for (int i = 0; i < 256; ++i) {
                    this->code[i] = 0;
                    this->codeLen[i] = 0;
            }
            for (int i = 0; i < 257; ++i) this->c[i] = 0;

            this->textLen = 0;
        }

public:

	FMDummyHWT() {
		this->initialize();
	}

	~FMDummyHWT() {
		this->free();
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            unsigned int bwtLen;
            unsigned char *bwt = getBWT(textFileName, text, this->textLen, bwtLen, 0, this->verbose);
            if (this->verbose) cout << "Huffman encoding ... " << flush;
            encodeHuffFromText(W, bwt, bwtLen, this->code, this->codeLen);
            if (this->verbose) cout << "Done" << endl;
            if (this->verbose) cout << "Building WT ... " << flush;
            switch (W) {
                case WTType::WT4:
                    this->wt = createWT4(T, bwt, bwtLen, 0, this->code, this->codeLen);
                    break;
                case WTType::WT8:
                    this->wt = createWT8(T, bwt, bwtLen, 0, this->code, this->codeLen);
                    break;
                default:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            this->wt = createWT2_1024_counter32(bwt, bwtLen, 0, this->code, this->codeLen);
                            break;
                        default:
                            this->wt = createWT2_512_counter40(bwt, bwtLen, 0, this->code, this->codeLen);
                            break;
                    }
                    break;
            }
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;
            fillArrayC(text, this->textLen, this->c, this->verbose);
            delete[] text;
            if (this->verbose) cout << "Index successfully built" << endl;
        }
        
	void save(const char *fileName) {
            bool nullPointer = false;
            bool notNullPointer = true;
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(this->code, (size_t)sizeof(unsigned long long), (size_t)256, outFile);
            fwrite(this->codeLen, (size_t)sizeof(unsigned int), (size_t)256, outFile);
            if (this->wt == NULL) fwrite(&nullPointer, (size_t)sizeof(bool), (size_t)1, outFile);
            else {
                    fwrite(&notNullPointer, (size_t)sizeof(bool), (size_t)1, outFile);
                    this->wt->save(outFile);
            }
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void load(const char *fileName) {
            this->free();
            bool isNotNullPointer;
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->code, (size_t)sizeof(unsigned long long), (size_t)256, inFile);
            if (result != 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->codeLen, (size_t)sizeof(unsigned int), (size_t)256, inFile);
            if (result != 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(&isNotNullPointer, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (isNotNullPointer) {
                    this->wt = new WT();
                    this->wt->load(inFile);
            }
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
        
	unsigned int getIndexSize() {
            unsigned int size = sizeof(this->wt);
            size += (257 * sizeof(unsigned int) + 256 * sizeof(unsigned int) + 256 * sizeof(unsigned long long));
            if (this->wt != NULL) size += this->wt->getWTSize();
            return size;
        }
        
	unsigned int getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }

	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            switch (W) {
                case WTType::WT4:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            return count_WT4_1024(pattern, patternLen - 1, this->c, this->wt, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1], this->code, this->codeLen);
                            break;
                        default:
                            return count_WT4_512(pattern, patternLen - 1, this->c, this->wt, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1], this->code, this->codeLen);
                            break;
                    }
                    break;
                case WTType::WT8:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            return count_WT8_1024(pattern, patternLen - 1, this->c, this->wt, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1], this->code, this->codeLen);
                            break;
                        default:
                            return count_WT8_512(pattern, patternLen - 1, this->c, this->wt, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1], this->code, this->codeLen);
                            break;
                    }
                    break;
                default:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            return count_WT2_1024_counter32(pattern, patternLen - 1, this->c, this->wt, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1], this->code, this->codeLen);
                            break;
                        default:
                            return count_WT2_512_counter40(pattern, patternLen - 1, this->c, this->wt, this->c[pattern[patternLen - 1]] + 1, this->c[pattern[patternLen - 1] + 1], this->code, this->codeLen);
                            break;
                    }
                    break;
            }
        }
};

template<FMDummyHWTType T, WTType W> class FMDummyHWTHash : public FMDummyHWT<T, W> {
private:
        HTExt<HTType::STANDARD> *ht = NULL;
        
	void freeMemory() {
            FMDummyHWT<T, W>::freeMemory();
            if (this->ht != NULL) this->ht->free();
        }

public:

	FMDummyHWTHash(unsigned int k, double loadFactor) {
		this->initialize();
		this->ht = new HTExt<HTType::STANDARD>(k, loadFactor);
	}

	~FMDummyHWTHash() {
		this->free();
                if (this->ht != NULL) delete this->ht;
	}

	void build(const char *textFileName) {
            this->free();
            unsigned char *text = readText(textFileName, this->textLen, 0);
            checkNullChar(text, this->textLen);
            unsigned int bwtLen;
            unsigned int saLen;
            unsigned int *sa = getSA(textFileName, text, this->textLen, saLen, 0, this->verbose);
            if (this->verbose) cout << "Building hash table ... " << flush;
            this->ht->build(text, this->textLen, sa, saLen);
            if (this->verbose) cout << "Done" << endl;
            unsigned char *bwt = getBWT(text, this->textLen, sa, saLen, bwtLen, 0, this->verbose);
            delete[] sa;
            if (this->verbose) cout << "Huffman encoding ... " << flush;
            encodeHuffFromText(W, bwt, bwtLen, this->code, this->codeLen);
            if (this->verbose) cout << "Done" << endl;
            if (this->verbose) cout << "Building WT ... " << flush;
            switch (W) {
                case WTType::WT4:
                    this->wt = createWT4(T, bwt, bwtLen, 0, this->code, this->codeLen);
                    break;
                case WTType::WT8:
                    this->wt = createWT8(T, bwt, bwtLen, 0, this->code, this->codeLen);
                    break;
                default:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            this->wt = createWT2_1024_counter32(bwt, bwtLen, 0, this->code, this->codeLen);
                            break;
                        default:
                            this->wt = createWT2_512_counter40(bwt, bwtLen, 0, this->code, this->codeLen);
                            break;
                    }
                    break;
            }
            delete[] bwt;
            if (this->verbose) cout << "Done" << endl;
            fillArrayC(text, this->textLen, this->c, this->verbose);
            delete[] text;
            if (this->verbose) cout << "Index successfully built" << endl;
        }

	void save(const char *fileName) {
            bool nullPointer = false;
            bool notNullPointer = true;
            if (this->verbose) cout << "Saving index in " << fileName << " ... " << flush;
            FILE *outFile;
            outFile = fopen(fileName, "w");
            fwrite(&this->verbose, (size_t)sizeof(bool), (size_t)1, outFile);
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(this->c, (size_t)sizeof(unsigned int), (size_t)257, outFile);
            fwrite(this->code, (size_t)sizeof(unsigned long long), (size_t)256, outFile);
            fwrite(this->codeLen, (size_t)sizeof(unsigned int), (size_t)256, outFile);
            if (this->wt == NULL) fwrite(&nullPointer, (size_t)sizeof(bool), (size_t)1, outFile);
            else {
                    fwrite(&notNullPointer, (size_t)sizeof(bool), (size_t)1, outFile);
                    this->wt->save(outFile);
            }
            this->ht->save(outFile);
            fclose(outFile);
            if (this->verbose) cout << "Done" << endl;
        }

	void load(const char *fileName) {
            this->free();
            bool isNotNullPointer;
            FILE *inFile;
            inFile = fopen(fileName, "rb");
            size_t result;
            result = fread(&this->verbose, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (this->verbose) cout << "Loading index from " << fileName << " ... " << flush;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->c, (size_t)sizeof(unsigned int), (size_t)257, inFile);
            if (result != 257) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->code, (size_t)sizeof(unsigned long long), (size_t)256, inFile);
            if (result != 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(this->codeLen, (size_t)sizeof(unsigned int), (size_t)256, inFile);
            if (result != 256) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            result = fread(&isNotNullPointer, (size_t)sizeof(bool), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading index from " << fileName << endl;
                    exit(1);
            }
            if (isNotNullPointer) {
                    this->wt = new WT();
                    this->wt->load(inFile);
            }
            delete this->ht;
            this->ht = new HTExt<HTType::STANDARD>();
            this->ht->load(inFile);
            fclose(inFile);
            if (this->verbose) cout << "Done" << endl;
        }

	void free() {
            this->freeMemory();
            this->initialize();
        }

	unsigned int getIndexSize() {
            return FMDummyHWT<T, W>::getIndexSize() + sizeof(this->ht) + this->ht->getHTSize();
        }

	unsigned int count(unsigned char *pattern, unsigned int patternLen) {
            if (patternLen < this->ht->k) return FMDummyHWT<T, W>::count(pattern, patternLen);
            unsigned int leftBoundary, rightBoundary;
            this->ht->getBoundaries(pattern + (patternLen - this->ht->k), leftBoundary, rightBoundary);
            switch (W) {
                case WTType::WT4:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            return count_WT4_1024(pattern, patternLen - this->ht->k, this->c, this->wt, leftBoundary + 1, rightBoundary, this->code, this->codeLen);
                            break;
                        default:
                            return count_WT4_512(pattern, patternLen - this->ht->k, this->c, this->wt, leftBoundary + 1, rightBoundary, this->code, this->codeLen);
                            break;
                    }
                    break;
                case WTType::WT8:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            return count_WT8_1024(pattern, patternLen - this->ht->k, this->c, this->wt, leftBoundary + 1, rightBoundary, this->code, this->codeLen);
                            break;
                        default:
                            return count_WT8_512(pattern, patternLen - this->ht->k, this->c, this->wt, leftBoundary + 1, rightBoundary, this->code, this->codeLen);
                            break;
                    }
                    break;
                default:
                    switch(T) {
                        case FMDummyHWTType::FMDHWT_1024:
                            return count_WT2_1024_counter32(pattern, patternLen - this->ht->k, this->c, this->wt, leftBoundary + 1, rightBoundary, this->code, this->codeLen);
                            break;
                        default:
                            return count_WT2_512_counter40(pattern, patternLen - this->ht->k, this->c, this->wt, leftBoundary + 1, rightBoundary, this->code, this->codeLen);
                            break;
                    }
                    break;
            }
        }
};

unsigned char *getBinDenseForChar(unsigned char *bwt, unsigned int bwtLen, int ordChar) {
	unsigned int bwtDenseLen = bwtLen / 8;
	if (bwtLen % 8 > 0) ++bwtDenseLen;
	unsigned char *bwtDense = new unsigned char[bwtDenseLen];
	unsigned int curr = 0, temp = 0;

	for (unsigned int i = 0; i < bwtLen; ++i) {
		if (bwt[i] == ordChar) temp |= (1 << (i % 8));
		if (i % 8 == 7) {
			bwtDense[curr] = (unsigned char)temp;
			++curr;
			temp = 0;
		}
	}

	if (bwtLen % 8 > 0) bwtDense[curr] = (unsigned char)temp;
	return bwtDense;
}

void buildRank_256_counter48(unsigned long long **bwtInLong, unsigned int bwtInLongLen, vector<unsigned char> selectedChars, unsigned long long **bwtWithRanks, unsigned int &bwtWithRanksLen, unsigned long long **alignedBWTWithRanks) {
	unsigned long long *p, pops, rank, b1, b2;
	for (int i = 0; i < 256; ++i) alignedBWTWithRanks[i] = NULL;
	bwtWithRanksLen = bwtInLongLen + (bwtInLongLen * 64) / 192 + 1;

	for (vector<unsigned char>::iterator it = selectedChars.begin(); it != selectedChars.end(); ++it) {
		unsigned int c = (*it);
		unsigned long long *resRank = new unsigned long long[(bwtInLongLen * 64) / 192 + 1];
		p = bwtInLong[c];
		rank = 0;
		pops = 0;
		for (unsigned int j = 0; p < bwtInLong[c] + bwtInLongLen; p += 3, ++j) {
			pops = __builtin_popcountll(*p);
			b1 = (pops << 56);
			pops += __builtin_popcountll(*(p + 1));
			b2 = (pops << 48);
			pops += __builtin_popcountll(*(p + 2));
			resRank[j] = rank + b1 + b2;
			rank += pops;
			pops = 0;
		}
		bwtWithRanks[c] = new unsigned long long[bwtWithRanksLen + 16];
		alignedBWTWithRanks[c] = bwtWithRanks[c];
		while ((unsigned long long)alignedBWTWithRanks[c] % 128) ++(alignedBWTWithRanks[c]);
		p = bwtInLong[c];
		unsigned int counter = 0;
		for (unsigned int j = 0; p < bwtInLong[c] + bwtInLongLen; ++p, ++j) {
			if (j % 3 == 0) alignedBWTWithRanks[c][counter++] = resRank[j / 3];
			alignedBWTWithRanks[c][counter++] = *p;
		}
		delete[] resRank;
	}
}

void buildRank_512_counter40(unsigned long long **bwtInLong, unsigned int bwtInLongLen, vector<unsigned char> selectedChars, unsigned long long **bwtWithRanks, unsigned int &bwtWithRanksLen, unsigned long long **alignedBWTWithRanks) {
	unsigned long long *p, pop1, pop2, pop3, rank, b1, b2, b3;
	for (int i = 0; i < 256; ++i) alignedBWTWithRanks[i] = NULL;
	bwtWithRanksLen = bwtInLongLen + (bwtInLongLen * 64) / 448 + 1;

	for (vector<unsigned char>::iterator it = selectedChars.begin(); it != selectedChars.end(); ++it) {
		unsigned int c = (*it);
		unsigned long long *resRank = new unsigned long long[(bwtInLongLen * 64) / 448 + 1];
		p = bwtInLong[c];
		rank = 0;
		for (unsigned int j = 0; p < bwtInLong[c] + bwtInLongLen; p += 7, ++j) {
			pop1 = __builtin_popcountll(*p) + __builtin_popcountll(*(p + 1));
			b1 = (pop1 << 56);
			pop2 = __builtin_popcountll(*(p + 2)) + __builtin_popcountll(*(p + 3));
			b2 = (pop2 << 48);
			pop3 = __builtin_popcountll(*(p + 4)) + __builtin_popcountll(*(p + 5));
			b3 = (pop3 << 40);
			resRank[j] = rank + b1 + b2 + b3;
			rank += pop1 + pop2 + pop3 + __builtin_popcountll(*(p + 6));
		}
		bwtWithRanks[c] = new unsigned long long[bwtWithRanksLen + 16];
		alignedBWTWithRanks[c] = bwtWithRanks[c];
		while ((unsigned long long)alignedBWTWithRanks[c] % 128) ++(alignedBWTWithRanks[c]);
		p = bwtInLong[c];
		unsigned int counter = 0;
		for (unsigned int j = 0; p < bwtInLong[c] + bwtInLongLen; ++p, ++j) {
			if (j % 7 == 0) alignedBWTWithRanks[c][counter++] = resRank[j / 7];
			alignedBWTWithRanks[c][counter++] = *p;
		}
		delete[] resRank;
	}
}

unsigned int getRank_256_counter48(unsigned char c, unsigned int i, unsigned long long **bwtWithRanks) {
	unsigned int j = i / 192;
	unsigned long long *p = bwtWithRanks[c] + 4 * j;
	unsigned int rank = (*p) & 0x00000000FFFFFFFFULL;
	unsigned int b1 = ((*p) >> 56) & 0x00000000000000FFULL;  // popcount for 64-bit prefix
	unsigned int b2 = ((*p) >> 48) & 0x00000000000000FFULL;  // popcount for 128-bit prefix

	++p;
	i -= (j * 192);

	switch (i / 64) {
	case 2:
		rank += b2 + __builtin_popcountll(*(p + 2) & ((1ULL << (i % 64)) - 1));
		break;
	case 1:
		rank += b1 + __builtin_popcountll(*(p + 1) & ((1ULL << (i % 64)) - 1));
		break;
	case 0:
		rank += __builtin_popcountll(*p & ((1ULL << (i % 64)) - 1));
	}

	return rank;
}

unsigned int count_256_counter48(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal) {
	unsigned char c;
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 4 * ((firstVal - 1) / 192), 0, 3);
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 4 * (lastVal / 192), 0, 3);

	while (firstVal <= lastVal && i > 1) {
		c = pattern[i - 1];
		if (bwtWithRanks[c] == NULL) return 0;
		firstVal = C[c] + getRank_256_counter48(c, firstVal - 1, bwtWithRanks) + 1;
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 4 * ((firstVal - 1) / 192), 0, 3);
		lastVal = C[c] + getRank_256_counter48(c, lastVal, bwtWithRanks);
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 4 * (lastVal / 192), 0, 3);
		--i;
	}

	if (firstVal <= lastVal) {
		c = pattern[i - 1];
		if (bwtWithRanks[c] == NULL) return 0;
		firstVal = C[c] + getRank_256_counter48(c, firstVal - 1, bwtWithRanks) + 1;
		lastVal = C[c] + getRank_256_counter48(c, lastVal, bwtWithRanks);
	}

	if (firstVal > lastVal) return 0;
	else return lastVal - firstVal + 1;
}

void getCountBoundaries_256_counter48(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal, unsigned int &leftBoundary, unsigned int &rightBoundary) {
	unsigned char c;
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 4 * ((firstVal - 1) / 192), 0, 3);
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 4 * (lastVal / 192), 0, 3);

	while (firstVal <= lastVal && i > 1) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_256_counter48(c, firstVal - 1, bwtWithRanks) + 1;
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 4 * ((firstVal - 1) / 192), 0, 3);
		lastVal = C[c] + getRank_256_counter48(c, lastVal, bwtWithRanks);
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 4 * (lastVal / 192), 0, 3);
		--i;
	}

	if (firstVal <= lastVal) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_256_counter48(c, firstVal - 1, bwtWithRanks) + 1;
		lastVal = C[c] + getRank_256_counter48(c, lastVal, bwtWithRanks);
	}

	leftBoundary = firstVal - 1;
	rightBoundary = lastVal;
}

unsigned int getRank_512_counter40(unsigned char c, unsigned int i, unsigned long long **bwtWithRanks) {

	unsigned int j = i / 448;
	unsigned long long *p = bwtWithRanks[c] + 8 * j;
	unsigned int rank = (*p) & 0x00000000FFFFFFFFULL;
	unsigned int b1 = ((*p) >> 56) & 0x00000000000000FFULL;  // popcount for 128-bit prefix
	unsigned int b2 = b1 + (((*p) >> 48) & 0x00000000000000FFULL);  // popcount for 256-bit prefix
	unsigned int b3 = b2 + (((*p) >> 40) & 0x00000000000000FFULL);  // popcount for 384-bit prefix
	unsigned int temp1, temp2;

	++p;
	i -= (j * 448);

	switch (i / 64) {
	case 6:
		rank += b3 + __builtin_popcountll(*(p + 6) & ((1ULL << (i % 64)) - 1));
		break;
	case 5:
		temp1 = __builtin_popcountll(*(p + 4));
		temp2 = __builtin_popcountll(*(p + 5) & ((1ULL << (i % 64)) - 1));
		rank += b2 + temp1 + temp2;
		break;
	case 4:
		rank += b2 + __builtin_popcountll(*(p + 4) & ((1ULL << (i % 64)) - 1));
		break;
	case 3:
		temp1 = __builtin_popcountll(*(p + 2));
		temp2 = __builtin_popcountll(*(p + 3) & ((1ULL << (i % 64)) - 1));
		rank += b1 + temp1 + temp2;
		break;
	case 2:
		rank += b1 + __builtin_popcountll(*(p + 2) & ((1ULL << (i % 64)) - 1));
		break;
	case 1:
		temp1 = __builtin_popcountll(*p);
		temp2 = __builtin_popcountll(*(p + 1) & ((1ULL << (i % 64)) - 1));
		rank += temp1 + temp2;
		break;
	case 0:
		rank += __builtin_popcountll(*p & ((1ULL << (i % 64)) - 1));
	}

	return rank;
}

unsigned int count_512_counter40(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal){
	unsigned char c;
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 8 * ((firstVal - 1) / 448), 0, 3);
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 8 * (lastVal / 448), 0, 3);

	while (firstVal <= lastVal && i > 1) {
		c = pattern[i - 1];
		if (bwtWithRanks[c] == NULL) return 0;
		firstVal = C[c] + getRank_512_counter40(c, firstVal - 1, bwtWithRanks) + 1;
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 8 * ((firstVal - 1) / 448), 0, 3);
		lastVal = C[c] + getRank_512_counter40(c, lastVal, bwtWithRanks);
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 8 * (lastVal / 448), 0, 3);
		--i;
	}

	if (firstVal <= lastVal) {
		c = pattern[i - 1];
		if (bwtWithRanks[c] == NULL) return 0;
		firstVal = C[c] + getRank_512_counter40(c, firstVal - 1, bwtWithRanks) + 1;
		lastVal = C[c] + getRank_512_counter40(c, lastVal, bwtWithRanks);
	}

	if (firstVal > lastVal) return 0;
	else return lastVal - firstVal + 1;
}

void getCountBoundaries_512_counter40(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned long long **bwtWithRanks, unsigned int firstVal, unsigned int lastVal, unsigned int &leftBoundary, unsigned int &rightBoundary) {
	unsigned char c;
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 8 * ((firstVal - 1) / 448), 0, 3);
	__builtin_prefetch(bwtWithRanks[pattern[i - 1]] + 8 * (lastVal / 448), 0, 3);

	while (firstVal <= lastVal && i > 1) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_512_counter40(c, firstVal - 1, bwtWithRanks) + 1;
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 8 * ((firstVal - 1) / 448), 0, 3);
		lastVal = C[c] + getRank_512_counter40(c, lastVal, bwtWithRanks);
		__builtin_prefetch(bwtWithRanks[pattern[i - 2]] + 8 * (lastVal / 448), 0, 3);
		--i;
	}

	if (firstVal <= lastVal) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_512_counter40(c, firstVal - 1, bwtWithRanks) + 1;
		lastVal = C[c] + getRank_512_counter40(c, lastVal, bwtWithRanks);
	}

	leftBoundary = firstVal - 1;
	rightBoundary = lastVal;
}

unsigned char *encode125(unsigned char* text, unsigned int textLen, vector<unsigned char> selectedChars, unsigned int &encodedTextLen) {
	encodedTextLen = textLen / 3;
	if (textLen % 3 > 0) ++encodedTextLen;
	unsigned char *textEnc125 = new unsigned char[encodedTextLen];

	for (unsigned int i = 0; i < encodedTextLen; ++i) {
		int temp = 0;
		for (int k = 0; k < 3; ++k) {
			bool encoded = false;
			if (3 * i + k < textLen) {
				for (int j = 0; j < 4; ++j) {
					if (text[3 * i + k] == selectedChars[j]) {
						encoded = true;
						temp += j * (int)pow(5.0, (double)k);
						break;
					}
				}
			}
			if (!encoded) {
				temp += 4 * (int)pow(5.0, (double)k);
			}
		}
		textEnc125[i] = (unsigned char)temp;
	}
	return textEnc125;
}

unsigned int occ(unsigned int *a, unsigned int aLen, unsigned int elem) {
	unsigned int occ = 0;
	for (unsigned int i = 0; i < aLen; ++i) if (a[i] == elem) ++occ;
	return occ;
}

void fill125LUT(vector<unsigned char> selectedChars, unsigned int lut[][125]) {
	for (int i = 0; i < 125; ++i) {
		unsigned int first = i % 5;
		unsigned int second = (i / 5) % 5;
		unsigned int third = i / 25;

		unsigned int a[3] = { first, second, third };

		lut[selectedChars[0]][i] = 0;
		lut[selectedChars[1]][i] = 0;
		lut[selectedChars[2]][i] = 0;
		lut[selectedChars[3]][i] = 0;
		lut['N'][i] = 0;

		switch (first) {
		case 0: case 1: case 2: case 3:
			lut[selectedChars[first]][i] = occ(a, 3, first);
			break;
		default:
			lut['N'][i] = occ(a, 3, 4);
		}

		switch (second) {
		case 0: case 1: case 2: case 3:
			lut[selectedChars[second]][i] = occ(a, 3, second);
			break;
		default:
			lut['N'][i] = occ(a, 3, 4);
		}

		switch (third) {
		case 0: case 1: case 2: case 3:
			lut[selectedChars[third]][i] = occ(a, 3, third);
			break;
		default:
			lut['N'][i] = occ(a, 3, 4);
		}
	}
}

unsigned int getRank_512_enc125(unsigned char c, unsigned int i, unsigned char *bwtWithRanks, unsigned int lut[][125]) {
	unsigned int rank;
	unsigned char *p, last;
	unsigned int j = i / 144;
	p = bwtWithRanks + 64 * j;
	switch (c) {
	case 'T':
		p += 4;
	case 'G':
		p += 4;
	case 'C':
		p += 4;
	case 'A':
		memcpy(&rank, p, (size_t)4);
		break;
	default:
		return 0;
	}
	p = bwtWithRanks + 64 * j + 16;
	i -= (j * 144);
	for (unsigned int k = 0; k < i / 3; ++k) {
		rank += lut[c][*p];
		++p;
	}
	switch (i % 3) {
	case 2:
		last = (*p) % 25 + 100;
		rank += lut[c][last];
		break;
	case 1:
		last = (*p) % 5 + 120;
		rank += lut[c][last];
		break;
	}
	return rank;
}

unsigned int count_512_enc125(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned char *bwtWithRanks, unsigned int lut[][125], unsigned int firstVal, unsigned int lastVal) {
	unsigned char c;
	__builtin_prefetch(bwtWithRanks + 64 * ((firstVal - 1) / 144), 0, 3);
	__builtin_prefetch(bwtWithRanks + 64 * (lastVal / 144), 0, 3);

	while (firstVal <= lastVal && i > 1) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_512_enc125(c, firstVal - 1, bwtWithRanks, lut) + 1;
		__builtin_prefetch(bwtWithRanks + 64 * ((firstVal - 1) / 144), 0, 3);
		lastVal = C[c] + getRank_512_enc125(c, lastVal, bwtWithRanks, lut);
		__builtin_prefetch(bwtWithRanks + 64 * (lastVal / 144), 0, 3);
		--i;
	}

	if (firstVal <= lastVal) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_512_enc125(c, firstVal - 1, bwtWithRanks, lut) + 1;
		lastVal = C[c] + getRank_512_enc125(c, lastVal, bwtWithRanks, lut);
	}

	if (firstVal > lastVal) return 0;
	else return lastVal - firstVal + 1;
}

unsigned int getRank_1024_enc125(unsigned char c, unsigned int i, unsigned char *bwtWithRanks, unsigned int lut[][125]) {
	unsigned int rank;
	unsigned char *p, last;
	unsigned int j = i / 336;
	p = bwtWithRanks + 128 * j;
	switch (c) {
	case 'T':
		p += 4;
	case 'G':
		p += 4;
	case 'C':
		p += 4;
	case 'A':
		memcpy(&rank, p, (size_t)4);
		break;
	default:
		return 0;
	}
	p = bwtWithRanks + 128 * j + 16;
	i -= (j * 336);
	for (unsigned int k = 0; k < i / 3; ++k) {
		rank += lut[c][*p];
		++p;
	}
	switch (i % 3) {
	case 2:
		last = (*p) % 25 + 100;
		rank += lut[c][last];
		break;
	case 1:
		last = (*p) % 5 + 120;
		rank += lut[c][last];
		break;
	}
	return rank;
}

unsigned int count_1024_enc125(unsigned char *pattern, unsigned int i, unsigned int *C, unsigned char *bwtWithRanks, unsigned int lut[][125], unsigned int firstVal, unsigned int lastVal){
	unsigned char c;
	__builtin_prefetch(bwtWithRanks + 128 * ((firstVal - 1) / 336), 0, 3);
	__builtin_prefetch(bwtWithRanks + 128 * (lastVal / 336), 0, 3);

	while (firstVal <= lastVal && i > 1) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_1024_enc125(c, firstVal - 1, bwtWithRanks, lut) + 1;
		__builtin_prefetch(bwtWithRanks + 128 * ((firstVal - 1) / 336), 0, 3);
		lastVal = C[c] + getRank_1024_enc125(c, lastVal, bwtWithRanks, lut);
		__builtin_prefetch(bwtWithRanks + 128 * (lastVal / 336), 0, 3);
		--i;
	}

	if (firstVal <= lastVal) {
		c = pattern[i - 1];
		firstVal = C[c] + getRank_1024_enc125(c, firstVal - 1, bwtWithRanks, lut) + 1;
		lastVal = C[c] + getRank_1024_enc125(c, lastVal, bwtWithRanks, lut);
	}

	if (firstVal > lastVal) return 0;
	else return lastVal - firstVal + 1;
}

bool sortCharsCount(unsigned int* i, unsigned int* j) {
	return (i[1] > j[1]);
}

}

#endif	/* FMDUMMY_HPP */

