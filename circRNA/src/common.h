#ifndef __COMMON_H__
#define __COMMON_H__

#define FILE_NAME_LENGTH 1000

typedef struct {
	uint32_t rpos;
	uint32_t qpos;
	uint32_t len;
} fragment_t;

typedef struct {
	fragment_t *frags;
	uint32_t chain_len;
	float score;
} chain_t;

extern bool pairedEnd;

extern int kmer;
extern int verboseMode;

extern char gtfFilename[FILE_NAME_LENGTH];
extern char referenceFilename[FILE_NAME_LENGTH];
extern char fastqFilename[FILE_NAME_LENGTH];
extern char outputFilename[FILE_NAME_LENGTH];
extern char outputDir[FILE_NAME_LENGTH];

extern FILE* outputJuncFile;

extern char versionNumberMajor[10];
extern char versionNumberMinor[10];

#endif
