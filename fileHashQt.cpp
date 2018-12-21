#include "fileHashQt.hpp"

#include "signalso/signal.hpp"
#include "cryptoQtso/hashQt.hpp"
#include "qmutexUMapQtso/qmutexUMapQt.hpp"

#include <QJsonArray>
#include <QDateTime>
#include <QDir>

fileStatus_s::fileStatus_s(
        const QString &filename_par_con
        , const uint_fast64_t hash_par_con
        , const int_fast64_t fileLastModificationDatetime_par_con
        , const uint_fast64_t fileSize_par_con)
    :
      filename_pub(filename_par_con)
    , hash_pub(hash_par_con)
    , fileLastModificationDatetime_pub(fileLastModificationDatetime_par_con)
    , fileSize_pub(fileSize_par_con)
{}


void fileStatus_s::read_f(const QJsonObject &json)
{
    filename_pub = json["filename"].toString();
    if (json["hash"].isUndefined())
    {
        hashed_pub = false;
    }
    else
    {
        hash_pub = json["hash"].toString().toULongLong();
        hashed_pub = true;
    }
    fileSize_pub = json["fileSize"].toString().toULongLong();
    fileLastModificationDatetime_pub = json["fileLastModificationDatetime"].toString().toLongLong();
}

void fileStatus_s::write_f(QJsonObject &json) const
{
    json["filename"] = filename_pub;
    //json/javascript can't fit 64bit numbers, IEEE 754 shit, which is floating point, not integer, so it's less than a 64bit integer
    //so... use string notation
    if (hashed_pub)
    {
        json["hash"] = QString::number(hash_pub);
    }
    json["fileSize"] = QString::number(fileSize_pub);
    json["fileLastModificationDatetime"] = QString::number(fileLastModificationDatetime_pub);
}

void fileStatusArray_s::read_f(const QJsonObject &json)
{
    QJsonArray arrayTmp(json["fileStatusArray"].toArray());
    fileStatusVector_pub.reserve(arrayTmp.size());
    for (const auto& item_ite_con : arrayTmp)
    {
        QJsonObject fileStatusJsonObject = item_ite_con.toObject();
        fileStatus_s fileStatusTmp;
        fileStatusTmp.read_f(fileStatusJsonObject);
        fileStatusVector_pub.emplace_back(fileStatusTmp);
    }
}

void fileStatusArray_s::write_f(QJsonObject &json) const
{
    QJsonArray fileStatusArrayTmp;
    for (const fileStatus_s& fileStatus_ite_con : fileStatusVector_pub)
    {
        QJsonObject jsonObjectTmp;
        fileStatus_ite_con.write_f(jsonObjectTmp);
        fileStatusArrayTmp.append(jsonObjectTmp);
    }
    json["fileStatusArray"] = fileStatusArrayTmp;
}

fileStatusArray_s::fileStatusArray_s(
        const std::unordered_map<std::string, fileStatus_s> &fileStatusUMAP_pub_con)
{
    fileStatusVector_pub.reserve(fileStatusUMAP_pub_con.size());
    for (const auto& item_ite_con : fileStatusUMAP_pub_con)
    {
        fileStatusVector_pub.emplace_back(item_ite_con.second);
    }
}

uint_fast64_t getFileHash_f(const QString& filepath_par_con)
{
    eines::hasher_c hasher
    (
        eines::hasher_c::inputType_ec::file
        , filepath_par_con
        , eines::hasher_c::outputType_ec::unsignedXbitInteger
        , eines::hasher_c::hashType_ec::XXHASH64
    );
    hasher.generateHash_f();
    return hasher.hash64BitNumberResult_f();
}

