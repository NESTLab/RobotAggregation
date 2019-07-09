#!/usr/bin/env python3

import csv
import math
import shutil
import sys

# Take file as input
if len(sys.argv) != 5:
    print("Usage: ", sys.argv[0], "<data.csv> <preamble.pov> <normal_up_to> <delta>")
    sys.exit(1)
FDATA = sys.argv[1]
FPREAMBLE = sys.argv[2]
NORMAL_UP_TO = int(sys.argv[3])
DELTA = int(sys.argv[4])
FOUTPUT = "".join(FDATA.split("/")[-1].split(".")[:-1])

# Parse CSV file
#   Each row is a step of the simulation
#   Each row is a record of 3 tuples
#   Each tuple has the elements [class, sensor reading, x, y, theta]
FIELDS_PER_RECORD = 5

# Parse one first time to find the max x and y across the simulation
# Open data file
xrnge = [None, None]
yrnge = [None, None]
with open(FDATA, 'r', newline='') as csvfile:
    # Create a CSV reader
    data = csv.reader(csvfile)
    # Go through each row
    for row in data:
        # Go through number of robots
        for i in range(0, int(len(row) / FIELDS_PER_RECORD)):
            # Get x and y
            x = float(row[2 + FIELDS_PER_RECORD * i])
            y = float(row[3 + FIELDS_PER_RECORD * i])
            # Update the range information
            xrnge[0] = min(xrnge[0], x) if xrnge[0] != None else x
            xrnge[1] = max(xrnge[1], x) if xrnge[1] != None else x
            yrnge[0] = min(yrnge[0], y) if yrnge[0] != None else y
            yrnge[1] = max(yrnge[1], y) if yrnge[1] != None else y
# Find the centroid of the distribution in x,y
centroid = [ (xrnge[0] + xrnge[1]) / 2, (yrnge[0] + yrnge[1]) / 2 ]
# Distance of camera from centroid (angle taken from POVRay reference)
CANGLE = (67.380 / 180 * math.pi) / 2
FACTOR=1.5
cdistance = -max(xrnge[1]*FACTOR-centroid[0], yrnge[1]*FACTOR-centroid[1]) / math.tan(CANGLE)
# Camera position and look_at
camerapos = ",".join([str(e) for e in [centroid[0], centroid[1], cdistance]])
cameralook = ",".join([str(e) for e in [centroid[0], centroid[1], 0]])
cameraconf = "camera { location <" + camerapos + "> look_at <" + cameralook + "> }"
# Light position
lightpos = ",".join([str(e) for e in [xrnge[1], yrnge[1], cdistance]])
lightconf = "light_source { <" + lightpos + "> color White }"

# Utility dict to transform CSV class data into POVRay color
CLASS_TO_COLOR = {
    "0" : "1.0, 0.0, 0.0",
    "1" : "0.0, 1.0, 0.0",
    "2" : "0.0, 0.0, 1.0",
    "3" : "0.0, 0.0, 0.0",
    "4" : "0.0, 1.0, 1.0",
    "5" : "1.0, 0.0, 1.0",
    "6" : "1.0, 1.0, 0.0",
    "7" : "1.0, 1.0, 1.0"
}

# Open data file
with open(FDATA, 'r', newline='') as csvfile:
    # Create a CSV reader
    data = csv.reader(csvfile)
    # Row counter (same as time step)
    t = 0
    # Frame counter
    frame = 0
    # Go through each row
    for row in data:
        # Skip ARGoS log
        if row[0][0] != '[':
            # Make sure we want this output
            if (t < NORMAL_UP_TO) or (t % DELTA == 0):
                print(t, row)
                # Output file name
                fcurout = FOUTPUT + str(int(frame)).rjust(5,'0') + ".pov"
                # Copy preamble file into output file
                shutil.copyfile(FPREAMBLE, fcurout)
                # Write robots
                with open(fcurout, 'a') as f:
                    # Write camera
                    print(cameraconf, file=f)
                    # Write light
                    print(lightconf, file=f)
                    # Count the number of robots
                    for i in range(0, int(len(row) / 5)):
                        pos = row[2 + FIELDS_PER_RECORD * i] + "," + row[3 + FIELDS_PER_RECORD * i]
                        rot = "0,0," + str(float(row[4 + FIELDS_PER_RECORD * i]) / math.pi * 180)
                        cl = CLASS_TO_COLOR[row[0 + FIELDS_PER_RECORD * i]]
                        print("Make_Robot(<" + pos + ",0>, <" + rot + ">, <" + cl + ">, 2)", file=f)
                # Increase frame counter
                frame = frame + 1
            else:
                print("Skipped")
            # Increase timestep
            t = t + 1
