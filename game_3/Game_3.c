#include "Game_3.h"
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "PWM.h"
#include "Buzzer.h"
#include "Joystick.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>

extern ST7789V2_cfg_t cfg0;
extern PWM_cfg_t pwm_cfg;      // LED PWM control
extern Buzzer_cfg_t buzzer_cfg; // Buzzer control
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;
extern RNG_HandleTypeDef hrng;

/**
 * @brief Game 3 Implementation - Student can modify
 * 
 */

// Game state - customize for your game
static int16_t moving_x = 0;
static int16_t bullet_x = 0;
static int16_t bullet_y = 0;
static uint8_t bullet_active = 0;
#define ASTEROID_COUNT 3

static int16_t asteroid_x[ASTEROID_COUNT];
static int16_t asteroid_y[ASTEROID_COUNT];
static uint8_t asteroid_active[ASTEROID_COUNT];
static int16_t asteroid_speed = 2;
static int score = 0;
static int lives = 3;
static uint8_t game_over = 0;
static uint32_t random_number = 0;

// Frame rate for this game (in milliseconds)
#define GAME3_FRAME_TIME_MS 30 
#define GAME_OVER_BEEP_FREQ 300
#define GAME_OVER_BEEP_MS 250
#define GAME_OVER_BEEP_VOLUME 50

static uint32_t buzzer_stop_tick = 0;
static uint8_t game_over_sound_played = 0;

static void Respawn_Asteroid(uint8_t index);
static void Draw_Asteroid(int16_t x, int16_t y);
static void Game1_Beep(uint32_t freq_hz, uint32_t time_ms);
static void Game1_Update_Buzzer(void);

