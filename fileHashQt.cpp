#include "fileHashQt.hpp"

//#include "essentialQtso/essentialQt.hpp"
#include "criptoQtso/hashQt.hpp"

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
    hash_pub = json["hash"].toString().toULongLong();
    fileSize_pub = json["fileSize"].toString().toULongLong();
    fileLastModificationDatetime_pub = json["fileLastModificationDatetime"].toString().toLongLong();
}

void fileStatus_s::write_f(QJsonObject &json) const
{
    json["filename"] = filename_pub;
    //json/javascript can't fit 64bit numbers, IEEE 754 shit, which is floating point, not integer, so it's less than a 64bit integer
    //so... use string notation
    json["hash"] = QString::number(hash_pub);
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
        const std::unordered_map<std::string, fileStatus_s> &fileStatusUMAP_par_con)
{
    fileStatusVector_pub.reserve(fileStatusUMAP_par_con.size());
    for (const auto& item_ite_con : fileStatusUMAP_par_con)
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

bool hashFileInUMAP_f(
    std::unordered_map<std::string, fileStatus_s>& fileStatusUMAP_par
    , const QFileInfo& source_par_con
)
{
    bool sourceChanged(false);
    if (not source_par_con.exists())
    {
        return sourceChanged;
    }

    bool sourceDatetimeChanged(false);
    fileStatus_s sourceFileStatus;
    //fileStatus_s destinationFileStatus;
    //check source
    auto findFileStatusResult(fileStatusUMAP_par.find(source_par_con.absoluteFilePath().toStdString()));
    //source found
    if (findFileStatusResult != fileStatusUMAP_par.end())
    {
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
            sourceDatetimeChanged = true;
        }
        //if time changed, check hash
        if (sourceDatetimeChanged)
        {
            //qout_glo << "hashing source (datetime changed)" << endl;
            auto fileNewHastTmp(getFileHash_f(source_par_con.absoluteFilePath()));
            //same hash
            if (fileNewHastTmp == findFileStatusResult->second.hash_pub)
            {
                //file hash hasn't changed, nothing to do
            }
            else
            {
                //update hash on source umap
                findFileStatusResult->second.hash_pub = fileNewHastTmp;
                sourceChanged = true;
            }
        }

        sourceFileStatus = findFileStatusResult->second;
    }
    //source element not found in umap, so create element and insert into umap
    else
    {
        //qout_glo << "hashing source (initial run)" << endl;
        sourceFileStatus.filename_pub = source_par_con.absoluteFilePath();
        sourceFileStatus.fileLastModificationDatetime_pub = source_par_con.lastModified().toMSecsSinceEpoch();
        sourceFileStatus.hash_pub = getFileHash_f(source_par_con.absoluteFilePath());
        sourceFileStatus.fileSize_pub = source_par_con.size();
        sourceFileStatus.iterated_pub = true;
        //sourceFileStatus.pathType = pathType_ec::file;

        fileStatusUMAP_par.emplace(source_par_con.absoluteFilePath().toStdString(), sourceFileStatus);
        sourceDatetimeChanged = true;
        sourceChanged = true;
    }
    return sourceChanged;
}

//source is dir and exists
//destination is dir (exists or not exists)
bool hashDirectoryInUMAP_f(std::unordered_map<std::string, fileStatus_s>& fileStatusUMAP_par
    , const QFileInfo& source_par_con
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
            QFileInfo sourceFileTmp(source_par_con.absoluteFilePath() + QDir::separator() + filename_ite_con);
            const auto resultFile(
                        hashFileInUMAP_f(
                            fileStatusUMAP_par
                            , sourceFileTmp
                            )
                        );
            //last result and a copy happened or nothing changed
            result = result or resultFile;
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
                QDir currentSubfolderDirTmp(source_par_con.absoluteFilePath() + QDir::separator() + subfolder_ite_con);

                //get the subfolders of the one it's iterating
                QStringList subFoldersTmp(currentSubfolderDirTmp.entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks));
                //for the found subfolder, prepend the previous subfolder path string
                for (auto& subfolderTmp_ite : subFoldersTmp)
                {
                    //qDebug() << "subfolder_ite_con + QDir::separator() + subfolderTmp_ite " << subfolder_ite_con + QDir::separator() + subfolderTmp_ite << endl;
                    //prepend the parent subfolder
                    subfolderTmp_ite.prepend(subfolder_ite_con + QDir::separator());
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
                        //qDebug() << "source_par_con.absoluteFilePath() + QDir::separator() + subfolder_ite_con + QDir::separator() + filename_ite_con " << source_par_con.absoluteFilePath() + QDir::separator() + subfolder_ite_con + QDir::separator() + filename_ite_con << endl;
                        QFileInfo sourceFileTmp(source_par_con.absoluteFilePath() + QDir::separator() + subfolder_ite_con + QDir::separator() + filename_ite_con);
                        const auto resultFile(
                                    hashFileInUMAP_f(
                                        fileStatusUMAP_par
                                        , sourceFileTmp
                                        )
                                    );
                        //if a copy happened or the file it's the same consider it as successful
                        result = result or resultFile;
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
