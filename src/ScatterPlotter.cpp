#include "ScatterPlotter.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TPad.h"
#include "TError.h"
#include "TAxis.h"
#include "TMath.h"
#include <iostream>
#include "TLine.h"
#include "TH2.h"
#include "TStyle.h"
#include "TColor.h"
#include <algorithm>
#include <set>

ScatterPlotter::ScatterPlotter(const std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<double>>>& filteredEnergyData,
                               const std::string& output_directory,
                               const double& maxSlope,
                               const double& minSlope,
                               const double& maxOffset,
                               const double& minOffset,
                               const std::map<std::pair<int, int>, std::pair<int, int>>& globalChannelMap)
    : filteredEnergyData_(filteredEnergyData), outputDirectory_(output_directory), 
      maxSlope_(maxSlope), minSlope_(minSlope), 
      maxOffset_(maxOffset), minOffset_(minOffset)
{
    // fill out maxRangeValues for later use
    maxRange_ = 0.0;
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            std::pair<int, int> key = std::make_pair(i, j);

            if (filteredEnergyData.find(key) == filteredEnergyData.end()){
                continue;
            }

            std::vector<double> currentRingEnergy = filteredEnergyData.at(key).first;
            std::vector<double> currentWedgeEnergy = filteredEnergyData.at(key).second;
            if (currentRingEnergy.size() == 0 || currentWedgeEnergy.size() ==0) continue;

            int maxEnergy = static_cast<int>(std::max(*std::max_element(currentRingEnergy.begin(), currentRingEnergy.end()),
                                                      *std::max_element(currentWedgeEnergy.begin(), currentWedgeEnergy.end())));
            maxRangeValues_[key] = static_cast<int>(1.3 * maxEnergy);
            maxRange_ = std::max(maxRange_, maxRangeValues_.at(key));
        }
    }

    // fill out maps that convert detector channel to global channel
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

        auto currentRingPair = std::make_pair(ringDChannel, ringGChannel);
        auto currentWedgePair = std::make_pair(wedgeDChannel, wedgeGChannel);

        if (seenRingPairs.insert(currentRingPair).second)
        {
            globalRingChanAt_[ringDChannel] = ringGChannel;
        }

        if (seenWedgePairs.insert(currentWedgePair).second)
        {
            globalWedgeChanAt_[wedgeDChannel] = wedgeGChannel;
        }
    }
}

