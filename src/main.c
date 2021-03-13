#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "./third_party/glad.h"
#include "./third_party/GLFW/glfw3.h"
#include "./third_party/noise.h"
#include "config.h"
#include "util.h"
#include "matrix.h"
#include "map.h"
#include "world.h"
#include "item.h"
#include "cube.h"

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define MAX_NAME_LENGTH 32


typedef struct {
    Map map;
    Map lights;
    int p;
    int q;
    int faces;
    int dirty;
    int miny;
    int maxy;
    GLuint buffer;
} Chunk;

typedef struct {
    int x;
    int y;
    int z;
    int w;
} Block;

typedef struct {
    float x;
    float y;
    float z;
    float rx;
    float ry;
    float t;
} State;

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    State state;
    State state1;
    State state2;
    GLuint buffer;
} Player;

typedef struct {
    GLuint program;
    GLuint position;
    GLuint normal;
    GLuint uv;
    GLuint matrix;
    GLuint sampler;
    GLuint camera;
    GLuint timer;
    GLuint extra1;
    GLuint extra2;
    GLuint extra3;
    GLuint extra4;
} Attrib;

typedef struct {
    GLFWwindow *window;
    int width;
    int height;
    Chunk chunks[MAX_CHUNKS];
    int chunk_count;
    int create_radius;
    int render_radius;
    // int delete_radius;
    Player players[MAX_PLAYERS];
    int player_count;
    int observe1;
    int observe2;
    int item_index;
    int flying;
    int typing;
    int scale;
    int ortho;
    float fov;
    int day_length;
    int time_changed;

} Model;

static Model model;
static Model *g = &model;


int get_scale_factor() {
    int window_width, window_height;
    int buffer_width, buffer_height;
    glfwGetWindowSize(g->window, &window_width, &window_height);
    glfwGetFramebufferSize(g->window, &buffer_width, &buffer_height);
    int result = buffer_width / window_width;
    result = MAX(1, result);
    result = MIN(2, result);
    return result;
}

void get_motion_vector(int flying, int sz, int sx, float rx, float ry, float *vx, float *vy, float *vz) {
    *vx = 0; *vy = 0; *vz = 0;
    if(!sz && !sx) {
	return;
    }
    float strafe = atan2f(sz, sx);
    if(flying) {
	float m = cosf(ry);
	float y = sinf(ry);
	if(sx) {
	    if(!sz) {
		y = 0;
	    }
	    m = 1;
	}
	if(sz > 0) {
	    y = -y;
	}
	*vx = cosf(rx + strafe) * m;
	*vy = y;
	*vz = sinf(rx + strafe) * m;
    }
    else {
	*vx = cosf(rx + strafe);
	*vy = 0;
	*vz = sinf(rx + strafe);
    }
}

GLuint gen_wireframe_buffer(float x, float y , float z, float n) {
    float data[72];
    make_cube_wireframe(data, x, y, z, n);
    return gen_buffer(sizeof(data), data);
}