bool fileHashControl_c::hashFileInUMAP_f(
    const QFileInfo& source_par_con)
{
    bool sourceChanged(false);
    if (not source_par_con.exists())
    {
        return sourceChanged;
    }

    if (not mutexName_pri.empty())
    {
        getAddMutex_f(mutexName_pri)->lock();
    }
    //check source
    auto findFileStatusResult(fileStatusUMAP_pub.find(source_par_con.absoluteFilePath().toStdString()));

    //source found
    if (findFileStatusResult != fileStatusUMAP_pub.end())
    {
        bool sourceDatetimeChanged(false);
        findFileStatusResult->second.iterated_pub = true;
        //check if time changed
        if (findFileStatusResult->second.fileLastModificationDatetime_pub == source_par_con.lastModified().toMSecsSinceEpoch())
        {
            //file time hasn't changed, nothing to do
        }
        else
        {
            //update time on source umap
            findFileStatusResult->second.fileLastModificationDatetime_pub = source_par_con.lastModified().toMSecsSinceEpoch();
            findFileStatusResult->second.fileSize_pub = source_par_con.size();
            fileStatusUMAPChanged_pri = true;
            sourceDatetimeChanged = true;
        }
        //if time changed or no hash, check hash
        if (sourceDatetimeChanged or not findFileStatusResult->second.hashed_pub)
        {
            //qout_glo << "hashing source (datetime changed)" << endl;
            findFileStatusResult->second.hashing_pub = true;
            uint_fast64_t currentHashTmp(findFileStatusResult->second.hash_pub);
            if (not mutexName_pri.empty())
            {
                getAddMutex_f(mutexName_pri)->unlock();
            }
            auto fileNewHastTmp(getFileHash_f(source_par_con.absoluteFilePath()));
            if (not mutexName_pri.empty())
            {
                getAddMutex_f(mutexName_pri)->lock();
                //since the UMap could have changed because of the unlocking, find again
                findFileStatusResult = fileStatusUMAP_pub.find(source_par_con.absoluteFilePath().toStdString());
            }
            //hash mismatch or no hash
            if (fileNewHastTmp != currentHashTmp or not findFileStatusResult->second.hashed_pub)
            {
                //update hash on source umap
                findFileStatusResult->second.hash_pub = fileNewHastTmp;
                if (not findFileStatusResult->second.hashed_pub)
                {
                    findFileStatusResult->second.hashed_pub = true;
                }
                fileStatusUMAPChanged_pri = true;
                sourceChanged = true;

            }
            else
            {
                //file hash hasn't changed, nothing to do
            }
            findFileStatusResult->second.hashing_pub = false;
        }
        //sourceFileStatus = findFileStatusResult->second;
    }
    //source element not found in umap, so create element and insert into umap
    else
    {
        if (not mutexName_pri.empty())
        {
            getAddMutex_f(mutexName_pri)->unlock();
        }
        //qout_glo << "hashing source (initial run)" << endl;
        fileStatus_s sourceFileStatus(
               source_par_con.absoluteFilePath()
               , 0
               , source_par_con.lastModified().toMSecsSinceEpoch()
               , source_par_con.size()
        );
        if (hashInitially_pri)
        {
            sourceFileStatus.hash_pub = getFileHash_f(source_par_con.absoluteFilePath());
            //hashed is true by default
        }
        else
        {
            sourceFileStatus.hashed_pub = false;
        }

        if (not mutexName_pri.empty())
        {
            getAddMutex_f(mutexName_pri)->lock();
        }
        fileStatusUMAP_pub.emplace(source_par_con.absoluteFilePath().toStdString(), sourceFileStatus);
        fileStatusUMAPChanged_pri = true;
        sourceChanged = true;
    }
    if (not mutexName_pri.empty())
    {
        getAddMutex_f(mutexName_pri)->unlock();
    }
    return sourceChanged;
}