void ScatterPlotter::CreateDistributionPlots(const std::map<std::pair<int, int>, std::vector<double>>& fit_distribution, 
                                             const std::map<std::pair<int, int>, double>& slopes,
                                             const std::map<std::pair<int, int>, std::pair<double, double>>& slopeBounds) 
{
    auto temp_gErrorIgnoreLevel = gErrorIgnoreLevel;
    gErrorIgnoreLevel = kFatal;

    TCanvas* c1 = new TCanvas("c1", "Plots", 2300, 800);

    std::string outputFileName = outputDirectory_ + "/distribution_plots.pdf";
    c1->Print((outputFileName + "[").c_str()); // start PDF file
    
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 8; j++){
            std::pair<int, int> key = std::make_pair(i, j);

            if(filteredEnergyData_.find(key) == filteredEnergyData_.end()){
                continue;
            }

            if(fit_distribution.find(key) == fit_distribution.end()){
                continue;
            }

            c1->Clear();
            c1->Divide(2, 1); // split canvas into two parts

            int currentMaxRange = maxRangeValues_.at(key);
            int nBins = static_cast<int>(0.1 * currentMaxRange);

            // create 2D histogram
            c1->cd(1); // switch to first pad
            gPad->SetMargin(0.15, 0.15, 0.1, 0.1);
            gPad->SetLogz();
            gStyle->SetStatX(0.5); // change 0.2 to whatever value you need
            gStyle->SetStatY(0.9); // change 0.9 to whatever value you need
            gStyle->SetStatW(0.12);  // set width
            gStyle->SetStatH(0.12);  // set height
            std::string histName = "ring: " + std::to_string(i) + " wedge: " + std::to_string(j);
            TH2D *h2 = new TH2D(histName.c_str(), histName.c_str(), nBins, 0, currentMaxRange, nBins, 0, currentMaxRange);
            for(int k=0; k<filteredEnergyData_[key].first.size(); k++){
                h2->Fill(filteredEnergyData_[key].first[k], filteredEnergyData_[key].second[k]);
            }
            h2->GetXaxis()->SetTitle("An (Ring energies) [a.u]");
            h2->GetYaxis()->SetTitle("Ap (Wedge energies) [a.u]");
            h2->Draw("COLZ");

            // create and draw line
            double slope = slopes.at(key);
            TLine *line = new TLine(0, 0, 0.95*currentMaxRange, 0.95*currentMaxRange*slope);
            line->SetLineColor(kGreen);
            line->SetLineWidth(1);
            line->SetLineStyle(2); // set line style to dashed
            line->Draw("same"); // "same" option means draw on top of the existing histogram

            // create line plot
            c1->cd(2); // switch to second pad
            gPad->SetMargin(0.15, 0.15, 0.1, 0.1);

            std::vector<double> SpnValues(fit_distribution.at(key).size());
            double dS = (maxSlope_ - minSlope_) / (fit_distribution.at(key).size() - 1);
            for (int i = 0; i < fit_distribution.at(key).size(); ++i) {
                SpnValues[i] = minSlope_ + i * dS;
            }
            TGraph *gr = new TGraph(fit_distribution.at(key).size());
            for (int i=0; i<fit_distribution.at(key).size(); i++){
                gr->SetPoint(i, SpnValues[i], fit_distribution.at(key)[i]);
            }
            double halfWindow = 0.5 * 0.05 * (maxSlope_ - minSlope_);
            double minX = slopes.at(key) - halfWindow;
            double maxX = slopes.at(key) + halfWindow;
            gr->GetXaxis()->SetRangeUser(minX, maxX);
            gr->GetXaxis()->SetTitle("Slope [a.u]");
            gr->GetYaxis()->SetTitle("Probability [a.u]");
            gr->SetTitle("Probability distribution of slope");
            gr->Draw("AL");

            double yMin = gr->GetYaxis()->GetXmin();
            double yMax = gr->GetYaxis()->GetXmax();

            double slopeValue = slopes.at(key);
            TLine *slopeLine = new TLine(slopeValue, yMin, slopeValue, yMax);
            slopeLine->SetLineColor(kGreen);
            slopeLine->SetLineStyle(2);
            slopeLine->Draw("same");

            double slopeLowerBound = slopeBounds.at(key).first;
            TLine *slopeLowerBoundLine = new TLine(slopeLowerBound, yMin, slopeLowerBound, yMax);
            slopeLowerBoundLine->SetLineColor(kBlue);
            slopeLowerBoundLine->SetLineStyle(2);
            slopeLowerBoundLine->Draw("same");

            double slopeUpperBound = slopeBounds.at(key).second;
            TLine *slopeUpperBoundLine = new TLine(slopeUpperBound, yMin, slopeUpperBound, yMax);
            slopeUpperBoundLine->SetLineColor(kBlue);
            slopeUpperBoundLine->SetLineStyle(2);
            slopeUpperBoundLine->Draw("same");

            gStyle->SetPalette(kInvertedDarkBodyRadiator);
            c1->Update();
            c1->Print(outputFileName.c_str()); // add canvas to PDF file
        }
    }

    c1->Print((outputFileName + "]").c_str()); // close PDF file
    delete c1;

    gErrorIgnoreLevel = temp_gErrorIgnoreLevel;
}

