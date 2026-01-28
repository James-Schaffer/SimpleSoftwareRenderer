#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "window.h"

#define SDL_MAIN_HANDLED

// WRITEN BY JAMES SCHAFFER 2026
//
// SIMPLE CUBE RENDERER, C & SDL
//
// Note world axis :
//
// x -left/right+
// y -front/back+
// z -up/down+

// ========== DEFINITIONS ==========

const double PI = 3.1415926535897932384;
#define DEG2RAD(x) ((x) * (PI / 180.0))

#define SDL_WINDOW_WIDTH	1920U
#define SDL_WINDOW_HEIGHT	1080U

#define CAM_FOV				(PI/2) // 90 degrees
#define CAM_CLIP_MIN		0.5

#define MAX_VERTEX			10000U
#define MAX_FACES			10000U

// ========== STRUCTS ==========

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
	v3 rotation;
	v3 defNormal;
	v3 defUp;
} CamState;
typedef struct {
	v3 position;
	v3 planePosition;
	v3 normalV;
	v3 upV;
	v3 rightV;
	double fov_scale;
} CamProjectionInfo;

// ========== OTHER VARS ==========

bool gameRunning = true;

// ========== INPUT BOOLS ==========
// Rotation
bool spaceDown = false;
bool spinToggle = false;

bool xDown = false;
bool yDown = false;
bool zDown = false;

// Scale
bool jDown = false;
bool kDown = false;

// Cam move
bool wDown = false;
bool aDown = false;
bool sDown = false;
bool dDown = false;

bool eDown = false;
bool qDown = false;



// ######################OLD TO REMOVE WHEN CHANGED###############################################

// ========== OLD MESH DATA ==========

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
// 6 faces, each with 4 vertices
const int CUBE_FACES[6][4] = {
	{2, 3, 6, 7}, // front
	{0, 1, 4, 5}, // back
	{1, 3, 5, 7}, // top
	{0, 2, 4, 6}, // bottom
	{0, 1, 2, 3}, // right
	{4, 5, 6, 7}  // left
};

// ========== OLD COLOR DATA ==========

SDL_FColor colorf[8];

// ========== OLD MESH INSTANCES ==========

Transform cube = {{0,0,0}, {0,0,0}, {1,1,1}};
Transform floorCube = {{0,50,5}, {0,0,0}, {5,50,0.1}};
Transform cube_wallL = {{-5,50,0}, {0,0,0}, {0.1,50,5}};

// ========== CAMERA TRANSFORM ==========

CamState cam = {{0, -2, 0}, {0,0,0}, {0,1,0}, {0,0,1}};

// ========== VECTOR FUNCTIONS ==========