void draw_triangles_3d_ao(Attrib *attrib, GLuint buffer, int count) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
    glVertexAttribPointer(attrib->normal,   3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
    glVertexAttribPointer(attrib->uv,       4, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_lines(Attrib *attrib, GLuint buffer, int components, int count) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(attrib->position, components, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_LINES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_chunk(Attrib *attrib, Chunk *chunk) {
    draw_triangles_3d_ao(attrib, chunk->buffer, chunk->faces * 6);
}

void occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4], float light[6][4]) {
    static const int lookup3[6][4][3] = {
        {{0, 1, 3}, {2, 1, 5}, {6, 3, 7}, {8, 5, 7}},
        {{18, 19, 21}, {20, 19, 23}, {24, 21, 25}, {26, 23, 25}},
        {{6, 7, 15}, {8, 7, 17}, {24, 15, 25}, {26, 17, 25}},
        {{0, 1, 9}, {2, 1, 11}, {18, 9, 19}, {20, 11, 19}},
        {{0, 3, 9}, {6, 3, 15}, {18, 9, 21}, {24, 15, 21}},
        {{2, 5, 11}, {8, 5, 17}, {20, 11, 23}, {26, 17, 23}}
    };
   static const int lookup4[6][4][4] = {
        {{0, 1, 3, 4}, {1, 2, 4, 5}, {3, 4, 6, 7}, {4, 5, 7, 8}},
        {{18, 19, 21, 22}, {19, 20, 22, 23}, {21, 22, 24, 25}, {22, 23, 25, 26}},
        {{6, 7, 15, 16}, {7, 8, 16, 17}, {15, 16, 24, 25}, {16, 17, 25, 26}},
        {{0, 1, 9, 10}, {1, 2, 10, 11}, {9, 10, 18, 19}, {10, 11, 19, 20}},
        {{0, 3, 9, 12}, {3, 6, 12, 15}, {9, 12, 18, 21}, {12, 15, 21, 24}},
        {{2, 5, 11, 14}, {5, 8, 14, 17}, {11, 14, 20, 23}, {14, 17, 23, 26}}
    };
    static const float curve[4] = {0.0, 0.25, 0.5, 0.75};
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            int corner = neighbors[lookup3[i][j][0]];
            int side1 = neighbors[lookup3[i][j][1]];
            int side2 = neighbors[lookup3[i][j][2]];
            int value = side1 && side2 ? 3 : corner + side1 + side2;
            float shade_sum = 0;
            float light_sum = 0;
            int is_light = lights[13] == 15;
            for (int k = 0; k < 4; k++) {
                shade_sum += shades[lookup4[i][j][k]];
                light_sum += lights[lookup4[i][j][k]];
            }
            if (is_light) {
                light_sum = 15 * 4 * 10;
            }
            float total = curve[value] + shade_sum / 4.0;
            ao[i][j] = MIN(total, 1.0);
            light[i][j] = light_sum / 15.0 / 4.0;
        }
    }
}

int chunked(float x) {
    return floorf(roundf(x) / CHUNK_SIZE);
}

Chunk* find_chunk(int p, int q) {
    for(int i = 0; i < g->chunk_count; i++) {
	Chunk *chunk = g->chunks + i;
	if(chunk->p == p && chunk->q == q) {
	    return chunk;
	}
    }
    return 0;
}

void _set_block(int p, int q, int x, int y, int z, int w, int dirty) {
    Chunk *chunk = find_chunk(p, q);
    if(chunk) {
	Map *map = &chunk->map;
	if(map_set(map, x, y, z, w)) {
	    if(dirty) {
		chunk->dirty = 1;	// @Change
		// dirty_block(chunk);
	    }
	}
    }
}

void set_block(int x, int y, int z, int w) {
    int p = chunked(x);
    int q = chunked(z);
    _set_block(p, q, x, y, z, w, 1);
    for(int dx = -1; dx <= 1; dx++) {
	for(int dz = -1; dz <= 1; dz++) {
	    if (dx == 0 && dz == 0) {
                continue;
            }
            if (dx && chunked(x + dx) == p) {
                continue;
            }
            if (dz && chunked(z + dz) == q) {
                continue;
            }
            _set_block(p + dx, q + dz, x, y, z, -w, 1);
	}
    }
}

int get_block(int x, int y, int z) {
    int p = chunked(x);
    int q = chunked(z);
    Chunk *chunk = find_chunk(p, q);
    if(chunk) {
	Map *map = &chunk->map;
	return map_get(map, x, y, z);
    }
    return 0;
}

int highest_block(float x, float z) {
    int result = -1;
    int nx = roundf(x);
    int nz = roundf(z);
    int p  = chunked(x);
    int q  = chunked(z);
    Chunk *chunk = find_chunk(p, q);
    if(chunk) {
	Map *map = &chunk->map;
	MAP_FOR_EACH(map, ex, ey, ez, ew) {
	    if(is_obstacle(ew) && ex == nx && ez == nz) {
		result = MAX(result, ey);
	    }
	} END_MAP_FOR_EACH;
    }
    return result;
}

