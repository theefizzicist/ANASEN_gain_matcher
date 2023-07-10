#include "ChiSquareMinimizer.h"
#include <Math/Functor.h>
#include <Fit/Fitter.h>
#include "TString.h"
#include <fstream>
#include <set>

ChiSquareMinimizer::ChiSquareMinimizer(const std::map<std::pair<int, int>, double>& slopes, 
                                       const std::map<std::pair<int, int>, double>& slopeErrors, 
                                       int detectorID)
    : slopes_(slopes), slopeErrors_(slopeErrors), detectorID_(detectorID)
{
    // Initialize ringCoefficients and wedgeCoefficients
    ringCoefficients_ = std::vector<double>(16, 1);
    wedgeCoefficients_ = std::vector<double>(16, 1);

    // Initialize paramVector
    for (int i = 0; i < 31; i++)
        slopeOnlyInitialParametersVector_[i] = 1;
}

ChiSquareMinimizer::ChiSquareMinimizer(const std::map<std::pair<int, int>, double>& slopes, 
                                       const std::map<std::pair<int, int>, double>& slopeErrors,
                                       const std::map<std::pair<int, int>, double>& offsets,
                                       const std::map<std::pair<int, int>, double>& offsetErrors,  
                                       int detectorID)
    : slopes_(slopes), slopeErrors_(slopeErrors), offsets_(offsets), offsetErrors_(offsetErrors), detectorID_(detectorID)
{
    // Initialize ringCoefficients and wedgeCoefficients
    ringCoefficients_ = std::vector<double>(16, 1);
    wedgeCoefficients_ = std::vector<double>(16, 1);

    // Initialize ringOffsets and wedgeOffsets
    ringOffsets_ = std::vector<double>(16, 0);
    wedgeOffsets_ = std::vector<double>(16, 0);

    // Initialize paramVector
    for (int i = 0; i < 62; i++) {
        if (i < 31) slopeAndOffsetInitialParametersVector_[i] = 1;
        else        slopeAndOffsetInitialParametersVector_[i] = 0;
    }
}

double ChiSquareMinimizer::Chi2FunctionSlopeOnly(const double *param) {
    ringCoefficients_[0] = 1.0;
    for(int i=0; i<31; i++) {
        if(i<15) ringCoefficients_[i+1] = param[i];
        else wedgeCoefficients_[i-15] = param[i];
    }

    double chi2 = 0.0;

    for(int i=0; i<16; i++) {
        for(int j=0; j<16; j++) {
            std::pair<int, int> key = std::make_pair(i, j);
            
            if (slopes_.find(key) == slopes_.end() || slopeErrors_.find(key) == slopeErrors_.end()) {
                // std::cout << "No slope or slope error at key (" << i << "," << j << ")\n";
                continue;
            }

            double residual = (ringCoefficients_[i]/wedgeCoefficients_[j] - slopes_.at(key))/slopeErrors_.at(key);
            chi2 += residual * residual;
        }
    }
    return chi2;
}

double ChiSquareMinimizer::Chi2FunctionSlopeAndOffset(const double *param) {
    ringCoefficients_[0] = 1.0;
    ringOffsets_[0] = 0.0;
    // Note that index  0->14 correspond to ring  channel 1->15
    //                 15->30 correspond to wedge channel 0->15
    //                 31->45 correspond to ring  channel 1->15
    //                 46->61 correspond to wedge channel 0->15
    for (int i=0; i<62; i++) {
        if (i < 31) {
            if (i < 15) ringCoefficients_[i + 1] = param[i];
            else        wedgeCoefficients_[i - 15] = param[i];
        }
        else {
            if (i < 46) ringOffsets_[i + 1 - 31] = param[i];
            else        wedgeOffsets_[i - 46] = param[i];
        }
    }

    double chi2 = 0.0;

    for(int i=0; i<16; i++) {
        for(int j=0; j<16; j++) {
            std::pair<int, int> key = std::make_pair(i, j);
            
            if (slopes_.find(key) == slopes_.end() || slopeErrors_.find(key) == slopeErrors_.end() ||
                offsets_.find(key) == offsets_.end() || offsetErrors_.find(key) == offsetErrors_.end()) {
                // std::cout << "No slope or slope error at key (" << i << "," << j << ")\n";
                continue;
            }

            // Compute residuals for slope and offset
            double slopeResidual = (ringCoefficients_[i]/wedgeCoefficients_[j] - slopes_.at(key))/slopeErrors_.at(key);
            double offsetResidual = (offsets_.at(key) - ((ringOffsets_[i] - wedgeOffsets_[j])/wedgeCoefficients_[j]))/offsetErrors_.at(key);

            chi2 += slopeResidual * slopeResidual + offsetResidual * offsetResidual;
        }
    }
    return chi2;
}

void ChiSquareMinimizer::FitSlopeOnly() {
    ROOT::Math::Functor fcn([this](const double *param) { return this->Chi2FunctionSlopeOnly(param); }, 31);
    ROOT::Fit::Fitter fitter;

    fitter.SetFCN(fcn, slopeOnlyInitialParametersVector_);
    for(int i=0; i<31; i++) {
        if(i<15)  fitter.Config().ParSettings(i).SetName(Form("ring%d_Coefficient",i+1));
        else fitter.Config().ParSettings(i).SetName(Form("wedge%d_Coeffiecient",i-15));
    }

    bool ok = fitter.FitFCN();
    if (!ok) {
        Error("Fit Error","Fit failed");
    }

    const ROOT::Fit::FitResult & result = fitter.Result();
    result.Print(std::cerr);

    const double *paramResults = result.GetParams();

    ringCoefficients_[0] = 1.0;
    for(int i=0; i<31; i++) {
        if(i<15) ringCoefficients_[i+1] = paramResults[i];
        else wedgeCoefficients_[i-15] = paramResults[i];
    }
}

