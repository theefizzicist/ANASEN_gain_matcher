#ifndef SCATTER_PLOTTER_H
#define SCATTER_PLOTTER_H

#include <map>
#include <vector>
#include <utility>
#include <string>

class ScatterPlotter {
public:
    ScatterPlotter(const std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<double>>>& filteredEnergyData,
                   const std::string& output_directory,
                   const double& maxSlope,
                   const double& minSlope,
                   const double& maxOffset,
                   const double& minOffset,
                   const std::map<std::pair<int, int>, std::pair<int, int>>& globalChannelMap);

    void CreateDistributionPlots(const std::map<std::pair<int, int>, std::vector<double>>& fit_distribution, 
                                 const std::map<std::pair<int, int>, double>& slopes,
                                 const std::map<std::pair<int, int>, std::pair<double, double>>& slopeBounds);
    void CreateDistributionPlots(const std::map<std::pair<int, int>, std::vector<double>>& slopeDistribution, 
                                 const std::map<std::pair<int, int>, double>& slopes,
                                 const std::map<std::pair<int, int>, std::pair<double, double>>& slopeBounds,
                                 const std::map<std::pair<int, int>, std::vector<double>>& offsetDistribution, 
                                 const std::map<std::pair<int, int>, double>& offsets,
                                 const std::map<std::pair<int, int>, std::pair<double, double>>& offsetBounds);
    void CreateOverlayPlot(const std::vector<double>& ringCoefficients,
                           const std::vector<double>& wedgeCoefficients);
    void CreateOverlayPlot(const std::vector<double>& ringCoefficients,
                           const std::vector<double>& wedgeCoefficients,
                           const std::vector<double>& ringOffsets,
                           const std::vector<double>& wedgeOffsets);

private:
    std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<double>>> filteredEnergyData_;
    std::string outputDirectory_;
    double maxSlope_, minSlope_, maxOffset_, minOffset_;
    std::map<int, int> globalRingChanAt_, globalWedgeChanAt_;
    std::map<std::pair<int, int>, int> maxRangeValues_;
    int maxRange_; // the maximum value out of all maxRangeValues_
};

#endif // SCATTER_PLOTTER_H
