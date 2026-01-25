#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "window.h"

#define SDL_WINDOW_WIDTH	740U
#define SDL_WINDOW_HEIGHT	740U

#define CAM_FOV				90.0
#define CAM_CLIP_MIN		0.5
//#define CAM_CLIP_MAX		500 // not relevent for this kind of renderer

bool gameRunning = true;

bool spaceDown = false;
bool xDown = false;
bool yDown = false;
bool zDown = false;

typedef struct v3 {
	double x;
	double y;
	double z;
} v3;

typedef struct v2 {
	double x;
	double y;
} v2;
typedef struct v2i {
	int x;
	int y;
} v2i;

typedef struct  {
	v3 position;
	v3 rotation; // r p y
	v3 scale;
} Transform;
typedef struct  {
	v3 position;
	v3 normal;
	v3 up;
} CamState;
typedef struct {
	v3 position;
	v3 planePosition;
	v3 normalV;
	v3 upV;
	v3 rightV;
	double fov_scale;
} CamProjectionInfo;

const double PI = 3.1415926535897932384; // WI?? (why but pi as a joke) (I don't need pi) (so like, why pi, that's the joke) (why you still reading)
#define DEG2RAD(x) ((x) * (PI / 180.0))

const int CUBE_POINTS = 8;
const v3 CUBE_POINT_VECTORS[] = {
	{1,1,1},
	{1,1,-1},
	{1,-1,1},
	{1,-1,-1},
	{-1,1,1},
	{-1,1,-1},
	{-1,-1,1},
	{-1,-1,-1}
};
const int CUBE_EDGES = 12;
const v2i CUBE_EDGE_INDEXS[] = {
	// Top
	{0,1},
	{1,3},
	{3,2},
	{2,0},
	// Down
	{0,4},
	{1,5},
	{2,6},
	{3,7},
	// Bottom
	{4,5},
	{5,7},
	{7,6},
	{6,4}
};

// x -left/right+
// y -front/back+
// z -up/down+

Transform cube = {{300,0,300}, {0,0,0}, {100,100,100}};

// For cam take it as pos, rotation, normal
CamState cam = {{0, -300, 0}, {0,1,0}, {0,0,1}};

int currentAxis = 0;

int clampi(int d, int min, int max) {
	const int t = d < min ? min : d;
	return t > max ? max : t;
}
double clampd(double d, double min, double max) {
	const double t = d < min ? min : d;
	return t > max ? max : t;
}
v3 v3Scale(const v3* a, double s) {
	v3 r;
	r.x = a->x * s;
	r.y = a->y * s;
	r.z = a->z * s;
	return r;
}
v3 v3Add(const v3* a, const v3* b) {
	v3 r;
	r.x = a->x + b->x;
	r.y = a->y + b->y;
	r.z = a->z + b->z;
	return r;
}
v3 v3Sub(const v3* a, const v3* b) {
	v3 r;
	r.x = a->x - b->x;
	r.y = a->y - b->y;
	r.z = a->z - b->z;
	return r;
}
double dotProduct(const v3* a, const v3* b) {
	return a->x * b->x + a->y * b->y + a->z * b->z;
}
v3 crossProduct(const v3* a, const v3* b) {
	v3 r;
	r.x = a->y*b->z - a->z*b->y;
	r.y = a->z*b->x - a->x*b->z;
	r.z = a->x*b->y - a->y*b->x;
	return r;
}
double v3Len(const v3* a) {
	return sqrt(dotProduct(a, a));
}
v3 normalize(const v3* v) {
	double len = v3Len(v);
	if (len == 0.0) return *v;
	return v3Scale(v, 1.0 / len);
}

