#ifndef GLOBAL_FACTOR_CALCULATOR_H
#define GLOBAL_FACTOR_CALCULATOR_H

#include <string>
#include <memory>
#include "TFile.h"
#include "TTree.h"

class GlobalFactorCalculator {
public:
    GlobalFactorCalculator(const std::string& fileName, const int& detectorID, const std::string& outputDirectory);
    void FindGlobalFactor();
    double GetGlobalFactor() const;

private:
    void OpenInputFile(const std::string& fileName);

    std::unique_ptr<TFile> am241File_;
    TTree* dataTree_;
    int detectorID_;
    std::string outputDirectory_;
    double globalFactor_;
    bool fileIsGood_;
};

#endif // GLOBAL_FACTOR_CALCULATOR_H