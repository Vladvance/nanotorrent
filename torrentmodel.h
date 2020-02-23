#ifndef PEERMODEL_H
#define PEERMODEL_H

#include <QAbstractTableModel>
#include <QLocale>
#include "client.h"

class TorrentModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TorrentModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void insertTorrent(TorrentState &ts);

private:
        const struct TorrentState* torrents;
        QList<int> indexes;
        const QLocale locale = QLocale::c();
        const QString columns[6] = {"Name", "Size", "Status", "Downloaded", "Percent", "Peers"};
        QString formatBytes(const uint64_t bytes) const;
        QString formatTorrentStatus(const TorrentState &ts) const;
        QString formatPercent(const float percent) const;
};

#endif // PEERMODEL_H
