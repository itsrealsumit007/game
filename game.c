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
const int POWERUP_SIZE = 20;
const int POWERUP_SPEED = 3;

typedef enum {
    EASY,
    MEDIUM,
    HARD
} Difficulty;

typedef enum {
    NONE,
    INCREASE_PADDLE_SIZE,
    DECREASE_PADDLE_SIZE,
    INCREASE_BALL_SPEED,
    DECREASE_BALL_SPEED,
    EXTRA_LIFE
} PowerUpType;

typedef struct {
    int x, y;
    int w, h;
    int dy;
    SDL_Texture* texture;
} Paddle;

typedef struct {
    int x, y;
    int w, h;
    int dx, dy;
    SDL_Texture* texture;
} Ball;

typedef struct {
    int x, y;
    int dx, dy;
    PowerUpType type;
    SDL_Texture* texture;
} PowerUp;

Mix_Chunk* paddleSound = NULL;
Mix_Chunk* scoreSound = NULL;
Mix_Chunk* powerUpSound = NULL;
SDL_Texture* pauseTexture = NULL;

void initializeSound() {
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

    powerUpSound = Mix_LoadWAV("powerup.wav");
    if (powerUpSound == NULL) {
        printf("Failed to load powerup sound effect! SDL_mixer Error: %s\n", Mix_GetError());
        exit(1);
    }
}

void closeSound() {
    Mix_FreeChunk(paddleSound);
    Mix_FreeChunk(scoreSound);
    Mix_FreeChunk(powerUpSound);
    paddleSound = NULL;
    scoreSound = NULL;
    powerUpSound = NULL;
    Mix_Quit();
}

void playPaddleSound() {
    Mix_PlayChannel(-1, paddleSound, 0);
}

void playScoreSound() {
    Mix_PlayChannel(-1, scoreSound, 0);
}

void playPowerUpSound() {
    Mix_PlayChannel(-1, powerUpSound, 0);
}

SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    if (surface == NULL) {
        printf("Unable to load image %s! SDL_Error: %s\n", path, SDL_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void drawPaddle(SDL_Renderer* renderer, Paddle* paddle) {
    SDL_Rect rect = { paddle->x, paddle->y, paddle->w, paddle->h };
    SDL_RenderCopy(renderer, paddle->texture, NULL, &rect);
}

void drawBall(SDL_Renderer* renderer, Ball* ball) {
    SDL_Rect rect = { ball->x, ball->y, ball->w, ball->h };
    SDL_RenderCopy(renderer, ball->texture, NULL, &rect);
}

void drawPowerUp(SDL_Renderer* renderer, PowerUp* powerUp) {
    SDL_Rect rect = { powerUp->x, powerUp->y, POWERUP_SIZE, POWERUP_SIZE };
    SDL_RenderCopy(renderer, powerUp->texture, NULL, &rect);
}

void drawText(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y, int w, int h) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = { x, y, w, h };
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);
}

void drawScore(SDL_Renderer* renderer, TTF_Font* font, int leftScore, int rightScore) {
    char scoreText[12];
    sprintf(scoreText, "%d - %d", leftScore, rightScore);
    SDL_Color textColor = { 255, 255, 255, 255 };
    drawText(renderer, font, scoreText, textColor, SCREEN_WIDTH / 2 - 50, 50, 100, 50);
}

void drawGameOver(SDL_Renderer* renderer, TTF_Font* font, const char* winnerText) {
    SDL_Color textColor = { 255, 0, 0, 255 };
    drawText(renderer, font, winnerText, textColor, SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 25, 300, 50);
}