int chunk_distance(Chunk *chunk, int p, int q) {
    int dp = ABS(chunk->p - p);
    int dq = ABS(chunk->q - q);
    return MAX(dp, dq);
}

int player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz) {
    int nx = roundf(x);
    int ny = roundf(y);
    int nz = roundf(z);
    for(int i = 0; i < height; i++) {
	if(nx == hx && ny == hy && nz == hz) {
	    return 1;
	}
    }
    return 0;
}

int collide(int height, float *x, float *y, float *z) {
    int result = 0;
    int p = chunked(*x);
    int q = chunked(*z);
    Chunk *chunk = find_chunk(p, q);
    if(!chunk) {
	return result;
    }
    Map *map = &chunk->map;
    int nx = roundf(*x);
    int ny = roundf(*y);
    int nz = roundf(*z);
    float px = *x - nx;
    float py = *y - ny;
    float pz = *z - nz;
    float pad = 0.25;
    for(int dy = 0; dy < height; dy++) {
	if(px < -pad && is_obstacle(map_get(map, nx - 1, ny - dy, nz))) {
	    *x = nx - pad;
	}
	if(px >  pad && is_obstacle(map_get(map, nx + 1, ny - dy, nz))) {
	    *x = nx + pad;
	}
	if(py < -pad && is_obstacle(map_get(map, nx, ny - dy - 1, nz))) {
	    *y = ny - pad;
	    result = 1;
	}
	if(py >  pad && is_obstacle(map_get(map, nx, ny - dy + 1, nz))) {
	    *y = ny + pad;
	    result = 1;
	}
	if(pz < -pad && is_obstacle(map_get(map, nx, ny - dy, nz - 1))) {
	    *z = nz - pad;
	}
	if(pz >  pad && is_obstacle(map_get(map, nx, ny - dy, nz + 1))) {
	    *z = nz + pad;
	}
    }
    return result;
}

void get_sight_vector(float rx, float ry, float *vx, float *vy, float *vz) {
    float m = cosf(ry);
    *vx = cosf(rx - RADIANS(90)) * m;
    *vy = sinf(ry);
    *vz = sinf(rx - RADIANS(90)) * m;
}

int _hit_test(Map *map, float max_distance, int previous, float x, float y, float z, float vx, float vy, float vz, int *hx, int *hy, int *hz) {
    int m = 32;
    int px = 0;
    int py = 0;
    int pz = 0;
    for(int i = 0; i < max_distance * m; i++) {
	int nx = roundf(x);
	int ny = roundf(y);
	int nz = roundf(z);
	if(nx != px || ny != py || nz != pz) {
	    int hw = map_get(map, nx, ny, nz);
	    if(hw > 0) {
		if(previous) {
		    *hx = px; *hy = py; *hz = pz;
		}
		else {
		    *hx = nx; *hy = ny; *hz = nz;
		}
		return hw;
	    }
	    px = nx; py = ny; pz = nz;
	}
	x += vx / m; y += vy / m; z += vz / m;
    }
    return 0;
}

int hit_test(int previous, float x, float y, float z, float rx, float ry, int *bx, int *by, int *bz) {
    int result = 0;
    float best = 0;
    int p = chunked(x);
    int q = chunked(z);
    float vx, vy, vz;
    get_sight_vector(rx, ry, &vx, &vy, &vz);
    for(int i = 0; i < g->chunk_count; i++) {
	Chunk *chunk = g->chunks + i;
	if(chunk_distance(chunk, p, q) > 1) {
	    continue;
	}
	int hx, hy, hz;
	int hw = _hit_test(&chunk->map, 8, previous, x, y, z, vx, vy, vz, &hx, &hy, &hz);
	if(hw >0) {
	    float d = sqrtf(powf(hx - x, 2) + powf(hy - y, 2) + powf(hz - z, 2));
	    if(best == 0 || d < best) {
		best = d;
		*bx = hx; *by = hy; *bz = hz;
		result = hw;
	    }
	}
    }
    return result;
}

