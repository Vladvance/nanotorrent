#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <libgen.h>
#include <unistd.h>
#include <pthread.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <openssl/sha.h>

#include "bencode.h"
#include "client.h"


const char* TORRENT_STATUS_STR[] = {"Seeding", "Downloading", "Paused", "Stopped", "Failed"};

//helper functions
int randint(int n);
void strip_ext(char *fname);
void setBIt(char* bitmap, int n, int index);
char* url_encode(char* str);

int sendall(int dest_fd, char* buf, int *len);

int openClientSocket();
int connectToTracker(char* trackerAddr, int clientFd);
void* torrentRoutine(void*);
//void* trackerRoutine(void*);
int prepareTrackerRequest(char* req, struct Metainfo* mi);
void* peerRoutine(void*);

void runApp(struct AppData *app) {
    pthread_t torrent_threads[64];
    printf("Starting app\n");

//    if(app->activeTorrentFlags == 0) return;
//    for(int i = 0; i < 64; i++) {
//        if(app->activeTorrentFlags & 1<<i) {
//            pthread_create(&torrent_threads[i], NULL, torrentRoutine, NULL);
//        }
//    }
}

int getEmptyTorrentSlot(struct AppData* app) {
    if(!~app->activeTorrentFlags) return -1;
    return __builtin_clz(~(app->activeTorrentFlags));
}

void* torrentRoutine(void* arg){
    pthread_t peer_threads[32];
    struct TorrentState* ts = (struct TorrentState*)arg;

    for(int i = 0; i < 32; i++) {
        if(ts->peers->IPv4 != 0) {
            pthread_create(&peer_threads[i], NULL, peerRoutine, (void*)ts);
        }
    }
    return NULL;
}

int prepareTrackerRequest(char* req, struct Metainfo* mi) {
    static const char* info_hash_str = "/?info_hash=";
    static const char* peer_id_str = "&peer_id=";
    static const char* port_str = "&port=";
    static const char* uploaded_str = "&uploaded=";
    static const char* downloaded_str = "&downloaded=";
    static const char* left_str = "&left=";

    strcpy(req, "GET ");
    char* req_ptr = &req[4];

    strcpy(req_ptr, mi->announce);
    req_ptr += strlen(mi->announce);

    strcpy(req_ptr, info_hash_str);
    req_ptr += strlen(info_hash_str);
    char* infohash = url_encode(mi->infohash);
    strcpy(req_ptr, infohash);
    req_ptr += strlen(infohash);
    free(infohash);

    strcpy(req_ptr, peer_id_str);
    req_ptr += strlen(peer_id_str);
    char* peerid = url_encode("-NT100-TESTPEERID");
    strcpy(req_ptr, peerid);
    req_ptr += strlen(peerid);
    free(peerid);

    strcpy(req_ptr, port_str);
    req_ptr += strlen(port_str);
    const char* port_val = "6969";
    strcpy(req_ptr, port_val);
    req_ptr += strlen(port_val);

    strcpy(req_ptr, uploaded_str);
    req_ptr += strlen(uploaded_str);
    const char* uploaded_val = "200";
    strcpy(req_ptr, uploaded_val);
    req_ptr += strlen(uploaded_val);

    strcpy(req_ptr, downloaded_str);
    req_ptr += strlen(downloaded_str);
    const char* downloaded_val = "200";
    strcpy(req_ptr, downloaded_val);
    req_ptr += strlen(downloaded_val);


    strcpy(req_ptr, left_str);
    req_ptr += strlen(left_str);
    const char* left_val = "200";
    strcpy(req_ptr, left_val);
    req_ptr += strlen(left_val);

    strcpy(req_ptr, " HTTP/1.1");
    fprintf(stderr, "REQUEST: %s\n", req);

    return 0;
}

void* peerRoutine(void* torrentState) {
//    struct TorrentState *ts = (struct TorrentState*) torrentState;

    int clientFd = openClientSocket();
//    int trackerFd = connectToTracker("localhost", clientFd);
    const char* msg = "Hello World!\n";
    int len = strlen(msg);
    sendall(clientFd, (char*)msg, &len);
    //char* torrentData = getDataFromTracker(socketFd);
    return NULL;
}

