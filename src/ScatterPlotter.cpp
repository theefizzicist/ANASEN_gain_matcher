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

ScatterPlotter::ScatterPlotter(const std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<double>>>& filteredEnergyData,
                               const std::string& output_directory,
                               const double& maxSlope,
                               const double& minSlope,
                               const double& maxOffset,
                               const double& minOffset)
    : filteredEnergyData_(filteredEnergyData), outputDirectory_(output_directory), maxSlope_(maxSlope), minSlope_(minSlope), maxOffset_(maxOffset), minOffset_(minOffset) 
{}

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
            h2->GetXaxis()->SetTitle("Ap (Ring energies) [a.u]");
            h2->GetYaxis()->SetTitle("An (Wedge energies) [a.u]");
            h2->Draw("COLZ");

            // create and draw line
            double slope = slopes.at(key);
            TLine *line = new TLine(0, 0, 5000, 5000*slope);
            line->SetLineColor(kGreen);
            line->SetLineWidth(1);
            line->SetLineStyle(2); // set line style to dashed
            line->Draw("same"); // "same" option means draw on top of the existing histogram

            // create line plot
            c1->cd(2); // switch to second pad

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

    TCanvas* c1 = new TCanvas("c1", "Plots", 2300, 800);

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

    TCanvas* c1 = new TCanvas("c1", "Combined Plots", 1800, 800);

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
                h2_after->Fill(filteredEnergyData_[key].first[k] * ringCoefficients[i], filteredEnergyData_[key].second[k] * wedgeCoefficients[j]);
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

void ScatterPlotter::CreateOverlayPlot(const std::vector<double>& ringCoefficients, 
                                       const std::vector<double>& wedgeCoefficients,
                                       const std::vector<double>& ringOffsets,
                                       const std::vector<double>& wedgeOffsets) 
{
    auto temp_gErrorIgnoreLevel = gErrorIgnoreLevel;
    gErrorIgnoreLevel = kFatal;

    TCanvas* c1 = new TCanvas("c1", "Combined Plots", 1800, 800);

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
