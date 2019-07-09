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
#declare START_ANGLE = 195;
#declare END_ANGLE = 270;
#declare CUR_ANGLE = START_ANGLE + (END_ANGLE - START_ANGLE) * clock;
#declare DISTANCE = 0.5;
camera {
  location <0, DISTANCE * cos(radians(CUR_ANGLE)), DISTANCE * sin(radians(CUR_ANGLE))>
  look_at  <0, 0,  0>
}

/*
 * Light sources
 */
light_source { <5, 5, -10> color White }

/*
 * Robot
 */
Make_Robot(<0,0,0>, <0,0,0>, <1,0,0>, 0)
