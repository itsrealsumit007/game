#include <SDL2/SDL.h>
#include <stdio.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PADDLE_WIDTH = 20;
const int PADDLE_HEIGHT = 100;
const int PADDLE_SPEED = 10;
const int BALL_SIZE = 20;
const int BALL_SPEED = 5;

typedef struct {
    int x, y;
    int w, h;
    int dy;
} Paddle;

typedef struct {
    int x, y;
    int w, h;
    int dx, dy;
} Ball;

void drawPaddle(SDL_Renderer* renderer, Paddle* paddle) {
    SDL_Rect rect = { paddle->x, paddle->y, paddle->w, paddle->h };
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(renderer, &rect);
}

void drawBall(SDL_Renderer* renderer, Ball* ball) {
    SDL_Rect rect = { ball->x, ball->y, ball->w, ball->h };
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(renderer, &rect);
}

void movePaddle(Paddle* paddle) {
    paddle->y += paddle->dy;
    if (paddle->y < 0) {
        paddle->y = 0;
    }
    if (paddle->y > SCREEN_HEIGHT - paddle->h) {
        paddle->y = SCREEN_HEIGHT - paddle->h;
    }
}

void moveBall(Ball* ball, Paddle* leftPaddle, Paddle* rightPaddle) {
    ball->x += ball->dx;
    ball->y += ball->dy;

    // Collision with paddles
    if (ball->x <= leftPaddle->x + leftPaddle->w &&
        ball->y + ball->h >= leftPaddle->y && ball->y <= leftPaddle->y + leftPaddle->h) {
        ball->dx = -ball->dx;
    }

    if (ball->x + ball->w >= rightPaddle->x &&
        ball->y + ball->h >= rightPaddle->y && ball->y <= rightPaddle->y + rightPaddle->h) {
        ball->dx = -ball->dx;
    }

    // Collision with screen edges
    if (ball->y <= 0 || ball->y >= SCREEN_HEIGHT - ball->h) {
        ball->dy = -ball->dy;
    }
}

int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Pong Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Paddle leftPaddle = { 50, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT, 0 };
    Paddle rightPaddle = { SCREEN_WIDTH - 50 - PADDLE_WIDTH, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT, 0 };
    Ball ball = { SCREEN_WIDTH / 2 - BALL_SIZE / 2, SCREEN_HEIGHT / 2 - BALL_SIZE / 2, BALL_SIZE, BALL_SIZE, BALL_SPEED, BALL_SPEED };

    int quit = 0;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_w:
                        leftPaddle.dy = -PADDLE_SPEED;
                        break;
                    case SDLK_s:
                        leftPaddle.dy = PADDLE_SPEED;
                        break;
                    case SDLK_UP:
                        rightPaddle.dy = -PADDLE_SPEED;
                        break;
                    case SDLK_DOWN:
                        rightPaddle.dy = PADDLE_SPEED;
                        break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_w:
                    case SDLK_s:
                        leftPaddle.dy = 0;
                        break;
                    case SDLK_UP:
                    case SDLK_DOWN:
                        rightPaddle.dy = 0;
                        break;
                }
            }
        }

        movePaddle(&leftPaddle);
        movePaddle(&rightPaddle);
        moveBall(&ball, &leftPaddle, &rightPaddle);

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        drawPaddle(renderer, &leftPaddle);
        drawPaddle(renderer, &rightPaddle);
        drawBall(renderer, &ball);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
