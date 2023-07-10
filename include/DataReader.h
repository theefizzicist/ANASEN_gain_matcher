#ifndef DATA_READER_H
#define DATA_READER_H

#include "DataStructs.h"
#include "ExtractedDataStruct.h"
#include "TFile.h"
#include <string>
#include <omp.h>
#include <memory>

class DataReader {
public:
    DataReader(const std::string& filePath, int runNumber, int detectorID);
    ~DataReader();
    void ReadAndExtractDataWTTree();
    const ExtractedData& GetExtractedData() const;

private:
    bool IsFileExisting(const std::string& filePath);
    void ExtractData(const CoincEvent& event);
    template <typename T>
    void CheckForEmptyVector(const std::vector<T>& vec, const std::string& vecName) const;
    
    std::unique_ptr<TFile> file_;
    int detectorID_;
    ExtractedData extractedData_;
    int numberOfThreads_ = omp_get_num_procs();
};

#endif // DATA_READER_H
