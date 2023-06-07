#ifndef DATA_FILTER_H
#define DATA_FILTER_H

#include "ChannelMap.h"
#include "ExtractedDataStruct.h"
#include <algorithm>
#include <map>
#include <vector>
#include <utility>


class DataFilter {
public:
    DataFilter(const ExtractedData& extractedData, int detectorID, std::string& channelMapFile);
    bool PassesAllChecks(const int& index, const int& globalRingChannel, const int& globalWedgeChannel);
    void FilterData();
    const std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<double>>>& GetFilteredEnergyData() const;
    void SaveFilteredEnergyDataToFile(const std::string& fileName) const;
    std::map<std::pair<int, int>, std::pair<int, int>> GetGlobalChannelMap () const;

private:
    void FillRingAndWedgeChannels();

    const ExtractedData& extractedData_;
    ChannelMap channelMap_;
    std::map<std::pair<int, int>, std::pair<int, int>> detectorChannelToGlobalChanelMap_;
    int detectorID_;
    std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<double>>> filteredEnergyData_;
};

#endif // DATA_FILTER_H