void drawPause(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color textColor = { 255, 255, 255, 255 };
    drawText(renderer, font, "Game Paused", textColor, SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 25, 200, 50);
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

void moveBall(Ball* ball, Paddle* leftPaddle, Paddle* rightPaddle, int* leftScore, int* rightScore, PowerUp* powerUp, int* powerUpActive, int* powerUpTimer) {
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

        *powerUpActive = 0;
        powerUp->x = -POWERUP_SIZE;
        powerUp->y = -POWERUP_SIZE;
        *powerUpTimer = 0;
    }

    if (ball->x > SCREEN_WIDTH) {
        (*leftScore)++;
        ball->x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
        ball->y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
        ball->dx = -BALL_SPEED;
        ball->dy = BALL_SPEED;
        playScoreSound();

        *powerUpActive = 0;
        powerUp->x = -POWERUP_SIZE;
        powerUp->y = -POWERUP_SIZE;
        *powerUpTimer = 0;
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

void spawnPowerUp(PowerUp* powerUp, int* powerUpActive) {
    if (*powerUpActive == 0) {
        powerUp->x = rand() % (SCREEN_WIDTH - POWERUP_SIZE);
        powerUp->y = rand() % (SCREEN_HEIGHT - POWERUP_SIZE);
        powerUp->dx = 0;
        powerUp->dy = POWERUP_SPEED;
        powerUp->type = (PowerUpType)(rand() % 6); // Randomly select a power-up type
        *powerUpActive = 1;
    }
}

void movePowerUp(PowerUp* powerUp, int* powerUpActive) {
    if (*powerUpActive) {
        powerUp->y += powerUp->dy;
        if (powerUp->y > SCREEN_HEIGHT) {
            *powerUpActive = 0;
            powerUp->x = -POWERUP_SIZE;
            powerUp->y = -POWERUP_SIZE;
        }
    }
}

void applyPowerUp(Paddle* leftPaddle, Paddle* rightPaddle, Ball* ball, int* leftScore, int* rightScore, PowerUp* powerUp, int* powerUpActive, int* powerUpTimer) {
    if (*powerUpActive && ball->x + ball->w >= powerUp->x && ball->x <= powerUp->x + POWERUP_SIZE &&
        ball->y + ball->h >= powerUp->y && ball->y <= powerUp->y + POWERUP_SIZE) {
        switch (powerUp->type) {
            case INCREASE_PADDLE_SIZE:
                leftPaddle->h += 20;
                rightPaddle->h += 20;
                break;
            case DECREASE_PADDLE_SIZE:
                leftPaddle->h -= 20;
                rightPaddle->h -= 20;
                break;
            case INCREASE_BALL_SPEED:
                ball->dx *= 2;
                ball->dy *= 2;
                break;
            case DECREASE_BALL_SPEED:
                ball->dx /= 2;
                ball->dy /= 2;
                break;
            case EXTRA_LIFE:
                if (*leftScore > 0) {
                    (*leftScore)--;
                } else if (*rightScore > 0) {
                    (*rightScore)--;
                }
                break;
            default:
                break;
        }
        playPowerUpSound();

        *powerUpActive = 0;
        powerUp->x = -POWERUP_SIZE;
        powerUp->y = -POWERUP_SIZE;
        *powerUpTimer = 0;
    }
}

int main() {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    TTF_Font* font = NULL;
    int gameRunning = 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Pong Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        return 1;
    }

    font = TTF_OpenFont("arial.ttf", 28);
    if (font == NULL) {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return 1;
    }

    initializeSound();



        if (!gameOver && !paused) {
            movePaddle(&leftPaddle);
            moveAIPaddle(&rightPaddle, &ball, aiDifficulty);
            moveBall(&ball, &leftPaddle, &rightPaddle, &leftScore, &rightScore, &powerUp, &powerUpActive, &powerUpTimer);
            if (rand() % 300 == 0) {
                spawnPowerUp(&powerUp, &powerUpActive);
            }
            movePowerUp(&powerUp, &powerUpActive);
            applyPowerUp(&leftPaddle, &rightPaddle, &ball, &leftScore, &rightScore, &powerUp, &powerUpActive, &powerUpTimer);
            powerUpTimer++;
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        drawPaddle(renderer, &leftPaddle);
        drawPaddle(renderer, &rightPaddle);
        drawBall(renderer, &ball);
        drawScore(renderer, font, leftScore, rightScore);

        if (gameOver) {
            if (leftScore >= WINNING_SCORE) {
                drawGameOver(renderer, font, "Left Player Wins! Press 'R' to Restart");
            } else if (rightScore >= WINNING_SCORE) {
                drawGameOver(renderer, font, "Right Player Wins! Press 'R' to Restart");
            }
        }

        if (paused && !gameOver) {
            drawPause(renderer, font);
        }

        if (powerUpActive) {
            drawPowerUp(renderer, &powerUp);
        }

        SDL_RenderPresent(renderer);
    }

    closeSound();

    TTF_CloseFont(font);
    SDL_DestroyTexture(paddleTexture);
    SDL_DestroyTexture(ballTexture);
    SDL_DestroyTexture(pauseTexture);
    for (int i = 0; i < 6; ++i) {
        SDL_DestroyTexture(powerUpTextures[i]);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
    return 0;
}
