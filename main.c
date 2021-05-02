#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>

#define HOR 32
#define VER 24
#define CELL 10

int WIDTH = HOR * CELL;
int HEIGHT = VER * CELL;

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
SDL_Texture *gTexture = NULL;
Uint32 *pixels;

Uint32 palette[] = {
	0x1D2021,
	0xEBDBB2,
	0xCC241D,
	0x98971A,
	0xD79921,
	0x458588,
	0xB16286,
	0x689D6A,
	0xA89984};

typedef enum {
	BG,
	FG,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	PURPLE,
	AQUA,
	GRAY
} Color;

typedef enum {
	DRAW,
	WRITE,
	SELECT,
	DELETE
} BrushMode;

typedef struct Brush {
	int down;
	BrushMode mode;
} Brush;

Brush brush;

typedef struct Cell {
	int x;
	int y;
	int state[CELL][CELL];
} Cell;

Cell cells[HOR][VER];

void
clear_cell(Cell *c)
{
	int i, j;
	for(i = 0; i < CELL; i++) {
		for(j = 0; j < CELL; j++) {
			cells[c->x][c->y].state[i][j] = 0;
		}
	}
}

void
init_cells(void)
{
	int i, j;
	for(i = 0; i < HOR; i++) {
		for(j = 0; j < VER; j++) {
			cells[i][j].x = i;
			cells[i][j].y = j;
			clear_cell(&cells[i][j]);
		}
	}
}

Cell
get_cell(int x, int y)
{
	return cells[x][y];
}

Cell *
get_cell_at_mouse(int x, int y)
{
	int cellX = (x - x % CELL) / CELL;
	int cellY = (y - y % CELL) / CELL;
	return &cells[cellX][cellY];
}

typedef struct Buffer {
	int x;
	int y;
	int size;
	int state[CELL][CELL];
} Buffer;

Buffer buffer;

void
clear_buffer(Buffer *b)
{
	int i, j;
	for(i = 0; i < b->size; i++) {
		for(j = 0; j < b->size; j++) {
			b->state[i][j] = 0;
		}
	}
}

void
init_buffer(Buffer *b)
{
	int i;
	b->size = CELL;
	b->x = 2;
	b->y = VER - 2 - CELL;
	clear_buffer(b);
	for(i = 0; i < CELL; i++) {
		b->state[i][0] = 1;
		b->state[i][CELL - 1] = 1;
	}
	for(i = 1; i < CELL - 1; i++) {
		b->state[0][i] = 1;
		b->state[CELL - 1][i] = 1;
	}
}

void
mod_buffer(Buffer *b, int x, int y, int state)
{
	if(x >= b->x && x < b->x + b->size &&
		y >= b->y && y < b->y + b->size) {
		if(state && b->state[x - b->x][y - b->y] == 0) {
			b->state[x - b->x][y - b->y] = 1;
		}
		if(!state && b->state[x - b->x][y - b->y] == 1) {
			b->state[x - b->x][y - b->y] = 0;
		}
	}
}

void
set_buffer_to_cell(Buffer *b, Cell *c)
{
	int i, j;
	for(i = 0; i < b->size; i++) {
		for(j = 0; j < b->size; j++) {
			cells[c->x][c->y].state[i][j] = b->state[i][j];
		}
	}
}

void
set_cell_to_buffer(Cell *c, Buffer *b)
{
	int i, j;
	for(i = 0; i < CELL; i++) {
		for(j = 0; j < CELL; j++) {
			b->state[i][j] = cells[c->x][c->y].state[i][j];
		}
	}
}

void
handle_mouse(SDL_Event *event)
{
	Cell *c;
	c = get_cell_at_mouse(event->motion.x / 2, event->motion.y / 2);
	switch(event->type) {
	case SDL_MOUSEBUTTONDOWN:
		brush.down = 1;
		if(event->button.button == SDL_BUTTON_LEFT) {
			brush.mode = DRAW;
			set_buffer_to_cell(&buffer, c);
			mod_buffer(&buffer, c->x, c->y, 1);
			break;
		}
		if(event->button.button == SDL_BUTTON_RIGHT) {
			brush.mode = DELETE;
			mod_buffer(&buffer, c->x, c->y, 0);
			clear_cell(c);
			break;
		}
		if(event->button.button == SDL_BUTTON_MIDDLE) {
			brush.mode = SELECT;
			set_cell_to_buffer(c, &buffer);
			break;
		}
		break;
	case SDL_MOUSEMOTION:
		if(brush.down && brush.mode == DRAW) {
			set_buffer_to_cell(&buffer, c);
			mod_buffer(&buffer, c->x, c->y, 1);
		}
		if(brush.down && brush.mode == DELETE) {
			clear_cell(c);
			mod_buffer(&buffer, c->x, c->y, 0);
		}
		break;
	case SDL_MOUSEBUTTONUP:
		brush.down = 0;
		break;
	}
}

void
handle_keyboard(SDL_Event *event)
{
	switch(event->key.keysym.sym) {
		brush.mode = WRITE;
	case SDLK_SPACE:
		break;
	}
}

void
putpixel(Uint32 *dst, int x, int y, int color)
{
	if(x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
		dst[WIDTH * y + x] = palette[color];
	}
}

void
draw_background(Uint32 *dst, int color)
{
	int i, j;
	for(i = 0; i < WIDTH; i++)
		for(j = 0; j < HEIGHT; j++)
			putpixel(dst, i, j, color);
}