#define Y_SIZE 258
#define XZ_SIZE (CHUNK_SIZE * 3 + 2)
#define XZ_LO (CHUNK_SIZE)
#define XZ_HI (CHUNK_SIZE * 2 + 1)
#define XZ(x, z) ((x) * XZ_SIZE  + (z))
#define XYZ(x, y, z) ((y) * XZ_SIZE * XZ_SIZE + (x) * XZ_SIZE + (z))

float* compute_chunk(Chunk *chunk, Map *block_maps[3][3], Map *light_maps[3][3]) {
    char *opaque  = (char*)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
    char *light   = (char*)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
    char *highest = (char*)calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

    int ox = chunk->p * CHUNK_SIZE - CHUNK_SIZE - 1;
    int oy = -1;
    int oz = chunk->q * CHUNK_SIZE - CHUNK_SIZE - 1;

    // Populate opaque array
    for(int a = 0; a < 3; a++) {
    	for(int b = 0; b < 3; b++) {
    	    Map *map = block_maps[a][b];
    	    if(!map) {
    		continue;
    	    }
    	    MAP_FOR_EACH(map, ex, ey, ez, ew) {
    		int x = ex - ox;
    		int y = ey - oy;
    		int z = ez - oz;
    		int w = ew;
                // TODO: this should be unnecessary
                if (x < 0 || y < 0 || z < 0) {
                    continue;
                }
                if (x >= XZ_SIZE || y >= Y_SIZE || z >= XZ_SIZE) {
                    continue;
                }
                // END TODO
    		opaque[XYZ(x, y, z)] = !is_transparent(w);
    		if(opaque[XYZ(x, y, z)]) {
    		    highest[XZ(x, z)] = MAX(highest[XZ(x, z)], y);
    		}
    	    } END_MAP_FOR_EACH
    	}
    }

    Map *map = block_maps[1][1];

    // Count exposed faces
    int maxy = 0;
    int miny = 256;
    int faces = 0;
    MAP_FOR_EACH(map, ex, ey, ez, ew) {
    	if(ew <= 0) {
    	    continue;
    	}
    	int x = ex - ox;
    	int y = ey - oy;
    	int z = ez - oz;
    	int f1 = !opaque[XYZ(x - 1, y, z)];
    	int f2 = !opaque[XYZ(x + 1, y, z)];
    	int f3 = !opaque[XYZ(x, y + 1, z)];
    	int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
    	int f5 = !opaque[XYZ(x, y, z - 1)];
    	int f6 = !opaque[XYZ(x, y, z + 1)];
    	int total = f1 + f2 + f3 + f4 + f5 + f6;
    	if(total == 0) {
    	    continue;
    	}
    	if(is_plant(ew)) {
    	    total = 4;
    	}
    	miny = MIN(miny, ey);
    	maxy = MAX(maxy, ey);
    	faces += total;
    } END_MAP_FOR_EACH;

    // Generate geometry
    int offset = 0;
    GLfloat *data = (GLfloat*)malloc(sizeof(GLfloat) * 6 * 10 * faces);
    
    MAP_FOR_EACH(map, ex, ey, ez, ew) {
    	if(ew <= 0) {
    	    continue;
    	}
    	int x = ex - ox;
    	int y = ey - oy;
    	int z = ez - oz;
    	int f1 = !opaque[XYZ(x - 1, y, z)];
    	int f2 = !opaque[XYZ(x + 1, y, z)];
    	int f3 = !opaque[XYZ(x, y + 1, z)];
    	int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
    	int f5 = !opaque[XYZ(x, y, z - 1)];
    	int f6 = !opaque[XYZ(x, y, z + 1)];
    	int total = f1 + f2 + f3 + f4 + f5 + f6;
    	if(total == 0) {
    	    continue;
    	}
	
    	int index = 0;
    	char neighbors[27] = { 0 };
    	char lights[27] = { 0 };
    	float shades[27] = { 0 };
    	for(int dx = -1; dx <= 1; dx++) {
    	    for(int dy = -1; dy <= 1; dy++) {
    		for(int dz = -1; dz <= 1; dz++) {
    		    shades[index] = 0;
    		    lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
    		    neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
    		    if(y + dy <= highest[XZ(x + dx, z + dz)]) {
    			for(int oy = 0; oy < 8; oy++) {
    			    if(opaque[XYZ(x + dx, y + dy, z + dz)]) {
    				shades[index] = 1.0 - oy * 0.125;
    				break;
    			    }
    			}
    		    }
    		    index++;
    		}
    	    }
    	}
	
    	float ao[6][4];
    	float light[6][4];
    	occlusion(neighbors, lights, shades, ao , light);
    	if(is_plant(ew)) {
    	    total = 4;
    	    float min_ao = 1;
    	    float max_light = 0;
    	    for(int a = 0; a < 6; a++) {
    		for(int b = 0; b < 4; b++) {
    		    min_ao = MIN(min_ao, ao[a][b]);
    		    max_light = MAX(max_light, light[a][b]);
    		}
    	    }
    	    float rotation = simplex2(ex, ez, 4, 0.5, 2) * 360;
    	    make_plant(data + offset, min_ao, max_light, ex, ey, ez, 0.5, ew, rotation);
    	}
    	else {
    	    make_cube(data + offset, ao, light, f1, f2, f3, f4, f5, f6, ex, ey, ez, 0.5, ew);
    	}
    	offset += total * 60;
    } END_MAP_FOR_EACH;

    free(opaque);
    free(light);
    free(highest);

    chunk->miny = miny;
    chunk->maxy = maxy;
    chunk->faces = faces;
    return data;
}

