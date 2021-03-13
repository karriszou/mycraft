#include "cube.h"
#include "item.h"
#include "matrix.h"
#include "util.h"


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
		     float n)
{
    static const float positions[6][4][3] = {
        {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
        {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
        {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
        {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
        {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
        {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}
    };
    static const float normals[6][3] = {
        {-1, 0, 0},
        {+1, 0, 0},
        {0, +1, 0},
        {0, -1, 0},
        {0, 0, -1},
        {0, 0, +1}
    };
    static const float uvs[6][4][2] = {
        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
        {{0, 1}, {0, 0}, {1, 1}, {1, 0}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{1, 0}, {1, 1}, {0, 0}, {0, 1}}
    };
    static const float indices[6][6] = {
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3}
    };
    static const float flipped[6][6] = {
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1},
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1},
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1}
    };

    float *d = data;
    float s = 0.0625;
    float a = 0 + 1 / 2048.0;
    float b = s - 1 / 2048.0;
    int faces[6] = {  left,  right,  top,  bottom,  front,  back };
    int tiles[6] = { wleft, wright, wtop, wbottom, wfront, wback };
    
    for(int i = 0; i < 6; i++) {
	if(faces[i] == 0) {
	    continue;
	}
	float du = (tiles[i] % 16) * s;
	float dv = (tiles[i] / 16) * s;
	int flip = ao[i][0] + ao[i][3] > ao[i][1] + ao[i][2];
	for(int v = 0; v < 6; v++) {
	    int j = flip ? flipped[i][v] : indices[i][v];
	    *(d++) = x + n * positions[i][j][0];
	    *(d++) = y + n * positions[i][j][1];
	    *(d++) = z + n * positions[i][j][2];
	    *(d++) = normals[i][0];
	    *(d++) = normals[i][1];
	    *(d++) = normals[i][2];
	    *(d++) = du + (uvs[i][j][0] ? b : a);
	    *(d++) = dv + (uvs[i][j][1] ? b : a);
	    *(d++) = ao[i][j];
	    *(d++) = light[i][j];
	}
    }
}

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
	       int w)
{
    int wleft   = blocks[w][0];
    int wright  = blocks[w][1];
    int wtop    = blocks[w][2];
    int wbottom = blocks[w][3];
    int wfront  = blocks[w][4];
    int wback   = blocks[w][5];

    make_cube_faces(data, ao, light, left, right, top, bottom, front, back, wleft, wright, wtop, wbottom, wfront, wback, x, y, z, n);
}

void make_plant(float *data,
		float ao,
		float light,
		float px,
		float py,
		float pz,
		float n,
		int w,
		float rotation)
{
    static const float positions[4][4][3] = {
        {{ 0, -1, -1}, { 0, -1, +1}, { 0, +1, -1}, { 0, +1, +1}},
        {{ 0, -1, -1}, { 0, -1, +1}, { 0, +1, -1}, { 0, +1, +1}},
        {{-1, -1,  0}, {-1, +1,  0}, {+1, -1,  0}, {+1, +1,  0}},
        {{-1, -1,  0}, {-1, +1,  0}, {+1, -1,  0}, {+1, +1,  0}}
    };
    static const float normals[4][3] = {
        {-1, 0, 0},
        {+1, 0, 0},
        {0, 0, -1},
        {0, 0, +1}
    };
    static const float uvs[4][4][2] = {
        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{1, 0}, {1, 1}, {0, 0}, {0, 1}}
    };
    static const float indices[4][6] = {
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3}
    };
    float *d = data;
    float s = 0.0625;
    float a = 0;
    float b = s;
    float du = (plants[w] % 16) * s;
    float dv = (plants[w] / 16) * s;
    for (int i = 0; i < 4; i++) {
        for (int v = 0; v < 6; v++) {
            int j = indices[i][v];
            *(d++) = n * positions[i][j][0];
            *(d++) = n * positions[i][j][1];
            *(d++) = n * positions[i][j][2];
            *(d++) = normals[i][0];
            *(d++) = normals[i][1];
            *(d++) = normals[i][2];
            *(d++) = du + (uvs[i][j][0] ? b : a);
            *(d++) = dv + (uvs[i][j][1] ? b : a);
            *(d++) = ao;
            *(d++) = light;
        }
    }
    float ma[16];
    float mb[16];
    mat_identity(ma);
    mat_rotate(mb, 0, 1, 0, RADIANS(rotation));
    mat_multiply(ma, mb, ma);
    mat_apply(data, ma, 24, 3, 10);
    mat_translate(mb, px, py, pz);
    mat_multiply(ma, mb, ma);
    mat_apply(data, ma, 24, 0, 10);
}

void make_cube_wireframe(float *data,
			 float x,
			 float y,
			 float z,
			 float n)
{
    static const float positions[8][3] = {
        {-1, -1, -1},
        {-1, -1, +1},
        {-1, +1, -1},
        {-1, +1, +1},
        {+1, -1, -1},
        {+1, -1, +1},
        {+1, +1, -1},
        {+1, +1, +1}
    };
    static const int indices[24] = {
        0, 1, 0, 2, 0, 4, 1, 3,
        1, 5, 2, 3, 2, 6, 3, 7,
        4, 5, 4, 6, 5, 7, 6, 7
    };
    float *d = data;
    for(int i = 0; i < 24; i++) {
	int j = indices[i];
	*(d++) = x + n * positions[j][0];
	*(d++) = y + n * positions[j][1];
	*(d++) = z + n * positions[j][2];
    }
}
