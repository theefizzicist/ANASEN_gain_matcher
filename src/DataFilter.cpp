#include "DataFilter.h"
#include <fstream>
#include <string>
#include <sstream>

DataFilter::DataFilter(const ExtractedData& extractedData, int detectorID, std::string& channelMapFile)
    : extractedData_(extractedData), detectorID_(detectorID)
{
    channelMap_.FillMap(channelMapFile);
    FillRingAndWedgeChannels();
}

bool DataFilter::PassesAllChecks(const int& index,
                                 const int& globalRingChannel,
                                 const int& globalWedgeChannel)
{
    if (extractedData_.ringMultiplicity.at(index) == 1 &&
        extractedData_.wedgeMultiplicity.at(index) == 1 &&
        extractedData_.qqqRingChannel.at(index) == globalRingChannel &&
        extractedData_.qqqWedgeChannel.at(index) == globalWedgeChannel &&
        extractedData_.qqqRingE.at(index) > 0 &&
        extractedData_.qqqWedgeE.at(index) > 0)
    {
        return true;
    }

    return false;
}

void DataFilter::FilterData()
{
    int dataSize = extractedData_.qqqRingE.size();
    for (int ringchan = 0; ringchan < 16; ++ringchan) {
        for (int wedgechan = 0; wedgechan < 16; ++wedgechan) {
            std::pair<int, int> key = std::make_pair(ringchan, wedgechan);
            std::pair<int, int> globalChannel = detectorChannelToGlobalChanelMap_[key];

            int currentFilteredCount = 0;
            std::vector<double> filteredRingE(dataSize);
            std::vector<double> filteredWedgeE(dataSize);
            for (int idx = 0; idx < dataSize; idx++)
            {
                if (PassesAllChecks(idx, globalChannel.first, globalChannel.second))
                {
                    filteredRingE[currentFilteredCount] = extractedData_.qqqRingE[idx];
                    filteredWedgeE[currentFilteredCount] = extractedData_.qqqWedgeE[idx];
                    currentFilteredCount++;
                }
            }

            filteredRingE.resize(currentFilteredCount);
            filteredWedgeE.resize(currentFilteredCount);
            if (filteredRingE.size() != 0 && filteredRingE.size() == filteredWedgeE.size())
                filteredEnergyData_[key] = std::make_pair(filteredRingE, filteredWedgeE);
            else if (filteredRingE.size() != filteredWedgeE.size())
                std::cout << "RING AND WEDGE ENERGY VECTORS DO NOT HAVE THE SAME NUMBER OF DATA POINTS!!!\n";
        }
    }
}

const std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<double>>>& DataFilter::GetFilteredEnergyData() const
{
    return filteredEnergyData_;
}

void DataFilter::FillRingAndWedgeChannels()
{
    DetType typeRing[4] = {FQQQ0Ring,FQQQ1Ring,FQQQ2Ring,FQQQ3Ring};
    DetType typeWedge[4] = {FQQQ0Wedge,FQQQ1Wedge,FQQQ2Wedge,FQQQ3Wedge};

    for (int ringChannel = 0; ringChannel < 16; ++ringChannel)
    {
        for (int wedgeChannel = 0; wedgeChannel < 16; ++wedgeChannel)
        {
            int found2 = 0;
            int currentChannel = 0;
            std::pair<int, int> ringWedgeKey = std::make_pair(ringChannel, wedgeChannel);
            for (auto iter = channelMap_.Begin(); iter != channelMap_.End(); iter++, currentChannel++)
            {
                auto chanInfo = channelMap_.FindChannel(currentChannel);
                if (chanInfo->second.type == typeWedge[detectorID_] && chanInfo->second.local_channel == wedgeChannel)
                {
                    detectorChannelToGlobalChanelMap_[ringWedgeKey].second = currentChannel;
                    found2++;
                }
                if (chanInfo->second.type == typeRing[detectorID_] && chanInfo->second.local_channel == ringChannel)
                {
                    detectorChannelToGlobalChanelMap_[ringWedgeKey].first = currentChannel;
                    found2++;
                }
                if (found2 == 2)
                    break; //save some time
            }
        }
    }
}

void DataFilter::SaveFilteredEnergyDataToFile(const std::string& fileName) const {
    std::ostringstream oss;

    for (const auto& pair : filteredEnergyData_) {
        oss << pair.first.first << "," << pair.first.second << "," << pair.second.first.size() << ",";

        // Write first vector
        for (const auto& val : pair.second.first) {
            oss << val << ",";
        }

        // Write second vector
        for (const auto& val : pair.second.second) {
            oss << val << ",";
        }

        // Remove trailing comma and add newline
        std::string line = oss.str();
        line = line.substr(0, line.size() - 1);  // remove trailing comma
        line += "\n";
        oss.str("");  // clear the stringstream
        oss << line;  // put the modified line back into the stringstream
    }

    // Write all to the file at once
    std::ofstream outputFile(fileName);
    if (outputFile.is_open()) {
        outputFile << oss.str();
        outputFile.close();
        std::cout << "Filtered energies are saved in " << fileName << std::endl;
    } else {
        std::cerr << "Unable to open file " << fileName << std::endl;
    }
}

std::map<std::pair<int, int>, std::pair<int, int>> DataFilter::GetGlobalChannelMap () const
{
    return detectorChannelToGlobalChanelMap_;
}
