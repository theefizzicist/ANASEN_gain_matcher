#include "DataReader.h"
#include <fstream>
#include <cstdlib>
#include "TTree.h"

DataReader::DataReader(const std::string& filePath, int runNumber, int detectorID) {
    // Create the input file path using the config's input directory and run number
    std::string inputFile = filePath + "/run_" + std::to_string(runNumber) + ".root";
    
    if (!IsFileExisting(inputFile)) {
        throw std::runtime_error("Error: File does not exist: " + inputFile);
    } else {
        file_ = std::unique_ptr<TFile>(TFile::Open(inputFile.c_str(), "READ"));
        if (!file_ || file_->IsZombie()) {
            throw std::runtime_error("Error: Unable to open file: " + inputFile);
        } else if (file_->GetSize() == 0) {
            throw std::runtime_error("Error: File is empty: " + inputFile);
        }
    }

    detectorID_ = detectorID;
}

DataReader::~DataReader() {
    if (file_ && file_->IsOpen()) {
        file_->Close();
    }
}

void DataReader::ReadAndExtractDataWTTree() {
    TTree *tree = static_cast<TTree*>(file_->Get("SPSTree"));
    ProcessedEvent *processed_event = nullptr;
    tree->SetBranchAddress("event", &processed_event);

    for (int i = 0; i < tree->GetEntries(); ++i) {
        tree->GetEntry(i);
        ExtractData(*processed_event);
    }
}

const ExtractedData& DataReader::GetExtractedData() const {
    CheckForEmptyVector(extractedData_.sabreRingChannel, "sabreRingChannel");
    CheckForEmptyVector(extractedData_.sabreWedgeChannel, "sabreWedgeChannel");
    CheckForEmptyVector(extractedData_.sabreRingE, "sabreRingE");
    CheckForEmptyVector(extractedData_.sabreWedgeE, "sabreWedgeE");
    CheckForEmptyVector(extractedData_.ringMultiplicity, "ringMultiplicity");
    CheckForEmptyVector(extractedData_.wedgeMultiplicity, "wedgeMultiplicity");
    return extractedData_;
}

bool DataReader::IsFileExisting(const std::string& filePath) {
    std::ifstream fileStream(filePath);
    return fileStream.good();
}

void DataReader::ExtractData(const ProcessedEvent& event) {
    extractedData_.sabreRingChannel.push_back(event.sabreRingChannel[detectorID_]);
    extractedData_.sabreWedgeChannel.push_back(event.sabreWedgeChannel[detectorID_]);
    extractedData_.sabreRingE.push_back(event.sabreRingE[detectorID_]);
    extractedData_.sabreWedgeE.push_back(event.sabreWedgeE[detectorID_]);
    extractedData_.ringMultiplicity.push_back(event.sabreArray[detectorID_].rings.size());
    extractedData_.wedgeMultiplicity.push_back(event.sabreArray[detectorID_].wedges.size());
}

template <typename T>
void DataReader::CheckForEmptyVector(const std::vector<T>& vec, const std::string& vecName) const {
    if (vec.empty()) {
        throw std::runtime_error("Error: Vector " + vecName + " is empty. Please ensure data has been read and processed.");
    }
}