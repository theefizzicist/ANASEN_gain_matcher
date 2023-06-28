void saveGainFactorsToFile(const std::string& outputPath, const std::map<int, double>& gainFactors) 
{
    std::ofstream outputFile(outputPath);
    for (const auto& kv : gainFactors) 
    {
        outputFile << "Chans " << kv.first << "\t" << kv.second << "\n";
    }
    outputFile.close();
    std::cout << "Saved data to " << outputPath << std::endl;
}


void printGainFactors(const std::map<int, double>& gainFactors) 
{
    for (const auto& kv : gainFactors) 
    {
        std::cout << "Channel: " << kv.first << ", Gain Factor: " << kv.second << std::endl;
    }
}

std::map<int, double> readGainFactors(TString filePath) 
{
    std::map<int, double> gainFactors;

    std::ifstream file(filePath);
    std::string line;
    while (std::getline(file, line)) 
    {
        std::istringstream ss(line);
        std::string dummy;
        int channel;
        double gainFactor;
        ss >> dummy >> channel >> gainFactor;

        if (gainFactors.find(channel) == gainFactors.end()) 
        {
            // Key doesn't exist, so we can add it
            gainFactors[channel] = gainFactor;
        } 
        else 
        {
            std::cout << "Warning: Repeated key " << channel << " in file " << filePath << ". Skipping this entry." << std::endl;
        }
    }

    return gainFactors;
}

void combineGainMaps(int runNumber) 
{
    std::map<int, double> globalGainFactors;
    std::map<int, double> internalGainFactors;

    for (int detNumber = 0; detNumber <= 4; detNumber++) {
        TString basePath = TString::Format("./outputs/run_%d/detector_%d/", runNumber, detNumber);
        TString globalGainFile = basePath + TString::Format("det%d_global_gain_factor.txt", detNumber);
        TString internalGainFile = basePath + TString::Format("det%d_internal_gain_factor.txt", detNumber);

        if (gSystem->AccessPathName(globalGainFile) || gSystem->AccessPathName(internalGainFile)) {
            std::cout << "One or both files for detector " << detNumber << " do not exist. Skipping this detector." << std::endl;
            continue;
        }

        std::map<int, double> globalGainFactorsTemp = readGainFactors(globalGainFile);
        std::map<int, double> internalGainFactorsTemp = readGainFactors(internalGainFile);

        globalGainFactors.insert(globalGainFactorsTemp.begin(), globalGainFactorsTemp.end());
        internalGainFactors.insert(internalGainFactorsTemp.begin(), internalGainFactorsTemp.end());
    }

    // Now you can access the data from outside the loop using the channel
    // For example:
    // double someGainFactor = globalGainFactors[112];

    // Prepare output paths
    TString outputBasePath = TString::Format("./outputs/run_%d/run_%d_", runNumber, runNumber);
    TString globalOutputPath = outputBasePath + "combinedGlobalGainFactors.txt";
    TString internalOutputPath = outputBasePath + "combinedInternalGainFactors.txt";

    // Print and save global gain factors
    std::cout << "\nGlobal Gain Factors:" << std::endl;
    // printGainFactors(globalGainFactors);
    saveGainFactorsToFile(globalOutputPath.Data(), globalGainFactors);

    // Print and save internal gain factors
    std::cout << "\nInternal Gain Factors:" << std::endl;
    // printGainFactors(internalGainFactors);
    saveGainFactorsToFile(internalOutputPath.Data(), internalGainFactors);
}