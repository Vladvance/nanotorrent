#include "torrentmodel.h"

TorrentModel::TorrentModel(QObject *parent)
    : QAbstractTableModel(parent)
{
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
    return 2;
}

int TorrentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return indexes.size();
}

QVariant TorrentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        int row = index.row();
        int col = index.column();
        switch(col) {
        case 0: return QString::fromUtf8(torrents[row].mi.info.name);
        case 1: return formatBytes(torrents[indexes.at(row)].mi.info.length);
        case 2: return formatTorrentStatus(torrents[row]);
        case 3: return formatBytes(torrents[row].bytesDownloaded);
        case 4: return formatPercent((float)torrents[row].bytesDownloaded/(float)torrents[row].mi.info.length);
        case 5: return QString::number(torrents[row].activePeersCount);
        }

    }

    return QVariant();
}

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
    return QString::number(percent, 'f', 1) + "%";
}

void TorrentModel::insertTorrent(struct TorrentState &ts)
{
   insertRow(1, QModelIndex());
   torrents.push_back(ts);
}
