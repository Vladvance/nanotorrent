#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

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
#define _GNU_SOURCE
#define _OE_SOCKETS

struct Info {
    uint32_t piecelen; //piece length
    char *pieces; //concatenated SHA1 values of pieces
    char *name; //filename
    uint32_t length; //length of the file in bytes
};

struct Metainfo {
    struct Info info;
    char infohash[20];
    char *announce; //announce URL of the tracker
};

struct Handshake {
    char reserved[8];
    char info_hash[20];
    char peer_id[21];
};

struct Peer {
    char peerid[21];
    uint32_t IPv4;
    uint16_t port;
    uint32_t uploaded;
    uint32_t downloaded;
    uint32_t left;
};

int generatePeerId();
int generateMetainfo(const char* originFileName, struct Metainfo* mi);
int saveToTorrentFile(const char* torrFileName, struct Metainfo* mi);
int readFromTorrentFile(const char* torrFileName, struct Metainfo* mi);
void freeMetainfo(struct Metainfo* mi);

int openClientSocket(struct addrinfo *client);
int connectToServer(struct Metainfo* mi, int clientSocketFD);
int processServer(int serverFD, struct Metainfo* mi, struct Peer* peers);
int connectToPeer(struct Peer* peer, int clientSocketFD);
int processPeer(int fd, struct Metainfo* mi);


//helper functions
int randint(int n);

void testApp() {
    struct Metainfo mi, mi2;
    //struct Peer peers[PEER_NUM];
    int clientSocketFD, serverFD, status;
    //int peerFD[PEER_NUM];
    struct addrinfo* client = NULL;
    generateMetainfo("WaP.txt", &mi);

    saveToTorrentFile("WaP.torrent", &mi);
    readFromTorrentFile("WaP.torrent", &mi2);

    clientSocketFD = openClientSocket(client);

    status = connectToServer(&mi, clientSocketFD);
    if(status == -1) perror("connectToServer");

    const char* str = "Hello Fucking World\n";
    status = send(clientSocketFD, str, strlen(str), 0);
    if(status == -1) perror("send");
    //sendRequestToServer(serverFD, &mi, &peers[0]);

//    for(int i = 0; i < PEER_NUM; i++) {
//        peerFD[i] = connectToPeer(&peers[i]);
//    }

    freeMetainfo(&mi);
    freeMetainfo(&mi2);
    freeaddrinfo(client);
    close(clientSocketFD);
    close(serverFD);
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
    mi->announce = strdup("127.0.0.1");

    mi->info.piecelen = PIECE_LENGTH;
    mi->info.name = strdup(originFileName);
    mi->info.length = stat_buf.st_size;

    //size needed for pieces field (Npieces * 40 + 1)
    int pieces_s = ceil((float)mi->info.length/PIECE_LENGTH)*40+1;
    mi->info.pieces = (char*)malloc(pieces_s);
    mi->info.pieces[pieces_s] = '\0';

    printf("File: %s, Size: %u\n", mi->info.name, mi->info.length);
    int k = 1;
    char *pieces_pt = &mi->info.pieces[0];

    do {
        int n = fread(ibuf, 1, PIECE_LENGTH, origin);

        if(ferror(origin) || n == 0) {
            perror("fread");
            exit(1);
        }

        SHA1(ibuf, n, obuf);

        unsigned char* sha_pt = &sha1_buf[0];
        for(int i = 0; i < 20; i++) {
            sha_pt += sprintf((char*)sha_pt, "%02x", obuf[i]);
        }
        sha1_buf[40] = '\0';

        strcpy(pieces_pt, (char*)sha1_buf);
        pieces_pt+=40;

        printf("SHA1 for %3d piece (size %7d is: %s\n", k++, n, sha1_buf);
    }
    while (!feof(origin));

    fclose(origin);
    return 0;
}

void freeMetainfo(struct Metainfo *mi)
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
    fprintf(dest,string,size);
    fclose(dest);
    return 0;
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


int openClientSocket(struct addrinfo *client) {
    int sockfd, yes=1;
    struct addrinfo hints, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, DEFAULT_PORT, &hints, &client)) != 0) {
        fprintf(stderr, "openClientSocket: getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    // loop through all the results and connect to the first we can
    for(p = client; p != NULL; p = p->ai_next) {
        struct sockaddr_in *sin = (struct sockaddr_in*)p->ai_addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sin->sin_addr), ip_str, INET_ADDRSTRLEN);
        printf("Opening socket: %s:%d\n", ip_str, ntohs(sin->sin_port));

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
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
        freeaddrinfo(client);
        return -1;
    }

    freeaddrinfo(client);
//    client = p;

    return sockfd;
}


int connectToServer(struct Metainfo* mi, int clientSocketFD) {
    struct addrinfo hints, *serv, *p;
    int serverFD, status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(mi->announce, "1234", &hints, &serv)) != 0) {
        fprintf(stderr, "openClientSocket: getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for(p = serv; p != NULL; p = p->ai_next) {

        struct sockaddr_in *sin = (struct sockaddr_in*)p->ai_addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sin->sin_addr), ip_str, INET_ADDRSTRLEN);
        printf("Connectinng to server: %s:%d\n", ip_str, ntohs(sin->sin_port));

        if ((status = connect(clientSocketFD, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("connectToServer: connect");
            continue;
        }

        break;
    }

    freeaddrinfo(serv);
    return 0;
}


int processServer(int servFD, struct Metainfo* mi) {

    return 0;
}


void addToQuery(char* query, char* key, void* value, int len) {
    strcat(query, "?");
    strcat(query, key);
    strcat(query, "=");
}


/* Converts an integer value to its hex character*/
char to_hex(char code) {
    static char hex[] = "0123456789abcdef";
    return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
    char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
            *pbuf++ = *pstr;
        else if (*pstr == ' ')
            *pbuf++ = '+';
        else
            *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}


int generateRequestToServer(struct Metainfo* mi, char* res) {
    res = strdup(mi->announce);
    char infohash_str[21];
    sprintf(infohash_str, "%s", mi->infohash);
    strcat(res, "?infohash=");

    strcat(res, url_encode(mi->infohash));
    free(res);
    return 0;
}


int connectToPeer(struct Peer* peer, int clientSocketFD) {
    char s[INET_ADDRSTRLEN];
    struct addrinfo hints, *peerinfo, *p;
    int peerFD, status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;

    char port_str[5], ip_str[16];
    sprintf(port_str, "%d", peer->port);
    inet_ntop(AF_INET, &(peer->IPv4), ip_str, INET_ADDRSTRLEN);

    if ((status = getaddrinfo(ip_str, port_str, &hints, &peerinfo)) != 0) {
        fprintf(stderr, "connectToPeer: getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for(p = peerinfo; p != NULL; p = p->ai_next) {
        if ((peerFD = connect(clientSocketFD, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("connectToPeer: connect");
            continue;
        }
        break;
    }

    inet_ntop(p->ai_family, (struct sockaddr_in*)p->ai_addr, s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(peerinfo);
    return peerFD;
}

int processPeer(int fd, struct Metainfo* mi) {
    return 0;
}

int sendHandshake(int fd) {
    printf("Sending Handshake");
    return fd;
}


int main(int argc, char* argv[])
{
    srand(time(NULL));
    testApp();
    return 0;
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
