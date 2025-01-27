#include "ZipFile.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "minizip/unzip.h"
#include "minizip/zip.h"

#ifdef Q_OS_WIN
#include "minizip/iowin32.h"
#endif

namespace
{
    zipFile zipOpenUtf8(const QString& filePath, int append)
    {
#ifdef Q_OS_WIN
        zlib_filefunc64_def ffunc;

        fill_win32_filefunc64W(&ffunc);

        const auto unicodePath = filePath.toStdWString();

        return zipOpen2_64(unicodePath.data(), append, NULL, &ffunc);
#else
        return zipOpen64(filePath.toUtf8().data(), 0);
#endif
    }

    zipFile unzipOpenUtf8(const QString& filePath)
    {
#ifdef Q_OS_WIN
        zlib_filefunc64_def ffunc;

        fill_win32_filefunc64W(&ffunc);

        const auto unicodePath = filePath.toStdWString();

        return unzOpen2_64(unicodePath.data(), &ffunc);
#else
        return unzOpen2(filePath.toUtf8().data(), 0);
#endif
    }
}

struct ZipFilePrivate
{
    //! Add data as a file to the archive
    bool addData(const QByteArray& data, const QString& zipfilepath, const QDateTime& birthTime)
    {
        zip_fileinfo zipInfo = {};

        zipInfo.tmz_date.tm_sec  = birthTime.time().second();
        zipInfo.tmz_date.tm_min  = birthTime.time().minute();
        zipInfo.tmz_date.tm_hour = birthTime.time().hour();
        zipInfo.tmz_date.tm_mday = birthTime.date().day();
        zipInfo.tmz_date.tm_mon  = birthTime.date().month() - 1; // 0-11 vs 1-12
        zipInfo.tmz_date.tm_year = birthTime.date().year();

        if (zipOpenNewFileInZip(fd, zipfilepath.toUtf8().data(), &zipInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
            return false;

        if (zipWriteInFileInZip(fd, data.data(), data.size()))
            return false;

        if (zipCloseFileInZip(fd))
            return false;

        return true;
    }

    //! Add file to archive
    bool addFile(const QString& filepath, const QString& zipfilepath)
    {
        QFile file(filepath);

        if (!file.open(QIODevice::ReadOnly))
            return false;

        return addData(file.readAll(), zipfilepath, QFileInfo(file).birthTime());
    }

    //! Add all files in directories to archive
    bool addFiles(const QDir& dir)
    {
        const auto entryNames = dir.entryList(QDir::Files);

        for (const auto& entryName : entryNames)
        {
            const auto entryPath = dir.absoluteFilePath(entryName);

            if (!addFile(entryPath, entryName))
                return false;
        }

        return true;
    }

    //! Add directory to archive
    bool addDir(const QDir& dir, const QString& prefix)
    {
        const auto entryNames = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

        for (const auto& entryName : entryNames)
        {
            const auto entryPath = dir.absoluteFilePath(entryName);

            if (QFileInfo(entryPath).isFile())
            {
                if (!addFile(entryPath, prefix + '/' + dir.dirName() + '/' + entryName))
                    return false;
            }
            else
            {
                if (!addDir(entryPath, prefix + '/' + dir.dirName()))
                    return false;
            }
        }

        return true;
    }

    QString           path;
    void*             fd;
    ZipFile::OpenMode flags;
};

ZipFile::ZipFile(const QString& path)
    : _d(new ZipFilePrivate)
{
    _d->path  = path;
    _d->fd    = nullptr;
    _d->flags = {};
}

bool ZipFile::open(OpenMode flags)
{
    if (flags.testFlag(OpenModeFlag::Zip))
    {
        _d->fd = zipOpenUtf8(_d->path, flags.testFlag(ZipFile::Append) ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE);
    }

    if (flags.testFlag(OpenModeFlag::Unzip))
    {
        _d->fd = unzipOpenUtf8(_d->path);
    }

    _d->flags = flags;

    return _d->fd;
}

bool ZipFile::addFile(const QString& filepath)
{
    return _d->addFile(filepath, QFileInfo(filepath).fileName());
}

bool ZipFile::addFiles(const QString& dirpath)
{
    return _d->addFiles(QDir(dirpath));
}

bool ZipFile::addDir(const QString& dirpath)
{
    return _d->addDir(dirpath, {});
}

bool ZipFile::unzip(const QString& path)
{
    bool success = true;

    char buffer[4096];

    if (unzGoToFirstFile(_d->fd) != UNZ_OK)
        return false;

    do // we iterate over all files
    {
        // open current file
        if (unzOpenCurrentFile(_d->fd) != UNZ_OK)
        {
            success = false;
            break;
        }

        // reading file info
        unz_file_info fileInfo;
        if (unzGetCurrentFileInfo(_d->fd, &fileInfo, NULL, 0, NULL, 0,
                                  NULL, 0) != UNZ_OK)
        {
            success = false;
            unzCloseCurrentFile(_d->fd);
            break;
        }

        // get filename from the fileInfo
        char* cfilename = new char[fileInfo.size_filename + 1];
        unzGetCurrentFileInfo(_d->fd, &fileInfo, cfilename,
                              fileInfo.size_filename + 1, NULL, 0, NULL, 0);
        cfilename[fileInfo.size_filename] = '\0';

        // check filename for paths
        QString filename = { cfilename };

        delete[] cfilename;

        // replace all '\' with '/'
        int index = -1;
        while ((index = filename.indexOf('\\')) != -1)
        {
            filename.replace(index, 1, '/');
        }
        QString fullpath = path + '/' + filename;

        // create directory / parent directories
        index = fullpath.lastIndexOf('/');
        if (index != -1)
        {
            QDir path = { fullpath.left(index) };
            path.mkpath(".");
        }

        QFile* file = nullptr;
        int    read = 0;
        while ((read = unzReadCurrentFile(_d->fd, buffer, 4096)) > 0)
        {
            if (file == nullptr)
            {
                if (QFile::exists(fullpath)) // comment it to permit rewriting
                    break;

                file = new QFile(fullpath);

                if (!file->open(QIODevice::WriteOnly))
                {
                    delete file;
                    file = nullptr;
                    break;
                }
            }
            QByteArray data = QByteArray::fromRawData(buffer, read);
            file->write(data);
        }

        // check if we got an error
        if (read < 0)
            success = false;

        if (file != nullptr)
        {
            delete file;
            file = nullptr;
        }

        // we are done with the file -> close the file
        if (unzCloseCurrentFile(_d->fd) != UNZ_OK)
        {
            success = false;
            break;
        }

    } // goto next file
    while (unzGoToNextFile(_d->fd) == UNZ_OK && success);

    return success;
}

bool ZipFile::unzip()
{
    return unzip({});
}

void ZipFile::close()
{
    if (!_d->fd)
        return;

    if (_d->flags.testFlag(ZipFile::Zip))
        zipClose(_d->fd, nullptr);

    if (_d->flags.testFlag(ZipFile::Unzip))
        unzClose(_d->fd);

    _d->fd    = nullptr;
    _d->flags = {};
}

ZipFile::~ZipFile()
{
    close();

    delete _d;
}
