#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "util.h"
#include "./third_party/lodepng.h"


char* load_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if(!file) {
	fprintf(stderr, "fopen %s failed: %d %s \n", path, errno, strerror(errno));
	exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    rewind(file);
    char *data = (char*)calloc(len + 1, sizeof(char));
    fread(data, 1, len, file);
    fclose(file);
    return data;
}

void flip_image_vertical(unsigned char *data, unsigned int width, unsigned int height) {
    unsigned int size   = width * height * 4;
    unsigned int stride = sizeof(char) * width * 4;
    unsigned char *new_data = (unsigned char *)malloc(sizeof(unsigned char) * size);
    for(unsigned int i = 0; i < height; i++) {
	unsigned int j = height - i - 1;
	memcpy(new_data + j * stride, data + i * stride, stride);
    }
    memcpy(data, new_data, size);
    free(new_data);
}

void load_png_texture(const char *file_name) {
    unsigned int error;
    unsigned char *data;
    unsigned int width, height;
    error = lodepng_decode32_file(&data, &width, &height, file_name);
    if(error) {
	fprintf(stderr, "load_png_texture %s failed, error: %u: %s\n", file_name, error, lodepng_error_text(error));
	exit(EXIT_FAILURE);
    }
    flip_image_vertical(data, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    free(data);
}

GLuint make_shader(GLenum type, const char *code) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
	GLint len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	GLchar *info = (GLchar*)calloc(len, sizeof(GLchar));
	glGetShaderInfoLog(shader, len, NULL, info);
	fprintf(stderr, "glCompileShader failed: \n%s\n", info);
	free(info);
    }
    return shader;
}

GLuint load_shader(GLenum type, const char *path) {
    char *data = load_file(path);
    GLuint shader = make_shader(type, data);
    free(data);
    return shader;
}

GLuint make_program(GLuint vs, GLuint fs) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(status != GL_TRUE) {
	GLint len;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	GLchar *info = (GLchar*)calloc(len, sizeof(GLchar));
	glGetProgramInfoLog(program, len, NULL, info);
	fprintf(stderr, "glLinkProgram failed: %s\n", info);
	free(info);
    }
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

GLuint load_program(const char *vs_path, const char *fs_path) {
    GLuint vs_shader = load_shader(GL_VERTEX_SHADER, vs_path);
    GLuint fs_shader = load_shader(GL_FRAGMENT_SHADER, fs_path);
    GLuint program   = make_program(vs_shader, fs_shader);
    return program;
}

GLuint gen_buffer(GLsizei size, GLfloat *data) {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

void del_buffer(GLuint buffer) {
    glDeleteBuffers(1, &buffer);
}