int generatePeerId(char *peer_id)
{
    srand(getpid());
    strcpy(peer_id, PEERID_PREFIX);
    sprintf(&peer_id[8], "%06d%06d", randint(1000000), randint(1000000));
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

    mi->announce = strdup("localhost");
    mi->info.piecelen = PIECE_LENGTH;
    mi->info.name = strdup(basename((char*)originFilePath));
    mi->info.length = stat_buf.st_size;

    //size needed for pieces field (Npieces * 40 + 1)

    int pieces_s = ceil((float)mi->info.length/PIECE_LENGTH)*40+1;
    mi->info.pieces = (char*)malloc(pieces_s);
    mi->info.pieces[pieces_s-1] = '\0';

    fprintf(stderr, "File: %s, Size: %ld\n", mi->info.name, mi->info.length);
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
        fprintf(stderr, "SHA1 for %3d piece (size %7d) is:", k++, n);
        for (int i = 0; i < 20; i++) {
            fprintf(stderr, "%02x ", obuf[i]);
        }
        fprintf(stderr, "\n");
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

    be_free(meta);
    char buf[500];

    strcpy(buf, destPath);
    strcat(buf, "/");
    strcat(buf, mi->info.name);
    strip_ext(buf);
    strcat(buf, ".torrent");

    FILE *dest = fopen(buf, "w+");
    if(dest == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(dest, "%s", string);
    free(string);
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

// strip file extension from the path
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


// initiate TorrentState struct depending on Metainfo and current torrent status
void initTorrentState(struct TorrentState *ts, const char* filePath)
{
    ts->pieceCount = ceil((float)ts->mi.info.length/(float)ts->mi.info.piecelen);
    ts->bitmapLength = ceil((float)ts->pieceCount/32.0);
    ts->bitmap = (uint32_t*)calloc(ts->bitmapLength, sizeof(uint32_t));
    ts->filePath = strdup(filePath);

    switch (ts->status) {
    case SEEDING: {
        //set all bits to 1
        memset((void*)ts->bitmap, ~0, ts->bitmapLength * sizeof(uint32_t));

        ts->bytesDownloaded = ts->mi.info.length;
        ts->activePeersCount = 0;
    }
    break;
    default: {
        generatePiecesBitmap(ts);
    }}
}

//generate pieces presence bitmap comparing hashes from <pieces> field with computed hashes
int generatePiecesBitmap(struct TorrentState *ts) {

    unsigned char ibuf[PIECE_LENGTH], obuf[20];

    FILE *origin = fopen(ts->filePath, "r+");
    if(origin == NULL) {
        perror("fopen:");
        exit(1);
    }

    struct stat stat_buf;
    int rc = stat(ts->filePath, &stat_buf);
    if(rc != 0) {
        perror("stat:");
        exit(1);
    }

    int length = stat_buf.st_size;

    //size needed for pieces field (Npieces * 40 + 1)

    uint32_t pieceCount = ceil((float)length/PIECE_LENGTH)*20;
    if(pieceCount != ts->pieceCount) {
        fprintf(stderr, "ERROR: Piece count is not consistent with file size\n");
        return -1;
    }
    if(strlen(ts->mi.info.pieces) != pieceCount * 40) {
        fprintf(stderr, "ERROR: <piece> field is not consistent with piece count\n");
        return -1;
    }

    int pieceidx = 0;
    char* pieces_pt = &(ts->mi.info.pieces[0]);
    unsigned char test_buf[20];


    do {
        int n = fread(ibuf, 1, PIECE_LENGTH, origin);
        if(ferror(origin) || n == 0) {
            perror("fread");
            exit(1);
        }

        SHA1(ibuf, n, obuf);

        for (int i = 0; i < 20; i++) {
            scanf(pieces_pt, "%02x", &(test_buf[i]));
            pieces_pt +=2;
        }
        if(memcmp(obuf, test_buf, 20) == 0) {
            int bitmap_idx = pieceidx/sizeof(uint32_t);
            int bitmap_bitidx = pieceidx%sizeof(uint32_t);
            ts->bitmap[bitmap_idx] |= 1<<(sizeof(uint32_t) - bitmap_bitidx - 1);
        }
        pieceidx++;
    } while (!feof(origin));
    fclose(origin);

    return 0;
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

int openClientSocket() {
    struct addrinfo* client;
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


int connectToTracker(char* trackerAddr, int clientSocketFD) {
    struct addrinfo hints, *serv, *p;
    int serverFD, status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(trackerAddr, "1234", &hints, &serv)) != 0) {
        fprintf(stderr, "connectToTracker: getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for(p = serv; p != NULL; p = p->ai_next) {

        struct sockaddr_in *sin = (struct sockaddr_in*)p->ai_addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sin->sin_addr), ip_str, INET_ADDRSTRLEN);
        printf("Connectinng to server: %s:%d\n", ip_str, ntohs(sin->sin_port));

        if ((status = connect(clientSocketFD, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("connectToTracker: connect");
            continue;
        }

        break;
    }

    freeaddrinfo(serv);
    return 0;
}

int sendall(int dst, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(dst, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
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
#ifdef __cplusplus
}
#endif

