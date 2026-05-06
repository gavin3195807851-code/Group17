#include "Game_2.h"
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "Buzzer.h"
#include "Joystick.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>

extern ST7789V2_cfg_t cfg0;
extern Buzzer_cfg_t buzzer_cfg;
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;

#define GAME2_FRAME_TIME_MS 50
#define MOVE_DELAY_MS 180

#define LANE_LEFT_X   45
#define LANE_MID_X    105
#define LANE_RIGHT_X  165

#define PLAYER_Y      165
#define MONSTER_Y     200

#define START_Y       70
#define END_Y         165

#define WHITE 1
#define RED   2
#define GREEN 3
#define BROWN 12
#define GOLD  10

static uint32_t frame_counter = 0;
static uint32_t score = 0;
static uint32_t last_move_tick = 0;

static int player_lane = 1;

static int obstacle_lane = 0;
static int obstacle_y = START_Y;

static int coin_lane = 2;
static int coin_y = START_Y;

static int obstacle_speed = 3;
static int coin_score = 10;
static int stage = 1;
static int coins_collected = 0;
static int speed_message_timer = 0;

static int game_over = 0;

static const uint32_t player_bitmap[32] = {
    0x00000000,0x00000000,0x001E0000,0x003F0000,
    0x007F8000,0x003F0000,0x001E0000,0x003F0000,
    0x007F8000,0x00FFF000,0x00FFF000,0x007F8000,
    0x003F0000,0x003F0000,0x003F0000,0x003F0000,
    0x003F0000,0x003F0000,0x001E0000,0x001E0000,
    0x00330000,0x00618000,0x00618000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000
};


static const uint32_t crate_bitmap[32] = {
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x007FF000,0x007FF000,0x00666000,0x00666000,
    0x007FF000,0x007FF000,0x00666000,0x00666000,
    0x007FF000,0x007FF000,0x00666000,0x00666000,
    0x007FF000,0x007FF000,0x00666000,0x00666000,
    0x007FF000,0x007FF000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000
};


static const uint32_t monster_bitmap[2][32] = {
{
    0x00000000,0x00000000,0x00200800,0x00301800,
    0x003FF800,0x003FF800,0x003FF800,0x00FFFE00,
    0x00E7CE00,0x00FFFE00,0x003FF800,0x003FF800,
    0x001EF000,0x003EF800,0x003EF800,0x001CC000,
    0x00366000,0x00633000,0x00633000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000
},
{
    0x00000000,0x00000000,0x00000000,0x00200800,
    0x00301800,0x003FF800,0x003FF800,0x00FFFE00,
    0x00E7CE00,0x00FFFE00,0x003FF800,0x003FF800,
    0x001EF000,0x003EF800,0x003EF800,0x00633000,
    0x00366000,0x001CC000,0x001CC000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000
}
};


static const uint16_t coin_bitmap[16] = {
    0b0000000000000000,
    0b0000000000000000,
    0b0000001110000000,
    0b0000011111000000,
    0b0000111111100000,
    0b0001111011110000,
    0b0001110101110000,
    0b0011010101011000,
    0b0011010101011000,
    0b0011010101011000,
    0b0001110101110000,
    0b0001111011110000,
    0b0000111111100000,
    0b0000011111000000,
    0b0000001110000000,
    0b0000000000000000
};

static int lane_to_x(int lane)
{
    if (lane == 0) return LANE_LEFT_X;
    if (lane == 1) return LANE_MID_X;
    return LANE_RIGHT_X;
}

static void draw_bitmap_32x32(const uint32_t bitmap[32], int x, int y, uint8_t colour)
{
    for (int row = 0; row < 32; row++)
    {
        for (int col = 0; col < 32; col++)
        {
            if (bitmap[row] & (1UL << (31 - col)))
            {
                if (x + col >= 0 && x + col < 240 && y + row >= 0 && y + row < 240)
                {
                    LCD_Set_Pixel(x + col, y + row, colour);
                }
            }
        }
    }
}

static void draw_bitmap_16x16(const uint16_t bitmap[16], int x, int y, uint8_t colour)
{
    for (int row = 0; row < 16; row++)
    {
        for (int col = 0; col < 16; col++)
        {
            if (bitmap[row] & (1U << (15 - col)))
            {
                if (x + col >= 0 && x + col < 240 && y + row >= 0 && y + row < 240)
                {
                    LCD_Set_Pixel(x + col, y + row, colour);
                }
            }
        }
    }
}

static void game_reset(void)
{
    frame_counter = 0;
    score = 0;
    last_move_tick = HAL_GetTick();

    player_lane = 1;

    obstacle_lane = 0;
    obstacle_y = START_Y;

    coin_lane = 2;
    coin_y = START_Y;

    obstacle_speed = 3;
    coin_score = 10;
    stage = 1;
    coins_collected = 0;
    speed_message_timer = 0;

    game_over = 0;
}

static void draw_road(void)
{
    LCD_printString("|", 75, 70, WHITE, 2);
    LCD_printString("|", 75, 100, WHITE, 2);
    LCD_printString("|", 75, 130, WHITE, 2);
    LCD_printString("|", 75, 160, WHITE, 2);

    LCD_printString("|", 135, 70, WHITE, 2);
    LCD_printString("|", 135, 100, WHITE, 2);
    LCD_printString("|", 135, 130, WHITE, 2);
    LCD_printString("|", 135, 160, WHITE, 2);
}