void ChiSquareMinimizer::FitSlopeAndOffset() {
    ROOT::Math::Functor fcn([this](const double *param) { return this->Chi2FunctionSlopeAndOffset(param); }, 62);
    ROOT::Fit::Fitter fitter;

    fitter.SetFCN(fcn, slopeAndOffsetInitialParametersVector_);
    for(int i=0; i<62; i++) {
        if (i < 31) {
            if(i<15)  fitter.Config().ParSettings(i).SetName(Form("ring%d_Coefficient",i+1));
            else fitter.Config().ParSettings(i).SetName(Form("wedge%d_Coefficient",i-15));
        }
        else {
            if (i < 46) fitter.Config().ParSettings(i).SetName(Form("ring%d_Offset",i+1-31));
            else        fitter.Config().ParSettings(i).SetName(Form("wedge%d_Offset",i-46));
        }
    }

    bool ok = fitter.FitFCN();
    if (!ok) {
        Error("Fit Error","Fit failed");
    }

    const ROOT::Fit::FitResult & result = fitter.Result();
    result.Print(std::cerr);

    const double *paramResults = result.GetParams();

    ringCoefficients_[0] = 1.0;
    ringOffsets_[0] = 0.0;
    for(int i=0; i<62; i++) {
        if (i < 31) {
            if(i<15) ringCoefficients_[i+1] = paramResults[i];
            else wedgeCoefficients_[i-15] = paramResults[i];
        }
        else {
            if (i < 46) ringOffsets_[i+1-31] = paramResults[i];
            else wedgeOffsets_[i-46] = paramResults[i];
        }
    }
}

std::vector<double> ChiSquareMinimizer::GetRingCoefficients() const {
    return ringCoefficients_;
}

std::vector<double> ChiSquareMinimizer::GetWedgeCoefficients() const {
    return wedgeCoefficients_;
}

std::vector<double> ChiSquareMinimizer::GetRingOffsets() const {
    return ringOffsets_;
}

std::vector<double> ChiSquareMinimizer::GetWedgeOffsets() const {
    return wedgeOffsets_;
}

void ChiSquareMinimizer::SaveCoefficientsToFile(const std::string& fileName, const std::map<std::pair<int, int>, std::pair<int, int>>& globalChannelMap) const
{
    std::ofstream file(fileName);

    if (!file.is_open())
    {
        std::cerr << "Unable to open file " << fileName << "\n";
        return;
    }

    std::set<std::pair<int, int>> seenRingPairs;
    std::set<std::pair<int, int>> seenWedgePairs;

    for (const auto& localAndGlobalPair : globalChannelMap)
    {
        const auto& detectorChannel = localAndGlobalPair.first;
        int ringDChannel = detectorChannel.first;
        int wedgeDChannel = detectorChannel.second;

        const auto& globalChannel = localAndGlobalPair.second;
        int ringGChannel = globalChannel.first;
        int wedgeGChannel = globalChannel.second;

        auto currentRingPair = std::make_pair(ringGChannel, ringDChannel);
        auto currentWedgePair = std::make_pair(wedgeGChannel, wedgeDChannel);

        if (seenRingPairs.insert(currentRingPair).second)
        {
            file << "Chans " << ringGChannel << "\t" << ringCoefficients_[ringDChannel] << std::endl;
        }

        if (seenWedgePairs.insert(currentWedgePair).second)
        {
            file << "Chans " << wedgeGChannel << "\t" << wedgeCoefficients_[wedgeDChannel] << std::endl;
        }
    }
}

void ChiSquareMinimizer::SaveCoefficientsWithScalingToFile(const std::string& fileName, const std::map<std::pair<int, int>, 
                                                           std::pair<int, int>>& globalChannelMap,
                                                           const double& scalingFactor) const
{
    std::ofstream file(fileName);

    if (!file.is_open())
    {
        std::cerr << "Unable to open file " << fileName << "\n";
        return;
    }

    std::set<std::pair<int, int>> seenRingPairs;
    std::set<std::pair<int, int>> seenWedgePairs;

    for (const auto& localAndGlobalPair : globalChannelMap)
    {
        const auto& detectorChannel = localAndGlobalPair.first;
        int ringDChannel = detectorChannel.first;
        int wedgeDChannel = detectorChannel.second;

        const auto& globalChannel = localAndGlobalPair.second;
        int ringGChannel = globalChannel.first;
        int wedgeGChannel = globalChannel.second;

        auto currentRingPair = std::make_pair(ringGChannel, ringDChannel);
        auto currentWedgePair = std::make_pair(wedgeGChannel, wedgeDChannel);

        if (seenRingPairs.insert(currentRingPair).second)
        {
            file << "Chans " << ringGChannel << "\t" << scalingFactor * ringCoefficients_[ringDChannel] << std::endl;
        }

        if (seenWedgePairs.insert(currentWedgePair).second)
        {
            file << "Chans " << wedgeGChannel << "\t" << scalingFactor * wedgeCoefficients_[wedgeDChannel] << std::endl;
        }
    }
}
