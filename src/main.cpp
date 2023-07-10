#include "ArgumentParser.h"
#include "ConfigParser.h"
#include "ProgressBars.h"
#include "DataReader.h"
#include "DataFilter.h"
#include "BayesianSpnCalculatorOMP.h"
#include "ChiSquareMinimizer.h"
#include "GlobalFactorCalculator.h"
#include "ScatterPlotter.h"
#include <iostream>
#include <TSystem.h> // needed for gSystem
#include <cstdlib> // for exit constants
#include <chrono> // for timer
#include <thread> // for progress bars

int main(int argc, char* argv[])
{
    // Check if the shared library is loaded
    if (gSystem->Load("lib/libEVBDict.so") == -1) 
    {
        std::cerr << "Failed to load libEVBDict.so" << std::endl;
        return EXIT_FAILURE;
    }

    // start the timer
    auto totalStart = std::chrono::high_resolution_clock::now();

    /*************************************/
    /**** Process the input arguments ****/
    /*************************************/

    ArgumentParser argParser(argc, argv);

    if (!argParser.IsValid())
    {
        std::cerr << "Error: Invalid arguments\n";
        std::cerr << argParser.GetUsage() << "\n";
        return EXIT_FAILURE;
    }

    int runNumber = argParser.GetRunNumber();
    std::filesystem::path configFile = argParser.GetConfigFile();

    /************************************/
    /**** Process configuration file ****/
    /************************************/

    std::cout << "Processing configuration file...";

    Config config;
    if (!ReadConfig(configFile, config)) 
    {
        return EXIT_FAILURE;
    }

    // Create the output directory
    std::string outputDirectory = "./outputs/run_" + std::to_string(runNumber) + "/detector_" + std::to_string(config.detectorID);
    std::filesystem::create_directories(outputDirectory);

    std::cout << "Done\n\n";

    /***********************************/
    /**** Extract necessary columns ****/
    /***********************************/

    std::cout << "Extracting necessary columns...\n";

    auto extractionStart = std::chrono::high_resolution_clock::now();

    // start the progress bar
    isRunning = true;
    std::thread extractionThread(DisplayIndeterminateProgressBar);

    DataReader* detectorData = nullptr;
    const ExtractedData* qqqDataColumns = nullptr;

    // Get the columns
    try {
        detectorData = new DataReader(config.inputDirectory, runNumber, config.detectorID);
        detectorData->ReadAndExtractDataWTTree();

        // Get the data from the DataReader using the getter functions
        qqqDataColumns = &detectorData->GetExtractedData();
    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred while creating the DataReader: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // stop the progress bar
    isRunning = false;
    extractionThread.join();
    std::cout << std::endl;

    auto extractionEnd = std::chrono::high_resolution_clock::now();
    auto extractionTime = std::chrono::duration_cast<std::chrono::milliseconds>(extractionEnd - extractionStart).count();

    std::cout << "Extraction completed. The process took " << extractionTime/1000.0 << " seconds.\n\n";

    /**************************************/
    /**** Filter the extracted columns ****/
    /**************************************/

    std::cout << "Filtering the columns...\n";

    auto filterStart = std::chrono::high_resolution_clock::now();

    // start the progress bar
    isRunning = true;
    std::thread filterThread(DisplayIndeterminateProgressBar);

    // Create a DataFilter object and call the filter_data function
    DataFilter dataFilterer(*qqqDataColumns, config.detectorID, config.channelMapFile);
    dataFilterer.FilterData();

    auto filteredEnergyData = dataFilterer.GetFilteredEnergyData();
    auto globalChannelMap = dataFilterer.GetGlobalChannelMap();

    // stop the progress bar
    isRunning = false;
    filterThread.join();
    std::cout << std::endl;

    // Save data to a file
    dataFilterer.SaveFilteredEnergyDataToFile(outputDirectory + "/Filtered_Energies.csv");
    
    auto filterEnd = std::chrono::high_resolution_clock::now();
    auto filterTime = std::chrono::duration_cast<std::chrono::milliseconds>(filterEnd - filterStart).count();

    std::cout << "Filter completed. Process took " << filterTime/1000.0 << " seconds.\n\n";

    /******************************/
    /**** Calculate the slopes ****/
    /******************************/

    std::cout << "Calculating slopes...\n";

    auto slopeStart = std::chrono::high_resolution_clock::now();

    BayesianSpnCalculatorOMP slopeCalculator(config.gridPointsForSlope, 
                                             config.gridPointsForOffset, 
                                             config.minSlope, 
                                             config.maxSlope, 
                                             config.minOffset, 
                                             config.maxOffset, 
                                             config.cauchyWidth);

    bool calculateOffset = false;

    /** Containers for slopes **/
    std::map<std::pair<int, int>, std::vector<double>> slopeDistributions;
    std::map<std::pair<int, int>, double> slopes;
    std::map<std::pair<int, int>, double> slopeErrors;
    std::map<std::pair<int, int>, std::pair<double, double>> slopeBounds;
    /***************************/

    /** Containers for offsets **/
    std::map<std::pair<int, int>, std::vector<double>> offsetDistributions;
    std::map<std::pair<int, int>, double> offsets;
    std::map<std::pair<int, int>, double> offsetErrors;
    std::map<std::pair<int, int>, std::pair<double, double>> offsetBounds;
    /****************************/

    double currentIteration = 0;
    for (int ringChannel = 0; ringChannel < 16; ++ringChannel) {
        for (int wedgeChannel = 0; wedgeChannel < 16; ++wedgeChannel) {
            std::pair<int, int> key = std::make_pair(ringChannel, wedgeChannel);
            const auto& An = filteredEnergyData[key].first; // ring energies
            const auto& Ap = filteredEnergyData[key].second; // wedge energies

            // Ignore bad channels
            if ( (config.detectorID==1 && ringChannel==4) || (config.detectorID==1 && ringChannel==5) || (config.detectorID==1 && ringChannel==8) || (config.detectorID==1 && ringChannel==13) || (config.detectorID==1 && wedgeChannel==11) || (config.detectorID==2 && ringChannel==0) || (config.detectorID==2 && ringChannel==1) || (config.detectorID==2 && ringChannel==2) || (config.detectorID==3 && wedgeChannel==9) || (config.detectorID==2 && wedgeChannel==14) || (config.detectorID==2 && wedgeChannel==4) ) {
                currentIteration++;
                continue;
            }
            
            if (An.size() != 0 && Ap.size() != 0) {

                auto calculationStart = std::chrono::high_resolution_clock::now();
                
                if (calculateOffset)
                {
                    /** Use this to calculate both slope and offset **/
                    auto [slope, slopeLowerBound, slopeUpperBound, slopeDist,
                          offset, offsetLowerBound, offsetUpperBound, offsetDist] = slopeCalculator.CalculateSlopeAndOffset(Ap, An);
                    slopeDistributions[key] = slopeDist;
                    slopes[key] = slope;
                    slopeBounds[key] = std::make_pair(slopeLowerBound, slopeUpperBound);
                    slopeErrors[key] = std::max(slopeUpperBound - slope, slope - slopeLowerBound);
                    offsetDistributions[key] = offsetDist;
                    offsets[key] = offset;
                    offsetBounds[key] = std::make_pair(offsetLowerBound, offsetUpperBound);
                    offsetErrors[key] = std::max(offsetUpperBound - offset, offset - offsetLowerBound);
                }
                else
                {
                    /** Use this to calculate only slope **/
                    auto [slope, slopeLowerBound, slopeUpperBound, slopeDist] = slopeCalculator.CalculateSlope(Ap, An);
                    slopeDistributions[key] = slopeDist;
                    slopes[key] = slope;
                    slopeBounds[key] = std::make_pair(slopeLowerBound, slopeUpperBound);
                    slopeErrors[key] = std::max(slopeUpperBound - slope, slope - slopeLowerBound);
                }
            }

            currentIteration++;
            DisplayDeterminateProgressBar(static_cast<double>(currentIteration) / (16*16));
        }
    }

    auto slopeEnd = std::chrono::high_resolution_clock::now();
    auto slopeTime = std::chrono::duration_cast<std::chrono::milliseconds>(slopeEnd - slopeStart).count();

    std::cout << std::endl << "Calculation completed. Process took " << slopeTime/1000.0 << " seconds.\n\n";

    /***************************************/
    /**** Compute global scaling factor ****/
    /***************************************/

    std::cout << "Calculating global scaling factor...\n";

    auto scalingStart = std::chrono::high_resolution_clock::now();

    // start the progress bar
    isRunning = true;
    std::thread scalingFactorThread(DisplayIndeterminateProgressBar);

    GlobalFactorCalculator factorCalculator(config.am241File, config.detectorID, outputDirectory);
    factorCalculator.FindGlobalFactor();
    const double globalScalingFactor = factorCalculator.GetGlobalFactor();

    // stop the progress bar
    isRunning = false;
    scalingFactorThread.join();
    std::cout << std::endl;

    auto scalingEnd = std::chrono::high_resolution_clock::now();
    auto scalingTime = std::chrono::duration_cast<std::chrono::milliseconds>(scalingEnd - scalingStart).count();
    std::cout << "Calculation completed. Process took " << scalingTime/1000.0 << " seconds.\n\n";

    /***********************************************/
    /**** Compute the calibration coefficients ****/
    /***********************************************/

    std::cout << "Calculating calibration coefficients...\n";

    auto chi2Start = std::chrono::high_resolution_clock::now();

    std::vector<double> ringCoefficients;
    std::vector<double> wedgeCoefficients;
    std::vector<double> ringOffsets;
    std::vector<double> wedgeOffsets;

    if (calculateOffset)
    {
        ChiSquareMinimizer minimizer(slopes, slopeErrors, offsets, offsetErrors, config.detectorID);

        minimizer.FitSlopeAndOffset();
        
        // Retrieve the optimized ringCoefficients and wedgeCoefficients
        ringCoefficients = minimizer.GetRingCoefficients();
        wedgeCoefficients = minimizer.GetWedgeCoefficients();

        // Retrieve the optimized ringOffsets and wedgeOffsets
        ringOffsets = minimizer.GetRingOffsets();
        wedgeOffsets = minimizer.GetWedgeOffsets();
    }
    else
    {
        ChiSquareMinimizer minimizer(slopes, slopeErrors, config.detectorID);

        minimizer.FitSlopeOnly();

        // Retrieve the optimized ringCoefficients and wedgeCoefficients
        ringCoefficients = minimizer.GetRingCoefficients();
        wedgeCoefficients = minimizer.GetWedgeCoefficients();

        std::string coefficientsFileName = outputDirectory + "/det" + std::to_string(config.detectorID) + "_internal_gain_factor.txt"; 
        minimizer.SaveCoefficientsToFile(coefficientsFileName, globalChannelMap);

        std::string gainMapFileName = outputDirectory + "/det" + std::to_string(config.detectorID) + "_global_gain_factor.txt";
        minimizer.SaveCoefficientsWithScalingToFile(gainMapFileName, globalChannelMap, globalScalingFactor);
    }

    auto chi2End = std::chrono::high_resolution_clock::now();
    auto chi2Time = std::chrono::duration_cast<std::chrono::milliseconds>(chi2End - chi2Start).count();

    std::cout << std::endl << "Calculation completed. Process took " << chi2Time/1000.0 << " seconds.\n\n";

    /*****************************/
    /**** Make and save plots ****/
    /*****************************/

    std::cout << "Creating plots...\n";

    auto plotStart = std::chrono::high_resolution_clock::now();

    // start the progress bar
    isRunning = true;
    std::thread plotThread(DisplayIndeterminateProgressBar);

    ScatterPlotter plotter(filteredEnergyData, outputDirectory, config.maxSlope, config.minSlope, config.maxOffset, config.minOffset, globalChannelMap);
    if (calculateOffset)
    {
        plotter.CreateDistributionPlots(slopeDistributions, slopes, slopeBounds, offsetDistributions, offsets, offsetBounds);
        plotter.CreateOverlayPlot(ringCoefficients, wedgeCoefficients, ringOffsets, wedgeOffsets);
    }
    else
    {
        plotter.CreateDistributionPlots(slopeDistributions, slopes, slopeBounds);
        plotter.CreateOverlayPlot(ringCoefficients, wedgeCoefficients);
    }

    // stop the progress bar
    isRunning = false;
    plotThread.join();
    std::cout << std::endl;

    auto plotEnd = std::chrono::high_resolution_clock::now();
    auto plotTime = std::chrono::duration_cast<std::chrono::milliseconds>(plotEnd - plotStart).count();

    std::cout << "Plots completed. Process took " << plotTime/1000.0 << " seconds.\n\n";

    /***********************/
    /**** Total runtime ****/
    /***********************/

    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

    std::cout << std::endl << "Total execution time: " << totalTime/1000.0 << " seconds.\n";

    return 0;
}
