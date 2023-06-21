#include "GlobalFactorCalculator.h"
#include <iostream>
#include "TH1D.h"
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RDF/RInterface.hxx>

GlobalFactorCalculator::GlobalFactorCalculator(const std::string& fileName, const int& detectorID, const std::string& outputDirectory)
    : detectorID_(detectorID), outputDirectory_(outputDirectory)
{
    OpenInputFile(fileName);
}

void GlobalFactorCalculator::OpenInputFile(const std::string& fileName)
{
    // Open the file
    am241File_ = std::unique_ptr<TFile>(TFile::Open(fileName.c_str(), "READ"));

    // Check if the file was properly opened
    if (!am241File_ || am241File_->IsZombie()) {
        std::cerr << "Error: Cannot open file or file does not exist: " << fileName << std::endl;
        std::cerr << "Setting scaling factor to 1.0." << std::endl;
        fileIsGood_ = false;
        return;
    }
    // Get the tree named "Data"
    dataTree_ = static_cast<TTree*>(am241File_->Get("Data"));

    // Check if the tree exists
    if (!dataTree_) {
        std::cerr << "Error: Tree 'Data' does not exist in file: " << fileName << std::endl;
        std::cerr << "Setting scaling factor to 1.0." << std::endl;
        fileIsGood_ = false;
        return;
    }

    fileIsGood_ = true;
}

void GlobalFactorCalculator::FindGlobalFactor()
{
    if (!fileIsGood_) {
        globalFactor_ = 1.0;
        return;
    }

    ROOT::EnableImplicitMT();  // Enable multi-threading

    ROOT::RDataFrame dataFrame(*dataTree_);

    const UShort_t energyThreshold = 1000;
    const UShort_t currentDetectorBoard = detectorID_ + 3;
    const UShort_t backMostRingForBoard3 = 15;
    const UShort_t backMostRingForOthers = 0;

    auto filteredDataFrame = dataFrame.Filter([=](UShort_t energy, UShort_t board, UShort_t channel) {
        return energy > energyThreshold && board == currentDetectorBoard && ((board == 3 && channel == backMostRingForBoard3) || channel == backMostRingForOthers);
    }, {"Energy", "Board", "Channel"});

    auto am241SpectrumHistogram = filteredDataFrame.Histo1D({"am241Spectrum", "Am-241 1D Spectrum", 1024, 0, 8192}, "Energy");

    // Write the histogram to a file
    TFile output((outputDirectory_ + "/am241_spectrum.root").c_str(), "RECREATE");
    am241SpectrumHistogram->Write();
    output.Close();

    ROOT::DisableImplicitMT();

    const double alphaEnergy = 5486.; // in MeV
    globalFactor_ = alphaEnergy / am241SpectrumHistogram->GetXaxis()->GetBinCenter(am241SpectrumHistogram->GetMaximumBin());
}

double GlobalFactorCalculator::GetGlobalFactor() const
{
    return globalFactor_;
}
