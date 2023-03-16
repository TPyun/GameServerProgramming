#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
#pragma comment (lib, "WS2_32.LIB")

#define WIDTH 800
#define HEIGHT 800

typedef struct two_ints {
	int x;
	int y;
} TI;

typedef struct two_floats {
	float x;
	float y;
}TF;

typedef struct key_state {
	bool up;
	bool down;
	bool left;
	bool right;
} KS;