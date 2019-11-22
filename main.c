#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <openssl/sha.h>

#include "bencode.h"

#define NUM_PEERS 50
#define PIECE_LENGTH 262144 //256 KiB = 2^18 bytes
#define BLOCK_LENGTH 32768 //32 KiB = 2^15 bytes
#define DEFAULT_PORT "6969"
#define PEERID_PREFIX "-NT0001-"
#define PEER_NUM 30

struct Info {
    uint32_t piecelen; //piece length
    char *pieces; //concatenated SHA1 values of pieces
    char *name; //filename
    uint32_t length; //length of the file in bytes
};

struct Metainfo {
    struct Info info;
    char *announce; //announce URL of the tracker
};

struct Handshake {
    char reserved[8];
    char info_hash[41];
    char peer_id[20];
};

int generateMetainfo(const char* originFileName, struct Metainfo* mi);
int saveToTorrentFile(const char* torrFileName, struct Metainfo* mi);
int readFromTorrentFile(const char* torrFileName, struct Metainfo* mi);
int connectToServer(struct Metainfo* mi);
int sendRequestToServer(int serverFD, struct Metainfo* mi, struct Peer* peers);
int connectToPeer(struct Peer* peer);


void testApp() {
    struct Metainfo mi, mi2;
    struct Peer peers[PEER_NUM];
    int serverFD, peerFD[PEER_NUM];
    generateMetainfo("WaP.txt", &mi);

    saveToTorrentFile("WaP.torrent", &mi);
    readFromTorrentFile("WaP.torrent", &mi2);

    serverFD = connectToServer(&mi);
    sendRequestToServer(serverFD, &mi, &peers[0]);

    for(int i = 0; i < PEER_NUM; i++) {
        peerFD[i] = connectToPeer(&peers[i]);
    }
}


int sendHandshake(int fd) {
    printf("Sending Handshake");
    return fd;
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

int generatePeerId(char *peer_id)
{
    strcpy(peer_id, PEERID_PREFIX);
    sprintf(&peer_id[8], "%d%d", randint(1000000), randint(1000000));
    printf("Generated peerid: %s\n", peer_id);
    return 0;
}

int generateMetainfo(const char *originFileName, struct Metainfo *mi)
{
    unsigned char ibuf[PIECE_LENGTH], obuf[20], sha1_buf[41];

    FILE *origin = fopen(originFileName, "r+");

    if(origin == NULL) {
        perror("fopen");
        exit(1);
    }

    struct stat stat_buf;

    int rc = stat(originFileName, &stat_buf);

    if(rc != 0) {
        perror("stat");
        exit(1);
    }

    mi->info.piecelen = PIECE_LENGTH;
    mi->info.name = strdup(originFileName);
    mi->info.length = stat_buf.st_size;

    //size needed for pieces field (Npieces * 20 + )
    int pieces_s = ceil((float)mi->info.length/PIECE_LENGTH)*40+1;
    mi->info.pieces = (char*)malloc(pieces_s);
    mi->info.pieces[pieces_s-1] = '\0';

    printf("File: %s, Size: %u\n", mi->info.name, mi->info.length);
    int k = 1, n;
    char *pieces_pt = &mi->info.pieces[0];

    do {
        n = fread(ibuf, 1, PIECE_LENGTH, origin);

        if(ferror(origin) || n == 0) {
            perror("fread");
            exit(1);
        }

        SHA1(ibuf, n, obuf);

        for(int i = 0; i < 20; i++) {
            sprintf((char*)(sha1_buf+2*i), "%02x", obuf[i]);
        }
        sha1_buf[40] = '\0';
        
        strcpy(pieces_pt, obuf);
        pieces_pt+=40;

        printf("SHA1 for %3d piece (size %7d is:", k++, n);

        for (int i = 0; i < 20; i++) {
            printf("%02x ", obuf[i]);
        }

        printf("\n");
    }
    while (!feof(origin));

    fclose(origin);
}

static void freeMetainfo(struct Metainfo *mi)
{
    free(mi->info.name);
    free(mi->info.pieces);
}

int saveToTorrentFile(const char *destName, struct Metainfo *mi)
{
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
    char *string = BE_MALLOC(size+1);
    size = be_encode(meta, string, size);
    string[size] = '\0';
    printf("%s\n", string);

    be_free(meta);
    FILE *dest = fopen(destName, "w");

    if(dest == NULL) {
        perror("fopen");
        exit(1);
    }
    putline(string,size,dest);
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
    if (metastr)
        free(metastr);
        exit(0);

   be_node_t *metainfo; 
   size = be_decode(metainfo, metastr, size);

   mi->announce = be_dict_lookup_cstr(metainfo,"announce");
   
   be_node_t *info = be_dict_lookup(metainfo,"info", &&info);
   mi->info.length = be_dict_lookup_cstr(info,"length");
   mi->info.name = be_dict_lookup_cstr(info,"name");
   mi->info.piecelen = be_dict_lookup_num(info,"piece length");
   mi->info.pieces = be_dict_lookup_cstr(info,"pieces");
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int openSocket(struct addrinfo *client) {
    int sockfd, yes=1;
    struct addrinfo hints, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, DEFAULT_PORT, &hints, &client)) != 0) {
        fprintf(stderr, "openSocket: getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    // loop through all the results and connect to the first we can
    for(p = client; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to open socket\n");
        return -1;
    }

    freeaddrinfo(client);

    return sockfd;
}

int connectToPeer(char* ip, char* port, int sockfd, struct addrinfo *client) {
    char s[INET6_ADDRSTRLEN];
    struct addrinfo hints, *peerinfo;
    int fd, status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(ip, DEFAULT_PORT, &hints, &peerinfo)) != 0) {
        fprintf(stderr, "connectToPeer: getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    if ((fd = connect(sockfd, peerinfo->ai_addr, peerinfo->ai_addrlen)) == -1) {
        perror("client: connect");
        close(sockfd);
    }

    inet_ntop(peerinfo->ai_family, get_in_addr((struct sockaddr *)peerinfo->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(peerinfo);
    return fd;
}


int main(int argc, char* argv[])
{
    srand(time(NULL));
    struct Metainfo mi;
    struct addrinfo client;
    generateMetainfo("WaP.txt", &mi);
    //    saveToTorrentFile(&mi, "test.torrent");
    freeMetainfo(&mi);

    char peer_id[20];
    generatePeerId(&peer_id[0]);
    int socketfd = openSocket(&client);
    connectToPeer("localhost", DEFAULT_PORT, socketfd, &client);



    return 0;
}
