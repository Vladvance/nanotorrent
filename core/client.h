#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLIENT_H
#define ClIENT_H

#include <stdint.h>

#define NUM_PEERS 50
#define PIECE_LENGTH 262144 //256 KiB = 2^18 bytes
#define BLOCK_LENGTH 32768 //32 KiB = 2^15 bytes
#define DEFAULT_PORT "6969"
#define PEERID_PREFIX "-NT0001-"
#define PEER_NUM 30
#define _OE_SOCKETS


struct Info {
    uint32_t piecelen; //piece length
    char *pieces; //concatenated SHA1 values of pieces
    char *name; //filename
    uint64_t length; //length of the file in bytes
};

struct Metainfo {
    struct Info info;
    unsigned char infohash[20];
    char *announce; //announce URL of the tracker
};

struct Handshake {
    char reserved[8];
    unsigned char info_hash[20];
    char peer_id[21];
};

enum PeerStatus {
    AM_CHOKING = (1<<0), AM_INTERESTED = (1<<2), PEER_CHOKING = (1<<3), PEER_INTERESTED = (1<<4)
};

enum TorrentStatus {
    SEEDING, DOWNLOADING, PAUSED, STOPPED, FAILED
};

extern const char* TORRENT_STATUS_STR[];

struct DataFlowInfo {
    uint32_t uploaded;
    uint32_t downloaded;
    uint32_t left;
};

struct Peer {
    char peerid[21];
    uint32_t IPv4;
    uint16_t port;
    struct DataFlowInfo dataflow;
};

struct TorrentState {
    struct Metainfo mi;
    uint8_t status;
    uint32_t* bitmap;
    uint32_t bitmapLength;
    uint64_t bytesDownloaded;
    uint32_t pieceCount;
    uint32_t blocksPerSecond;
    struct Peer peers[32];
    uint8_t activePeersCount;
    struct DataFlowInfo dataflow;
    char* filePath;
};

struct AppData {
    char peerid[21];
    struct TorrentState torrents[64];
    uint64_t activeTorrentFlags;
    struct Peer client;
};

void runApp(struct AppData *app);
int getEmptyTorrentSlot(struct AppData* app);

void initTorrentState(struct TorrentState *ts, const char* filePath);
int generatePiecesBitmap(struct TorrentState *ts);
int getRequiredPieceIndex(uint32_t* bitmap, int n);
float getPercentDownloaded(uint32_t* bitmap, int pieceCount);


int generatePeerId();
void generateInfoHash(struct Metainfo *mi);
void generateMetainfo(const char *originFileName, struct Metainfo *mi);
void saveToTorrentFile(struct Metainfo *mi, const char *destName);
int readFromTorrentFile(const char *srcName, struct Metainfo *mi);
void freeMetainfo(struct Metainfo *mi);


#endif //CLIENT_H



#ifdef __cplusplus
}
#endif