void gen_chunk_buffer(Chunk *chunk) {
    Map *block_maps[3][3];
    Map *light_maps[3][3];

    for(int dp = -1; dp <= 1; dp++) {
	for(int dq = -1; dq <= 1; dq++) {
	    Chunk *other = chunk;
	    if(dp || dq) {
		other = find_chunk(chunk->p + dp, chunk->q + dq);
	    }
	    if(other) {
		block_maps[dp + 1][dq + 1] = &other->map;
		light_maps[dp + 1][dq + 1] = &other->lights;
	    }
	    else {
		block_maps[dp + 1][dq + 1] = 0;
		light_maps[dp + 1][dq + 1] = 0;
	    }
	}
    }
    
    float *data = compute_chunk(chunk, block_maps, light_maps);

    // GLuint vao;
    // glGenVertexArrays(1, &vao);
    // glBindVertexArray(vao);
    glDeleteBuffers(1, &chunk->buffer);
    GLuint buffer;
    GLsizei size = sizeof(float) * 6 * 10 * chunk->faces;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    chunk->buffer = buffer;
    chunk->dirty = 0;
}

void init_chunk(Chunk *chunk, int p, int q) {
    chunk->p = p;
    chunk->q = q;
    chunk->faces = 0;
    chunk->buffer = 0;
    Map *block_map = &chunk->map;
    Map *light_map = &chunk->lights;
    int dx = p * CHUNK_SIZE - 1;
    int dy = 0;
    int dz = q * CHUNK_SIZE - 1;
    map_alloc(block_map, dx, dy, dz, 0x7fff);
    map_alloc(light_map, dx, dy, dz, 0xf);
}

void map_set_func(int x, int y, int z, int w, void *arg) {
    Map *map = (Map*)arg;
    map_set(map, x, y, z, w);
}

void create_chunk(Chunk *chunk, int p, int q) {
    init_chunk(chunk, p, q);
    create_world(p, q, map_set_func, &chunk->map);
}

void force_chunks(Player *player) {
    State *s = &player->state;
    int p = chunked(s->x);
    int q = chunked(s->z);
    int r = 1;
    for(int dp = -r; dp <= r; dp++) {
	for(int dq = -r; dq <= r; dq++) {
	    int a = p + dp;
	    int b = q + dq;
	    Chunk *chunk = find_chunk(a, b);
	    if(chunk) {
		if(chunk->dirty) {
		    gen_chunk_buffer(chunk);
		}
	    }
	    else if(g->chunk_count < MAX_CHUNKS) {
		chunk = g->chunks + g->chunk_count++;
		create_chunk(chunk, a, b);
		gen_chunk_buffer(chunk);
	    }
	}
    }
}

