// Copyright (c) 2016-2020 The Karbowanec developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QMessageBox>
#include <QtNetwork>
#include <QVersionNumber>

#include "WalletUpdate.h"
#include "Settings/Settings.h"
#include "WalletLogger/WalletLogger.h"

using namespace WalletGui;

Updater::Updater(QObject *parent) :
    QObject(parent)
{
}

void Updater::checkForUpdate()
{
    WalletLogger::info(tr("[Update] Checking for an update..."));
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    QNetworkRequest request((QUrl(KARBO_UPDATE_URL)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->get(request);
}

void Updater::replyFinished (QNetworkReply *reply)
{
    if (reply->error())
    {
        QString errorStr = QString(tr("Unable to check for update: %1")).arg(reply->errorString());
        WalletLogger::warning(errorStr);
    }
    else
    {
        QString ourVersionStr = Settings::instance().getVersion().split("-")[0];
        if (ourVersionStr.startsWith(QStringLiteral("v.")))
            ourVersionStr.remove(0, 2);
        else if (ourVersionStr.startsWith('v'))
                 ourVersionStr.remove(0, 1);

        QJsonArray jsonArray = QJsonDocument::fromJson(reply->readAll()).array();
        QString remoteVersionStr = jsonArray[0].toObject()["name"].toString();
        if (remoteVersionStr.startsWith(QStringLiteral("v.")))
            remoteVersionStr.remove(0, 2);
        else if (remoteVersionStr.startsWith('v'))
                 remoteVersionStr.remove(0, 1);

        int suffixIndex;
        QVersionNumber ourVersion = QVersionNumber::fromString(ourVersionStr, &suffixIndex);
        QVersionNumber remoteVersion = QVersionNumber::fromString(remoteVersionStr, &suffixIndex);

        WalletLogger::info(QString(tr("[Update] Latest version on repository: %1")).arg(remoteVersionStr));

        bool r = QVersionNumber::compare(ourVersion, remoteVersion) < 0;
        if (r) {
             if (QMessageBox::warning(nullptr, QObject::tr("New version available"), QObject::tr("There is an update available.\nDo you want to go to the download page?"), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {
                 QDesktopServices::openUrl(QUrl(KARBO_DOWNLOAD_URL));
             }
        }
    }
    reply->deleteLater();
}
