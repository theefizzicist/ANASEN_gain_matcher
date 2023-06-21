#include "ConfigParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

bool ReadConfig(const std::filesystem::path& configFile, Config& config) 
{
    std::ifstream file(configFile);
    if (!file.is_open()) 
    {
        std::cerr << "Error: Cannot open config file." << std::endl;
        return false;
    }

    std::map<std::string, std::string> configMap;
    std::string line, key, value;
    while (std::getline(file, line)) 
    {
        if (line.empty()) continue;
        if (line[0] == '[') 
        {
            key = line.substr(1, line.size() - 2);
        } 
        else 
        {
            value = line;
            configMap[key] = value;
        }
    }

    file.close();

    try {
        config.inputDirectory = configMap["input_directory"];
        config.channelMapFile = configMap["channel_map_file"];
        config.am241File = configMap["Am241_file"];
        config.detectorID = std::stoi(configMap["detector_id"]);
        config.gridPointsForSlope = std::stoi(configMap["grid_points_for_slope"]);
        config.gridPointsForOffset = std::stoi(configMap["grid_points_for_offset"]);
        config.minSlope = std::stod(configMap["min_slope"]);
        config.maxSlope = std::stod(configMap["max_slope"]);
        config.minOffset = std::stod(configMap["min_offset"]);
        config.maxOffset = std::stod(configMap["max_offset"]);
        config.cauchyWidth = std::stod(configMap["cauchy_width"]);
    } 
    catch (const std::invalid_argument &e) 
    {
        std::string currentKey = key;
        std::cerr << "Error: Invalid value in config file for key '" << currentKey << "' with value '" << value << "'. " << e.what() << std::endl;
        return false;
    }

    return true;
}
