#ifndef BAYESIAN_SPN_CALCULATOR_OMP_H
#define BAYESIAN_SPN_CALCULATOR_OMP_H

#include <vector>
#include <tuple>

class BayesianSpnCalculatorOMP {
public:
    BayesianSpnCalculatorOMP(int Ks, int Ko, double SlopeMin, double SlopeMax, double OffsetMin, double OffsetMax, double Width);
    std::tuple<double, double, double, std::vector<double>> CalculateSlope(const std::vector<double>& Ap, const std::vector<double>& An);
    std::tuple<double, double, double, std::vector<double>, double, double, double, std::vector<double>> CalculateSlopeAndOffset(const std::vector<double>& Ap, const std::vector<double>& An);

private:
    double LikelihoodWithoutOffset(double Ap, double An, double Spn, double W) const;
    double LikelihoodWithOffset(double Ap, double An, double Spn, double Opn, double W) const;

    int ks_; // number of grid points for slope
    int ko_; // number of grid points for offset
    double slopeMin_, slopeMax_;
    double offsetMin_, offsetMax_;
    double  width_;
};

#endif // BAYESIAN_SPN_CALCULATOR_OMP_H
