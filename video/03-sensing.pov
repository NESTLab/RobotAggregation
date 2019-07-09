#include "colors.inc"
#include "shapes.inc"
#include "robot.inc"

/*
 * Background color
 */
background { color Cyan }

/*
 * Floor
 */
plane { z, 0 pigment { color White } }

/*
 * Camera location
 */
#declare CAM_X_POSITION = 0.125;
#declare CAM_Z_POSITION = 1.2;
camera {
  location <CAM_X_POSITION, 0, -CAM_Z_POSITION>
  look_at  <CAM_X_POSITION, 0, 0>
}

/*
 * Light sources
 */
light_source { <5, 5, -10> color White }

/*
 * Robot
 */
#declare ROBOT_DISTANCE = 0.4;
#declare SENSOR_RANGE = 0.25;
Make_Robot(<-ROBOT_DISTANCE,0,0>, <0,0,0>, <1,0,0>, SENSOR_RANGE)
Make_Robot(<              0,0,0>, <0,0,0>, <1,0,0>, SENSOR_RANGE)
Make_Robot(< ROBOT_DISTANCE,0,0>, <0,0,0>, <0,1,0>, SENSOR_RANGE)