static void draw_score(void)
{
    char text[32];

    sprintf(text, "Score:%lu", score);
    LCD_printString(text, 10, 8, WHITE, 2);

    sprintf(text, "Stage:%d", stage);
    LCD_printString(text, 10, 28, WHITE, 1);

    sprintf(text, "Coin:%d", coin_score);
    LCD_printString(text, 80, 28, GOLD, 1);

    sprintf(text, "%d/20", coins_collected);
    LCD_printString(text, 155, 28, WHITE, 1);

    if (speed_message_timer > 0)
    {
        LCD_printString("SPEED UP!", 55, 65, RED, 2);
        speed_message_timer--;
    }
}

static void draw_player(void)
{
    int x = lane_to_x(player_lane);
    draw_bitmap_32x32(player_bitmap, x - 16, PLAYER_Y - 16, GREEN);
}

static void draw_obstacle(void)
{
    int x = lane_to_x(obstacle_lane);
    draw_bitmap_32x32(crate_bitmap, x - 16, obstacle_y - 16, BROWN);
}

static void draw_coin(void)
{
    int x = lane_to_x(coin_lane);
    draw_bitmap_16x16(coin_bitmap, x - 8, coin_y - 8, GOLD);
}

static void draw_monster(void)
{
    int x = lane_to_x(player_lane);
    int frame = (frame_counter / 6) % 2;

    draw_bitmap_32x32(monster_bitmap[frame], x - 16, MONSTER_Y - 16, RED);
}

static void draw_game(void)
{
    LCD_clear();

    draw_score();

    LCD_printString("TEMPLE DASH", 45, 45, WHITE, 2);

    draw_road();
    draw_coin();
    draw_obstacle();
    draw_player();
    draw_monster();

    LCD_printString("BT6 Menu", 55, 225, WHITE, 1);

    LCD_Refresh(&cfg0);
}

static void draw_game_over(void)
{
    char text[32];

    LCD_clear();

    LCD_printString("GAME", 20, 40, RED, 5);
    LCD_printString("OVER", 20, 100, RED, 5);

    sprintf(text, "Score:%lu", score);
    LCD_printString(text, 50, 170, WHITE, 2);

    sprintf(text, "Stage:%d", stage);
    LCD_printString(text, 60, 195, WHITE, 2);

    LCD_printString("BT7 Restart", 30, 220, WHITE, 1);
    LCD_printString("BT6 Menu", 130, 220, WHITE, 1);

    LCD_Refresh(&cfg0);
}

static void update_input(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - last_move_tick) >= MOVE_DELAY_MS && joystick_data.direction != CENTRE)
    {
        if (joystick_data.direction == W ||
            joystick_data.direction == NW ||
            joystick_data.direction == SW)
        {
            if (player_lane > 0)
            {
                player_lane--;

                buzzer_tone(&buzzer_cfg, 1000, 20);
                HAL_Delay(30);
                buzzer_off(&buzzer_cfg);

                last_move_tick = now;
            }
        }
        else if (joystick_data.direction == E ||
                 joystick_data.direction == NE ||
                 joystick_data.direction == SE)
        {
            if (player_lane < 2)
            {
                player_lane++;

                buzzer_tone(&buzzer_cfg, 1000, 20);
                HAL_Delay(30);
                buzzer_off(&buzzer_cfg);

                last_move_tick = now;
            }
        }
    }
}

static void next_stage(void)
{
    stage++;
    coins_collected = 0;

    coin_score += 5;

    if (obstacle_speed < 10)
    {
        obstacle_speed++;
    }

    speed_message_timer = 40;

    buzzer_tone(&buzzer_cfg, 1800, 30);
    HAL_Delay(80);
    buzzer_off(&buzzer_cfg);
}

static void update_coin(void)
{
    coin_y += obstacle_speed;

    if (coin_y >= END_Y)
    {
        if (coin_lane == player_lane)
        {
            score += coin_score;
            coins_collected++;

            buzzer_tone(&buzzer_cfg, 1500, 20);
            HAL_Delay(20);
            buzzer_off(&buzzer_cfg);

            if (coins_collected >= 20)
            {
                next_stage();
            }
        }

        coin_y = START_Y;

        coin_lane++;

        if (coin_lane > 2)
        {
            coin_lane = 0;
        }
    }
}

static void update_obstacle(void)
{
    obstacle_y += obstacle_speed;

    if (obstacle_y >= END_Y)
    {
        if (obstacle_lane == player_lane)
        {
            game_over = 1;

            buzzer_tone(&buzzer_cfg, 300, 50);
            HAL_Delay(100);
            buzzer_off(&buzzer_cfg);

            return;
        }

        obstacle_y = START_Y;

        obstacle_lane++;

        if (obstacle_lane > 2)
        {
            obstacle_lane = 0;
        }
    }
}

MenuState Game2_Run(void)
{
    game_reset();

    buzzer_tone(&buzzer_cfg, 1200, 30);
    HAL_Delay(50);
    buzzer_off(&buzzer_cfg);

    MenuState exit_state = MENU_STATE_HOME;

    while (1)
    {
        uint32_t frame_start = HAL_GetTick();

        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);

        if (current_input.btn6_pressed)
        {
            exit_state = MENU_STATE_HOME;
            break;
        }

        if (!game_over)
        {
            update_input();
            update_coin();
            update_obstacle();

            frame_counter++;

            draw_game();
        }
        else
        {
            draw_game_over();

            if (current_input.btn7_pressed)
            {
                game_reset();
            }
        }

        uint32_t frame_time = HAL_GetTick() - frame_start;

        if (frame_time < GAME2_FRAME_TIME_MS)
        {
            HAL_Delay(GAME2_FRAME_TIME_MS - frame_time);
        }
    }

    return exit_state;
}
