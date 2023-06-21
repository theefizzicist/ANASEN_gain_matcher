#ifndef CHISQUAREMINIMIZER_H
#define CHISQUAREMINIMIZER_H

#include <map>
#include <vector>
#include <string>

class ChiSquareMinimizer {
private:
    std::map<std::pair<int, int>, double> slopes_, slopeErrors_;
    std::map<std::pair<int, int>, double> offsets_, offsetErrors_;
    std::vector<double> ringCoefficients_, wedgeCoefficients_;
    std::vector<double> ringOffsets_, wedgeOffsets_;
    double slopeOnlyInitialParametersVector_[23];
    double slopeAndOffsetInitialParametersVector_[46];
    int detectorID_;

public:
    // Constructor for slope-only minimization
    ChiSquareMinimizer(const std::map<std::pair<int, int>, double>& slopes, 
                       const std::map<std::pair<int, int>, double>& slopeErrors, 
                       int detectorID);

    // Constructor for slope and offset minimization
    ChiSquareMinimizer(const std::map<std::pair<int, int>, double>& slopes, 
                       const std::map<std::pair<int, int>, double>& slopeErrors,
                       const std::map<std::pair<int, int>, double>& offsets,
                       const std::map<std::pair<int, int>, double>& offsetErrors,  
                       int detectorID);

    double Chi2FunctionSlopeOnly(const double *param);

    double Chi2FunctionSlopeAndOffset(const double *param);

    void FitSlopeOnly();

    void FitSlopeAndOffset();

    std::vector<double> GetRingCoefficients() const;

    std::vector<double> GetWedgeCoefficients() const;

    std::vector<double> GetRingOffsets() const;

    std::vector<double> GetWedgeOffsets() const;

    void SaveCoefficientsToFile(const std::string& fileName, const std::map<std::pair<int, int>, std::pair<int, int>>& globalChannelMap) const;

    void SaveCoefficientsWithScalingToFile(const std::string& fileName, const std::map<std::pair<int, int>, 
                                           std::pair<int, int>>& globalChannelMap,
                                           const double& scalingFactor) const;
};

#endif
