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

//sorts peak positions from lowest to highest
bool SortPosition(const double i, const double j)
{
  return i<j;
}

void GlobalFactorCalculator::FindGlobalFactor()
{
    if (!fileIsGood_) {
        globalFactor_ = 1.0;
        return;
    }

    ROOT::EnableImplicitMT();  // Enable multi-threading

    ROOT::RDataFrame dataFrame(*dataTree_);

    const UShort_t energyThreshold = 500;
    const UShort_t currentDetectorBoard = (detectorID_ > 1) ? 5 : 0;
    const UShort_t backMostRingForDet0 = 10;
    const UShort_t backMostRingForDet1 = 30;
    const UShort_t backMostRingForDet2 = 21;
    const UShort_t backMostRingForDet3 = 9;
    //Note: picking out board & channel numbers for rings with least straggling (alpha data)
    // detID board chan
    // 0     0     10
    // 1     0     30
    // 2     5     21
    // 3     5     9

    auto filteredDataFrame = dataFrame.Filter([=](UShort_t energy, UShort_t board, UShort_t channel) {
        return energy > energyThreshold && board == currentDetectorBoard && ( (detectorID_ == 0 && channel == backMostRingForDet0) || (detectorID_ == 1 && channel == backMostRingForDet1) || (detectorID_ == 2 && channel == backMostRingForDet2) || (detectorID_ == 3 && channel == backMostRingForDet3));
    }, {"Energy", "Board", "Channel"});

    auto am241SpectrumHistogram = filteredDataFrame.Histo1D({"alphaSpectrum", "Pu-239, Am-241, Cm-244 1D Spectrum", 1024, 0, 16384}, "Energy");

    // Write the histogram to a file
    TFile output((outputDirectory_ + "/alpha_spectrum.root").c_str(), "RECREATE");
    am241SpectrumHistogram->Write();
    output.Close();

    ROOT::DisableImplicitMT();

    //get 2-3 tallest peaks from triple-alpha spectrum
    double xmax = am241SpectrumHistogram->GetMaximum();
    int binmax = am241SpectrumHistogram->GetNbinsX();
    int bintemp;
    std::vector<double> xVec;

    //threshold set high enough so only center bins of 2 highest peaks get found
    while(am241SpectrumHistogram->FindLastBinAbove(xmax*0.8,1,1,binmax)>-1) //FindLastBinAbove(threshold,1,firstBin,lastBin) returns -1 if none found
    {
      //grab that bin
      bintemp = am241SpectrumHistogram->FindLastBinAbove(xmax*0.8,1,1,binmax);
      //push the bin center to vector of x values
      xVec.push_back(am241SpectrumHistogram->GetXaxis()->GetBinCenter(bintemp));
      //std::cout<<"\tfound peak: "<<xVec[xVec.size()-1]<<std::endl;
      //reset binmax
      binmax = bintemp-1;
    }
    std::sort(xVec.begin(), xVec.end(), SortPosition); //sort lowest to highest energy (position)
    //if(xVec[1]>0)std::cout<<"\nfound Am-241 peak at "<<xVec[1]<<" for detector "<<detectorID_<<std::endl;

    const double alphaEnergy = 5486.; // in MeV
    globalFactor_ = alphaEnergy / xVec[1]; //2nd peak is Am-241
    std::cout<<"\nglobalFactor = "<<globalFactor_<<std::endl;
}

double GlobalFactorCalculator::GetGlobalFactor() const
{
    return globalFactor_;
}
