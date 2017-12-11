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
    //no need for sets because, initially the last modification datetime = 0 means 1970...
    //I don't think anyone has a file of that "epoch" pun intended
    QString filename_pub;
    uint_fast64_t hash_pub = 0;
    int_fast64_t fileLastModificationDatetime_pub = 0;
    uint_fast64_t fileSize_pub = 0;
    //this helps to deal with the case when file is deleted
    bool iterated_pub = true;
    bool hashed_pub = true;
    bool hashing_pub = false;
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

//uses xxhash
uint_fast64_t getFileHash_f(const QString &filepath_par_con);

class fileHashControl_c
{
    bool fileStatusUMAPChanged_pri = false;
    //when iterating any file for the first time don't hash it,
    //but next time if it's in the UMap it will be hashed even if there aren't any changes
    //and hashFileInUMAP_f will return true

    //The reasoning behind this is that
    //size check is another way to know a file is different/changed only when it's the same
    //size hashing is required and since size checking is "instant" compared to hashing
    //this allows for a "lazy" initial scan.
    //With this change, and if only hashInitially_pri=false (which is not the default value),
    //it's required to check if the hash is set or not when doing hash comparisons
    bool hashInitially_pri = true;
    std::string mutexName_pri;
public:
    std::unordered_map<std::string, fileStatus_s> fileStatusUMAP_pub;
    //if a mutexname is not empty, a mutex from the qmutexUMapQtso will be used with that name
    //when accessing/modifying fileStatusUMAP_pub
    fileHashControl_c(
            const bool hashInitially_par_con = true
            , const std::string& mutexName_par_con = std::string());
    //add file information into a dictionary
    //returns true if there has been a change, or said in another way, only if a file hash is the same it will return false
    bool hashFileInUMAP_f(
            const QFileInfo& source_par_con
    );

    //add all the files of a directory (and subdirectories) into a dictionary
    //returns true if any file hash has changed
    bool hashDirectoryInUMAP_f(
            const QFileInfo& source_par_con
            , const QStringList& filenameFilters_par_con = QStringList()
            , const bool includeSubdirectories_par_con = true
            , const QString &includeDirectoriesWithFileX_par_con = QString()
            );

    //this function returns the current value and changes the internal value to false
    bool fileStatusUMAPChanged_f();
    std::string mutexName_f() const;
};





#endif // FILEHASHQTSO_FILEHASH_HPP
