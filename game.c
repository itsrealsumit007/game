#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PADDLE_WIDTH = 20;
const int PADDLE_HEIGHT = 100;
const int PADDLE_SPEED = 10;
const int BALL_SIZE = 20;
const int BALL_SPEED = 5;
const int WINNING_SCORE = 5;

typedef enum {
    EASY,
    MEDIUM,
    HARD
} Difficulty;

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


Mix_Chunk* paddleSound = NULL;
Mix_Chunk* scoreSound = NULL;

void initializeSound() {
    // Initialize SDL Mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        exit(1);
    }


    paddleSound = Mix_LoadWAV("paddle_hit.wav");
    if (paddleSound == NULL) {
        printf("Failed to load paddle hit sound effect! SDL_mixer Error: %s\n", Mix_GetError());
        exit(1);
    }

    scoreSound = Mix_LoadWAV("score_update.wav");
    if (scoreSound == NULL) {
        printf("Failed to load score update sound effect! SDL_mixer Error: %s\n", Mix_GetError());
        exit(1);
    }
}

void closeSound() {

    Mix_FreeChunk(paddleSound);
    Mix_FreeChunk(scoreSound);
    paddleSound = NULL;
    scoreSound = NULL;


    Mix_Quit();
}

void playPaddleSound() {
    Mix_PlayChannel(-1, paddleSound, 0);
}

void playScoreSound() {
    Mix_PlayChannel(-1, scoreSound, 0);
}

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

void drawScore(SDL_Renderer* renderer, TTF_Font* font, int leftScore, int rightScore) {
    char scoreText[12];
    sprintf(scoreText, "%d - %d", leftScore, rightScore);
    SDL_Color textColor = { 255, 255, 255, 255 };
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText, textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = { SCREEN_WIDTH / 2 - 50, 50, 100, 50 };
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);
}

void drawGameOver(SDL_Renderer* renderer, TTF_Font* font, const char* winnerText) {
    SDL_Color textColor = { 255, 0, 0, 255 };
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, winnerText, textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = { SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 25, 300, 50 };
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);
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

void moveBall(Ball* ball, Paddle* leftPaddle, Paddle* rightPaddle, int* leftScore, int* rightScore) {
    ball->x += ball->dx;
    ball->y += ball->dy;

    if (ball->x <= leftPaddle->x + leftPaddle->w &&
        ball->y + ball->h >= leftPaddle->y && ball->y <= leftPaddle->y + leftPaddle->h) {
        ball->dx = -ball->dx;
        playPaddleSound();
    }

    if (ball->x + ball->w >= rightPaddle->x &&
        ball->y + ball->h >= rightPaddle->y && ball->y <= rightPaddle->y + rightPaddle->h) {
        ball->dx = -ball->dx;
        playPaddleSound();
    }

    if (ball->y <= 0 || ball->y >= SCREEN_HEIGHT - ball->h) {
        ball->dy = -ball->dy;
    }

    if (ball->x < 0) {
        (*rightScore)++;
        ball->x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
        ball->y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
        ball->dx = BALL_SPEED;
        ball->dy = BALL_SPEED;
        playScoreSound();
    }

    if (ball->x > SCREEN_WIDTH) {
        (*leftScore)++;
        ball->x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
        ball->y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
        ball->dx = -BALL_SPEED;
        ball->dy = BALL_SPEED;
        playScoreSound();
    }
}

void moveAIPaddle(Paddle* paddle, Ball* ball, Difficulty difficulty) {
    int aiSpeed = PADDLE_SPEED;
    switch (difficulty) {
        case EASY:
            aiSpeed = PADDLE_SPEED / 2;
            break;
        case MEDIUM:
            aiSpeed = PADDLE_SPEED;
            break;
        case HARD:
            aiSpeed = PADDLE_SPEED * 2;
            break;
    }

    if (ball->dy > 0) {
        if (paddle->y + paddle->h / 2 < ball->y + ball->h / 2) {
            paddle->dy = aiSpeed;
        } else if (paddle->y + paddle->h / 2 > ball->y + ball->h / 2) {
            paddle->dy = -aiSpeed;
        } else {
            paddle->dy = 0;
        }
    }
    movePaddle(paddle);
}

int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
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

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("path_to_font.ttf", 28);
    if (font == NULL) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    initializeSound();

    Paddle leftPaddle = { 50, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT, 0 };
    Paddle rightPaddle = { SCREEN_WIDTH - 50 - PADDLE_WIDTH, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT, 0 };
    Ball ball = { SCREEN_WIDTH / 2 - BALL_SIZE / 2, SCREEN_HEIGHT / 2 - BALL_SIZE / 2, BALL_SIZE, BALL_SIZE, BALL_SPEED, BALL_SPEED };

    int leftScore = 0;
    int rightScore = 0;
    int quit = 0;
    int gameOver = 0;
    SDL_Event e;
    srand(time(NULL)); 

    Difficulty aiDifficulty = MEDIUM; 

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
                    case SDLK_r:
                        if (gameOver) {
                            leftScore = 0;
                            rightScore = 0;
                            leftPaddle.y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
                            rightPaddle.y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
                            ball.x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
                            ball.y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
                            ball.dx = BALL_SPEED;
                            ball.dy = BALL_SPEED;
                            gameOver = 0;
                        }
                        break;
                    case SDLK_1:
                        aiDifficulty = EASY;
                        break;
                    case SDLK_2:
                        aiDifficulty = MEDIUM;
                        break;
                    case SDLK_3:
                        aiDifficulty = HARD;
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

        if (!gameOver) {
            movePaddle(&leftPaddle);
            moveAIPaddle(&rightPaddle, &ball, aiDifficulty);
            moveBall(&ball, &leftPaddle, &rightPaddle, &leftScore, &rightScore);
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        drawPaddle(renderer, &leftPaddle);
        drawPaddle(renderer, &rightPaddle);
        drawBall(renderer, &ball);
        drawScore(renderer, font, leftScore, rightScore);

        if (leftScore >= WINNING_SCORE) {
            drawGameOver(renderer, font, "Left Player Wins! Press 'R' to Restart");
            SDL_RenderPresent(renderer);
            gameOver = 1;
        } else if (rightScore >= WINNING_SCORE) {
            drawGameOver(renderer, font, "Right Player Wins! Press 'R' to Restart");
            SDL_RenderPresent(renderer);
            gameOver = 1;
        } else {
            SDL_RenderPresent(renderer);
        }
    }

    closeSound();

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
