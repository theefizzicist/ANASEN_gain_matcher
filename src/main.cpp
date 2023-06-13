#include "ArgumentParser.h"
#include "ConfigParser.h"
#include "ProgressBars.h"
#include "DataReader.h"
#include "DataFilter.h"
#include "BayesianSpnCalculatorOMP.h"
#include "ChiSquareMinimizer.h"
#include "ScatterPlotter.h"
#include <iostream>
#include <TSystem.h> // needed for gSystem
#include <cstdlib> // for exit constants
#include <chrono> // for timer
#include <thread> // for progress bars

int main(int argc, char* argv[])
{
    // Check if the shared library is loaded
    if (gSystem->Load("lib/libSPSDict.so") == -1) 
    {
        std::cerr << "Failed to load libSPSDict.so" << std::endl;
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
    const ExtractedData* sabreDataColumns = nullptr;

    // Get the columns
    try {
        detectorData = new DataReader(config.inputDirectory, runNumber, config.detectorID);
        detectorData->ReadAndExtractDataWTTree();

        // Get the data from the DataReader using the getter functions
        sabreDataColumns = &detectorData->GetExtractedData();
    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred while creating the DataReader: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // stop the progress bar
    isRunning = false;
    extractionThread.join();

    auto extractionEnd = std::chrono::high_resolution_clock::now();
    auto extractionTime = std::chrono::duration_cast<std::chrono::milliseconds>(extractionEnd - extractionStart).count();

    std::cout << std::endl << "Extraction completed. The process took " << extractionTime/1000.0 << " seconds.\n\n";

    /**************************************/
    /**** Filter the extracted columns ****/
    /**************************************/

    std::cout << "Filtering the columns...\n";

    auto filterStart = std::chrono::high_resolution_clock::now();

    // start the progress bar
    isRunning = true;
    std::thread filterThread(DisplayIndeterminateProgressBar);

    // Create a DataFilter object and call the filter_data function
    DataFilter dataFilterer(*sabreDataColumns, config.detectorID, config.channelMapFile);
    dataFilterer.FilterData();

    auto filteredEnergyData = dataFilterer.GetFilteredEnergyData();
    auto globalChannelMap = dataFilterer.GetGlobalChannelMap();

    // Save data to a file
    // dataFilterer.SaveFilteredEnergyDataToFile(outputDirectory + "/Energies.csv");

    // stop the progress bar
    isRunning = false;
    filterThread.join();
    
    auto filterEnd = std::chrono::high_resolution_clock::now();
    auto filterTime = std::chrono::duration_cast<std::chrono::milliseconds>(filterEnd - filterStart).count();

    std::cout << std::endl << "Filter completed. Process took " << filterTime/1000.0 << " seconds.\n\n";

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
        for (int wedgeChannel = 0; wedgeChannel < 8; ++wedgeChannel) {
            std::pair<int, int> key = std::make_pair(ringChannel, wedgeChannel);
            const auto& An = filteredEnergyData[key].first; // ring energies
            const auto& Ap = filteredEnergyData[key].second; // wedge energies

            // Ignore bad channels
            // if (ringChannel == 4 || ringChannel == 0) {
            //     currentIteration++;
            //     continue;
            // }
            
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
            DisplayDeterminateProgressBar(static_cast<double>(currentIteration) / (16*8));
        }
    }

    auto slopeEnd = std::chrono::high_resolution_clock::now();
    auto slopeTime = std::chrono::duration_cast<std::chrono::milliseconds>(slopeEnd - slopeStart).count();

    std::cout << std::endl << "Calculation completed. Process took " << slopeTime/1000.0 << " seconds.\n\n";

    /***********************************************/
    /****  Compute the calibration coefficients ****/
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

    auto plotEnd = std::chrono::high_resolution_clock::now();
    auto plotTime = std::chrono::duration_cast<std::chrono::milliseconds>(plotEnd - plotStart).count();

    std::cout << std::endl << "Plots completed. Process took " << plotTime/1000.0 << " seconds.\n\n";

    /***********************/
    /**** Total runtime ****/
    /***********************/

    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

    std::cout << std::endl << "Total execution time: " << totalTime/1000.0 << " seconds.\n";

    return 0;
}
