#!/bin/bash

# Check if the argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <run-number>"
    exit 1
fi

# Get the argument
arg=$1

# Run all instances concurrently in separate terminals
# gnome-terminal -- bash -c "./bin/gainMatch $arg det0_config.txt; echo 'Press Enter to close...'; read"
# gnome-terminal -- bash -c "./bin/gainMatch $arg det1_config.txt; echo 'Press Enter to close...'; read"
# gnome-terminal -- bash -c "./bin/gainMatch $arg det2_config.txt; echo 'Press Enter to close...'; read"
# gnome-terminal -- bash -c "./bin/gainMatch $arg det3_config.txt; echo 'Press Enter to close...'; read"
# gnome-terminal -- bash -c "./bin/gainMatch $arg det4_config.txt; echo 'Press Enter to close...'; read"

# Replace the above commands with these if you want the terminals to close once the executions are done
gnome-terminal -- bash -c "./bin/gainMatch $arg det0_config.txt;"
gnome-terminal -- bash -c "./bin/gainMatch $arg det1_config.txt;"
gnome-terminal -- bash -c "./bin/gainMatch $arg det2_config.txt;"
gnome-terminal -- bash -c "./bin/gainMatch $arg det3_config.txt;"
gnome-terminal -- bash -c "./bin/gainMatch $arg det4_config.txt;"

echo "All instances launched."
