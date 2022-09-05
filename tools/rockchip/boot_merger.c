/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "boot_merger.h"
#include <u-boot/sha256.h>
#include <time.h>
#include <sys/stat.h>
#include <version.h>

/* #define USE_P_RC4 */

bool gDebug =
#ifdef DEBUG
        true;
#else
        false;
#endif /* DEBUG */

#define ENTRY_ALIGN (2048)
options gOpts;
char gLegacyPath[MAX_LINE_LEN] = { 0 };
char gNewPath[MAX_LINE_LEN] = { 0 };
static char *gPrePath;
char gSubfix[MAX_LINE_LEN] = OUT_SUBFIX;
char gEat[MAX_LINE_LEN];
char *gConfigPath;
uint8_t *gBuf;
bool enableRC4 = false;
bool newMerger = true;

static uint32_t g_merge_max_size = MAX_MERGE_SIZE;

uint32_t gTable_Crc32[256] = {
	0x00000000, 0x04c10db7, 0x09821b6e, 0x0d4316d9, 0x130436dc, 0x17c53b6b,
	0x1a862db2, 0x1e472005, 0x26086db8, 0x22c9600f, 0x2f8a76d6, 0x2b4b7b61,
	0x350c5b64, 0x31cd56d3, 0x3c8e400a, 0x384f4dbd, 0x4c10db70, 0x48d1d6c7,
	0x4592c01e, 0x4153cda9, 0x5f14edac, 0x5bd5e01b, 0x5696f6c2, 0x5257fb75,
	0x6a18b6c8, 0x6ed9bb7f, 0x639aada6, 0x675ba011, 0x791c8014, 0x7ddd8da3,
	0x709e9b7a, 0x745f96cd, 0x9821b6e0, 0x9ce0bb57, 0x91a3ad8e, 0x9562a039,
	0x8b25803c, 0x8fe48d8b, 0x82a79b52, 0x866696e5, 0xbe29db58, 0xbae8d6ef,
	0xb7abc036, 0xb36acd81, 0xad2ded84, 0xa9ece033, 0xa4aff6ea, 0xa06efb5d,
	0xd4316d90, 0xd0f06027, 0xddb376fe, 0xd9727b49, 0xc7355b4c, 0xc3f456fb,
	0xceb74022, 0xca764d95, 0xf2390028, 0xf6f80d9f, 0xfbbb1b46, 0xff7a16f1,
	0xe13d36f4, 0xe5fc3b43, 0xe8bf2d9a, 0xec7e202d, 0x34826077, 0x30436dc0,
	0x3d007b19, 0x39c176ae, 0x278656ab, 0x23475b1c, 0x2e044dc5, 0x2ac54072,
	0x128a0dcf, 0x164b0078, 0x1b0816a1, 0x1fc91b16, 0x018e3b13, 0x054f36a4,
	0x080c207d, 0x0ccd2dca, 0x7892bb07, 0x7c53b6b0, 0x7110a069, 0x75d1adde,
	0x6b968ddb, 0x6f57806c, 0x621496b5, 0x66d59b02, 0x5e9ad6bf, 0x5a5bdb08,
	0x5718cdd1, 0x53d9c066, 0x4d9ee063, 0x495fedd4, 0x441cfb0d, 0x40ddf6ba,
	0xaca3d697, 0xa862db20, 0xa521cdf9, 0xa1e0c04e, 0xbfa7e04b, 0xbb66edfc,
	0xb625fb25, 0xb2e4f692, 0x8aabbb2f, 0x8e6ab698, 0x8329a041, 0x87e8adf6,
	0x99af8df3, 0x9d6e8044, 0x902d969d, 0x94ec9b2a, 0xe0b30de7, 0xe4720050,
	0xe9311689, 0xedf01b3e, 0xf3b73b3b, 0xf776368c, 0xfa352055, 0xfef42de2,
	0xc6bb605f, 0xc27a6de8, 0xcf397b31, 0xcbf87686, 0xd5bf5683, 0xd17e5b34,
	0xdc3d4ded, 0xd8fc405a, 0x6904c0ee, 0x6dc5cd59, 0x6086db80, 0x6447d637,
	0x7a00f632, 0x7ec1fb85, 0x7382ed5c, 0x7743e0eb, 0x4f0cad56, 0x4bcda0e1,
	0x468eb638, 0x424fbb8f, 0x5c089b8a, 0x58c9963d, 0x558a80e4, 0x514b8d53,
	0x25141b9e, 0x21d51629, 0x2c9600f0, 0x28570d47, 0x36102d42, 0x32d120f5,
	0x3f92362c, 0x3b533b9b, 0x031c7626, 0x07dd7b91, 0x0a9e6d48, 0x0e5f60ff,
	0x101840fa, 0x14d94d4d, 0x199a5b94, 0x1d5b5623, 0xf125760e, 0xf5e47bb9,
	0xf8a76d60, 0xfc6660d7, 0xe22140d2, 0xe6e04d65, 0xeba35bbc, 0xef62560b,
	0xd72d1bb6, 0xd3ec1601, 0xdeaf00d8, 0xda6e0d6f, 0xc4292d6a, 0xc0e820dd,
	0xcdab3604, 0xc96a3bb3, 0xbd35ad7e, 0xb9f4a0c9, 0xb4b7b610, 0xb076bba7,
	0xae319ba2, 0xaaf09615, 0xa7b380cc, 0xa3728d7b, 0x9b3dc0c6, 0x9ffccd71,
	0x92bfdba8, 0x967ed61f, 0x8839f61a, 0x8cf8fbad, 0x81bbed74, 0x857ae0c3,
	0x5d86a099, 0x5947ad2e, 0x5404bbf7, 0x50c5b640, 0x4e829645, 0x4a439bf2,
	0x47008d2b, 0x43c1809c, 0x7b8ecd21, 0x7f4fc096, 0x720cd64f, 0x76cddbf8,
	0x688afbfd, 0x6c4bf64a, 0x6108e093, 0x65c9ed24, 0x11967be9, 0x1557765e,
	0x18146087, 0x1cd56d30, 0x02924d35, 0x06534082, 0x0b10565b, 0x0fd15bec,
	0x379e1651, 0x335f1be6, 0x3e1c0d3f, 0x3add0088, 0x249a208d, 0x205b2d3a,
	0x2d183be3, 0x29d93654, 0xc5a71679, 0xc1661bce, 0xcc250d17, 0xc8e400a0,
	0xd6a320a5, 0xd2622d12, 0xdf213bcb, 0xdbe0367c, 0xe3af7bc1, 0xe76e7676,
	0xea2d60af, 0xeeec6d18, 0xf0ab4d1d, 0xf46a40aa, 0xf9295673, 0xfde85bc4,
	0x89b7cd09, 0x8d76c0be, 0x8035d667, 0x84f4dbd0, 0x9ab3fbd5, 0x9e72f662,
	0x9331e0bb, 0x97f0ed0c, 0xafbfa0b1, 0xab7ead06, 0xa63dbbdf, 0xa2fcb668,
	0xbcbb966d, 0xb87a9bda, 0xb5398d03, 0xb1f880b4,
};

