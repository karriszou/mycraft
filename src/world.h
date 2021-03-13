#ifndef WORLD_H
#define WORLD_H


typedef void (*world_func)(int x, int y, int z, int w, void *arg);

void create_world(int p, int q, world_func func, void *arg);

#endif