void ScatterPlotter::CreateDistributionPlots(const std::map<std::pair<int, int>, std::vector<double>>& slopeDistribution, 
                                             const std::map<std::pair<int, int>, double>& slopes,
                                             const std::map<std::pair<int, int>, std::pair<double, double>>& slopeBounds,
                                             const std::map<std::pair<int, int>, std::vector<double>>& offsetDistribution, 
                                             const std::map<std::pair<int, int>, double>& offsets,
                                             const std::map<std::pair<int, int>, std::pair<double, double>>& offsetBounds) 
{
    auto temp_gErrorIgnoreLevel = gErrorIgnoreLevel;
    gErrorIgnoreLevel = kFatal;

    TCanvas* c1 = new TCanvas("c1", "Plots", 2500, 800);

    std::string outputFileName = outputDirectory_ + "/distribution_plots.pdf";
    c1->Print((outputFileName + "[").c_str()); // start PDF file
    
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 8; j++){
            std::pair<int, int> key = std::make_pair(i, j);

            if(filteredEnergyData_.find(key) == filteredEnergyData_.end()){
                continue;
            }

            if(slopeDistribution.find(key) == slopeDistribution.end()){
                continue;
            }

            c1->Clear();
            c1->Divide(3, 1); // split canvas into two parts

            // create 2D histogram
            c1->cd(1); // switch to first pad
            gPad->SetLogz();
            gStyle->SetStatX(0.5); // change 0.2 to whatever value you need
            gStyle->SetStatY(0.9); // change 0.9 to whatever value you need
            std::string histName = "ring: " + std::to_string(i) + " wedge: " + std::to_string(j);
            TH2D *h2 = new TH2D(histName.c_str(), histName.c_str(), 500, 0, 5000, 500, 0, 5000);
            for(int k=0; k<filteredEnergyData_[key].first.size(); k++){
                h2->Fill(filteredEnergyData_[key].first[k], filteredEnergyData_[key].second[k]);
            }
            h2->GetXaxis()->SetTitle("An (Ring energies) [a.u]");
            h2->GetYaxis()->SetTitle("Ap (Wedge energies) [a.u]");
            h2->Draw("COLZ");

            // create and draw line
            double slope = slopes.at(key);
            double offset = offsets.at(key);
            TLine *line = new TLine(0, 0, 5000, 5000*slope + offset);
            line->SetLineColor(kGreen);
            line->SetLineWidth(1);
            line->SetLineStyle(2); // set line style to dashed
            line->Draw("same"); // "same" option means draw on top of the existing histogram

            // create slope plot
            c1->cd(2); // switch to second pad

            std::vector<double> SpnValues(slopeDistribution.at(key).size());
            double dS = (maxSlope_ - minSlope_) / (slopeDistribution.at(key).size() - 1);
            for (int i = 0; i < slopeDistribution.at(key).size(); ++i) {
                SpnValues[i] = minSlope_ + i * dS;
            }
            TGraph *gr = new TGraph(slopeDistribution.at(key).size());
            for (int i=0; i<slopeDistribution.at(key).size(); i++){
                gr->SetPoint(i, SpnValues[i], slopeDistribution.at(key)[i]);
            }
            double halfWindow = 0.5 * 0.05 * (maxSlope_ - minSlope_);
            double minX = slopes.at(key) - halfWindow;
            double maxX = slopes.at(key) + halfWindow;
            gr->GetXaxis()->SetRangeUser(minX, maxX);
            gr->Draw("AL");

            double slopeValue = slopes.at(key);
            TLine *slopeLine = new TLine(slopeValue, gPad->GetUymin(), slopeValue, gPad->GetUymax());
            slopeLine->SetLineColor(kGreen);
            slopeLine->SetLineStyle(2);
            slopeLine->Draw("same");
            double slopeLowerBound = slopeBounds.at(key).first;
            TLine *slopeLowerBoundLine = new TLine(slopeLowerBound, gPad->GetUymin(), slopeLowerBound, gPad->GetUymax());
            slopeLowerBoundLine->SetLineColor(kBlue);
            slopeLowerBoundLine->SetLineStyle(2);
            slopeLowerBoundLine->Draw("same");
            double slopeUpperBound = slopeBounds.at(key).second;
            TLine *slopeUpperBoundLine = new TLine(slopeUpperBound, gPad->GetUymin(), slopeUpperBound, gPad->GetUymax());
            slopeUpperBoundLine->SetLineColor(kBlue);
            slopeUpperBoundLine->SetLineStyle(2);
            slopeUpperBoundLine->Draw("same");

            // create offset plot
            c1->cd(3); // switch to second pad

            std::vector<double> OpnValues(offsetDistribution.at(key).size());
            double dO = (maxOffset_ - minOffset_) / (offsetDistribution.at(key).size() - 1);
            for (int i = 0; i < offsetDistribution.at(key).size(); ++i) {
                OpnValues[i] = minOffset_ + i * dO;
            }
            TGraph *gr2 = new TGraph(offsetDistribution.at(key).size());
            for (int i=0; i<offsetDistribution.at(key).size(); i++){
                gr2->SetPoint(i, OpnValues[i], offsetDistribution.at(key)[i]);
            }
            double halfWindow2 = 0.5 * 0.3 * (maxOffset_ - minOffset_);
            double minX2 = offsets.at(key) - halfWindow;
            double maxX2 = offsets.at(key) + halfWindow;
            // gr2->GetXaxis()->SetRangeUser(minX2, maxX2);
            gr2->GetXaxis()->SetRangeUser(offsets.at(key) - 10.0, offsets.at(key) + 10);
            gr2->Draw("AL");

            double offsetValue = offsets.at(key);
            TLine *offsetLine = new TLine(offsetValue, gPad->GetUymin(), offsetValue, gPad->GetUymax());
            offsetLine->SetLineColor(kGreen);
            offsetLine->SetLineStyle(2);
            offsetLine->Draw("same");
            double offsetLowerBound = offsetBounds.at(key).first;
            TLine *offsetLowerBoundLine = new TLine(offsetLowerBound, gPad->GetUymin(), offsetLowerBound, gPad->GetUymax());
            offsetLowerBoundLine->SetLineColor(kBlue);
            offsetLowerBoundLine->SetLineStyle(2);
            offsetLowerBoundLine->Draw("same");
            double offsetUpperBound = offsetBounds.at(key).second;
            TLine *offsetUpperBoundLine = new TLine(offsetUpperBound, gPad->GetUymin(), offsetUpperBound, gPad->GetUymax());
            offsetUpperBoundLine->SetLineColor(kBlue);
            offsetUpperBoundLine->SetLineStyle(2);
            offsetUpperBoundLine->Draw("same");

            gStyle->SetPalette(kInvertedDarkBodyRadiator);
            c1->Update();
            c1->Print(outputFileName.c_str()); // add canvas to PDF file
        }
    }

    c1->Print((outputFileName + "]").c_str()); // close PDF file
    delete c1;

    gErrorIgnoreLevel = temp_gErrorIgnoreLevel;
}