void
draw_line(Uint32 *dst, int x1, int y1, int x2, int y2, int color)
{
	int i;
	float x = x1;
	float y = y1;
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int steps;
	if(dx > dy) {
		steps = dx;
	} else {
		steps = dy;
	}
	float xinc = dx / (float)steps;
	float yinc = dy / (float)steps;

	for(i = 0; i < steps; i++) {
		x = x + xinc;
		y = y + yinc;
		putpixel(dst, roundf(x), roundf(y), color);
	}
}

void
draw_rect(Uint32 *dst, int x, int y, int w, int h, int color)
{
	if(x >= 0 && x < WIDTH &&
		y >= 0 && y < HEIGHT &&
		(x + w) >= 0 && (x + w) < WIDTH &&
		(y + h) >= 0 && (y + h) < HEIGHT) {
		int i;
		for(i = 0; i < w; i++) {
			putpixel(pixels, x + i, y, color);
			putpixel(pixels, x + i, y + h - 1, color);
		}
		for(i = 1; i < h - 1; i++) {
			putpixel(pixels, x, y + i, color);
			putpixel(pixels, x + w - 1, y + i, color);
		}
	}
}

void
draw_filled_rect(Uint32 *dst, int x, int y, int w, int h, int color)
{
	if(x >= 0 && x < WIDTH &&
		y >= 0 && y < HEIGHT &&
		(x + w) >= 0 && (x + w) < WIDTH &&
		(y + h) >= 0 && (y + h) < HEIGHT) {
		int i, j;

		for(i = 0; i < w; i++) {
			for(j = 0; j < h; j++) {
				putpixel(pixels, x + i, y + j, color);
			}
		}
	}
}

void
draw_filled_cell(Uint32 *dst, int x, int y, int color)
{
	draw_filled_rect(pixels, x * CELL, y * CELL, CELL, CELL, color);
}

void
draw_grid(void)
{
	int i, j;
	for(i = 0; i < HOR; i++)
		draw_line(pixels, CELL * i, 0, CELL * i, HEIGHT, FG);
	for(j = 0; j < VER; j++)
		draw_line(pixels, 0, CELL * j, WIDTH, CELL * j, FG);
}

void
draw_buffer(Buffer *b)
{
	int i, j;
	draw_rect(pixels, b->x * CELL - 1, b->y * CELL - 1, b->size * CELL + 2, b->size * CELL + 2, FG);
	for(i = 0; i < b->size; i++)
		for(j = 0; j < b->size; j++) {
			if(b->state[i][j] == 0)
				draw_filled_cell(pixels, i + b->x, j + b->y, BG);
			if(b->state[i][j] == 1)
				draw_filled_cell(pixels, i + b->x, j + b->y, FG);
		}
}

void
draw_cell(Cell *c)
{
	int i, j;
	for(i = 0; i < CELL; i++) {
		for(j = 0; j < CELL; j++) {
			if(c->state[i][j] == 1) {
				putpixel(pixels, c->x * CELL + i, c->y * CELL + j, FG);
			} else {
				putpixel(pixels, c->x * CELL + i, c->y * CELL + j, BG);
			}
		}
	}
}

void
draw_cells(void)
{
	int i, j;
	for(i = 0; i < HOR; i++) {
		for(j = 0; j < VER; j++) {
			draw_cell(&cells[i][j]);
		}
	}
}

void
draw(Uint32 *dst)
{
	draw_background(dst, 0);
	draw_grid();
	draw_cells();
	draw_buffer(&buffer);
	SDL_UpdateTexture(gTexture, NULL, dst, WIDTH * sizeof(Uint32));
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
	SDL_RenderPresent(gRenderer);
}

int
error(char *msg, const char *err)
{
	printf("Error %s : %s\n", msg, err);
	return 0;
}

void
quit(void)
{
	free(pixels);
	SDL_DestroyTexture(gTexture);
	gTexture = NULL;
	SDL_DestroyRenderer(gRenderer);
	gRenderer = NULL;
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	SDL_Quit();
	exit(0);
}

int
init(void)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return error("Init", SDL_GetError());
	gWindow = SDL_CreateWindow("Viewpoint",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH * 2,
		HEIGHT * 2,
		SDL_WINDOW_SHOWN);
	if(gWindow == NULL)
		return error("Window", SDL_GetError());
	gRenderer = SDL_CreateRenderer(gWindow, -1, 0);
	if(gRenderer == NULL)
		return error("Renderer", SDL_GetError());
	gTexture = SDL_CreateTexture(gRenderer,
		SDL_PIXELFORMAT_RGB888,
		SDL_TEXTUREACCESS_STATIC,
		WIDTH,
		HEIGHT);
	if(gTexture == NULL)
		return error("Texture", SDL_GetError());

	pixels = (Uint32 *)malloc(WIDTH * HEIGHT * sizeof(Uint32));
	init_buffer(&buffer);
	init_cells();

	return 0;
}

int
main(int argc, char **argv)
{
	int running = 1;

	init();

	while(running) {
		SDL_Event event;
		draw(pixels);
		while(SDL_PollEvent(&event) != 0) {
			if(event.type == SDL_QUIT)
				quit();
			else if(event.type == SDL_MOUSEBUTTONUP ||
					event.type == SDL_MOUSEBUTTONDOWN ||
					event.type == SDL_MOUSEMOTION) {
				handle_mouse(&event);
			}

			draw(pixels);
		}
	}

	quit();

	return 0;
}
