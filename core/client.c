#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <libgen.h>
#include <unistd.h>


#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/sha.h>

#include "bencode.h"
#include "client.h"

//helper functions
int randint(int n);
void strip_ext(char *fname);
void setBIt(char* bitmap, int n, int index);

int generatePeerId(char *peer_id)
{
    strcpy(peer_id, PEERID_PREFIX);
    sprintf(&peer_id[8], "%d%d", randint(1000000), randint(1000000));
    printf("Generated peerid: %s\n", peer_id);
    return 0;
}

void generateInfoHash(struct Metainfo *mi) {
    be_node_t *info;
    info = be_alloc(DICT);
    be_dict_add_num(info, "length", mi->info.length);
    be_dict_add_str(info, "name", mi->info.name);
    be_dict_add_num(info, "piece length", mi->info.piecelen);
    be_dict_add_str(info, "pieces", mi->info.pieces);

    int size = be_encode(info, NULL, 0);
    char* string = BE_MALLOC(size+1);
    size = be_encode(info, string, size);

    SHA1((const unsigned char*) string, size, mi->infohash);
    BE_FREE(string);
    BE_FREE(info);
}

void generateMetainfo(const char *originFilePath, struct Metainfo *mi) {
	unsigned char ibuf[PIECE_LENGTH], obuf[20];
	
    FILE *origin = fopen(originFilePath, "r+");
	if(origin == NULL) {
		perror("fopen:");
		exit(1);
	}

	struct stat stat_buf;
    int rc = stat(originFilePath, &stat_buf);
	if(rc != 0) {
		perror("stat:");
	       	exit(1);
	}

    strcpy(mi->announce, "localhost");
	mi->info.piecelen = PIECE_LENGTH;
    mi->info.name = strdup(basename((char*)originFilePath));
	mi->info.length = stat_buf.st_size;
	
    //size needed for pieces field (Npieces * 40 + )
    int pieces_s = ceil((float)mi->info.length/PIECE_LENGTH)*40+1;
	mi->info.pieces = (char*)malloc(pieces_s);
	mi->info.pieces[pieces_s-1] = '\0';

	printf("File: %s, Size: %u\n", mi->info.name, mi->info.length);
    int k = 1;
	char* pieces_pt = &mi->info.pieces[0];

	do {
        int n = fread(ibuf, 1, PIECE_LENGTH, origin);
		if(ferror(origin) || n == 0) {
			perror("fread");
			exit(1);
		}

		SHA1(ibuf, n, obuf);

        for (int i = 0; i < 20; i++) {
            sprintf(pieces_pt, "%02x", obuf[i]);
            pieces_pt +=2;
        }
        printf("SHA1 for %3d piece (size %7d) is:", k++, n);
        for (int i = 0; i < 20; i++) {
            printf("%02x ", obuf[i]);
        }
        printf("\n");
	} while (!feof(origin));
	fclose(origin);

    generateInfoHash(mi);
}

void saveToTorrentFile(struct Metainfo *mi, const char *destPath) {

	be_node_t *info, *meta;
	meta = be_alloc(DICT);
	info = be_alloc(DICT);
    be_dict_add_str(meta, "announce", mi->announce);
	be_dict_add(meta, "info", info);

	be_dict_add_num(info, "length", mi->info.length);
	be_dict_add_str(info, "name", mi->info.name);
    be_dict_add_num(info, "piece length", mi->info.piecelen);
	be_dict_add_str(info, "pieces", mi->info.pieces);

	int size = be_encode(meta, NULL, 0);
	char* string = BE_MALLOC(size+1);
	size = be_encode(meta, string, size);
	string[size] = '\0';
	printf("%s\n", string);

    BE_FREE(meta);
    char buf[100];

    strcpy(buf, mi->info.name);
    strip_ext(buf);
    strcat((char*)destPath, "/");
    strcat((char*)destPath, buf);
    strcat((char*)destPath, ".torrent");
    printf("%s", destPath);

    FILE *dest = fopen(destPath, "a+");
    if(dest == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(dest, "%s", string);
    BE_FREE(string);
	fclose(dest);
}

int readFromTorrentFile(const char *srcname, struct Metainfo *mi)
{
    char* metastr = NULL;
    size_t len = 0;
    ssize_t size;

    FILE* src = fopen(srcname, "r");

    if(src == NULL) {
        perror("readFromTorrentFile: fopen");
        exit(1);
    }

    size = getline(&metastr, &len, src);

    fclose(src);
    if (!metastr) {
        printf("readFormTorrentFile: error reading file, file is empty\n");
        free(metastr);
        exit(0);
    }

    be_node_t *metainfo;
    size_t readlen;
    metainfo = be_decode(metastr, size, &readlen);

    mi->announce = strdup(be_dict_lookup_cstr(metainfo,"announce"));

    be_node_t *info = be_dict_lookup(metainfo,"info",NULL);
    mi->info.length = (uint32_t)be_dict_lookup_num(info,"length");
    mi->info.name = strdup(be_dict_lookup_cstr(info,"name"));
    mi->info.piecelen = be_dict_lookup_num(info,"piece length");
    mi->info.pieces = strdup(be_dict_lookup_cstr(info,"pieces"));
    be_free(metainfo);
    return 0;
}

void freeMetainfo(struct Metainfo *mi) {
	free(mi->info.name);
	free(mi->info.pieces);
    free(mi->announce);
}


//pretend to generate uniform number
int randint(int n)
{
    if ((n - 1) == RAND_MAX) {
        return rand();
    } else {
        assert (n <= RAND_MAX);

        int end = RAND_MAX / n;
        assert (end > 0);
        end *= n;
        int r;

        while ((r = rand()) >= end);

        return r % n;
    }
}

void strip_ext(char *fname)
{
    char *end = fname + strlen(fname);

    while (end > fname && *end != '.' && *end != '\\' && *end != '/') {
        --end;
    }
    if ((end > fname && *end == '.') &&
        (*(end - 1) != '\\' && *(end - 1) != '/')) {
        *end = '\0';
    }
}


void initTorrentState(struct TorrentState *ts)
{
    int pieceCount = ceil(ts->mi.info.length/ts->mi.info.piecelen);
    ts->bitmap = (uint32_t*)malloc(sizeof(uint32_t) * ceil(pieceCount/sizeof(uint32_t)));
}

void freeTorrentState(struct TorrentState *ts)
{
    free(ts->bitmap);
    freeMetainfo(&(ts->mi));
}

float getPercentDownloaded(uint32_t *bitmap, int pieceCount) {
    int bitCount = 0;
    for(int i = 0; i < ceil(pieceCount/32.0); i++) {
        bitCount += __builtin_popcount(bitmap[i]);
    }
    return  (float)bitCount/(float)pieceCount;
}

void setBIt(char* bitmap, int n, int index) {
    if(index >= n) return;
    bitmap[index/32] &= (1<<(32-(index%32)));
}

int getRequiredPieceIndex(uint32_t *bitmap, int n)
{
    for(int i = 0; i < n; i++){
        if(bitmap[i] == 0xFFFF) continue;
        return __builtin_clz(~bitmap[i]);
    }
    return -1;
}

#ifdef __cplusplus
}
#endif