int render_chunks(Attrib *attrib, Player *player) {
    int result = 0;
    State *s = &player->state;
    force_chunks(player);
    // int p = chunked(s->x);
    // int q = chunked(s->z);
    
    float matrix[16];
    set_matrix_3d(matrix, g->width, g->height, s->x, s->y, s->z, s->rx, s->ry, g->fov, g->ortho, g->render_radius);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->extra3, g->render_radius * CHUNK_SIZE);

    for(int i = 0; i < g->chunk_count; i++) {
	Chunk *chunk = g->chunks + i;
	draw_chunk(attrib, chunk);
	result += chunk->faces;
    }
    return result;
}

void render_wireframe(Attrib *attrib, Player *player) {
    State *s = &player->state;
    float matrix[16];
    set_matrix_3d(matrix, g->width, g->height, s->x, s->y, s->z, s->rx, s->ry, g->fov, g->ortho, g->render_radius);
    int hx, hy, hz;
    int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if(is_obstacle(hw)) {
	glUseProgram(attrib->program);
	glLineWidth(1);
	glEnable(GL_COLOR_LOGIC_OP);
	glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
	GLuint wireframe_buffer = gen_wireframe_buffer(hx, hy, hz, 0.53);
	draw_lines(attrib, wireframe_buffer, 3, 24);
	glDeleteBuffers(1, &wireframe_buffer);
	glDisable(GL_COLOR_LOGIC_OP);
    }
}

void on_light() {
    
}

void on_right_click() {
    State *s = &g->players->state;
    int hx, hy, hz;
    int hw = hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if(hy > 0 && hy < 256 && is_obstacle(hw)) {
	if(!player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz)) {
	    set_block(hx, hy, hz, items[g->item_index]);
	}
    }
}

void on_left_click() {
    State *s = &g->players->state;
    int hx, hy, hz;
    int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if(hw > 0 && hw < 256 && is_destructable(hw)) {
	set_block(hx, hy, hz, 0);
	if(is_plant(get_block(hx, hy + 1, hz))) {
	    set_block(hx, hy + 1, hz, 0);
	}
    }
}

void on_key(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    int control   = mods & ( GLFW_MOD_CONTROL | GLFW_MOD_SUPER);
    int exclusive = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    if(action == GLFW_RELEASE) {
	return;
    }
    if(key == GLFW_KEY_BACKSPACE) {
	// Delete input char
    }
    if(action != GLFW_PRESS) {
	return;
    }
    if(key == GLFW_KEY_ESCAPE) {
	if(g->typing) {
	    g->typing = 0;
	}
	else if(exclusive) {
	    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
    }
    if(key == GLFW_KEY_ENTER) {
	// Enter input char and signs
	if(g->typing) {
	    
	}
	else {
	    if(control) {
		on_right_click();
	    }
	    else {
		on_left_click();
	    }
	}
    }
    if(control && key == 'V') {
	// const char *buffer = glfwGetClipboardString(window);
    }
    if(!g->typing) {
	if(key == CRAFT_KEY_FLY) {
	    g->flying = !g->flying;
	}
	// if(key == '0') {
	//     g->item_index = '9';
	// }
	// if(key >= '1' && key <= '9') {
	//     g->item_index = key - '1';
	// }
        // if (key == CRAFT_KEY_ITEM_NEXT) {
        //     g->item_index = (g->item_index + 1) % item_count;
        // }
        // if (key == CRAFT_KEY_ITEM_PREV) {
        //     g->item_index--;
        //     if (g->item_index < 0) {
        //         g->item_index = item_count - 1;
        //     }
        // }
        // if (key == CRAFT_KEY_OBSERVE) {
        //     g->observe1 = (g->observe1 + 1) % g->player_count;
        // }
        // if (key == CRAFT_KEY_OBSERVE_INSET) {
        //     g->observe2 = (g->observe2 + 1) % g->player_count;
        // }
    }
    
}

void on_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    int control   = mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER);
    int exclusive = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    if(action != GLFW_PRESS) {
	return;
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
	if(exclusive) {
	    if(control) {
		on_right_click();
	    }
	    else {
		on_left_click();
	    }
	}
	else {
	    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
    }
    if(button == GLFW_MOUSE_BUTTON_RIGHT) {
	if(exclusive) {
	    if(control) {
		on_light();
	    }
	    else {
		on_right_click();
	    }
	}
    }
    // if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    //     if (exclusive) {
    //         on_middle_click();
    //     }
    // }
}