//source is dir and exists
//destination is dir (exists or not exists)
bool fileHashControl_c::hashDirectoryInUMAP_f(
    const QFileInfo& source_par_con
    , const QStringList& filenameFilters_par_con
    , const bool includeSubdirectories_par_con
    , const QString& includeDirectoriesWithFileX_par_con)
{
    bool result(false);

    QDir sourceDir(source_par_con.absoluteFilePath());
    if (not sourceDir.exists())
    {
        return result;
    }

    //if the setting is empty
    //account it as it is found
    bool xFileFoundRoot(includeDirectoriesWithFileX_par_con.isEmpty());
    if (not xFileFoundRoot)
    {
        //source dir, get the root files
        QStringList rootFileListUnfiltered(sourceDir.entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot));
        for (const QString& filename_ite_con : rootFileListUnfiltered)
        {
            if (filename_ite_con == includeDirectoriesWithFileX_par_con)
            {
                xFileFoundRoot = true;
                break;
            }
        }
    }
    if (xFileFoundRoot)
    {
        //source dir, get the root files
        QStringList rootFileList(sourceDir.entryList(filenameFilters_par_con, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot));
        for (const QString& filename_ite_con : rootFileList)
        {
            //QOUT_TS("filename_ite_con " << filename_ite_con << endl);
            QFileInfo sourceFileTmp(source_par_con.absoluteFilePath() + "/" + filename_ite_con);
            const auto resultFile(
                        hashFileInUMAP_f(
                            sourceFileTmp
                            )
                        );
            //last result and a copy happened or nothing changed
            result = result or resultFile;
            if (not signalso::isRunning_f())
            {
                return result;
            }
        }
    }

    if (includeSubdirectories_par_con)
    {
        //source dir, get the all the subfolders (names) of the base folder
        QStringList subfolders(sourceDir.entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks));

        //while subfolders keep being found
        while (not subfolders.isEmpty())
        {
            QStringList newSubfoldersTmp;
            //for every subfolder gathered
            for (const auto& subfolder_ite_con : subfolders)
            {
                //qDebug() << "folder_ite_con " << subfolder_ite_con << endl;

                //do a QDir of the subfolder, using the initial folder + all the subfolder "depth" that has been traveled/iterated
                QDir currentSubfolderDirTmp(source_par_con.absoluteFilePath() + "/" + subfolder_ite_con);

                //get the subfolders of the one it's iterating
                QStringList subFoldersTmp(currentSubfolderDirTmp.entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks));
                //for the found subfolder, prepend the previous subfolder path string
                for (auto& subfolderTmp_ite : subFoldersTmp)
                {
                    //qDebug() << "subfolder_ite_con + "/" + subfolderTmp_ite " << subfolder_ite_con + "/" + subfolderTmp_ite << endl;
                    //prepend the parent subfolder
                    subfolderTmp_ite.prepend(subfolder_ite_con + "/");
                }
                newSubfoldersTmp.append(subFoldersTmp);

                //if the setting is empty
                //account it as it is found
                bool xFileFoundSubdirectory(includeDirectoriesWithFileX_par_con.isEmpty());
                if (not xFileFoundSubdirectory)
                {
                    //get the files of the subfolder
                    QStringList subDirectoryFileListUnfiltered(currentSubfolderDirTmp.entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot));
                    for (const QString& filename_ite_con : subDirectoryFileListUnfiltered)
                    {
                        if (filename_ite_con == includeDirectoriesWithFileX_par_con)
                        {
                            xFileFoundSubdirectory = true;
                            break;
                        }
                    }
                }

                if (xFileFoundSubdirectory)
                {
                    //get the files of the subfolder
                    QStringList subDirectoryFileList(currentSubfolderDirTmp.entryList(filenameFilters_par_con, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot));
                    for (const auto& filename_ite_con : subDirectoryFileList)
                    {
                        //qDebug() << "source_par_con.absoluteFilePath() + "/" + subfolder_ite_con + "/" + filename_ite_con " << source_par_con.absoluteFilePath() + "/" + subfolder_ite_con + "/" + filename_ite_con << endl;
                        QFileInfo sourceFileTmp(source_par_con.absoluteFilePath() + "/" + subfolder_ite_con + "/" + filename_ite_con);
                        const auto resultFile(
                                    hashFileInUMAP_f(
                                        sourceFileTmp
                                        )
                                    );
                        //if a copy happened or the file it's the same consider it as successful
                        result = result or resultFile;
                        if (not signalso::isRunning_f())
                        {
                            return result;
                        }
                    }
                }
            }
            //        if (not newFoundFolders.isEmpty())
            //        {
            //assign the new subfolders to the previous subfolders
            //        qDebug() << "newFoundFolders.size() " << newFoundFolders.size() << endl;
            subfolders = newSubfoldersTmp;
            //        }
        }
    }
    return result;
}

std::string fileHashControl_c::mutexName_f() const
{
    return mutexName_pri;
}

fileHashControl_c::fileHashControl_c(
        const bool hashInitially_par_con
        , const std::string& mutexName_pri)
    : hashInitially_pri(hashInitially_par_con)
    , mutexName_pri(mutexName_pri)
{
}

bool fileHashControl_c::fileStatusUMAPChanged_f()
{
    if (not mutexName_pri.empty())
    {
        QMutexLocker locker1(getAddMutex_f(mutexName_pri));
        bool result(fileStatusUMAPChanged_pri);
        fileStatusUMAPChanged_pri = false;
        return result;
    }
    else
    {
        bool result(fileStatusUMAPChanged_pri);
        fileStatusUMAPChanged_pri = false;
        return result;
    }
}
