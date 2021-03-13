#ifndef CUBE_H
#define CUBE_H

#include <math.h>


void make_cube_faces(float *data,
		     float ao[6][4],
		     float light[6][4], 
		     int left,
		     int right,
		     int top,
		     int bottom,
		     int front,
		     int back,
		     int wleft,
		     int wright,
		     int wtop,
		     int wbottom,
		     int wfront,
		     int wback,
		     float x,
		     float y,
		     float z,
		     float n);

void make_cube(float *data,
	       float ao[6][4],
	       float light[6][4], 
	       int left,
	       int right,
	       int top,
	       int bottom,
	       int front,
	       int back,
	       float x,
	       float y,
	       float z,
	       float n,
	       int w);

void make_plant(float *data,
		float ao,
		float light,
		float px,
		float py,
		float pz,
		float n,
		int w,
		float rotation);

void make_cube_wireframe(float *data,
			 float x,
			 float y,
			 float z,
			 float n);


#endif
