#include "colors.inc"
#include "shapes.inc"

/* Wheel */
#declare ROBOT_WHEEL_RADIUS = 0.05;
#declare ROBOT_WHEEL_HEIGHT = 0.03;
#declare ROBOT_WHEEL_OBJ = cylinder {
  <0,0,-ROBOT_WHEEL_HEIGHT/2>, <0,0,ROBOT_WHEEL_HEIGHT/2>, ROBOT_WHEEL_RADIUS
  pigment { rgb <0.3, 0.3, 0.3> }
  rotate <90,0,0>
  translate <0,0,-ROBOT_WHEEL_RADIUS>
};

/* Body */
#declare ROBOT_BODY_RADIUS = 0.085036758;
#declare ROBOT_BODY_HEIGHT = 0.146899733;
#declare ROBOT_BODY_OBJ = cylinder {
  <0,0,0>, <0,0,-ROBOT_BODY_HEIGHT>, ROBOT_BODY_RADIUS
  translate -0.02 * z
};

/* Sensor */
#declare ROBOT_SENSOR_RADIUS = 0.005;

/* Robot */
#macro Make_Robot(pos, rot, col, srange)
  union {
    // left wheel
    object {
      ROBOT_WHEEL_OBJ
      translate (ROBOT_BODY_RADIUS+ROBOT_WHEEL_HEIGHT/2) * y
    }
    // right wheel
    object {
      ROBOT_WHEEL_OBJ
      translate -(ROBOT_BODY_RADIUS+ROBOT_WHEEL_HEIGHT/2) * y
    }
    // body
    object {
      ROBOT_BODY_OBJ
      pigment { rgb col }
    }
    // sensor
    #if (srange > 0)
      cylinder {
	<ROBOT_BODY_RADIUS, 0, -ROBOT_BODY_HEIGHT/2>,
	<ROBOT_BODY_RADIUS+srange, 0, -ROBOT_BODY_HEIGHT/2>,
	ROBOT_SENSOR_RADIUS
	no_shadow
	pigment { color Black }
      }
    #end
    // pose
    rotate rot
    translate pos
  }
#end
