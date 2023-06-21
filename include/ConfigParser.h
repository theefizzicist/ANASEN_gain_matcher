#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <filesystem>

struct Config 
{
    std::string inputDirectory;
    std::string channelMapFile;
    std::string am241File;
    int detectorID;
    int gridPointsForSlope;
    int gridPointsForOffset;
    double minSlope;
    double maxSlope;
    double minOffset;
    double maxOffset;
    double cauchyWidth;
};

bool ReadConfig(const std::filesystem::path& configFile, Config& config);

#endif // CONFIG_PARSER_H
