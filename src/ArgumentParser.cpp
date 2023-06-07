#include "ArgumentParser.h"
#include <iostream>
#include <stdexcept>

ArgumentParser::ArgumentParser(int argc, char *argv[])
    : runNumber_(0), valid_(false)
{
    if (argc != 3)
    {
        usage_ = std::string(argv[0]) + " <run number> <config file>";
        return;
    }

    try
    {
        size_t end_pos;
        runNumber_ = std::stoi(argv[1], &end_pos);

        if (end_pos != std::string(argv[1]).length())
        {
            throw std::invalid_argument("Incomplete parsing");
        }
    }
    catch (const std::invalid_argument &)
    {
        usage_ = "Invalid run number (not an integer): " + std::string(argv[1]) + "\n";
        usage_ += std::string(argv[0]) + " <run number> <config file>";
        return;
    }
    catch (const std::out_of_range &)
    {
        usage_ = "Run number out of range: " + std::string(argv[1]) + "\n";
        usage_ += std::string(argv[0]) + " <run number> <config file>";
        return;
    }

    configFile_ = argv[2];
    if (!std::filesystem::exists(configFile_))
    {
        usage_ = "Config file does not exist: " + std::string(argv[2]) + "\n";
        usage_ += std::string(argv[0]) + " <run number> <config file>";
        return;
    }

    valid_ = true;
}

bool ArgumentParser::IsValid() const
{
    return valid_;
}

int ArgumentParser::GetRunNumber() const
{
    return runNumber_;
}

std::filesystem::path ArgumentParser::GetConfigFile() const
{
    return configFile_;
}

std::string ArgumentParser::GetUsage() const
{
    return usage_;
}
