#include <QDebug>
#include <QFile>

#include "ZipFile.h"

bool ZipFile::unzip(const QByteArray& archive, const QString& dirpath)
{
    if (archive.size() == 22)
    {
        qInfo() << qPrintable(QObject::tr("An attempt to unpack an empty archive to %1").arg(dirpath));
        return true;
    }

    // write the archive to a temporary archive file

    QFile tmpfile(dirpath + '/' + "tmp.zip");

    QSharedPointer<QFile> tmpfiledeleter(&tmpfile, [](QFile* file) { file->remove(); });

    if (!tmpfile.open(QFile::WriteOnly))
    {
        qWarning() << qPrintable(tmpfile.errorString());
        return false;
    }

    if (tmpfile.write(archive) == -1)
    {
        qWarning() << qPrintable(tmpfile.errorString());
        return false;
    }

    tmpfile.close();

    // unpack the tmp archive file to dirpath

    ZipFile zipfile(tmpfile.fileName());

    if (!zipfile.open(ZipFile::Unzip))
    {
        qWarning() << qPrintable(QObject::tr("Error opening archive %1").arg(tmpfile.fileName()));
        return false;
    }

    if (!zipfile.unzip(dirpath))
    {
        qWarning() << qPrintable(QObject::tr("Error unpacking archive"));
        return false;
    }

    zipfile.close();

    //

    return true;
}

QByteArray ZipFile::zip(const QString& dirpath)
{
    // create a temporary archive file on disk

    QFile tmpfile(dirpath + '/' + ".." + '/' + "tmp.zip"); // tmp dir can't be dirpath

    QSharedPointer<QFile> tmpfiledeleter(&tmpfile, [](QFile* file) { file->remove(); });

    // fill the archive file

    ZipFile zipFile(tmpfile.fileName());

    if (!zipFile.open(ZipFile::Zip))
    {
        qWarning() << qPrintable(tmpfile.errorString());
        return {};
    }

    if (!zipFile.addFiles(dirpath))
    {
        qWarning() << qPrintable(QObject::tr("Error filling archive %1").arg(tmpfile.fileName()));
        return {};
    }

    zipFile.close();

    // serialization

    if (!tmpfile.open(QFile::ReadOnly))
    {
        qWarning() << qPrintable(QObject::tr("Archive serialization error"));
        return {};
    }

    const auto archive = tmpfile.readAll();

    tmpfile.close();

    //

    return archive;
}