v3 transformV3(const v3* v, const Transform* t) {
	v3 ret = (*v);

	// Scale
	ret.x *= t->scale.x;// + t->position.x;
	ret.y *= t->scale.y;// = (ret.y * t->scale.y) + t->position.y;
	ret.z *= t->scale.z;//= (ret.z * t->scale.z) + t->position.z;

	// Rotate
	double x = ret.x;
	double y = ret.y;
	double z = ret.z;

	double cx = cos(t->rotation.x);
	double sx = sin(t->rotation.x);
	double cy = cos(t->rotation.y);
	double sy = sin(t->rotation.y);
	double cz = cos(t->rotation.z);
	double sz = sin(t->rotation.z);

	ret.x = (x*(cy*cz)) + (y*((sx*sy*cz)-(cx*sz))) + (z*((cx*sy*cz)+(sx*sz)));
	ret.y = (x*(cy*sz)) + (y*((sx*sy*sz)+(cx*cz))) + (z*((cx*sy*sz)+(sx*cz)));
	ret.z = (x*(-sy)) + (y*(sx*cy)) + (z*(cx*cy));

	// ret.x = ((*v).x*(cos(r.y)*cos(r.z))) + ((*v).y*((sin(r.x)*cos(r.y)*cos(r.z))-(cos(r.x)*sin(r.z)))) + ((*v).z*((cos(r.x)*sin(r.y)*cos(r.z))+(sin(r.x)*sin(r.z))));
	// ret.y = ((*v).x*(cos(r.y)*sin(r.z))) + ((*v).y*((sin(r.x)*sin(r.y)*cos(r.z))+(cos(r.x)*cos(r.z)))) + ((*v).z*((cos(r.x)*sin(r.y)*sin(r.z))-(sin(r.x)*cos(r.z))));
	// ret.z = ((*v).x*(-sin(r.y))) + ((*v).y*(sin(r.x)*sin(r.y))) + ((*v).z*(cos(r.x)*cos(r.y)));

	// ret.x = (ret.x*(cos(r.x)*cos(r.y))) + (ret.y*((cos(r.x)*sin(r.y)*sin(r.z))-(sin(r.x)*cos(r.z)))) + (ret.z*((cos(r.x)*sin(r.y)*cos(r.z))+(sin(r.x)*sin(r.z))));
	// ret.y = (ret.x*(cos(r.x)*cos(r.y))) + (ret.y*((sin(r.x)*sin(r.y)*sin(r.z))+(cos(r.x)*cos(r.z)))) + (ret.z*((sin(r.x)*sin(r.y)*cos(r.z))-(cos(r.x)*sin(r.z))));
	// ret.z = (ret.x*(-sin(r.y))) + (ret.y*(cos(r.y)*sin(r.z))) + (ret.z*(cos(r.y)*sin(r.z)));

	// Transform position
	ret.x += t->position.x;
	ret.y += t->position.y;
	ret.z += t->position.z;

	return ret;
}

// Leaks memory, make sure to delete later :D
v3* getCubePoints(const Transform* t) {
	v3* points = malloc(sizeof(struct v3) * CUBE_POINTS);

	for (int i=0; i < CUBE_POINTS; i++) {
		points[i] = transformV3(&CUBE_POINT_VECTORS[i], t);
	}

	return points;
}

CamProjectionInfo getCamProjectionInfo(const CamState* camera) {
	CamProjectionInfo ret;

	ret.position = camera->position;
	ret.normalV = camera->normal;
	ret.upV = camera->up;

	// Projection plane
	const v3 scaledNormal = v3Scale(&camera->normal, CAM_CLIP_MIN);
	const v3 planePoint = v3Add(&camera->position, &scaledNormal);

	ret.planePosition = planePoint;

	// Right vector
	const v3 rightV = crossProduct(&camera->up, &camera->normal);
	ret.rightV = normalize(&rightV);

	// fov scale
	ret.fov_scale = SDL_WINDOW_WIDTH / (2 * tan(DEG2RAD(CAM_FOV) / 2));

	return ret;
}

int project3DtoScreen(const v3* point, const CamProjectionInfo* camState, v2* outV) {
	// Ray
	const v3 ray = v3Sub(point, &camState->position);

	double denom = dotProduct(&camState->normalV, &ray);
	if (fabs(denom) < 1e-6) return 0; // parallel to plane

	const v3 s = v3Sub(&camState->planePosition, &camState->position);
	const double t = dotProduct(&camState->normalV, &s) / denom;
	if (t <= 0.0)
		return 0; // Behind camera

	const v3 scaledRay = v3Scale(&ray, t);
	v3 hit = v3Add(&camState->position, &scaledRay);

	v3 hit_planeSpace = v3Sub(&hit, &camState->planePosition);

	double x = dotProduct(&hit_planeSpace, &camState->rightV);
	double y = dotProduct(&hit_planeSpace, &camState->upV);

	x *= camState->fov_scale;
	y *= camState->fov_scale;

	outV->x = x + SDL_WINDOW_WIDTH / 2;
	outV->y = y + SDL_WINDOW_HEIGHT / 2;

	return 1;
}

// Leaks memory, make sure to delete later :D
v2* projectPoints3DtoScreen(const v3* v, const int n, const CamState* camState) {
	v2* points = malloc(sizeof(struct v2) * n);

	const CamProjectionInfo camInfo = getCamProjectionInfo(camState);

	for (int i=0; i<n; ++i) {
		v2 projectionPoint;

		int out = project3DtoScreen(&v[i], &camInfo, &projectionPoint);

		if (out==1) points[i] = projectionPoint;
		else {
			points[i].x = 0;
			points[i].y = 0;
		};
	}

	return points;
}