uint32_t CRC_32(uint8_t *pData, uint32_t ulSize)
{
	uint32_t i;
	uint32_t nAccum = 0;
	for (i = 0; i < ulSize; i++) {
		nAccum = (nAccum << 8) ^ gTable_Crc32[(nAccum >> 24) ^ (*pData++)];
	}
	return nAccum;
}

void P_RC4(uint8_t *buf, uint32_t len)
{
	uint8_t S[256], K[256], temp;
	uint32_t i, j, t, x;
	uint8_t key[16] = { 124, 78, 3, 4, 85, 5, 9, 7, 45, 44, 123, 56, 23, 13, 23,
	                    17
	                  };

	j = 0;
	for (i = 0; i < 256; i++) {
		S[i] = (uint8_t) i;
		j &= 0x0f;
		K[i] = key[j];
		j++;
	}

	j = 0;
	for (i = 0; i < 256; i++) {
		j = (j + S[i] + K[i]) % 256;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
	}

	i = j = 0;
	for (x = 0; x < len; x++) {
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
		t = (S[i] + (S[j] % 256)) % 256;
		buf[x] = buf[x] ^ S[t];
	}
}

static inline void fixPath(char *path)
{
	int i, len = strlen(path);
	char tmp[MAX_LINE_LEN];
	char *start, *end;

	for (i = 0; i < len; i++) {
		if (path[i] == '\\')
			path[i] = '/';
		else if (path[i] == '\r' || path[i] == '\n')
			path[i] = '\0';
	}

	if (strlen(gLegacyPath) && strlen(gNewPath)) {
		start = strstr(path, gLegacyPath);
		if (start) {
			end = start + strlen(gLegacyPath);
			/* Backup, so tmp can be src for strcat() */
			strcpy(tmp, end);
			/* Terminate, so path can be dest for strcat() */
			*start = '\0';
			strcat(path, gNewPath);
			strcat(path, tmp);
		} else {
			strcpy(tmp, path);
			strcpy(path, gNewPath);
			strcat(path, tmp);
		}
	} else if ((ulong)path != (ulong)gOpts.outPath && /* ignore output */
		    gPrePath && strncmp(path, gPrePath, strlen(gPrePath))) {
		strcpy(tmp, path);
		strcpy(path, gPrePath);
		strcat(path, tmp);
	}
}

static bool parseChip(FILE *file)
{
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_NAME "=%s", gOpts.chip) != 1) {
		return false;
	}
	LOGD("chip:%s\n", gOpts.chip);
	return true;
}

static bool parseVersion(FILE *file)
{
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_MAJOR "=%d", &gOpts.major) != 1)
		return false;
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_MINOR "=%d", &gOpts.minor) != 1)
		return false;
	LOGD("major:%d, minor:%d\n", gOpts.major, gOpts.minor);
	return true;
}

static bool parse471(FILE *file)
{
	int i, index, pos;
	char buf[MAX_LINE_LEN];

	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_NUM "=%d", &gOpts.code471Num) != 1)
		return false;
	LOGD("num:%d\n", gOpts.code471Num);
	if (!gOpts.code471Num)
		return true;
	if (gOpts.code471Num < 0)
		return false;
	gOpts.code471Path = (line_t *)malloc(sizeof(line_t) * gOpts.code471Num);
	for (i = 0; i < gOpts.code471Num; i++) {
		if (SCANF_EAT(file) != 0) {
			return false;
		}
		if (fscanf(file, OPT_PATH "%d=%[^\r^\n]", &index, buf) != 2)
			return false;
		index--;
		fixPath(buf);
		strcpy((char *)gOpts.code471Path[index], buf);
		LOGD("path%i:%s\n", index, gOpts.code471Path[index]);
	}
	pos = ftell(file);
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_SLEEP "=%d", &gOpts.code471Sleep) != 1)
		fseek(file, pos, SEEK_SET);
	LOGD("sleep:%d\n", gOpts.code471Sleep);
	return true;
}

static bool parse472(FILE *file)
{
	int i, index, pos;
	char buf[MAX_LINE_LEN];

	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_NUM "=%d", &gOpts.code472Num) != 1)
		return false;
	LOGD("num:%d\n", gOpts.code472Num);
	if (!gOpts.code472Num)
		return true;
	if (gOpts.code472Num < 0)
		return false;
	gOpts.code472Path = (line_t *)malloc(sizeof(line_t) * gOpts.code472Num);
	for (i = 0; i < gOpts.code472Num; i++) {
		if (SCANF_EAT(file) != 0) {
			return false;
		}
		if (fscanf(file, OPT_PATH "%d=%[^\r^\n]", &index, buf) != 2)
			return false;
		fixPath(buf);
		index--;
		strcpy((char *)gOpts.code472Path[index], buf);
		LOGD("path%i:%s\n", index, gOpts.code472Path[index]);
	}
	pos = ftell(file);
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_SLEEP "=%d", &gOpts.code472Sleep) != 1)
		fseek(file, pos, SEEK_SET);
	LOGD("sleep:%d\n", gOpts.code472Sleep);
	return true;
}