int clampi(int d, int min, int max) {
	const int t = d < min ? min : d;
	return t > max ? max : t;
}
double clampd(double d, double min, double max) {
	const double t = d < min ? min : d;
	return t > max ? max : t;
}
v3 v3Scale(const v3 a, double s) {
	v3 r;
	r.x = a.x * s;
	r.y = a.y * s;
	r.z = a.z * s;
	return r;
}
v3 v3Add(const v3 a, const v3 b) {
	v3 r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;
	return r;
}
v3 v3Sub(const v3 a, const v3 b) {
	v3 r;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;
	return r;
}
double dotProduct(const v3 a, const v3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
v3 crossProduct(const v3 a, const v3 b) {
	v3 r;
	r.x = a.y*b.z - a.z*b.y;
	r.y = a.z*b.x - a.x*b.z;
	r.z = a.x*b.y - a.y*b.x;
	return r;
}
double v3Len(const v3 a) {
	return sqrt(dotProduct(a, a));
}
v3 normalize(const v3 v) {
	double len = v3Len(v);
	if (len == 0.0) return v;
	return v3Scale(v, 1.0 / len);
}

v2 scalev2(const v2 a, const double s) {
	v2 r;
	r.x = a.x * s;
	r.y = a.y * s;
	return r;
}
v2 normalizev2(const v2 v) {
	double len = sqrt( (v.x*v.x) + (v.y*v.y) );
	if (len == 0.0) return v;
	return scalev2(v, 1.0 / len);
}

v3 transformV3(const v3* v, const Transform* t) {
	v3 ret = *v;

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
	ret.y = (x*(cy*sz)) + (y*((sx*sy*sz)+(cx*cz))) + (z*((cx*sy*sz)-(sx*cz)));
	ret.z = (x*(-sy)) + (y*(sx*cy)) + (z*(cx*cy));

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

// ========== SETUP CAM PROJECTION VARS FOR EACH FRAME ==========

CamProjectionInfo getCamProjectionInfo(const CamState* camera) {
	CamProjectionInfo ret;

	ret.position = camera->position;

	Transform camTransform = {
		{0,0,0},
		camera->rotation,
		{1,1,1}
	};

	ret.normalV = normalize(transformV3(&camera->defNormal,&camTransform));
	ret.upV = normalize(transformV3(&camera->defUp,&camTransform));

	// Projection plane
	const v3 scaledNormal = v3Scale(ret.normalV, CAM_CLIP_MIN);
	const v3 planePoint = v3Add(camera->position, scaledNormal);

	ret.planePosition = planePoint;

	// Right vector
	ret.rightV = normalize(crossProduct(ret.upV, ret.normalV));

	// fov scale
	ret.fov_scale = SDL_WINDOW_WIDTH / (2 * tan(CAM_FOV / 2));

	return ret;
}

// ========== PROJECT A POINT IN 3D SPACE TO A 2D POSITION ON SCREEN ==========

int project3DtoScreen(const v3 point, const CamProjectionInfo* camState, v2* outV) {
	// Ray
	const v3 ray = normalize(v3Sub(point, camState->position));

	// given t = (a-p0).n / v.n (a=planeCenter , p0=camPos, n=normal, v=rayVector(normalized))

	const double vn = dotProduct(ray, camState->normalV);
	if (fabs(vn) < 1e-6) return 0; // parallel to plane (fabs = float absolute value)

	const double t = dotProduct(v3Sub(camState->planePosition, camState->position), camState->normalV) / vn;
	if (t <= 0.0) return 0; // Behind camera

	// Find intersection point
	v3 hit = v3Add(camState->position, v3Scale(ray, t));

	// Find local intersection point (relative to plane center)
	const v3 hit_planeSpace = v3Sub(hit, camState->planePosition);

	// Find x y coords on plane for intersection (0,0 center and + axis is up and right)
	double x = dotProduct(hit_planeSpace, camState->rightV);
	double y = dotProduct(hit_planeSpace, camState->upV);

	x *= camState->fov_scale;
	y *= camState->fov_scale;

	// the dot product gives x and y where 0 is center of screen so :
	// re-map 0,0 to top left and + axis to right down
	outV->x = x + SDL_WINDOW_WIDTH / 2;
	outV->y = y + SDL_WINDOW_HEIGHT / 2;

	return 1;
}

// ========== Projects an array of points to the screen ==========

// Leaks memory, make sure to delete later :D
v2* projectPoints3DtoScreen(const v3* v, const int n, const CamState* camState) {
	v2* points = malloc(sizeof(struct v2) * n);

	const CamProjectionInfo camInfo = getCamProjectionInfo(camState);

	for (int i=0; i<n; ++i) {
		v2 projectionPoint;

		int out = project3DtoScreen(v[i], &camInfo, &projectionPoint);

		if (out==1) points[i] = projectionPoint;
		else {
			points[i].x = 0;
			points[i].y = 0;
		};
	}

	return points;
}

// ===== UPDATE LOOP =====

void update(double delta) {
	if (spinToggle) {
		cube.rotation.x += PI * 0.4 * delta;
		cube.rotation.y += PI * 0.3 * delta;
		cube.rotation.z += PI * 0.5 * delta;
	}

	if (xDown) {
		cube.rotation.x += PI * 0.4 * delta;
	}
	if (yDown) {
		cube.rotation.y += PI * 0.4 * delta;
	}
	if (zDown) {
		cube.rotation.z += PI * 0.4 * delta;
	}

	// x,y plane
	v2 moveDir = {0,0};

	if (wDown) {
		moveDir.y += 1;
	}
	if (sDown) {
		moveDir.y -= 1;
	}
	if (aDown) {
		moveDir.x += 1;
	}
	if (dDown) {
		moveDir.x -= 1;
	}

	moveDir = normalizev2(moveDir);

	double ct = cos(cam.rotation.z);
	double st = sin(cam.rotation.z);

	moveDir = (v2){
		moveDir.x*ct - moveDir.y*st,
		moveDir.x*st + moveDir.y*ct
	};

	cam.position.x += moveDir.x * 2 * delta;
	cam.position.y += moveDir.y * 2 * delta;

	// Up down
	if (qDown) {
		cam.position.z -= 2 * delta;
	}
	if (eDown) {
		cam.position.z += 2 * delta;
	}

	if (jDown) {
		cube.scale.x += 0.1 * delta;
		cube.scale.y += 0.1 * delta;
		cube.scale.z += 0.1 * delta;
	}
	if (kDown) {
		cube.scale.x -= 0.1 * delta;
		cube.scale.y -= 0.1 * delta;
		cube.scale.z -= 0.1 * delta;
	}
}

// ===== RENDER FRAME =====

void render(SDL_Renderer* renderer) {
	// Clear screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	const int N = 2;

	// Mesh array
	Transform* meshes[2] = {
		&floorCube,
		//&cube_wallL,
		&cube
	};

	// for each mesh
	for (int i=0; i<N; ++i) {
		v3* points = getCubePoints(meshes[i]);
		v2* projectedPoints = projectPoints3DtoScreen(points, CUBE_POINTS, &cam);

		double centerDist = v3Len(v3Sub(meshes[i]->position, cam.position));

		// Render faces
		for (int f = 0; f < 6; f++) {
			SDL_Vertex verts[6];

			int i0 = CUBE_FACES[f][0];
			int i1 = CUBE_FACES[f][1];
			int i2 = CUBE_FACES[f][2];
			int i3 = CUBE_FACES[f][3];

			// Find center of face (halfway between corners) length of average distance to corners from cam
			double faceDist = v3Len(v3Scale(v3Add(
				v3Sub(points[i1], cam.position),
				v3Sub(points[i2], cam.position)
				), 0.5));

			if (faceDist > centerDist) continue;

			// Triangle 1 (0,1,2)
			verts[0] = (SDL_Vertex){ {projectedPoints[i0].x, projectedPoints[i0].y}, colorf[f], {0,0} };
			verts[1] = (SDL_Vertex){ {projectedPoints[i1].x, projectedPoints[i1].y}, colorf[f], {0,0} };
			verts[2] = (SDL_Vertex){ {projectedPoints[i2].x, projectedPoints[i2].y}, colorf[f], {0,0} };

			// Triangle 2 (2,1,3)
			verts[3] = (SDL_Vertex){ {projectedPoints[i2].x, projectedPoints[i2].y}, colorf[f], {0,0} };
			verts[4] = (SDL_Vertex){ {projectedPoints[i1].x, projectedPoints[i1].y}, colorf[f], {0,0} };
			verts[5] = (SDL_Vertex){ {projectedPoints[i3].x, projectedPoints[i3].y}, colorf[f], {0,0} };

			SDL_RenderGeometry(renderer, NULL, verts, 6, NULL, 0);
		}

		free(points);
		free(projectedPoints);
	}

	SDL_RenderPresent(renderer);
}


// HANDLE INPUTS

void quitGame() {
	printf("Quitting...\n");
	gameRunning = false;
}

void manageKeyDownEvent(const SDL_KeyboardEvent *e) {
	switch (e->key) {
		case SDLK_ESCAPE:
			quitGame();
			break;

		case SDLK_SPACE:
			if (spaceDown) break;
			spinToggle = !spinToggle;
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

		case SDLK_W:
			if (wDown) break;
			wDown=true;
			break;
		case SDLK_A:
			if (aDown) break;
			aDown=true;
			break;
		case SDLK_S:
			if (sDown) break;
			sDown=true;
			break;
		case SDLK_D:
			if (dDown) break;
			dDown=true;
			break;

		case SDLK_E:
			if (eDown) break;
			eDown=true;
			break;
		case SDLK_Q:
			if (qDown) break;
			qDown=true;
			break;

		case SDLK_J:
			if (jDown) break;
			jDown=true;
			break;
		case SDLK_K:
			if (kDown) break;
			kDown=true;
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

		case SDLK_W:
			if (!wDown) break;
			wDown=false;
			break;
		case SDLK_A:
			if (!aDown) break;
			aDown=false;
			break;
		case SDLK_S:
			if (!sDown) break;
			sDown=false;
			break;
		case SDLK_D:
			if (!dDown) break;
			dDown=false;
			break;

		case SDLK_E:
			if (!eDown) break;
			eDown=false;
			break;
		case SDLK_Q:
			if (!qDown) break;
			qDown=false;
			break;

		case SDLK_J:
			if (!jDown) break;
			jDown=false;
			break;
		case SDLK_K:
			if (!kDown) break;
			kDown=false;
			break;
		default:
			//printf("KeyUp\n");
			break;
	}
}

void manageMouseMotion(const SDL_MouseMotionEvent *e) {
	double sense = 0.005;

	double deltaX = e->xrel * sense;
	double deltaY = e->yrel * sense;

	cam.rotation.z += deltaX;
	cam.rotation.x += deltaY;

	if (cam.rotation.x > PI/2) {
		cam.rotation.x = PI/2;
	} else if (cam.rotation.x < -PI/2) {
		cam.rotation.x = -PI/2;
	}
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

	//Lock mouse to screen center
	SDL_SetWindowRelativeMouseMode(window->window, true);

	Uint64 now = SDL_GetPerformanceCounter();
	Uint64 last = 0;
	double deltaTime = 0.0;

	long double timeAccum = 0.0;
	Uint64 frames = 0;

	SDL_Event e;


	for (int i=0; i<8; i++) {
		colorf[i] = (SDL_FColor) {
			(float)(rand() % 256) / 255.0f,
			(float)(rand() % 256) / 255.0f,
			(float)(rand() % 256) / 255.0f,
			1.0f
		};
	}


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

		// Event handler
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
				case SDL_EVENT_MOUSE_MOTION:
					manageMouseMotion(&e.motion);
					break;
				default:
					//printf("Event\n");
					break;
			}
		}

		update(deltaTime);
		render(renderer);
	}

	// Cleanup
	SDL_DestroyRenderer(renderer);
	destroyWindow(window);
	SDL_Quit();

	return 0;
}