void quitGame() {
	printf("Quitting...\n");
	gameRunning = false;
}

void manageKeyDownEvent(const SDL_KeyboardEvent *e) {
	switch (e->key) {
		case SDLK_Q:
			quitGame();
			break;
		case SDLK_SPACE:
			if (spaceDown) break;
			spaceDown=true;
			break;
		case SDLK_X:
			if (xDown) break;
			xDown=true;
			break;
		case SDLK_Y:
			if (yDown) break;
			yDown=true;
			break;
		case SDLK_Z:
			if (zDown) break;
			zDown=true;
			break;
		default:
			//printf("KeyDown\n");
			break;
	}
}

void manageKeyUpEvent(const SDL_KeyboardEvent *e) {
	switch (e->key) {
		case SDLK_SPACE:
			if (!spaceDown) break;
			spaceDown=false;
			break;
		case SDLK_X:
			if (!xDown) break;
			xDown=false;
			break;
		case SDLK_Y:
			if (!yDown) break;
			yDown=false;
			break;
		case SDLK_Z:
			if (!zDown) break;
			zDown=false;
			break;
		default:
			//printf("KeyUp\n");
			break;
	}
}

void update(double delta) {
	if (xDown) {
		cube.rotation.x += PI * 0.4 * delta;
	}
	if (yDown) {
		cube.rotation.y += PI * 0.4 * delta;
	}
	if (zDown) {
		cube.rotation.z += PI * 0.4 * delta;
	}
}

void render(SDL_Renderer* renderer) {
	// Clear screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// Render Points
	v3* points = getCubePoints(&cube);

	v2* projectedPoints = projectPoints3DtoScreen(points, CUBE_POINTS, &cam);

	free(points);

	// Render edges
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for (int i=0; i < CUBE_EDGES; i++) {
		SDL_RenderLine(renderer,
			projectedPoints[CUBE_EDGE_INDEXS[i].x].x, projectedPoints[CUBE_EDGE_INDEXS[i].x].y,
			projectedPoints[CUBE_EDGE_INDEXS[i].y].x, projectedPoints[CUBE_EDGE_INDEXS[i].y].y
		);
	}

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for (int i=0; i < CUBE_POINTS; i++) {
		SDL_RenderPoint(renderer, projectedPoints[i].x, projectedPoints[i].y);
	}

	/*
	// Render edges
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for (int i=0; i < CUBE_EDGES; i++) {
		SDL_RenderLine(renderer,
			points[CUBE_EDGE_INDEXS[i].x].x, points[CUBE_EDGE_INDEXS[i].x].z,
			points[CUBE_EDGE_INDEXS[i].y].x, points[CUBE_EDGE_INDEXS[i].y].z
		);
	}

	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	for (int i=0; i < CUBE_POINTS; i++) {
		SDL_RenderPoint(renderer, points[i].x, points[i].z);
	}
	*/

	//free(points);
	free(projectedPoints);
	SDL_RenderPresent(renderer);
}

int main(void) {
	printf("Hello, World!\n");

	Window window = createWindow(SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT);

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	initWindow(window);
	SDL_Renderer* renderer = SDL_CreateRenderer(window->window, NULL);

	if (!renderer) {
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
	}

	Uint64 now = SDL_GetPerformanceCounter();
	Uint64 last = 0;
	double deltaTime = 0.0;

	long double timeAccum = 0.0;
	Uint64 frames = 0;

	SDL_Event e;

	while (gameRunning) {
		// Update deltaTime
		last = now;
		now = SDL_GetPerformanceCounter();
		deltaTime = (double)(now - last) / (double)SDL_GetPerformanceFrequency();

		// FPS
		timeAccum += deltaTime;
		frames++;
		if (timeAccum > 1) {
			timeAccum -= 1;
			printf("%ifps\n", frames);
			frames = 0;
		}

		// Event handeler
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_EVENT_QUIT:
					quitGame();
					break;
				case SDL_EVENT_KEY_DOWN:
					manageKeyDownEvent(&e.key);
					break;
				case SDL_EVENT_KEY_UP:
					manageKeyUpEvent(&e.key);
					break;
				default:
					//printf("Event\n");
					break;
			}
		}

		//printf("%d", player_y_position);

		update(deltaTime);
		render(renderer);
	}

	// Cleanup
	SDL_DestroyRenderer(renderer);
	destroyWindow(window);
	SDL_Quit();

	return 0;
}