static bool parseLoader(FILE *file)
{
	int i, j, index, pos;
	char buf[MAX_LINE_LEN];
	char buf2[MAX_LINE_LEN];

	if (SCANF_EAT(file) != 0) {
		return false;
	}
	pos = ftell(file);
	if (fscanf(file, OPT_NUM "=%d", &gOpts.loaderNum) != 1) {
		fseek(file, pos, SEEK_SET);
		if (fscanf(file, OPT_LOADER_NUM "=%d", &gOpts.loaderNum) != 1) {
			return false;
		}
	}
	LOGD("num:%d\n", gOpts.loaderNum);
	if (!gOpts.loaderNum)
		return false;
	if (gOpts.loaderNum < 0)
		return false;
	gOpts.loader = (name_entry *)malloc(sizeof(name_entry) * gOpts.loaderNum);
	for (i = 0; i < gOpts.loaderNum; i++) {
		if (SCANF_EAT(file) != 0) {
			return false;
		}
		if (fscanf(file, OPT_LOADER_NAME "%d=%s", &index, buf) != 2)
			return false;
		index--;
		strcpy(gOpts.loader[index].name, buf);
		LOGD("name%d:%s\n", index, gOpts.loader[index].name);
	}
	for (i = 0; i < gOpts.loaderNum; i++) {
		if (SCANF_EAT(file) != 0) {
			return false;
		}
		if (fscanf(file, "%[^=]=%[^\r^\n]", buf, buf2) != 2)
			return false;
		for (j = 0; j < gOpts.loaderNum; j++) {
			if (!strcmp(gOpts.loader[j].name, buf)) {
				fixPath(buf2);
				strcpy(gOpts.loader[j].path, buf2);
				LOGD("%s=%s\n", gOpts.loader[j].name, gOpts.loader[j].path);
				break;
			}
		}
		if (j >= gOpts.loaderNum) {
			return false;
		}
	}
	return true;
}

static bool parseOut(FILE *file)
{
	fpos_t prev;

	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_OUT_PATH "=%[^\r^\n]", gOpts.outPath) != 1)
		return false;
	/* fixPath(gOpts.outPath); */
	printf("out:%s\n", gOpts.outPath);

	if (fgetpos(file, &prev))
		return false;
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_IDB_PATH "=%[^\r^\n]", gOpts.idbPath) != 1) {
		if (fsetpos(file, &prev))
			return false;
	} else {
		LOGD("idbPath:%s\n", gOpts.idbPath);
	}

	return true;
}

static bool parseBool(const char *buf, bool *result)
{
	if (!strcasecmp(buf, "true")) {
		*result = true;
		return true;
	} else if (!strcasecmp(buf, "false")) {
		*result = false;
		return true;
	}
	return false;
}

static bool parseSystem(FILE *file)
{
	char buf[MAX_LINE_LEN];

	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_NEW_IDB "=%[^\r^\n]", buf) != 1)
		return false;
	if (!parseBool(buf, &gOpts.newIDB)) {
		LOGE("non bool value for NEWIDB: %s\n", buf);
		return false;
	}
	LOGD("newIDB:%s\n", gOpts.newIDB ? "true" : "false");
	return true;
}

static bool parseFlag(FILE *file)
{
	char buf[MAX_LINE_LEN];
	fpos_t prev;

	if (fgetpos(file, &prev))
		return false;
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_471_RC4_OFF "=%[^\r^\n]", buf) != 1) {
		if (fsetpos(file, &prev))
			return false;
	} else {
		if (!parseBool(buf, &gOpts.rc4Off471)) {
			LOGE("non bool value for 471_RC4_OFF: %s\n", buf);
			return false;
		}
		LOGD("rc4Off471:%s\n", gOpts.rc4Off471 ? "true" : "false");
	}

	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_RC4_OFF "=%[^\r^\n]", buf) != 1)
		return false;
	if (!parseBool(buf, &gOpts.rc4Off)) {
		LOGE("non bool value for RC4_OFF: %s\n", buf);
		return false;
	}
	LOGD("rc4Off:%s\n", gOpts.rc4Off ? "true" : "false");

	if (fgetpos(file, &prev))
		return false;
	if (SCANF_EAT(file) != 0) {
		return false;
	}
	if (fscanf(file, OPT_CREATE_IDB "=%[^\r^\n]", buf) != 1) {
		if (fsetpos(file, &prev))
			return false;
	} else {
		if (!parseBool(buf, &gOpts.createIDB)) {
			LOGE("non bool value for CREATE_IDB: %s\n", buf);
			return false;
		}
		LOGD("createIDB:%s\n", gOpts.createIDB ? "true" : "false");
	}

	return true;
}

static bool parseBootParam(FILE *file, uint32_t *param, int count)
{
	char buf[MAX_LINE_LEN], *p;
	char pattern[32];

	if (SCANF_EAT(file) != 0) {
		return false;
	}
	for (int i = 0; i < count; i++) {
		if (SCANF_EAT(file) != 0) {
			return false;
		}
		snprintf(pattern, sizeof(pattern), OPT_WORD "%d=%%[^\r^\n]", i);
		if (fscanf(file, pattern, buf) != 1)
			return false;
		param[i] = strtol(buf, &p, 0);
		if (*p) {
			LOGE("non int value for WORD_%d: %s\n", i, buf);
			return false;
		}
		LOGD("WORD_%d:0x%x\n", i, param[i]);
	}
	return true;
}

void printOpts(FILE *out)
{
	uint32_t i;
	fprintf(out, SEC_CHIP "\n" OPT_NAME "=%s\n", gOpts.chip);
	fprintf(out, SEC_VERSION "\n" OPT_MAJOR "=%d\n" OPT_MINOR "=%d\n",
	        gOpts.major, gOpts.minor);

	fprintf(out, SEC_471 "\n" OPT_NUM "=%d\n", gOpts.code471Num);
	for (i = 0; i < gOpts.code471Num; i++) {
		fprintf(out, OPT_PATH "%d=%s\n", i + 1, gOpts.code471Path[i]);
	}
	if (gOpts.code471Sleep > 0)
		fprintf(out, OPT_SLEEP "=%d\n", gOpts.code471Sleep);

	fprintf(out, SEC_472 "\n" OPT_NUM "=%d\n", gOpts.code472Num);
	for (i = 0; i < gOpts.code472Num; i++) {
		fprintf(out, OPT_PATH "%d=%s\n", i + 1, gOpts.code472Path[i]);
	}
	if (gOpts.code472Sleep > 0)
		fprintf(out, OPT_SLEEP "=%d\n", gOpts.code472Sleep);

	fprintf(out, SEC_LOADER "\n" OPT_NUM "=%d\n", gOpts.loaderNum);
	for (i = 0; i < gOpts.loaderNum; i++) {
		fprintf(out, OPT_LOADER_NAME "%d=%s\n", i + 1, gOpts.loader[i].name);
	}
	for (i = 0; i < gOpts.loaderNum; i++) {
		fprintf(out, "%s=%s\n", gOpts.loader[i].name, gOpts.loader[i].path);
	}

	fprintf(out, SEC_OUT "\n" OPT_OUT_PATH "=%s\n", gOpts.outPath);
}

