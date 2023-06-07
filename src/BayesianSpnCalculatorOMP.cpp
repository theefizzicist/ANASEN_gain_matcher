#include "BayesianSpnCalculatorOMP.h"
#include <numeric>
#include <omp.h>
#include <cmath>
#include "ProgressBars.h"
#include <iostream>
#include <algorithm>
#include "TCanvas.h"
#include "TH2.h"

BayesianSpnCalculatorOMP::BayesianSpnCalculatorOMP(int Ks, int Ko, double SlopeMin, double SlopeMax,double OffsetMin, double OffsetMax, double Width) : ks_(Ks), ko_(Ko), slopeMin_(SlopeMin), slopeMax_(SlopeMax), offsetMin_(OffsetMin), offsetMax_(OffsetMax), width_(Width) {}

std::tuple<double, double, double, std::vector<double>> BayesianSpnCalculatorOMP::CalculateSlope(const std::vector<double>& Ap, const std::vector<double>& An) {
    size_t M = Ap.size(); // number of amplitude points

    // Initialize prior distribution
    std::vector<double> prior(ks_, 1.0 / ks_);
    std::vector<double> SpnValues(ks_);
    double dS = (slopeMax_ - slopeMin_) / (ks_ - 1);
    for (int i = 0; i < ks_; ++i) {
        SpnValues[i] = slopeMin_ + i * dS;
    }

    std::vector<double> posterior(ks_);

    // Bayesian updating
    for (size_t i = 0; i < M; ++i) {
        double normalizationFactor = 0.0;

        #pragma omp parallel for reduction(+:normalizationFactor)
        for (int j = 0; j < ks_; ++j) {
            double Spn = SpnValues[j];
            posterior[j] = prior[j] * LikelihoodWithoutOffset(Ap[i], An[i], Spn, width_);
            normalizationFactor += posterior[j];
        }

        // Normalize the posterior distribution
        #pragma omp parallel for
        for (int j = 0; j < ks_; ++j) {
            posterior[j] /= normalizationFactor;
        }

        // Update the prior distribution for the next iteration
        prior = posterior;
    }

    // Compute the median and the 68% credible interval in a single loop
    double median = 0.0;
    double lowerBound = 0.0;
    double upperBound = 0.0;
    double cumulativeProb = 0.0;
    double targetProbMedian = 0.5;
    double targetProbCI = 0.68; // for credible interval
    double alpha = (1.0 - targetProbCI) / 2.0;
    bool medianFound = false;

    for (int i = 0; i < ks_; ++i) {
        cumulativeProb += prior[i];

        // Compute the median
        if (!medianFound && cumulativeProb >= targetProbMedian) {
            median = SpnValues[i];
            medianFound = true;
        }

        // Compute the lower bound of the 68% credible interval
        if (cumulativeProb <= alpha && (cumulativeProb + prior[i + 1]) > alpha) {
            lowerBound = SpnValues[i];
        }

        // Compute the upper bound of the 68% credible interval
        if (cumulativeProb >= (1.0 - alpha)) {
            upperBound = SpnValues[i];
            break;
        }
    }

    return {median, lowerBound, upperBound, prior};
}

