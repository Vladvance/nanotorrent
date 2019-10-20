#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/sha.h>

#include "bencode.h"

#define NUM_PEERS 50
#define PIECE_LENGTH 262144 

struct Info {
	uint32_t piecelen; //piece length
	char* pieces; //concatenated SHA1 values of pieces
	char* name; //filename
	uint32_t length; //length of the file in bytes
};

struct Metainfo {
	struct Info info;
	const char* announce = "hlvl.pl/announce; //announce URL of the tracker
};

struct Handshake {
};

static void generateMetainfo(char* originFileName, struct Metainfo *mi) {
	unsigned char ibuf[PIECE_LENGTH], obuf[20];
	
	FILE *origin = fopen(originFileName, "r+");
	if(origin == NULL) {
		perror("fopen:");
		exit(1);
	}

	struct stat stat_buf;
	int rc = stat(originFileName, &stat_buf);
	if(rc != 0) {
		perror("stat:");
	       	exit(1);
	}

	mi->info.piecelen = PIECE_LENGTH;
	mi->info.name = strdup(originFileName);
	mi->info.length = stat_buf.st_size;
	
	//size needed for pieces field (Npieces * 20 + )
	int pieces_s = ceil((float)mi->info.length/PIECE_LENGTH)*20+1;
	mi->info.pieces = (char*)malloc(pieces_s);
	mi->info.pieces[pieces_s-1] = '\0';

	printf("File: %s, Size: %u\n", mi->info.name, mi->info.length);
	int k = 1, n;
	char* pieces_pt = &mi->info.pieces[0];
	do {
		n = fread(ibuf, 1, PIECE_LENGTH, origin);
		if(ferror(origin) || n == 0) {
			perror("fread");
			exit(1);
		}
		SHA1(ibuf, n, obuf);
		memcpy(pieces_pt, obuf, 20);
		pieces_pt+=20;

		printf("SHA1 for %3d piece (size %7d is:", k++, n);
		for (int i = 0; i < 20; i++) {
			printf("%02x ", obuf[i]);
		}
		printf("\n");
	} while (!feof(origin));
	for (int i = 0; i < pieces_s; i++) {
		printf("%02x ", mi->info.pieces[i]);
	}
	printf("\n");
	fclose(origin);
}

static void saveToTorrentFile(struct Metainfo *mi, unsigned char* destName) {

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

	be_free(meta);
	FILE *dest = fopen(destName, "w");
	
	if(dest == NULL) {
		perror("fopen");
		exit(1);
		}
	fclose(dest);
}

static void freeMetainfo(struct Metainfo *mi) {
	free(mi->info.name);
	free(mi->info.pieces);
}

int main(void) 
{
    struct Metainfo mi;
    generateMetainfo("WaP.txt", &mi);
    saveToTorrentFile(&mi, "test.torrent");
    freeMetainfo(&mi);

    return 0;
}
