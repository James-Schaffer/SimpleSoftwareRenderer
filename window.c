// FILE INFOMATION HERE
// Created by James Schaffer on 21/01/2026.

#include "window.h"

Window createWindow(int width, int height) {
	Window window = malloc(sizeof(struct Window));

	window->height = height;
	window->width = width;

	return window;
}

void destroyWindow(Window window) {
	SDL_DestroyWindow(window->window);
	window->window = NULL;

	free(window);
	//SDL_Quit();
}

void initWindow(Window window) {
	window->window = SDL_CreateWindow(
		"TEST_HELLO_WORLD",
		window->width, window->height,
		SDL_WINDOW_FULLSCREEN
	);
}