std::tuple<double, double, double, std::vector<double>, double, double, double, std::vector<double>> BayesianSpnCalculatorOMP::CalculateSlopeAndOffset(const std::vector<double>& Ap, const std::vector<double>& An) {
    size_t M = Ap.size(); // number of amplitude points

    // Initialize SpnValues and OpnValues
    std::vector<double> SpnValues(ks_);
    std::vector<double> OpnValues(ko_);
    double dS = (slopeMax_ - slopeMin_) / (ks_ - 1);
    double dO = (offsetMax_ - offsetMin_) / (ko_ - 1);
    for (int i = 0; i < ks_; ++i) {
        SpnValues[i] = slopeMin_ + i * dS;
    }
    for (int i = 0; i < ko_; ++i) {
        OpnValues[i] = offsetMin_ + i * dO;
    }

    // Initialize prior distribution
    std::vector<std::vector<double>> posterior(ks_, std::vector<double>(ko_, 1.0 / (ks_ * ko_)));

    // Bayesian updating
    for (size_t i = 0; i < M; ++i) {
        double normalizationFactor = 0.0;

        #pragma omp parallel for collapse(2) reduction(+:normalizationFactor)
        for (int j = 0; j < ks_; ++j) {
            for (int m = 0; m < ko_; ++m) {
                double Spn = SpnValues[j];
                double Opn = OpnValues[m];
                posterior[j][m] *= LikelihoodWithOffset(Ap[i], An[i], Spn, Opn, width_);
                normalizationFactor += posterior[j][m];
            }
        }

        // Normalize the posterior distribution
        #pragma omp parallel for collapse(2)
        for (int j = 0; j < ks_; ++j) {
            for (int m = 0; m < ko_; ++m) {
                posterior[j][m] /= normalizationFactor;
            }
        }
    }

    // Compute the marginal distributions for Spn and Opn
    std::vector<double> marginalSpn(ks_, 0.0);
    std::vector<double> marginalOpn(ko_, 0.0);

    for (int i = 0; i < ks_; ++i) {
        for (int j = 0; j < ko_; ++j) {
            marginalSpn[i] += posterior[i][j];
            marginalOpn[j] += posterior[i][j];
        }
    }

    // Compute the median and the 68% credible interval for both Spn and Opn
    double medianSpn, lowerBoundSpn, upperBoundSpn;
    double medianOpn, lowerBoundOpn, upperBoundOpn;
    double cumulativeProbSpn = 0.0;
    double cumulativeProbOpn = 0.0;
    double targetProbMedian = 0.5;
    double targetProbCI = 0.68;
    double alpha = (1.0 - targetProbCI) / 2.0;
    bool medianSpnFound = false;
    bool medianOpnFound = false;

    for (int i = 0; i < ks_; ++i) {
        cumulativeProbSpn += marginalSpn[i];
        
        if (!medianSpnFound && cumulativeProbSpn >= targetProbMedian) {
            medianSpn = SpnValues[i];
            medianSpnFound = true;
        }

        if (cumulativeProbSpn <= alpha && (cumulativeProbSpn + marginalSpn[i + 1]) > alpha) {
            lowerBoundSpn = SpnValues[i];
        }

        if (cumulativeProbSpn >= (1.0 - alpha)) {
            upperBoundSpn = SpnValues[i];
            break;
        }
    }

    for (int j = 0; j < ko_; ++j) {
        cumulativeProbOpn += marginalOpn[j];

        if (!medianOpnFound && cumulativeProbOpn >= targetProbMedian) {
            medianOpn = OpnValues[j];
            medianOpnFound = true;
        }

        if (cumulativeProbOpn <= alpha && (cumulativeProbOpn + marginalOpn[j + 1]) > alpha) {
            lowerBoundOpn = OpnValues[j];
        }

        if (cumulativeProbOpn >= (1.0 - alpha)) {
            upperBoundOpn = OpnValues[j];
            break;
        }
    }

    return {medianSpn, lowerBoundSpn, upperBoundSpn, marginalSpn, medianOpn, lowerBoundOpn, upperBoundOpn, marginalOpn};
}

double BayesianSpnCalculatorOMP::LikelihoodWithoutOffset(double Ap, double An, double Spn, double W) const {
    double log_ratio = std::log(Ap / An) - std::log(Spn);
    return 1.0 / (W * W + log_ratio * log_ratio);
}

double BayesianSpnCalculatorOMP::LikelihoodWithOffset(double Ap, double An, double Spn, double Opn, double W) const {
    if (Ap <= Opn) {
        return 1e-10; // Very small likelihood value
    }
    
    double log_ratio = std::log((Ap - Opn) / An) - std::log(Spn);
    return 1.0 / (W * W + log_ratio * log_ratio);
}
