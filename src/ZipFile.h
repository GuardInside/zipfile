#include "ZipFileGlobal.h"

class ZIP_FILE_EXPORT ZipFile
{
public:
    enum OpenModeFlag
    {
        Append = 0x01,
        Zip    = 0x02,
        Unzip  = 0x04
    };

    Q_DECLARE_FLAGS(OpenMode, OpenModeFlag)

public:
    explicit ZipFile(const QString& path);

    //!
    bool open(ZipFile::OpenMode flags);

    //!
    bool addFile(const QString& filepath);

    //!
    bool addFiles(const QString& dirpath);

    //!
    bool addDir(const QString& dirpath);

    //!
    bool unzip(const QString& path);

    //!
    bool unzip();

    //!
    void close();

    //!
    ~ZipFile();

    //! Unzip the archive from memory into a directory
    static bool unzip(const QByteArray& archive, const QString& dirpath);

    //! Create an archive of a directory in memory
    //!
    //! @note Required write access to the dir over dirpath
    static QByteArray zip(const QString& dirpath);

private:
    struct ZipFilePrivate* _d;
};
