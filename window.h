// FILE INFOMATION HERE
// Created by James Schaffer on 21/01/2026.

#ifndef SDL_DEMO_WINDOW_H
#define SDL_DEMO_WINDOW_H

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

typedef struct Window* Window;

struct Window {
	int height;
	int width;

	SDL_Window* window;
};

Window createWindow(int width, int height);
void destroyWindow(Window window);
void initWindow(Window window);

#endif //SDL_DEMO_WINDOW_H