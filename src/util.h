#ifndef UTIL_H
#define UTIL_H

#include "./third_party/glad.h"

#define PI 3.14159265359
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define SIGN(x) (((x) > 0) - ((x) < 0))
#define RADIANS(degress) ((degress) * PI / 180)
#define DEGRESS(radians) ((radians) * 180 / PI)


char* load_file(const char *path);

void flip_image_vertical(unsigned char *data, unsigned int width, unsigned int height);

void load_png_texture(const char *file_name);

GLuint make_shader(GLenum type, const char *code);

GLuint load_shader(GLenum type, const char *path);

GLuint make_program(GLuint vs, GLuint fs);

GLuint load_program(const char *vs_path, const char *fs_path);

GLuint gen_buffer(GLsizei size, GLfloat *data);

void del_buffer(GLuint buffer);

#endif
