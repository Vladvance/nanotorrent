#include "torrentmodel.h"

TorrentModel::TorrentModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    indexes.reserve(64);
}

QVariant TorrentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return(columns[section]);
    }
    return QVariant();
}

int TorrentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return indexes.size();
}

int TorrentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return 6;
}

QVariant TorrentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        int row = index.row();
        int col = index.column();
        switch(col) {
        case 0: return QString::fromUtf8(torrents[indexes.at(row)].mi.info.name);
        case 1: return formatBytes(torrents[indexes.at(row)].mi.info.length);
        case 2: return formatTorrentStatus(torrents[indexes.at(row)]);
        case 3: return formatBytes(torrents[indexes.at(row)].bytesDownloaded);
        case 4: return formatPercent((float)torrents[indexes.at(row)].bytesDownloaded/(float)torrents[indexes.at(row)].mi.info.length);
        case 5: return QString::number(torrents[indexes.at(row)].activePeersCount);
        }
    }

    return QVariant();
}

void TorrentModel::setTorrents(struct TorrentState* torrents) {this->torrents = torrents;}

QString TorrentModel::formatBytes(const uint64_t bytes) const {
    return locale.formattedDataSize(bytes);
}

QString TorrentModel::formatTorrentStatus(const struct TorrentState &ts) const{
    QString result = QString::fromUtf8(TORRENT_STATUS_STR[ts.status]);
    if(ts.status == DOWNLOADING) {
        result += " " + formatBytes(ts.blocksPerSecond) + "/s";
    }
    return result;
}

QString TorrentModel::formatPercent(const float percent) const{
    return QString::number(100 * percent, 'f', 1) + "%";
}

int TorrentModel::insertTorrent(struct AppData* app, QString filePath)
{
   beginInsertRows(QModelIndex(), rowCount(), rowCount());


   int idx = getEmptyTorrentSlot(app);
   app->activeTorrentFlags |= 1ULL<<(63 - idx);

   indexes.push_back(idx);

   struct Metainfo *mi = &(app->torrents[idx].mi);
   generateMetainfo(filePath.toLocal8Bit().constData(), mi);
   app->torrents[idx].status = SEEDING;
   initTorrentState(&(app->torrents[idx]), filePath.toLocal8Bit().constData());

   endInsertRows();
   return idx;
}