static bool parseOpts_from_file(void)
{
	bool ret = false;
	bool chipOk = false;
	bool versionOk = false;
	bool code471Ok = true;
	bool code472Ok = true;
	bool loaderOk = false;
	bool outOk = false;
	bool systemOk = false;
	bool flagOk = false;
	bool boot0ParamOk = false;
	bool boot1ParamOk = false;
	char buf[MAX_LINE_LEN];

	char *configPath = (gConfigPath == NULL) ? DEF_CONFIG_FILE : gConfigPath;
	FILE *file;
	file = fopen(configPath, "r");
	if (!file) {
		fprintf(stderr, "config(%s) not found!\n", configPath);
		if (configPath == (char *)DEF_CONFIG_FILE) {
			file = fopen(DEF_CONFIG_FILE, "w");
			if (file) {
				fprintf(stderr, "create defconfig\n");
				printOpts(file);
			}
		}
		goto end;
	}

	LOGD("start parse\n");

	if (SCANF_EAT(file) != 0) {
		goto end;
	}
	while (fscanf(file, "%s", buf) == 1) {
		if (!strcmp(buf, SEC_CHIP)) {
			chipOk = parseChip(file);
			if (!chipOk) {
				LOGE("parseChip failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_VERSION)) {
			versionOk = parseVersion(file);
			if (!versionOk) {
				LOGE("parseVersion failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_471)) {
			code471Ok = parse471(file);
			if (!code471Ok) {
				LOGE("parse471 failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_472)) {
			code472Ok = parse472(file);
			if (!code472Ok) {
				LOGE("parse472 failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_LOADER)) {
			loaderOk = parseLoader(file);
			if (!loaderOk) {
				LOGE("parseLoader failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_OUT)) {
			outOk = parseOut(file);
			if (!outOk) {
				LOGE("parseOut failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_SYSTEM)) {
			systemOk = parseSystem(file);
			if (!systemOk) {
				LOGE("parseSystem failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_FLAG)) {
			flagOk = parseFlag(file);
			if (!flagOk) {
				LOGE("parseFlag failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_BOOT0_PARAM)) {
			boot0ParamOk = parseBootParam(file, gOpts.boot0Param, 10);
			if (!boot0ParamOk) {
				LOGE("parseBoot0Param failed!\n");
				goto end;
			}
		} else if (!strcmp(buf, SEC_BOOT1_PARAM)) {
			boot1ParamOk = parseBootParam(file, gOpts.boot1Param, 8);
			if (!boot1ParamOk) {
				LOGE("parseBoot1Param failed!\n");
				goto end;
			}
		} else if (buf[0] == '#') {
			continue;
		} else {
			LOGE("unknown sec: %s!\n", buf);
			goto end;
		}
		if (SCANF_EAT(file) != 0) {
			goto end;
		}
	}

	if (chipOk && versionOk && code471Ok && code472Ok && loaderOk && outOk)
		ret = true;
end:
	if (file)
		fclose(file);
	return ret;
}

static bool parseOpts_from_cmdline(int argc, char **argv)
{
	int i;
	int tag = 0;
	int v0, v1, v2, v3;

	for (i = 2; i < argc; i++) {
		if (!strcmp(OPT_471, argv[i])) {
			i++;
			snprintf(gOpts.code471Path[0], sizeof(gOpts.code471Path[0]), "%s",
			         argv[i]);
			tag |= 1;
		} else if (!strcmp(OPT_472, argv[i])) {
			i++;
			snprintf(gOpts.code472Path[0], sizeof(gOpts.code472Path[0]), "%s",
			         argv[i]);
			tag |= 2;
		} else if (!strcmp(OPT_DATA, argv[i])) {
			i++;
			snprintf(gOpts.loader[0].path, sizeof(gOpts.loader[0].path), "%s",
			         argv[i]);
			tag |= 4;
		} else if (!strcmp(OPT_BOOT, argv[i])) {
			i++;
			snprintf(gOpts.loader[1].path, sizeof(gOpts.loader[1].path), "%s",
			         argv[i]);
			tag |= 8;
		} else if (!strcmp(OPT_OUT, argv[i])) {
			i++;
			snprintf(gOpts.outPath, sizeof(gOpts.outPath), "%s", argv[i]);
			tag |= 0x10;
		} else if (!strcmp(OPT_CHIP, argv[i])) {
			i++;
			snprintf(gOpts.chip, sizeof(gOpts.chip), "%s", argv[i]);
			tag |= 0x20;
		} else if (!strcmp(OPT_VERSION, argv[i])) {
		}
	}

	sscanf(gOpts.loader[0].path, "%*[^v]v%d.%d.bin", &v0, &v1);
	sscanf(gOpts.loader[1].path, "%*[^v]v%d.%d.bin", &v2, &v3);
	gOpts.major = v2;
	gOpts.minor = v3;
	snprintf(gOpts.outPath, sizeof(gOpts.outPath),
	         "%s_loader_v%d.%02d.%d%02d.bin", gOpts.chip, v0, v1, v2, v3);
	return ((tag & 0x0f) == 0x0f) ? true : false;
}

bool initOpts(int argc, char **argv)
{
	bool ret;

	/* set default opts */
	gOpts.major = DEF_MAJOR;
	gOpts.minor = DEF_MINOR;
	strcpy(gOpts.chip, DEF_CHIP);
	gOpts.code471Sleep = DEF_CODE471_SLEEP;
	gOpts.code472Sleep = DEF_CODE472_SLEEP;
	gOpts.code471Num = DEF_CODE471_NUM;
	gOpts.code471Path = (line_t *)malloc(sizeof(line_t) * gOpts.code471Num);
	strcpy((char *)gOpts.code471Path[0], DEF_CODE471_PATH);
	gOpts.code472Num = DEF_CODE472_NUM;
	gOpts.code472Path = (line_t *)malloc(sizeof(line_t) * gOpts.code472Num);
	strcpy((char *)gOpts.code472Path[0], DEF_CODE472_PATH);
	gOpts.loaderNum = DEF_LOADER_NUM;
	gOpts.loader = (name_entry *)malloc(sizeof(name_entry) * gOpts.loaderNum);
	strcpy(gOpts.loader[0].name, DEF_LOADER0);
	strcpy(gOpts.loader[0].path, DEF_LOADER0_PATH);
	strcpy(gOpts.loader[1].name, DEF_LOADER1);
	strcpy(gOpts.loader[1].path, DEF_LOADER1_PATH);
	strcpy(gOpts.outPath, DEF_OUT_PATH);

	if (argc > 10)
		ret = parseOpts_from_cmdline(argc, argv);
	else
		ret = parseOpts_from_file();

	return ret;
}

/************merge code****************/

static inline uint32_t getBCD(unsigned short value)
{
	uint8_t tmp[2] = { 0 };
	int i;
	uint32_t ret;
	if (value > 0xFFFF) {
		return 0;
	}
	for (i = 0; i < 2; i++) {
		tmp[i] = (((value / 10) % 10) << 4) | (value % 10);
		value /= 100;
	}
	ret = ((uint16_t)(tmp[1] << 8)) | tmp[0];

	LOGD("ret:%x\n", ret);
	return ret & 0xFF;
}

static inline void str2wide(const char *str, uint16_t *wide, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		wide[i] = (uint16_t) str[i];
	}
}

static inline void getName(char *path, uint16_t *dst)
{
	char *end;
	char *start;
	int len;
	if (!path || !dst)
		return;
	start = strrchr(path, '/');
	if (!start)
		start = path;
	else
		start++;
	end = newMerger ? strchr(path, '.') : strrchr(path, '.');
	if (!end)
		end = path + strlen(path);
	len = end - start;
	if (len >= MAX_NAME_LEN)
		len = newMerger ? MAX_NAME_LEN : MAX_NAME_LEN - 1;
	str2wide(start, dst, len);

	if (gDebug) {
		char name[MAX_NAME_LEN + 1];
		memset(name, 0, sizeof(name));
		memcpy(name, start, len);
		LOGD("path:%s, name:%s\n", path, name);
	}
}

static inline bool getFileSize(const char *path, uint32_t *size)
{
	struct stat st;
	if (stat(path, &st) < 0)
		return false;
	*size = st.st_size;
	LOGD("path:%s, size:%d\n", path, *size);
	return true;
}

static inline rk_time getTime(void)
{
	rk_time rkTime;

	struct tm *tm;
	time_t tt = time(NULL);
	tm = localtime(&tt);
	rkTime.year = tm->tm_year + 1900;
	rkTime.month = tm->tm_mon + 1;
	rkTime.day = tm->tm_mday;
	rkTime.hour = tm->tm_hour;
	rkTime.minute = tm->tm_min;
	rkTime.second = tm->tm_sec;
	LOGD("%d-%d-%d %02d:%02d:%02d\n", rkTime.year, rkTime.month, rkTime.day,
	     rkTime.hour, rkTime.minute, rkTime.second);
	return rkTime;
}

static bool writeFile(FILE *outFile, const char *path, bool fix, bool rc4Off)
{
	bool ret = false;
	uint32_t size = 0, fixSize = 0;
	uint8_t *buf;

	FILE *inFile = fopen(path, "rb");
	if (!inFile)
		goto end;

	if (!getFileSize(path, &size))
		goto end;
	if (fix) {
		fixSize = ((size - 1) / SMALL_PACKET + 1) * SMALL_PACKET;
		uint32_t tmp = fixSize % ENTRY_ALIGN;
		tmp = tmp ? (ENTRY_ALIGN - tmp) : 0;
		fixSize += tmp;
		memset(gBuf, 0, fixSize);
	} else {
		memset(gBuf, 0, size + ENTRY_ALIGN);
	}
	if (!fread(gBuf, size, 1, inFile))
		goto end;

	if (fix) {

		buf = gBuf;
		size = fixSize;
		if (!rc4Off)
			while (1) {
				P_RC4(buf, fixSize < SMALL_PACKET ? fixSize : SMALL_PACKET);
				buf += SMALL_PACKET;
				if (fixSize <= SMALL_PACKET)
					break;
				fixSize -= SMALL_PACKET;
			}
	} else {
		uint32_t tmp = size % ENTRY_ALIGN;
		tmp = tmp ? (ENTRY_ALIGN - tmp) : 0;
		size += tmp;
		if (!rc4Off)
			P_RC4(gBuf, size);
	}

	if (!fwrite(gBuf, size, 1, outFile))
		goto end;
	ret = true;
end:
	if (inFile)
		fclose(inFile);
	if (!ret)
		LOGE("write entry(%s) failed\n", path);
	return ret;
}

static bool saveEntry(FILE *outFile, char *path, rk_entry_type type,
                      uint16_t delay, uint32_t *offset, char *fixName,
                      bool fix)
{
	LOGD("write:%s\n", path);
	uint32_t size;
	rk_boot_entry entry;
	memset(&entry, 0, sizeof(rk_boot_entry));

	LOGD("write:%s\n", path);

	getName(fixName ? fixName : path, entry.name);
	entry.size = sizeof(rk_boot_entry);
	entry.type = type;
	entry.dataOffset = *offset;
	if (!getFileSize(path, &size)) {
		LOGE("save entry(%s) failed:\n\tcannot get file size.\n", path);
		return false;
	}
	if (fix)
		size = ((size - 1) / SMALL_PACKET + 1) * SMALL_PACKET;
	uint32_t tmp = size % ENTRY_ALIGN;
	size += tmp ? (ENTRY_ALIGN - tmp) : 0;
	LOGD("align size:%d\n", size);
	entry.dataSize = size;
	entry.dataDelay = delay;
	*offset += size;
	fwrite(&entry, sizeof(rk_boot_entry), 1, outFile);
	return true;
}

static inline uint32_t convertChipType(const char *chip)
{
	char buffer[5];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s", chip);
	return buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

static inline uint32_t getChipType(const char *chip)
{
	LOGD("chip:%s\n", chip);
	int chipType = RKNONE_DEVICE;
	if (!chip) {
		goto end;
	}
	if (!strcmp(chip, CHIP_RK28)) {
		chipType = RK28_DEVICE;
	} else if (!strcmp(chip, CHIP_RK28)) {
		chipType = RK28_DEVICE;
	} else if (!strcmp(chip, CHIP_RK281X)) {
		chipType = RK281X_DEVICE;
	} else if (!strcmp(chip, CHIP_RKPANDA)) {
		chipType = RKPANDA_DEVICE;
	} else if (!strcmp(chip, CHIP_RK27)) {
		chipType = RK27_DEVICE;
	} else if (!strcmp(chip, CHIP_RKNANO)) {
		chipType = RKNANO_DEVICE;
	} else if (!strcmp(chip, CHIP_RKSMART)) {
		chipType = RKSMART_DEVICE;
	} else if (!strcmp(chip, CHIP_RKCROWN)) {
		chipType = RKCROWN_DEVICE;
	} else if (!strcmp(chip, CHIP_RKCAYMAN)) {
		chipType = RKCAYMAN_DEVICE;
	} else if (!strcmp(chip, CHIP_RK29)) {
		chipType = RK29_DEVICE;
	} else if (!strcmp(chip, CHIP_RK292X)) {
		chipType = RK292X_DEVICE;
	} else if (!strcmp(chip, CHIP_RK30)) {
		chipType = RK30_DEVICE;
	} else if (!strcmp(chip, CHIP_RK30B)) {
		chipType = RK30B_DEVICE;
	} else if (!strcmp(chip, CHIP_RK31)) {
		chipType = RK31_DEVICE;
	} else if (!strcmp(chip, CHIP_RK32)) {
		chipType = RK32_DEVICE;
	} else {
		chipType = convertChipType(chip + 2);
	}

end:
	LOGD("type:0x%x\n", chipType);
	if (chipType == RKNONE_DEVICE) {
		LOGE("chip type not support!\n");
	}
	return chipType;
}

static inline void getBoothdr(rk_boot_header *hdr)
{
	memset(hdr, 0, sizeof(rk_boot_header));
	hdr->tag = gOpts.newIDB ? TAG_NEW : TAG;
	hdr->size = sizeof(rk_boot_header);
	hdr->version = newMerger ? (gOpts.major << 8) | gOpts.minor :
		(getBCD(gOpts.major) << 8) | getBCD(gOpts.minor);
	hdr->mergerVersion = newMerger ? MERGER_VERSION_NEW : MERGER_VERSION;
	hdr->releaseTime = getTime();
	hdr->chipType = getChipType(gOpts.chip);

	hdr->code471Num = gOpts.code471Num;
	hdr->code471Offset = sizeof(rk_boot_header);
	hdr->code471Size = sizeof(rk_boot_entry);

	hdr->code472Num = gOpts.code472Num;
	hdr->code472Offset = hdr->code471Offset + gOpts.code471Num * hdr->code471Size;
	hdr->code472Size = sizeof(rk_boot_entry);

	hdr->loaderNum = gOpts.loaderNum;
	hdr->loaderOffset = hdr->code472Offset + gOpts.code472Num * hdr->code472Size;
	hdr->loaderSize = sizeof(rk_boot_entry);
	if (newMerger ? gOpts.rc4Off : !enableRC4)
		hdr->rc4Flag = 1;
	if (gOpts.newIDB && gOpts.rc4Off471)
		hdr->rc4_471Flag = 1;
}

static inline uint32_t getCrc(const char *path)
{
	uint32_t size = 0;
	uint32_t crc = 0;
	FILE *file = fopen(path, "rb");
	getFileSize(path, &size);
	if (!file)
		goto end;
	if (!fread(gBuf, size, 1, file))
		goto end;
	crc = CRC_32(gBuf, size);
	LOGD("crc:0x%08x\n", crc);
end:
	if (file)
		fclose(file);
	return crc;
}

struct newidb_file {
	uint16_t ofs, size;
	uint32_t unk_ff, unk_0, idx, unk1, unk2;
	uint8_t hash[64];
};
struct newidb_data {
	uint32_t tag;
	uint32_t zero;
	uint32_t size;
	uint32_t unk_1;
	uint32_t unk[8];
	uint32_t boot0Param[10];
	uint32_t boot1Param[8];
	struct newidb_file files[16];
	uint8_t unk2[8];
	uint8_t hash[64];
};

static bool createNewIDB(int num, char **paths, char *outPath)
{
	struct newidb_data idb;
	long fileSize;
	FILE *f;

	memset(&idb, 0, sizeof(idb));
	idb.tag = BLOCK_TAG;
	idb.size = (num << 16) + 0x180;
	idb.unk_1 = 1;
	memcpy(idb.boot0Param, gOpts.boot0Param, sizeof(idb.boot0Param));
	memcpy(idb.boot1Param, gOpts.boot1Param, sizeof(idb.boot1Param));

	int ofs = gOpts.keepCert ? 8 : 4;

	for (int i = 0; i < num; i++) {
		FILE *f;
		char *path = paths[i];
		if (!(f = fopen(path, "rb"))) {
			LOGE("open %s failed\n", path);
			return false;
		}
		if (fseek(f, 0, SEEK_END) == -1 ||
			(fileSize = ftell(f)) == -1 ||
			fseek(f, 0, SEEK_SET) == -1) {
			LOGE("seek %s failed\n", path);
			fclose(f);
			return false;
		}
		int blocks = ((fileSize - 1) >> 11) + 1;
		int bufsize = blocks << 11;
		void *buf = malloc(bufsize);
		memset(buf, 0, bufsize);
		if (!fread(buf, 1, fileSize, f)) {
			LOGE("read %s failed\n", path);
			fclose(f);
			return false;
		}
		fclose(f);
		idb.files[i].ofs = ofs;
		idb.files[i].size = blocks * 4;
		idb.files[i].unk_ff = 0xffffffff;
		idb.files[i].unk_0 = 0;
		idb.files[i].idx = i + 1;
		sha256_csum(buf, bufsize, idb.files[i].hash);
		free(buf);
		ofs += blocks * 4;
	}
	sha256_csum((void *)&idb, sizeof(idb) - sizeof(idb.hash), idb.hash);
	f = fopen(outPath, "wb");
	if (!f) {
		LOGE("cannot create %s\n", outPath);
		return false;
	}
	fwrite(&idb, 1, sizeof(idb), f);
	fclose(f);
	return true;
}

static bool mergeBoot(int argc, char **argv)
{
	uint32_t dataOffset;
	bool ret = false;
	int i;
	FILE *outFile;
	uint32_t crc;
	rk_boot_header hdr;

	if (!initOpts(argc, argv))
		return false;
	{
		char *subfix = strstr(gOpts.outPath, OUT_SUBFIX);
		char version[MAX_LINE_LEN];
		snprintf(version, sizeof(version), "%s", gSubfix);
		if (subfix && !strcmp(subfix, OUT_SUBFIX)) {
			subfix[0] = '\0';
		}
		strcat(gOpts.outPath, version);
		printf("fix opt:%s\n", gOpts.outPath);
	}

	if (gDebug) {
		printf("---------------\nUSING CONFIG:\n");
		printOpts(stdout);
		printf("---------------\n\n");
	}

	if (gOpts.newIDB) {
		int num = gOpts.code471Num + gOpts.code472Num;
		char **paths;
		void *newBuf;

		if (!(paths = malloc(num * sizeof(char *))))
			return false;
		for (int i = 0; i < gOpts.code471Num; i++)
			paths[i] = gOpts.code471Path[i];
		for (int i = 0; i < gOpts.code472Num; i++)
			paths[i + gOpts.code471Num] = gOpts.code472Path[i];
		if (!createNewIDB(num, paths, "UsbHead.bin")) {
			free(paths);
			return false;
		}
		free(paths);

		if (!(newBuf = realloc(gOpts.code471Path,
			(gOpts.code471Num + 1)* sizeof(line_t))))
			return false;
		gOpts.code471Path = newBuf;
		memmove(gOpts.code471Path + 1, gOpts.code471Path,
			gOpts.code471Num * sizeof(line_t));
		gOpts.code471Num++;
		strcpy(gOpts.code471Path[0], "UsbHead.bin");

		if (!(paths = malloc(gOpts.loaderNum * sizeof(char *))))
			return false;
		for (int i = 0; i < gOpts.loaderNum; i++)
			paths[i] = gOpts.loader[i].path;
		if (!createNewIDB(num, paths, "FlashHead.bin")) {
			free(paths);
			return false;
		}
		free(paths);

		if (!(newBuf = realloc(gOpts.loader,
			(gOpts.loaderNum + 1)* sizeof(name_entry))))
			return false;
		gOpts.loader = newBuf;
		memmove(gOpts.loader + 1, gOpts.loader,
			gOpts.loaderNum * sizeof(name_entry));
		gOpts.loaderNum++;
		strcpy(gOpts.loader[0].name, "FlashHead");
		strcpy(gOpts.loader[0].path, "FlashHead.bin");
	}

	outFile = fopen(gOpts.outPath, "wb+");
	if (!outFile) {
		LOGE("open out file(%s) failed\n", gOpts.outPath);
		goto end;
	}

	getBoothdr(&hdr);
	LOGD("write hdr\n");
	fwrite(&hdr, 1, sizeof(rk_boot_header), outFile);

	dataOffset = sizeof(rk_boot_header) +
	             (gOpts.code471Num + gOpts.code472Num + gOpts.loaderNum) *
	             sizeof(rk_boot_entry);

	LOGD("write code 471 entry\n");
	for (i = 0; i < gOpts.code471Num; i++) {
		if (!saveEntry(outFile, (char *)gOpts.code471Path[i], ENTRY_471,
		               gOpts.code471Sleep, &dataOffset, NULL, false))
			goto end;
	}
	LOGD("write code 472 entry\n");
	for (i = 0; i < gOpts.code472Num; i++) {
		if (!saveEntry(outFile, (char *)gOpts.code472Path[i], ENTRY_472,
		               gOpts.code472Sleep, &dataOffset, NULL, false))
			goto end;
	}
	LOGD("write loader entry\n");
	for (i = 0; i < gOpts.loaderNum; i++) {
		if (!saveEntry(outFile, gOpts.loader[i].path, ENTRY_LOADER, 0, &dataOffset,
		               gOpts.loader[i].name, true))
			goto end;
	}

	LOGD("write code 471\n");
	for (i = 0; i < gOpts.code471Num; i++) {
		if (!writeFile(outFile, (char *)gOpts.code471Path[i], false, gOpts.rc4Off471))
			goto end;
	}
	LOGD("write code 472\n");
	for (i = 0; i < gOpts.code472Num; i++) {
		if (!writeFile(outFile, (char *)gOpts.code472Path[i], false, gOpts.rc4Off))
			goto end;
	}
	LOGD("write loader\n");
	for (i = 0; i < gOpts.loaderNum; i++) {
		if (!writeFile(outFile, gOpts.loader[i].path, true, false))
			goto end;
	}
	fflush(outFile);

	LOGD("write crc\n");
	crc = getCrc(gOpts.outPath);
	if (!fwrite(&crc, sizeof(crc), 1, outFile))
		goto end;

	ret = true;
end:
	if (outFile)
		fclose(outFile);
	if (gOpts.newIDB) {
		remove("UsbHead.bin");
		remove("FlashHead.bin");
	}
	return ret;
}

/************merge code end************/
/************unpack code***************/

static inline void wide2str(const uint16_t *wide, char *str, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		str[i] = (char)(wide[i] & 0xFF);
	}
}

static bool unpackEntry(rk_boot_entry *entry, const char *name, FILE *inFile)
{
	bool ret = false;
	int size, i;
	FILE *outFile = fopen(name, "wb+");
	if (!outFile)
		goto end;
	printf("unpack entry(%s)\n", name);
	fseek(inFile, entry->dataOffset, SEEK_SET);
	size = entry->dataSize;
	if (!fread(gBuf, size, 1, inFile))
		goto end;
	if (entry->type == ENTRY_LOADER) {
		for (i = 0; i < size / SMALL_PACKET; i++)
			P_RC4(gBuf + i * SMALL_PACKET, SMALL_PACKET);
		if (size % SMALL_PACKET) {
			P_RC4(gBuf + i * SMALL_PACKET, size - SMALL_PACKET * 512);
		}
	} else {
		P_RC4(gBuf, size);
	}
	if (!fwrite(gBuf, size, 1, outFile))
		goto end;
	ret = true;
end:
	if (outFile)
		fclose(outFile);
	return ret;
}

static bool unpackBoot(char *path)
{
	bool ret = false;
	FILE *inFile = fopen(path, "rb");
	int entryNum, i;
	char name[MAX_NAME_LEN];
	rk_boot_entry *entrys;
	if (!inFile) {
		fprintf(stderr, "loader(%s) not found\n", path);
		goto end;
	}

	rk_boot_header hdr;
	if (!fread(&hdr, sizeof(rk_boot_header), 1, inFile)) {
		fprintf(stderr, "read header failed\n");
		goto end;
	}

	entryNum = hdr.code471Num + hdr.code472Num + hdr.loaderNum;
	entrys = (rk_boot_entry *)malloc(sizeof(rk_boot_entry) * entryNum);
	if (!fread(entrys, sizeof(rk_boot_entry) * entryNum, 1, inFile)) {
		fprintf(stderr, "read data failed\n");
		goto end;
	}

	LOGD("entry num:%d\n", entryNum);
	for (i = 0; i < entryNum; i++) {
		wide2str(entrys[i].name, name, MAX_NAME_LEN);

		LOGD("entry:t=%d, name=%s, off=%d, size=%d\n", entrys[i].type, name,
		     entrys[i].dataOffset, entrys[i].dataSize);
		if (!unpackEntry(entrys + i, name, inFile)) {
			fprintf(stderr, "unpack entry(%s) failed\n", name);
			goto end;
		}
	}

	ret = true;
end:
	if (inFile)
		fclose(inFile);
	return ret;
}

/************unpack code end***********/

static void printHelp(void)
{
	printf("Usage1: boot_merger [options]... FILE\n");
	printf("Merge or unpack Rockchip's loader (Default action is to merge.)\n");
	printf("Options:\n");
	printf("\t" OPT_MERGE "\t\t\tMerge loader with specified config.\n");
	printf("\t" OPT_UNPACK "\t\tUnpack specified loader to current dir.\n");
	printf("\t" OPT_VERBOSE "\t\tDisplay more runtime informations.\n");
	printf("\t" OPT_HELP "\t\t\tDisplay this information.\n");
	printf("\t" OPT_VERSION "\t\tDisplay version information.\n");
	printf("\t" OPT_SUBFIX "\t\tSpec subfix.\n");
	printf("\t" OPT_REPLACE "\t\tReplace some part of binary path.\n");
	printf("\t" OPT_PREPATH "\t\tAdd prefix path of binary path.\n");
	printf("\t" OPT_SIZE
	       "\t\tImage size.\"--size [image KB size]\", must be 512KB aligned\n");
	printf("Usage2: boot_merger [options] [parameter]\n");
	printf("All below five option are must in this mode!\n");
	printf("\t" OPT_CHIP "\t\tChip type, used for check with usbplug.\n");
	printf("\t" OPT_471 "\t\t471 for download, ddr.bin.\n");
	printf("\t" OPT_472 "\t\t472 for download, usbplug.bin.\n");
	printf("\t" OPT_DATA "\t\tloader0 for flash, ddr.bin.\n");
	printf("\t" OPT_BOOT "\t\tloader1 for flash, miniloader.bin.\n");
	printf("\n./tools/boot_merger --pack --verbose -c RK322A -1 "
	       "rkbin/rk322x_ddr_300MHz_v1.04.bin -2 "
	       "rkbin/rk32/rk322x_usbplug_v2.32.bin -d "
	       "rkbin/rk32/rk322x_ddr_300MHz_v1.04.bin -b "
	       "rkbin/rk32/rk322x_miniloader_v2.32.bin\n");
}

int main(int argc, char **argv)
{

	int i;
	bool merge = true;
	char *optPath = NULL;

	for (i = 1; i < argc; i++) {
		if (!strcmp(OPT_VERBOSE, argv[i])) {
			gDebug = true;
			printf("enable debug\n");
		} else if (!strcmp(OPT_HELP, argv[i])) {
			printHelp();
			return 0;
		} else if (!strcmp(OPT_VERSION, argv[i])) {
			printf("boot_merger (cjf@rock-chips.com)\t" VERSION "\n");
			return 0;
		} else if (!strcmp(OPT_MERGE, argv[i])) {
			merge = true;
		} else if (!strcmp(OPT_UNPACK, argv[i])) {
			merge = false;
		} else if (!strcmp(OPT_RC4, argv[i])) {
			printf("enable RC4 for IDB data(both ddr and preloader)\n");
			enableRC4 = true;
		} else if (!strcmp(OPT_SUBFIX, argv[i])) {
			i++;
			snprintf(gSubfix, sizeof(gSubfix), "%s", argv[i]);
		} else if (!strcmp(OPT_REPLACE, argv[i])) {
			i++;
			snprintf(gLegacyPath, sizeof(gLegacyPath), "%s", argv[i]);
			i++;
			snprintf(gNewPath, sizeof(gNewPath), "%s", argv[i]);
		} else if (!strcmp(OPT_PREPATH, argv[i])) {
			i++;
			gPrePath = argv[i];
		} else if (!strcmp(OPT_SIZE, argv[i])) {
			g_merge_max_size = strtoul(argv[++i], NULL, 10);
			if (g_merge_max_size % 512) {
				printHelp();
				return -1;
			}
			g_merge_max_size *= 1024; /* bytes */
		} else {
			optPath = argv[i];
			break;
		}
	}
	if (!merge && !optPath) {
		fprintf(stderr, "need set out path to unpack!\n");
		printHelp();
		return -1;
	}

	gBuf = calloc(g_merge_max_size, 1);
	if (!gBuf) {
		LOGE("Merge image: calloc buffer error.\n");
		return -1;
	}

	if (merge) {
		LOGD("do_merge\n");
		gConfigPath = optPath;
		if (!mergeBoot(argc, argv)) {
			fprintf(stderr, "merge failed!\n");
			return -1;
		}
		printf("merge success(%s)\n", gOpts.outPath);
	} else {
		LOGD("do_unpack\n");
		if (!unpackBoot(optPath)) {
			fprintf(stderr, "unpack failed!\n");
			return -1;
		}
		printf("unpack success\n");
	}
	return 0;
}
