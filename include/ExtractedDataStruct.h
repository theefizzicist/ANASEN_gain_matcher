#ifndef EXTRACTED_DATA_H
#define EXTRACTED_DATA_H

#include <vector>
#include <cstddef>

struct ExtractedData {
    std::vector<double> qqqRingChannel;
    std::vector<double> qqqWedgeChannel;
    std::vector<double> qqqRingE;
    std::vector<double> qqqWedgeE;
    std::vector<size_t> ringMultiplicity;
    std::vector<size_t> wedgeMultiplicity;
};

#endif // EXTRACTED_DATA_H