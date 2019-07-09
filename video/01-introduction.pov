#include "colors.inc"
#include "shapes.inc"

/*
 * Random number generator
 */
#declare RNG = seed(123);

/*
 * Object constants
 */
#declare CYL_RADIUS = 0.2;
#declare CYL_HEIGHT = 0.2;
#declare OBJ_DEF = cylinder {
  <0,0,-CYL_HEIGHT/2>, <0,0,CYL_HEIGHT>, CYL_RADIUS
};
#declare OBJ_DIST = CYL_RADIUS * 3;

/*
 * Background color
 */
background { color White }

/*
 * Camera location
 */
camera {
  location <0, 0, -6>
  look_at  <0, 0,  0>
}

/*
 * Light sources
 */
light_source { <5, 5, -10> color White }
light_source { <-5, -5, -10> color White }

/*
 * Object definition
 */
#macro Make_Object(i, col, center)
  #local startpos = <2*rand(RNG)-1, 2*rand(RNG)-1, center.z>;
  #local endx     = OBJ_DIST * (floor(mod(i, 3)) - 1);
  #local endy     = OBJ_DIST * (floor(i / 3) - 1);
  #local endpos   = center + <endx, endy, 0>;
  object {
    OBJ_DEF
    translate (endpos-startpos) * clock + startpos
    pigment { color col }
  }
#end

/*
 * Draw objects
 */
#for(i, 0, 8)
  Make_Object(i, Red,   <       0,        1,  0.05>)
  Make_Object(i, Green, < sqrt(2), -sqrt(2),  0.00>)
  Make_Object(i, Blue,  <-sqrt(2), -sqrt(2), -0.05>)
#end