MenuState Game3_Run(void) {
    // Initialize game state
    moving_x = 0;
    bullet_x = 0;
    bullet_y = 0;
    bullet_active = 0;
    asteroid_speed = 2;
    score = 0;
    lives = 3;
    game_over = 0;
    buzzer_stop_tick = 0;
    game_over_sound_played = 0;
    buzzer_off(&buzzer_cfg);

    for (uint8_t i = 0; i < ASTEROID_COUNT; i++) {
        Respawn_Asteroid(i);
        asteroid_y[i] = 30 - (i * 50);
    }
    
    // Play a brief startup sound
    buzzer_tone(&buzzer_cfg, 1000, 30); 
    HAL_Delay(50);  
    buzzer_off(&buzzer_cfg); 
    
    MenuState exit_state = MENU_STATE_HOME; 
    
    // Game's own loop - runs until exit condition
    while (1) {
        uint32_t frame_start = HAL_GetTick();
        
        // input read
        Input_Read();
        Game1_Update_Buzzer();
        
        // check whether exit
        if (current_input.btn6_pressed) {
            exit_state = MENU_STATE_HOME;
            buzzer_off(&buzzer_cfg);
            buzzer_stop_tick = 0;
            break;  
        }

        if (game_over == 1) {
            LCD_Fill_Buffer(0);

            LCD_printString("GAME OVER", 40, 85, 1, 3);

            char final_score_text[25];
            snprintf(final_score_text, sizeof(final_score_text), "Score: %d", score);
            LCD_printString(final_score_text, 65, 125, 1, 2);

            LCD_printString("BT6: Menu", 55, 170, 1, 2);

            LCD_Refresh(&cfg0);
            continue;
        }
        
        // update game
        // Joystick control: move spaceship left and right
        Joystick_Read(&joystick_cfg, &joystick_data);

        if (joystick_data.coord.x > 0.3f) {
            moving_x += 3;
        }
        else if (joystick_data.coord.x < -0.3f) {
            moving_x -= 3;
        }

        // Keep spaceship inside the screen
        if (moving_x < 0) {
            moving_x = 0;
        }

        if (moving_x > 180) {
            moving_x = 180;
        }

        // Shoot bullet with BT2
        if (current_input.btn2_pressed && bullet_active == 0) {
            bullet_x = 20 + moving_x + 15;
            bullet_y = 170;
            bullet_active = 1;
        }

        // move bullet
        if (bullet_active == 1) {
            bullet_y -= 16;

            if (bullet_y < 20) {
                bullet_active = 0;
            }
        }

        // move asteroid
        for (uint8_t i = 0; i < ASTEROID_COUNT; i++) {
            if (asteroid_active[i] == 1) {
                asteroid_y[i] += asteroid_speed;

                if (asteroid_y[i] > 210) {
                    Respawn_Asteroid(i);
                }
            }
        }

        // check collision
        for (uint8_t i = 0; i < ASTEROID_COUNT; i++) {
            if (bullet_active == 1 && asteroid_active[i] == 1) {
                if ((bullet_x - 2 < asteroid_x[i] + 9) &&
                    (bullet_x + 2 > asteroid_x[i] - 9) &&
                    (bullet_y - 2 < asteroid_y[i] + 9) &&
                    (bullet_y + 2 > asteroid_y[i] - 9)) {

                    bullet_active = 0;
                    score++;
                    Respawn_Asteroid(i);
                }
            }
        }

        for (uint8_t i = 0; i < ASTEROID_COUNT; i++) {
            if (asteroid_active[i] == 1 && lives > 0) {
                if ((asteroid_x[i] - 9 < 20 + moving_x + 40) &&
                    (asteroid_x[i] + 9 > 20 + moving_x) &&
                    (asteroid_y[i] - 9 < 175 + 32) &&
                    (asteroid_y[i] + 9 > 175)) {

                    lives--;

                    if (lives <= 0) {
                        game_over = 1;
                    }

                    if (game_over == 1 && game_over_sound_played == 0) {
                        Game1_Beep(GAME_OVER_BEEP_FREQ, GAME_OVER_BEEP_MS);
                        game_over_sound_played = 1;
                    }

                    Respawn_Asteroid(i);
                }
            }
        }

        // render
        LCD_Fill_Buffer(0);
        
        // Title
        LCD_printString("SPACE DODGE", 25, 10, 1, 2);
        char score_text[20];
        snprintf(score_text, sizeof(score_text), "Score: %d", score);
        LCD_printString(score_text, 5, 35, 1, 1);
        char lives_text[20];
        snprintf(lives_text, sizeof(lives_text), "Lives: %d", lives);
        LCD_printString(lives_text, 5, 50, 1, 1);
        
        // Simple animated spaceship
        LCD_printString("  /\\  ", 20 + moving_x, 175, 1, 1);
        LCD_printString(" /##\\ ", 20 + moving_x, 183, 1, 1);
        LCD_printString("<-##->", 20 + moving_x, 191, 1, 1);
        LCD_printString("  ||  ", 20 + moving_x, 199, 1, 1);

        for (uint8_t i = 0; i < ASTEROID_COUNT; i++) {
            if (asteroid_active[i] == 1) {
                Draw_Asteroid(asteroid_x[i], asteroid_y[i]);
            }
        }

        // Draw bullet
        if (bullet_active == 1) {
            LCD_Draw_Circle(bullet_x, bullet_y, 2, 1, 1);
        }

        LCD_Refresh(&cfg0);
        
        // Frame timing - wait for remainder of frame time
        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < GAME3_FRAME_TIME_MS) {
            HAL_Delay(GAME3_FRAME_TIME_MS - frame_time);
        }
    }
    
    return exit_state;  
}

static void Respawn_Asteroid(uint8_t index)
{
    HAL_RNG_GenerateRandomNumber(&hrng, &random_number);

    asteroid_x[index] = 30 + (random_number % 170);
    asteroid_y[index] = 30;
    asteroid_active[index] = 1;
}

static void Draw_Asteroid(int16_t x, int16_t y)
{
    LCD_Draw_Circle(x, y, 6, 1, 1);
    LCD_Draw_Circle(x + 5, y - 2, 3, 1, 1);
    LCD_Draw_Circle(x - 4, y + 3, 3, 1, 1);
    LCD_Draw_Rect(x - 3, y - 5, 6, 3, 1, 1);
}

static void Game1_Beep(uint32_t freq_hz, uint32_t time_ms)
{
    buzzer_tone(&buzzer_cfg, freq_hz, GAME_OVER_BEEP_VOLUME);
    buzzer_stop_tick = HAL_GetTick() + time_ms;
}

static void Game1_Update_Buzzer(void)
{
    if (buzzer_stop_tick != 0) {
        if ((int32_t)(HAL_GetTick() - buzzer_stop_tick) >= 0) {
            buzzer_off(&buzzer_cfg);
            buzzer_stop_tick = 0;
        }
    }
}