void ScatterPlotter::CreateOverlayPlot(const std::vector<double>& ringCoefficients, 
                                       const std::vector<double>& wedgeCoefficients) 
{
    auto temp_gErrorIgnoreLevel = gErrorIgnoreLevel;
    gErrorIgnoreLevel = kFatal;

    TCanvas* c1 = new TCanvas("c1", "Combined Plots", 800, 800);

    std::string outputFileName = outputDirectory_ + "/before_vs_after_histograms.pdf";
    c1->Print((outputFileName + "[").c_str()); // start PDF file
    
    c1->Divide(2,3); // Divide the canvas into six parts

    /** Define the histograms and fill them **/

    // Wedges vs Rings histograms
    int WvRBins = static_cast<int>(0.1 * maxRange_); // make the number of bins a tenth of the maximum range
    TH2D *WvRBefore = new TH2D("WvRBefore", "Wedges vs Rings Before Gain Match", WvRBins, 0, maxRange_, WvRBins, 0, maxRange_);
    TH2D *WvRAfter = new TH2D("WvRAfter", "Wedges vs Rings After Gain Match", WvRBins, 0, maxRange_, WvRBins, 0, maxRange_);
    // Ring Energies vs Channels histograms
    int EvCBins = std::min(static_cast<int>(0.04*maxRange_), 200); // make the number of bins at most 200
    TH2D *RvCBefore = new TH2D("RvCBefore", "Ring Energies Before Gain Match", 128, 0, 127, EvCBins, 0, maxRange_);
    TH2D *RvCAfter = new TH2D("RvCAfter", "Ring Energies After Gain Match", 128, 0, 127, EvCBins, 0, maxRange_);
    // Wedge Energies vs Channels histograms
    TH2D *WvCBefore = new TH2D("WvCBefore", "Wedge Energies Before Gain Match", 128, 0, 127, EvCBins, 0, maxRange_);
    TH2D *WvCAfter = new TH2D("WvCAfter", "Wedge Energies After Gain Match", 128, 0, 127, EvCBins, 0, maxRange_);

    // Fill them
    std::set<int> seenRings, seenWedges;
    for (const auto& chansVsEnergies : filteredEnergyData_)
    {
        auto currentRingChannel = chansVsEnergies.first.first;
        auto currentWedgeChannel = chansVsEnergies.first.second;
        auto currentRingEnergies = chansVsEnergies.second.first;
        auto currentWedgeEnergies = chansVsEnergies.second.second;

        bool isRingUnique = seenRings.insert(currentRingChannel).second;
        bool isWedgeUnique = seenWedges.insert(currentWedgeChannel).second;

        for(int k=0; k<currentRingEnergies.size(); k++)
        {
            WvRBefore->Fill(currentRingEnergies[k], currentWedgeEnergies[k]);
            WvRAfter->Fill(currentRingEnergies[k] * ringCoefficients[currentRingChannel], currentWedgeEnergies[k] * wedgeCoefficients[currentWedgeChannel]);
        }

        if (isRingUnique)
        {
            for (const auto& ringEnergy : currentRingEnergies)
            {
                RvCBefore->Fill(globalRingChanAt_[currentRingChannel], ringEnergy);
                RvCAfter->Fill(globalRingChanAt_[currentRingChannel], ringEnergy * ringCoefficients[currentRingChannel]);
            }
        }
        
        if (isWedgeUnique)
        {
            for (const auto& wedgeEnergy : currentWedgeEnergies)
            {
                WvCBefore->Fill(globalWedgeChanAt_[currentWedgeChannel], wedgeEnergy);
                WvCAfter->Fill(globalWedgeChanAt_[currentWedgeChannel], wedgeEnergy * wedgeCoefficients[currentWedgeChannel]);
            }
        }
    }

    /** Draw the histograms **/

    // Wedges vs Rings histograms
    c1->cd(1);
    gPad->SetMargin(0.1, 0.1, 0.1, 0.1);
    gPad->SetLogz();
    WvRBefore->GetXaxis()->SetTitle("An (Ring energies) [a.u]");
    WvRBefore->GetYaxis()->SetTitle("Ap (Wedge energies) [a.u]");
    WvRBefore->SetStats(0); // turn off the stat box
    WvRBefore->Draw("COLZ");

    c1->cd(2);
    gPad->SetMargin(0.15, 0.15, 0.1, 0.1);
    gPad->SetLogz();
    WvRAfter->GetXaxis()->SetTitle("An (Ring energies) [a.u]");
    WvRAfter->GetYaxis()->SetTitle("Ap (Wedge energies) [a.u]");
    WvRAfter->SetStats(0);
    WvRAfter->Draw("COLZ");

    // Ring Energies vs Channels histograms
    double halfWindow = 0.5 * 0.35 * maxRange_; // make zoomed in window half the size of 35 % of the full range
    double ringsMaxZ = TMath::Max(RvCBefore->GetMaximum(), RvCAfter->GetMaximum());
    double wedgesMaxZ = TMath::Max(WvCBefore->GetMaximum(), WvCAfter->GetMaximum());

    c1->cd(3);
    gPad->SetMargin(0.15, 0.15, 0.1, 0.1);
    RvCBefore->GetXaxis()->SetTitle("Channel Index [a.u]");
    RvCBefore->GetYaxis()->SetTitle("Energy [a.u]");
    RvCBefore->GetXaxis()->SetRangeUser(globalRingChanAt_.begin()->second - 1, globalRingChanAt_.rbegin()->second + 1);
    int ringsMaxBeforeBin = RvCBefore->GetMaximumBin(), ringsMaxBeforeBinx, ringsMaxBeforeBiny, ringsMaxBeforeBinz;
    RvCBefore->GetBinXYZ(ringsMaxBeforeBin, ringsMaxBeforeBinx, ringsMaxBeforeBiny, ringsMaxBeforeBinz);
    double ringsBeforeYMin = RvCBefore->GetYaxis()->GetBinCenter(ringsMaxBeforeBiny) - halfWindow;
    double ringsBeforeYMax = RvCBefore->GetYaxis()->GetBinCenter(ringsMaxBeforeBiny) + halfWindow;
    RvCBefore->GetYaxis()->SetRangeUser(std::max(0.0, ringsBeforeYMin), ringsBeforeYMax);
    RvCBefore->GetZaxis()->SetRangeUser(0, ringsMaxZ);
    RvCBefore->SetStats(0);
    RvCBefore->Draw("COLZ");

    c1->cd(4);
    gPad->SetMargin(0.15, 0.15, 0.1, 0.1);
    RvCAfter->GetXaxis()->SetTitle("Channel Index [a.u]");
    RvCAfter->GetYaxis()->SetTitle("Energy [a.u]");
    RvCAfter->GetXaxis()->SetRangeUser(globalRingChanAt_.begin()->second - 1, globalRingChanAt_.rbegin()->second + 1);
    int ringsMaxAfterBin = RvCAfter->GetMaximumBin(), ringsMaxAfterBinx, ringsMaxAfterBiny, ringsMaxAfterBinz;
    RvCAfter->GetBinXYZ(ringsMaxAfterBin, ringsMaxAfterBinx, ringsMaxAfterBiny, ringsMaxAfterBinz);
    double ringsAfterYMin = RvCAfter->GetYaxis()->GetBinCenter(ringsMaxAfterBiny) - halfWindow;
    double ringsAfterYMax = RvCAfter->GetYaxis()->GetBinCenter(ringsMaxAfterBiny) + halfWindow;
    RvCAfter->GetYaxis()->SetRangeUser(std::max(0.0, ringsAfterYMin), ringsAfterYMax);
    RvCAfter->GetZaxis()->SetRangeUser(0, ringsMaxZ);
    RvCAfter->SetStats(0);
    RvCAfter->Draw("COLZ");

    // Wedge Energies vs Channels histograms
    c1->cd(5);
    gPad->SetMargin(0.15, 0.15, 0.1, 0.1);
    WvCBefore->GetXaxis()->SetTitle("Channel Index [a.u]");
    WvCBefore->GetYaxis()->SetTitle("Energy [a.u]");
    WvCBefore->GetXaxis()->SetRangeUser(globalWedgeChanAt_.begin()->second - 1, globalWedgeChanAt_.rbegin()->second + 1);
    int wedgesMaxBeforeBin = WvCBefore->GetMaximumBin(), wedgesMaxBeforeBinx, wedgesMaxBeforeBiny, wedgesMaxBeforeBinz;
    WvCBefore->GetBinXYZ(wedgesMaxBeforeBin, wedgesMaxBeforeBinx, wedgesMaxBeforeBiny, wedgesMaxBeforeBinz);
    double wedgesBeforeYMin = WvCBefore->GetYaxis()->GetBinCenter(wedgesMaxBeforeBiny) - halfWindow;
    double wedgesBeforeYMax = WvCBefore->GetYaxis()->GetBinCenter(wedgesMaxBeforeBiny) + halfWindow;
    WvCBefore->GetYaxis()->SetRangeUser(std::max(0.0, wedgesBeforeYMin), wedgesBeforeYMax);
    WvCBefore->GetZaxis()->SetRangeUser(0, wedgesMaxZ);
    WvCBefore->SetStats(0);
    WvCBefore->Draw("COLZ");

    c1->cd(6);
    gPad->SetMargin(0.15, 0.15, 0.1, 0.1);
    WvCAfter->GetXaxis()->SetTitle("Channel Index [a.u]");
    WvCAfter->GetYaxis()->SetTitle("Energy [a.u]");
    WvCAfter->GetXaxis()->SetRangeUser(globalWedgeChanAt_.begin()->second - 1, globalWedgeChanAt_.rbegin()->second + 1);
    int wedgesMaxAfterBin = WvCAfter->GetMaximumBin(), wedgesMaxAfterBinx, wedgesMaxAfterBiny, wedgesMaxAfterBinz;
    WvCAfter->GetBinXYZ(wedgesMaxAfterBin, wedgesMaxAfterBinx, wedgesMaxAfterBiny, wedgesMaxAfterBinz);
    double wedgesAfterYMin = WvCAfter->GetYaxis()->GetBinCenter(wedgesMaxAfterBiny) - halfWindow;
    double wedgesAfterYMax = WvCAfter->GetYaxis()->GetBinCenter(wedgesMaxAfterBiny) + halfWindow;
    WvCAfter->GetYaxis()->SetRangeUser(std::max(0.0, wedgesAfterYMin), wedgesAfterYMax);
    WvCAfter->GetZaxis()->SetRangeUser(0, wedgesMaxZ);
    WvCAfter->SetStats(0);
    WvCAfter->Draw("COLZ");

    gStyle->SetPalette(kInvertedDarkBodyRadiator);
    c1->Update();
    c1->Print(outputFileName.c_str()); // add canvas to PDF file

    c1->Print((outputFileName + "]").c_str()); // close PDF file
    delete c1;

    gErrorIgnoreLevel = temp_gErrorIgnoreLevel;
}

