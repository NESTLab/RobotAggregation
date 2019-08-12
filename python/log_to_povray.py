import argparse

import numpy as np

prefix = """
#include "colors.inc"
#include "shapes.inc"
#include "robot.inc"
background { color Cyan }
plane { z, 0 pigment { color White } }
camera { location <0.5,0.2,-1.5> look_at <0.5,0.2,0> }
light_source { <1.50865,1.5,-2.2666750705102414> color White }
"""
template = """
Make_Robot(<{},{},0>, <0,0,{}>, <{},{},{}>, {}, <{},{},{}>)
Make_Robot(<{},{},0>, <0,0,{}>, <{},{},{}>, {}, <{},{},{}>)
Make_Robot(<{},{},0>, <0,0,{}>, <{},{},{}>, {}, <{},{},{}>)
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('log')

    args = parser.parse_args()

    data = np.genfromtxt(args.log, delimiter=',')
    data = data[:, :-1]
    data = data.reshape(data.shape[0], -1, 6)

    for t, data_t in enumerate(data):
        args = []
        for j, robot_data in enumerate(data_t):
            class_id, sensor_state, x, y, yaw_rad, sensor_range = robot_data
            yaw_deg = np.rad2deg(yaw_rad)
            if sensor_state == 0:
                sr = 0.2
                sg = 0.2
                sb = 0.2
            elif sensor_state == 1:
                sr = 0.5
                sg = 0.2
                sb = 0.1
            else:
                sr = 0.2
                sg = 0.5
                sb = 0.1
            if class_id == 0:
                r = 1
                g = 0
                b = 0
            elif class_id == 1:
                r = 0
                g = 1
                b = 0
            else:
                raise NotImplementedError()
            if sensor_range < 0.01:
                sensor_range = 5
            if j != 0:
                sensor_range = 0
            args.extend([x, y, yaw_deg, r, g, b, sensor_range, sr, sg, sb])
        pov_str = prefix + template.format(*args)
        with open("out{:04d}.pov".format(t), 'w') as outfile:
            outfile.writelines(pov_str)


if __name__ == '__main__':
    main()
