#ifndef EXTRACTED_DATA_H
#define EXTRACTED_DATA_H

#include <vector>
#include <cstddef>

struct ExtractedData {
    std::vector<double> sabreRingChannel;
    std::vector<double> sabreWedgeChannel;
    std::vector<double> sabreRingE;
    std::vector<double> sabreWedgeE;
    std::vector<size_t> ringMultiplicity;
    std::vector<size_t> wedgeMultiplicity;
};

#endif // EXTRACTED_DATA_H