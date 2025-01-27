#include <QTemporaryDir>
#include <QTest>

#include "../src/zipfile.h"

namespace
{
    bool writeToFile(const QByteArray& data, const QString& fileName)
    {
        QFile file(fileName);

        if (!file.open(QFile::WriteOnly))
            return false;

        return file.write(data);
    }

    QByteArray readFile(const QString& fileName)
    {
        QFile file(fileName);

        if (!file.open(QFile::ReadOnly))
            return {};

        return file.readAll();
    }
}

class ZipFileTests : public QObject
{
    Q_OBJECT

private slots:
    void zip_unzip_data()
    {
        QTest::addColumn<QByteArray>("data");

        QTest::newRow("KByte") << QByteArray(1024, 0x1);
        QTest::newRow("MByte") << QByteArray(1024, 0x2);
    }

    void zip_unzip()
    {
        static const auto fileName = "data.bin";

        QFETCH(QByteArray, data);

        QByteArray archive;

        // Create an archive in memory

        {
            QTemporaryDir dir;

            QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

            QVERIFY(writeToFile(data, dir.filePath(fileName)));

            archive = ZipFile::zip(dir.path());

            QVERIFY(!archive.isEmpty());

            QVERIFY(archive.size() < data.size());
        }

        // Extract a file data from archive

        {
            QTemporaryDir dir;

            QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

            QVERIFY(ZipFile::unzip(archive, dir.path()));

            QVERIFY(QFile::exists(dir.filePath(fileName)));

            const auto fileData = readFile(dir.filePath(fileName));

            QCOMPARE(fileData, data);
        }
    }
};

QTEST_MAIN(ZipFileTests)

#include "ZipFileTests.moc"
