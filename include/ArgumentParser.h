#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <string>
#include <filesystem>

class ArgumentParser
{
public:
    ArgumentParser(int argc, char *argv[]);
    bool IsValid() const;
    int GetRunNumber() const;
    std::filesystem::path GetConfigFile() const;
    std::string GetUsage() const;

private:
    int runNumber_;
    std::filesystem::path configFile_;
    std::string usage_;
    bool valid_;
};

#endif
