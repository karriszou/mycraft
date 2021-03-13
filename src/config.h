#ifndef CONFIG_H
#define CONFIG_H

// App parameters
#define DEBUG 0
#define FULLSCREEN 0
#define WIDTH 1024
#define HEIGHT 768
#define VSYNC 1
#define INVERT_MOUSE 0

// Rendering options
#define SHOW_WIREFRAME 1
#define SHOW_PLANTS 1
#define SHOW_CLOUDS 1
#define SHOW_TREES 1

// Key bindings
#define CRAFT_KEY_FORWARD 'W'
#define CRAFT_KEY_BACKWARD 'S'
#define CRAFT_KEY_LEFT 'A'
#define CRAFT_KEY_RIGHT 'D'
#define CRAFT_KEY_JUMP GLFW_KEY_SPACE
#define CRAFT_KEY_FLY GLFW_KEY_TAB
#define CRAFT_KEY_OBSERVE 'O'
#define CRAFT_KEY_OBSERVE_INSET 'P'
#define CRAFT_KEY_ITEM_NEXT 'E'
#define CRAFT_KEY_ITEM_PREV 'R'
#define CRAFT_KEY_ZOOM GLFW_KEY_LEFT_SHIFT
#define CRAFT_KEY_ORTHO 'F'
#define CRAFT_KEY_CHAT 't'
#define CRAFT_KEY_COMMAND '/'
#define CRAFT_KEY_SIGN '`'

// Advanced parameters 
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 10
#define CHUNK_SIZE 32

#endif
