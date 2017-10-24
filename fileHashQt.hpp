#ifndef FILEHASHQTSO_FILEHASH_HPP
#define FILEHASHQTSO_FILEHASH_HPP

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QFileInfo>

#include <string>
#include <vector>
#include <unordered_map>

struct fileStatus_s
{
    //no need for sets because, initially to the last modification datetime is used to check
    //if there is any modification and on the first time it will get TRIGGERED and will get the hash too
    QString filename_pub;
    uint_fast64_t hash_pub = 0;
    int_fast64_t fileLastModificationDatetime_pub = 0;
    uint_fast64_t fileSize_pub = 0;
    bool iterated_pub = false;

    fileStatus_s() = default;
    fileStatus_s(
            const QString& filename_par_con
            , const uint_fast64_t hash_par_con
            , const int_fast64_t fileLastModificationDatetime_par_con
            , const uint_fast64_t fileSize_par_con
    );
    void read_f(const QJsonObject &json);
    void write_f(QJsonObject &json) const;
};

struct fileStatusArray_s
{
    std::vector<fileStatus_s> fileStatusVector_pub;

    void read_f(const QJsonObject &json);
    void write_f(QJsonObject &json) const;

    fileStatusArray_s() = default;
    fileStatusArray_s(const std::unordered_map<std::string, fileStatus_s>& fileStatusUMAP_par_con);
};

uint_fast64_t getFileHash_f(const std::string& filepath_par_con);

//add file information into a dictionary
//returns true if there has been a change, or said in another way, only if a file hash is the same it will return false
bool hashFileInUMAP_f(
    std::unordered_map<std::string, fileStatus_s>& fileStatusUMAP_par
    , const QFileInfo& source_par_con
);

//add all the files of a directory (and subdirectories) into a dictionary
//returns true if any file hash has changed
bool hashDirectoryInUMAP_f(std::unordered_map<std::string, fileStatus_s>& fileStatusUMAP_par
    , const QFileInfo& source_par_con
    , const QStringList& filenameFilters_par_con
    , const bool includeSubdirectories_par_con = true
    , const QString &includeDirectoriesWithFileX_par_con = QString()
);


#endif // FILEHASHQTSO_FILEHASH_HPP