int create_window() {
    if(!glfwInit())
	return 0;
    int width  = WIDTH;
    int height = HEIGHT;
    GLFWmonitor *monitor = NULL;
    if(FULLSCREEN)
    {
	int mode_count;
	monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *modes = glfwGetVideoModes(monitor, &mode_count);
	width  = modes[mode_count - 1].width;
	height = modes[mode_count - 1].height;
    }
    g->window = glfwCreateWindow(width, height, "MyCraft", monitor, NULL);
    return 1;
}

void handle_mouse_input() {
    int exclusive = glfwGetInputMode(g->window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    static double px = 0;
    static double py = 0;
    State *s = &g->players->state;
    if (exclusive && (px || py)) {
	double mx, my;
	glfwGetCursorPos(g->window, &mx, &my);
	float m = 0.0025;
	s->rx += (mx - px) * m;
	if(INVERT_MOUSE) {
	    s->ry += (my - py) * m;
	}
	else {
	    s->ry -= (my - py) * m;	    
	}
	if (s->rx < 0) {
	    s->rx += RADIANS(360);
	}
	if (s->rx >= RADIANS(360)) {
	    s->rx -= RADIANS(360);
	}
	s->ry = MAX(s->ry, -RADIANS(90));
	s->ry = MIN(s->ry,  RADIANS(90));
	px = mx;
	py = my;
    }
    else {
	glfwGetCursorPos(g->window, &px, &py);
    }
}

void handle_movement(double dt) {
    static float dy = 0;
    State *s = &g->players->state;
    int sx = 0, sz = 0;
    if(!g->typing) {
	float m = dt * 1.0;
	g->ortho = glfwGetKey(g->window, CRAFT_KEY_ORTHO) ? 64 : 0;
	g->fov = glfwGetKey(g->window, CRAFT_KEY_ZOOM) ? 15 : 65;
	if(glfwGetKey(g->window, CRAFT_KEY_FORWARD)) sz--;
	if(glfwGetKey(g->window, CRAFT_KEY_BACKWARD)) sz++;
	if(glfwGetKey(g->window, CRAFT_KEY_LEFT)) sx--;
	if(glfwGetKey(g->window, CRAFT_KEY_RIGHT)) sx++;
	if(glfwGetKey(g->window, GLFW_KEY_LEFT)) s->rx -= m;
	if(glfwGetKey(g->window, GLFW_KEY_RIGHT)) s->rx += m;
	if(glfwGetKey(g->window, GLFW_KEY_UP)) s->ry += m;
	if(glfwGetKey(g->window, GLFW_KEY_DOWN)) s->ry -= m;	
    }
    float vx, vy, vz;
    get_motion_vector(g->flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
    if(!g->typing) {
	if(glfwGetKey(g->window, CRAFT_KEY_JUMP)) {
	    if(g->flying) {
		vy = 1;
	    }
	    else if(dy == 0) {
		dy = 8;
	    }
	}
    }
    float speed = g->flying ? 20 : 5;
    int estimate = roundf(sqrtf
			  (powf(vx * speed, 2) +
			   powf(vy * speed + ABS(dy) * 2, 2) +
			   powf(vz * speed, 2)) * dt * 8);
    int step = MAX(8, estimate);
    float ut = dt / step;
    vx = vx * ut * speed;
    vy = vy * ut * speed;
    vz = vz * ut * speed;
    for(int i = 0; i < step; i++) {
	if(g->flying) {
	    dy = 0;
	}
	else {
	    dy -= ut * 25;
	    dy = MAX(dy, -250);
	}
	s->x += vx;
	s->y += vy + dy * ut;
	s->z += vz;
	if(collide(2, &s->x, &s->y, &s->z)) {
	    dy = 0;
	}
    }
    if(s->y < 0) {
	s->y = highest_block(s->x, s->z) + 2;
    }
}

int main() {
    srand(time(NULL));
    rand();
    
    if(!create_window())
    {
	printf("ERROR::Can not creat window!\n");
	return -1;
    }

    if(!g->window)
    {
	printf("ERROR::Create window failed!!\n");
	glfwTerminate();
	return -1;
    }
    glfwMakeContextCurrent(g->window);
    glfwSwapInterval(VSYNC);
    glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(g->window, on_key);
    glfwSetMouseButtonCallback(g->window, on_mouse_button);
    // glfwSetScrollCallback(g->window, on_scroll);
    
    if(!gladLoadGL())
    {
	printf("ERROR::Can not load OpenGL!\n");
	return -1;
    }

    // glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    // Invert clear color?
    glLogicOp(GL_INVERT);
    // glClearColor(.2, .2 , .2, 1);
    glClearColor(0, 0, 0, 1);
    
    // Load texture
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("../shared/texture.png");

    // Load shaders
    Attrib block_attrib = { 0 };
    Attrib line_attrib  = { 0 };

    GLuint program;

    program = load_program("../glsl/line.vs", "../glsl/line.fs");
    // program = load_program("../shaders/line_vertex.glsl", "../shaders/line_fragment.glsl");
    glUseProgram(program);
    line_attrib.program = program;
    line_attrib.position = 0;
    line_attrib.matrix = glGetUniformLocation(program, "matrix");

    // program = load_program("../shaders/block_vertex.glsl", "../shaders/block_fragment.glsl");
    program = load_program("../glsl/block.vs", "../glsl/block.fs");
    glUseProgram(program);
    block_attrib.program = program;
    block_attrib.position = 0;
    block_attrib.normal = 1;
    block_attrib.uv = 2;
    block_attrib.matrix = glGetUniformLocation(program, "matrix");
    block_attrib.sampler = glGetUniformLocation(program, "sampler");
    block_attrib.camera = glGetUniformLocation(program, "camera");
    block_attrib.timer = glGetUniformLocation(program, "timer");
    block_attrib.extra1 = glGetUniformLocation(program, "sky_sampler");
    block_attrib.extra2 = glGetUniformLocation(program, "daylight");
    block_attrib.extra3 = glGetUniformLocation(program, "fog_distance");
    block_attrib.extra4 = glGetUniformLocation(program, "ortho");

    g->create_radius = CREATE_CHUNK_RADIUS;
    g->render_radius = RENDER_CHUNK_RADIUS;
    
    // Outer loop
    int running = 1;
    while(running)
    {
	
	Player* me = g->players;
	
	force_chunks(me);
	
	double previous = glfwGetTime();
	// Main loop
	while(1)
	{
	    // Window size and scale
	    g->scale = get_scale_factor();
	    glfwGetFramebufferSize(g->window, &g->width, &g->height);
	    glViewport(0, 0, g->width, g->height);
	    
	    // Frame rate
	    if(g->time_changed)
	    {
		g->time_changed = 0;
		
	    }

	    double now = glfwGetTime();
	    double dt  = now - previous;
	    dt = MIN(dt, 0.2);
	    dt = MAX(dt, 0.0);
	    previous   = now;

	    // Handle mouse input and movement
	    handle_mouse_input();

	    handle_movement(dt);

	    // Prepare to render
	    
	    Player *player = g->players;

	    // Render 3-D scene
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    int face_count = render_chunks(&block_attrib, player); (void)face_count;

	    if(SHOW_WIREFRAME) {
		render_wireframe(&line_attrib, player);
	    }


	    glfwPollEvents();
	    glfwSwapBuffers(g->window);
	    if(glfwWindowShouldClose(g->window))
	    {
	    	running = 0;
	    	break;
	    }
	}

    }

    glfwTerminate();
    return 0;
}
