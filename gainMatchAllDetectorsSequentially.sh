#!/bin/bash

# Check if the argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <run-number>"
    exit 1
fi

# Get the argument
runNumber=$1

##########################################################
# Run all instances sequentially in the current terminal #
##########################################################

for i in {0..3}; do
    # Print the start of calibration process for each detector
    echo -e "**************************************************************"
    echo -e "Gain matching detector ${i} ...\n"
    
    # Run the gain matching for each detector
    ./bin/gainMatch $runNumber "det${i}_config.txt"
    
    # Print the end of calibration process for each detector
    echo -e "\nFinished detector ${i}."
    echo -e "**************************************************************\n"
done

# Print when all detectors have been calibrated
echo "All detectors are done."

###############################################
# Combine the gain files using the ROOT macro #
###############################################
root -l -q "combineGainMaps.C($runNumber)"
