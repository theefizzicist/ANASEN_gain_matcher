#!/bin/bash

# Check if the argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <run-number>"
    exit 1
fi

# Get the argument
runNumber=$1

########################################################
# Run all instances concurrently in separate terminals #
########################################################
touch /tmp/det0_done
gnome-terminal -- bash -c "./bin/gainMatch $runNumber det0_config.txt; rm /tmp/det0_done; echo 'Press Enter to close...'; read"

touch /tmp/det1_done
gnome-terminal -- bash -c "./bin/gainMatch $runNumber det1_config.txt; rm /tmp/det1_done; echo 'Press Enter to close...'; read"

touch /tmp/det2_done
gnome-terminal -- bash -c "./bin/gainMatch $runNumber det2_config.txt; rm /tmp/det2_done; echo 'Press Enter to close...'; read"

touch /tmp/det3_done
gnome-terminal -- bash -c "./bin/gainMatch $runNumber det3_config.txt; rm /tmp/det3_done; echo 'Press Enter to close...'; read"

#########################################################################################################
# Replace the above commands with these if you want the terminals to close once the executions are done #
#########################################################################################################
# touch /tmp/det0_done
# gnome-terminal -- bash -c "./bin/gainMatch $runNumber det0_config.txt; rm /tmp/det0_done;"

# touch /tmp/det1_done
# gnome-terminal -- bash -c "./bin/gainMatch $runNumber det1_config.txt; rm /tmp/det1_done;"

# touch /tmp/det2_done
# gnome-terminal -- bash -c "./bin/gainMatch $runNumber det2_config.txt; rm /tmp/det2_done;"

# touch /tmp/det3_done
# gnome-terminal -- bash -c "./bin/gainMatch $runNumber det3_config.txt; rm /tmp/det3_done;"

##################################################################################################
# Check to see that all detectors are gain-matched, then run the macro to combine the gain files #
##################################################################################################
while [[ -f /tmp/det0_done || -f /tmp/det1_done || -f /tmp/det2_done || -f /tmp/det3_done ]]; do
    sleep 1
done

echo "All detectors are done."

root -l -q "combineGainMaps.C($runNumber)"