void ScatterPlotter::CreateOverlayPlot(const std::vector<double>& ringCoefficients, 
                                       const std::vector<double>& wedgeCoefficients,
                                       const std::vector<double>& ringOffsets,
                                       const std::vector<double>& wedgeOffsets) 
{
    auto temp_gErrorIgnoreLevel = gErrorIgnoreLevel;
    gErrorIgnoreLevel = kFatal;

    TCanvas* c1 = new TCanvas("c1", "Combined Plots", 2300, 800);

    std::string outputFileName = outputDirectory_ + "/before_vs_after_histograms.pdf";
    c1->Print((outputFileName + "[").c_str()); // start PDF file
    
    c1->Divide(2,1); // Divide the canvas into two parts
    
    // Create 2D histogram before gain match
    c1->cd(1); // Switch to first pad
    gPad->SetLogz();
    gStyle->SetStatX(0.5); // change 0.2 to whatever value you need
    gStyle->SetStatY(0.9); // change 0.9 to whatever value you need
    TH2D *h2_before = new TH2D("h2_before", "Before gain match", 500, 0, 5000, 500, 0, 5000);
    
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 8; j++){
            std::pair<int, int> key = std::make_pair(i, j);

            if(filteredEnergyData_.find(key) == filteredEnergyData_.end()){
                continue;
            }

            for(int k=0; k<filteredEnergyData_[key].first.size(); k++){
                h2_before->Fill(filteredEnergyData_[key].first[k], filteredEnergyData_[key].second[k]);
            }
        }
    }
    
    h2_before->Draw("COLZ");

    // Create 2D histogram after gain match
    c1->cd(2); // Switch to second pad
    gPad->SetLogz();
    gStyle->SetStatX(0.5); // change 0.2 to whatever value you need
    gStyle->SetStatY(0.9); // change 0.9 to whatever value you need
    TH2D *h2_after = new TH2D("h2_after", "After gain match", 500, 0, 5000, 500, 0, 5000);

    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 8; j++){
            std::pair<int, int> key = std::make_pair(i, j);

            if(filteredEnergyData_.find(key) == filteredEnergyData_.end()){
                continue;
            }

            for(int k=0; k<filteredEnergyData_[key].first.size(); k++){
                h2_after->Fill(filteredEnergyData_[key].first[k] * ringCoefficients[i] + ringOffsets[i], filteredEnergyData_[key].second[k] * wedgeCoefficients[j] + wedgeOffsets[j]);
            }
        }
    }

    h2_after->Draw("COLZ");

    gStyle->SetPalette(kInvertedDarkBodyRadiator);
    c1->Update();
    c1->Print(outputFileName.c_str()); // add canvas to PDF file

    c1->Print((outputFileName + "]").c_str()); // close PDF file
    delete c1;

    gErrorIgnoreLevel = temp_gErrorIgnoreLevel;
}
