#include "Game_1.h"
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "PWM.h"
#include "Buzzer.h"
#include "stm32l4xx_hal.h"
#include <complex.h>
#include <errno.h>
#include <stdio.h>
#include "Joystick.h"

extern ST7789V2_cfg_t cfg0;    // LCD CONFIGURATION 
extern PWM_cfg_t pwm_cfg;      // LED PWM control
extern PWM_cfg_t pwm_cfg_2;   // LED PWM 2 control
extern Buzzer_cfg_t buzzer_cfg; // Buzzer control
extern Joystick_cfg_t joystick_cfg; // configuation joystick
extern Joystick_t joystick_data; 

// Frame rate for this game (in milliseconds)
#define FPS 60
#define GAME1_FRAME_TIME_MS (1000 / FPS)
#define MOVE_DELAY_MS 200  // Milliseconds between movement updates
#define GAME_TIME_MS 60000  //duration of  game round
#define WATER_TO_GROWING_TIME_MS 2500 // The time form water state to growing state
#define GROWING_TO_RIPE_TIME_MS 2500 //The time form growing state to ripe state
#define SEEDED_TO_SAPLESS_TIME_MS 5000//The time form seed state to sapless
#define RED_LED_BLINK_TIME_MS 300 //blinking interval of the red LED
#define ERROR_MESSAGE_TIME_MS 3000// the time of error
#define LOADING_DELAY_MS 120//The animation delay
//two buzzer sound parameter settings
#define BUZZER_STARTUP_FREQ_HZ 1000 
#define BUZZER_STARTUP_DURATION_MS 30
#define BUZZER_STARTUP_DELAY_MS 50
#define BUZZER_MODE_FREQ_HZ 1000
#define BUZZER_MODE_DURATION_MS 50
#define BUZZER_MODE_DELAY_MS 50
#define BUZZER_ERROR_FREQ_HZ 700
#define BUZZER_ERROR_DURATION_MS 70
#define BUZZER_ERROR_DELAY_MS 40
//Game status variable
static int initial_selectedrow = 0;
static int initial_selectedcol = 0;
static int start_x = 60;
static int start_y = 60;
static int scale=4;
static int col = 3;
static int row = 3;
static int edge=40;
static int gap=10;
static int start_midllesoil_x=110;
static int start_midllesoil_y=110;
static int score=0;
static int best_score = 0;
volatile uint8_t game_over = 0;
static uint32_t last_move_tick = 0;
static const char *error_message = NULL;
static uint32_t error_stick = 0;
static uint32_t game_start_tick = 0;
static uint32_t game_time = GAME_TIME_MS;
static uint8_t check_screen_updata= 1;
static int last_time_left = -1;

// create a grass Sprites
const uint8_t GRASS[8][8] = {
    {255,255,255, 3,255,255,255,255},
    {255,255, 3, 3,255,255,255,255},
    {255, 3,255, 3, 3,255,255,255},
    {255,255, 3,255, 3,255,255,255},
    {255, 3, 3,255,255, 3,255,255},
    { 3,255, 3,255, 3,255, 3,255},
    {255, 3,255, 3,255, 3,255,255},
    {255,255,255,255,255,255,255,255}
};
// create a soil Sprites
const uint8_t Soil[10][10] = {
    {5,5,5,5,5,5,5,5,5,5},
    {5,5,5,5,5,5,5,5,5,5},
    {5,5,5,5,255,255,255,5,5,5},
    {5,5,5,255,5,5,255,5,5,5},
    {5,5,255,5,5,255,5,5,255,5},
    {5,255,5,5,255,5,5,255,5,5},
    {5,5,255,5,5,255,5,5,255,5},
    {5,255,5,255,5,5,255,5,5,5},
    {5,5,5,5,255,5,5,255,5,5},
    {5,5,255,5,5,5,255,5,5,5}
};
// create a soil_seeded Sprites
const uint8_t Soil_seeded[10][10] = {
    {5,5,5,5,5,5,5,5,5,5},
    {5,5,5,5,5,5,5,5,5,5},
    {5,5,5,5,3,3,3,5,5,5},
    {5,5,5,3,5,5,3,5,5,5},
    {5,5,3,5,5,3,5,5,3,5},
    {5,3,5,5,3,5,5,3,5,5},
    {5,5,3,5,5,3,5,5,3,5},
    {5,3,5,3,5,5,3,5,5,5},
    {5,5,5,5,3,5,5,3,5,5},
    {5,5,3,5,5,5,3,5,5,5}
};
// create a soil_waterd Sprites
const uint8_t Soil_waterd[10][10] = {
    {5,5,5,5,5,5,5,5,5,5},
    {5,5,5,5,5,5,5,5,5,5},
    {5,5,5,5,4,4,4,5,5,5},
    {5,5,5,4,5,5,4,5,5,5},
    {5,5,4,5,5,4,5,5,4,5},
    {5,4,5,5,4,5,5,4,5,5},
    {5,5,4,5,5,4,5,5,4,5},
    {5,4,5,4,5,5,4,5,5,5},
    {5,5,5,5,4,5,5,4,5,5},
    {5,5,4,5,5,5,4,5,5,5}
};
// create a soil_growing Sprites
const uint8_t Soil_growing[10][10] =  {

    {255,255,255,255,255,255,255,255,255,255},

    {255,255,255,255,  3,255,255,255,255,255},

    {255,255,255,  3,  3,  3,255,255,255,255},

    {255,255,255,255,  3,255,255,255,255,255},

    {255,255,255,255,  3,255,255,255,255,255},

    {255,255,255,  5,  5,  5,255,255,255,255},

    {255,255,  5,  5,  5,  5,  5,255,255,255},

    {255,255,  5,  5,  5,  5,  5,255,255,255},

    {255,255,  5,  5,  5,  5,  5,255,255,255},

    {255,255,255,255,255,255,255,255,255,255}

};
// create a soil_ripe Sprites
const uint8_t Soil_ripe[10][10] = {
    {255,255,255,255,  3,  3,255,255,255,255},
    {255,255,255,255,  3,  3,255,255,255,255},
    {255,255,255,  3,  3,  3,  3,255,255,255},
    {255,255,255,  3,  3,  3,  3,255,255,255},
    {255,  3,  3,  3,  3,  3,  3,  3,  3,255},
    {255,  3,  3,  3,  3,  3,  3,  3,  3,255},
    {255,255,255,255,  3,  3,255,255,255,255},
    {255,255,255,255,  3,  3,255,255,255,255},
    {255,255,255,255,  3,  3,255,255,255,255},
    {255,255,255,255,  3,  3,255,255,255,255}
};
// create a soil_sapless Sprites
const uint8_t Soil_sapless[10][10] = {
    {255,255,255,255,255,255,255,255,255,255},
    {255,255,255,  7,  7,  7,255,255,255,255},
    {255,255,255,  7,  7,  7,255,255,  7,255},
    {255,255,  7,  7,  7,  7,255,255,  7,255},
    {255,255,  7,255,255,255,  7,  7,  7,255},
    {255,255,  7,255,255,  7,  7,  7,  7,255},
    {255,255,  7,255,255,  7,  7,  7,  7,255},
    {255,255,  7,255,255,255,255,255,255,255},
    {255,255,  7,255,255,255,255,255,255,255},
    {255,255,255,255,255,255,255,255,255,255}
};
// Define the soil state
typedef enum{

    soil=0,
    seeded=1,
    waterd=2,
    growing=3,
    ripe=4,
    sapless=5
} soilstate;
// Define the player's action mode
typedef enum{
    seed=0,
    water=1,
    harvest=2,
    cut=3
} playermode;
//store every soil state in this array
static soilstate farm[3][3]={0};
//store every soil time in this array
static uint32_t farmtime[3][3] = {0};
static playermode defalutmode=seed;

// ===== FUNCTION PROTOTYPES =====
static void loading_animation(void);
static void new_game_startloading(void);
static void soil_generated(void);
static void showscore(void);
static void recover_selectblokcs(int r, int c);
static void Init_game_screen(void);
static void show_ripestate(void);
static void Init_game_value(void);
static void show_errormessage(const char *error);
static void handle_input_botton_mode(void);
static void handle_input_botton_action(void);
static void handle_input_joystick(void);
static void handle_input_data(void);
static void update_game(void);
static void render_game(void);
static void lcdprint(void);
static void buzzer_mode_sound(void);
static void buzzer_error_sound(void);
static void show_lefttime(void);
static int get_time_left_second(void);
static void red_led_on(void);
static void red_led_off(void);
static void red_led_blink(void);
MenuState Game1_Run(void) {
    // Optional: startup sound
    buzzer_tone(&buzzer_cfg, BUZZER_STARTUP_FREQ_HZ, BUZZER_STARTUP_DURATION_MS);
    HAL_Delay(BUZZER_STARTUP_DELAY_MS);
    buzzer_off(&buzzer_cfg);
//    /* Initialize peripherals */
PWM_Init(&pwm_cfg); 
PWM_SetFreq(&pwm_cfg, 1000);
PWM_Init(&pwm_cfg_2);
PWM_SetFreq(&pwm_cfg_2, 1000);
last_move_tick = HAL_GetTick();
Input_Init();
new_game_startloading(); 
printf("Game start running\r\n");


    MenuState exit_state = MENU_STATE_HOME;

    // Game loop
    while (1) {

        while(!game_over){
uint32_t frame_start = HAL_GetTick();

        // Step 1: Read input
        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);



        // Step 2: Check exit or rest game
        if (current_input.btn6_pressed) {
            check_screen_updata= 1;
            PWM_SetDuty(&pwm_cfg, 0);
            PWM_SetDuty(&pwm_cfg_2, 0);
            buzzer_off(&buzzer_cfg);
            printf("BTN6 back to the menu \r\n");
            exit_state = MENU_STATE_HOME;
            return exit_state;
       }
       if (current_input.btn7_pressed) {
        check_screen_updata= 1;
        Init_game_value();
        Init_game_screen();
        printf("BTN7 reset pressed\r\n");
        buzzer_mode_sound();
       
       }
   else{handle_input_data();
        // Step 3: UPDATE
        // update soilstate ,score,,red and green led
  
        update_game();

        int time_left_now = get_time_left_second();
        if (time_left_now != last_time_left) {
            last_time_left = time_left_now;
            check_screen_updata= 1;
        }

        if(error_message != NULL && HAL_GetTick() - error_stick >= ERROR_MESSAGE_TIME_MS) {
            error_message = NULL;
            check_screen_updata= 1;
        }

        // Step 4: RENDER
         // if  check_screen_updata = 1 means the screen need update
        if (check_screen_updata) {
            render_game();
            check_screen_updata = 0;
        }
        
    
    }


        // Frame timing
        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < GAME1_FRAME_TIME_MS) {
            HAL_Delay(GAME1_FRAME_TIME_MS - frame_time);
        
    }
}
//Updata the best score
        if (score > best_score) {best_score = score;
            printf(" The best score = %d\r\n", best_score);
            
        
        }
        LCD_Fill_Buffer(0);
        LCD_printString("GAME OVER", 20, 50, 2, 4);

        char user_score[20];
        char best_score_str[20];
        //Convert the score and best score to strings
        sprintf(user_score, "SCORE:%d", score);
        sprintf(best_score_str, "BEST:%d", best_score);
        LCD_printString(user_score, 50, 100, 3, 3);
        LCD_printString(best_score_str, 60, 135, 6, 2);
        LCD_printString("Press BTN7 to start again", 20, 170, 1, 1);
        LCD_printString("Press BTN6 back to MENU", 20, 195, 1, 1);
        LCD_Refresh(&cfg0);
//choose reset or back to main menu
        while(game_over){
            Input_Read();

            if (current_input.btn6_pressed) {
                PWM_SetDuty(&pwm_cfg, 0);
                check_screen_updata= 1;
                PWM_SetDuty(&pwm_cfg_2, 0);
                buzzer_off(&buzzer_cfg);
                exit_state = MENU_STATE_HOME;
                printf("Press BTN6 return to menu\r\n");
                return exit_state;
            }

            if (current_input.btn7_pressed) {
                check_screen_updata= 1;
                Init_game_value();
                Init_game_screen();
                buzzer_mode_sound();
                printf("Press BTN7 restart game\r\n");
                break;
            }
        }
}

    return exit_state;
}
// ===== UPDATE & RENDER FUNCTIONS =====
//crate a loading animation
static void loading_animation(void)
{
    char loadingstr[8];

    for (int loading = 0; loading <= 100; loading += 5) {
       
        LCD_Fill_Buffer(1);   
        LCD_printString("LOADING", 60, 40, 5, 3);
        LCD_Draw_Rect(20, 80, 200, 20, 0, 0);

        int progress_bar_width= loading * 2;  
        if (progress_bar_width > 0) {
            LCD_Draw_Rect(20, 80, progress_bar_width, 20, 6, 1);  
        }

        LCD_Draw_Sprite_Colour_Scaled(18,  102, 8, 8, (uint8_t*)GRASS, 8, 2);
        LCD_Draw_Sprite_Colour_Scaled(48,  104, 8, 8, (uint8_t*)GRASS, 5, 2);
        LCD_Draw_Sprite_Colour_Scaled(78,  101, 8, 8, (uint8_t*)GRASS, 4, 2);
        LCD_Draw_Sprite_Colour_Scaled(108, 103, 8, 8, (uint8_t*)GRASS, 8, 2);
        LCD_Draw_Sprite_Colour_Scaled(138, 100, 8, 8, (uint8_t*)GRASS, 5, 2);
        LCD_Draw_Sprite_Colour_Scaled(168, 103, 8, 8, (uint8_t*)GRASS, 4, 2);
        LCD_Draw_Sprite_Colour_Scaled(198, 102, 8, 8, (uint8_t*)GRASS, 8, 2);
        sprintf(loadingstr, "%d%%", loading);
        
        LCD_printString(loadingstr, 90, 80, 0, 3);
        LCD_Refresh(&cfg0);
        printf("Loading... %d%%\n", loading);

        HAL_Delay(LOADING_DELAY_MS);
    }
}
//Generate the corresponding sprite depend on different soil states.
static void soil_generated(void)
{  
    for (int r = 0; r < row; r++){
        for(int c=0; c< col;c++){
            int x=start_x+ c * (edge+gap);
        int y=start_y+ r * (edge+gap);

        if (farm[r][c]==soil) {LCD_Draw_Sprite_Scaled(x,y, 10, 10, (uint8_t*)Soil,scale);}
        else if (farm[r][c]==seeded) {LCD_Draw_Sprite_Scaled(x,y, 10, 10, (uint8_t*)Soil_seeded,scale);}
        else if (farm[r][c]==waterd) {LCD_Draw_Sprite_Scaled(x,y, 10, 10, (uint8_t*)Soil_waterd,scale);}
        else if (farm[r][c]==ripe) {LCD_Draw_Sprite_Scaled(x,y, 10, 10, (uint8_t*)Soil_ripe,scale);}
        else if (farm[r][c]==growing){LCD_Draw_Sprite_Scaled(x,y, 10, 10, (uint8_t*)Soil_growing,scale);}
        else if (farm[r][c]==sapless) {LCD_Draw_Sprite_Scaled(x,y, 10, 10, (uint8_t*)Soil_sapless,scale);}

       }
    
    }
}
//The all initialization in this function
static void new_game_startloading(void)
{
    LCD_Fill_Buffer(0); 
    loading_animation(); 
    Init_game_value(); 
    Init_game_screen();
}
// Create a yellow hightlight box to select
static void selectblokcs(void)
{

    int select_x=start_midllesoil_x+initial_selectedcol*50;
    int select_y=start_midllesoil_y+initial_selectedrow*50;
    LCD_Draw_Rect(select_x-2, select_y-2, 44, 44, 6, 0);
    LCD_Draw_Rect(select_x-3, select_y-3, 46, 46, 6, 0);
    LCD_Draw_Rect(select_x-1, select_y-1, 42, 42, 6, 0);
    LCD_Draw_Rect(select_x-4, select_y-4, 48, 48, 6, 0);
}
//removing the yellow hightlight box
static void recover_selectblokcs(int r, int c)
{  
    int select_x=start_midllesoil_x+c*50; 
    int select_y=start_midllesoil_y+r*50;

    LCD_Draw_Rect(select_x - 2, select_y - 2, 44, 44, 0, 0);
    LCD_Draw_Rect(select_x - 3, select_y - 3, 46, 46, 0, 0);
    LCD_Draw_Rect(select_x - 1, select_y - 1, 42, 42, 0, 0);
    LCD_Draw_Rect(select_x - 4, select_y - 4, 48, 48, 0, 0);
}
//showing current mode in lcd
static void showcurrentmode(void){
    LCD_Draw_Rect(0, 0, 170, 30, 0, 1);
    if (defalutmode==seed)
    {LCD_printString("MODE: SEED", 10, 10, 1, 2);}
    else if (defalutmode == water)
    {LCD_printString("MODE: WATER", 10, 10, 1, 2);}
    else if (defalutmode==harvest) {LCD_printString("MODE: Harvest", 10, 10, 1, 2);}
    else if (defalutmode==cut) {LCD_printString("MODE: cut", 10, 10, 1, 2);}
}
//showing current score in lcd
static void showscore(void){
    LCD_Draw_Rect(0, 30, 170, 30, 0, 1);  
    char scorestr[10];
    sprintf(scorestr, "SCORE:%d", score);
    LCD_printString(scorestr, 10, 40, 3, 2);
}
//Display left time in this round
static int get_time_left_second(void){
    uint32_t elapsed = HAL_GetTick() - game_start_tick;

    if (elapsed < game_time) {
        return (game_time - elapsed) / 1000;
    }
    else {
        return 0;
    }
}
static void show_lefttime(void){
    char time[20];
    int time_left = get_time_left_second();

    LCD_Draw_Rect(150, 0, 70, 30, 0, 1);
    sprintf(time, "TIME:%d", time_left);
    LCD_printString(time, 150, 10, 6, 2);
}

// crate on and off state for red led
static void red_led_on(void){
    PWM_SetDuty(&pwm_cfg_2, 100);
}
static void red_led_off(void){
    PWM_SetDuty(&pwm_cfg_2, 0);
}
//led blink function
static void red_led_blink(void){
        static uint32_t last_blink = 0;
        static int red_led_on_flag = 0;
        uint32_t now = HAL_GetTick();


        if (now - last_blink >= RED_LED_BLINK_TIME_MS) {
            last_blink = now;

            if (red_led_on_flag == 0) {
                red_led_on_flag = 1;
                red_led_on();
            }
            else {
                red_led_on_flag = 0;
                red_led_off();
            }
        }

}
//Indicate whether ripe state or sapless state by green led and red led
static void show_ripestate(void)

{
    int ripe_count = 0;
    int sapless_count = 0;

    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            if (farm[r][c] == ripe) {
                ripe_count++;
            }
            else if (farm[r][c] == sapless) {
                sapless_count++;
            }
        }
    }
    if (ripe_count == 0) {
        PWM_SetDuty(&pwm_cfg, 0);
    }
    else if (ripe_count == 1) {
        PWM_SetDuty(&pwm_cfg, 30);
    }
    else if (ripe_count == 2) {
        PWM_SetDuty(&pwm_cfg, 70);
    }
    else {
        PWM_SetDuty(&pwm_cfg, 100);
    }
    if (sapless_count > 0) {
        red_led_blink();
        }
        else {
        red_led_off();
        }
    
}
//Init lcd screen
static void Init_game_screen(void){
    soil_generated();
    showcurrentmode();
    showscore();
    selectblokcs();
    show_ripestate();
    LCD_Refresh(&cfg0);
}
//Init the parameter of game
static void Init_game_value(void){
    score=0;
    game_start_tick = HAL_GetTick();
    initial_selectedcol=0;
    initial_selectedrow=0;
    defalutmode=seed;
    game_over = 0;
    error_message = NULL;
    error_stick = 0;
    last_move_tick = HAL_GetTick();
    PWM_SetDuty(&pwm_cfg, 0);
    red_led_off();
    check_screen_updata= 1;
    last_time_left = -1;

    for (int r=0; r<row; r++) {
        for (int c=0; c<col; c++) {
            farm[r][c]=soil;
            farmtime[r][c]=0;
        
        }
    
    }
}
//show an error message
static void show_errormessage(const char *error)
{ error_message=error;
    error_stick = HAL_GetTick();
    check_screen_updata= 1;

}
//print error message on lcd
static void lcdprint(void){
    
     if(error_message != NULL && HAL_GetTick() - error_stick < ERROR_MESSAGE_TIME_MS)
          {LCD_Draw_Rect(0, 210, 240, 30, 0, 1);
            LCD_printString(error_message, 5, 220, 5, 1);}
        }
    
//handle the bt2 to chance the player mode
static void handle_input_botton_mode(void){
        if (current_input.btn2_pressed){
            if (defalutmode==seed) {
            defalutmode=water;
            }
            else if(defalutmode==water){
            defalutmode = harvest;
            }
            else if(defalutmode==harvest) {defalutmode=cut; }
            else {defalutmode=seed;}
        printf("Mode has been changed to %d\r\n", defalutmode);
        check_screen_updata= 1;
        buzzer_mode_sound();}
}
// handle the player action by bt3
static void handle_input_botton_action(void){

    int seededr = initial_selectedrow + 1;
    int seededc = initial_selectedcol + 1;


    if (current_input.btn3_pressed)
        { check_screen_updata= 1;
          if (defalutmode==seed&& farm[seededr][seededc]==soil)  {
            farm[seededr][seededc] = seeded;
            printf("The soil(%d,%d) has been seeded\r\n",seededr, seededc);
            farmtime[seededr][seededc] = HAL_GetTick();
            show_errormessage(NULL);
            buzzer_mode_sound();
            }
            else if (defalutmode==cut) {
                farm[seededr][seededc] = soil;
                show_errormessage(NULL);
                printf("The soil(%d,%d) has been cut\r\n",seededr, seededc);
                farmtime[seededr][seededc] = 0;
                buzzer_mode_sound(); 
            }
            else if (defalutmode==water&& farm[seededr][seededc] == seeded) {
                farm[seededr][seededc] = waterd;
                printf("The soil(%d,%d) has been watered\r\n",seededr, seededc);
                farmtime[seededr][seededc] = HAL_GetTick(); 
            show_errormessage(NULL);
            buzzer_mode_sound();}
            else if (defalutmode==harvest && farm[seededr][seededc] == ripe) {
            farm[seededr][seededc] = soil;
            score++;
            farmtime[seededr][seededc] = 0;
            printf("The soil(%d,%d) has been harvested\r\n",seededr, seededc);
            printf("score + 1, the current score is %d\r\n",score);
            buzzer_mode_sound(); 
            show_errormessage(NULL);}
            else{if (defalutmode==seed&& farm[seededr][seededc]==seeded) {
            show_errormessage("Already planted");
            printf("error\r\n");
            buzzer_error_sound();
            
            }
            else if (defalutmode==seed&& farm[seededr][seededc]==waterd) {
            show_errormessage("Already planted");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==seed&& farm[seededr][seededc]==ripe) {
            show_errormessage("using harvest mode");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==water&& farm[seededr][seededc]==ripe) {
            show_errormessage("using harvest mode");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==water&& farm[seededr][seededc]==waterd) {
            show_errormessage("Already planted");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==water&& farm[seededr][seededc]==soil) {
            show_errormessage("Using seed mode");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==harvest&& farm[seededr][seededc]==seeded) {
            show_errormessage("using water mode");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==harvest&& farm[seededr][seededc]==waterd) {
            show_errormessage("wait ripe");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==harvest&& farm[seededr][seededc]==growing) {
            show_errormessage("wait ripe");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==seed&& farm[seededr][seededc]==sapless) {
            show_errormessage("Using cut mode");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==water&& farm[seededr][seededc]==sapless) {
            show_errormessage("Using cut mode");
            printf("error\r\n");
            buzzer_error_sound();}
            else if (defalutmode==harvest&& farm[seededr][seededc]==sapless) {
            show_errormessage("Using cut mode");
            printf("error\r\n");
           buzzer_error_sound();}
        }
    }
}
//handle joystick data
static void handle_input_joystick(void){
 uint32_t now = HAL_GetTick();
        if ((now - last_move_tick) >= MOVE_DELAY_MS && joystick_data.direction != CENTRE){
            int prev_col = initial_selectedcol;
            int prev_row = initial_selectedrow;
              switch (joystick_data.direction)
      {
        case N: initial_selectedrow-- ; break;
        case NE:initial_selectedrow--;initial_selectedcol++; break;
        case E: initial_selectedcol++ ; break;
        case SE: initial_selectedrow++;initial_selectedcol++; break;
        case S: initial_selectedrow++ ; break;
        case SW: initial_selectedrow++;initial_selectedcol--; break;
        case W: initial_selectedcol-- ; break;
        case NW: initial_selectedrow--;initial_selectedcol--; break;
        default: break;
      }
      printf("dir = %d\r\n", joystick_data.direction);
    if (initial_selectedrow <= -1) initial_selectedrow = -1;
    if (initial_selectedrow >=1) initial_selectedrow = 1;
    if (initial_selectedcol <= -1) initial_selectedcol = -1;
    if (initial_selectedcol >=1 ) initial_selectedcol = 1;
    if (prev_col != initial_selectedcol|| prev_row != initial_selectedrow)
    {recover_selectblokcs(prev_row, prev_col);
    check_screen_updata= 1;}
    last_move_tick = now;

    
}
}
//put all handle input in one function
static void handle_input_data(void){
    handle_input_botton_mode();
    handle_input_botton_action();
    handle_input_joystick();      
}
//Update the game and determine whether  soil  becomes growing, ripe, or sapless by time
static void update_game(void){
uint32_t now = HAL_GetTick();
        for(int r=0; r<row;r++){

            for(int c=0; c<col; c++){

                if(farm[r][c]==waterd){
                    if (now - farmtime[r][c] >= WATER_TO_GROWING_TIME_MS) {
                        farm[r][c]=growing;
                        check_screen_updata= 1;
                        farmtime[r][c]=now;}}

                if(farm[r][c]==growing){
                    if (now - farmtime[r][c] >= GROWING_TO_RIPE_TIME_MS) {
                        printf("The soil(%d,%d) has matured\r\n", r, c);
                        farm[r][c]=ripe;
                        check_screen_updata= 1;}}

                if(farm[r][c]==seeded){
                    if(now - farmtime[r][c] >= SEEDED_TO_SAPLESS_TIME_MS){
                        farm[r][c]=sapless;
                        check_screen_updata= 1;
                        printf("The soil(%d,%d) has become sapless\r\n", r, c);
                    }}}}
                        if (HAL_GetTick() - game_start_tick >= game_time) {
                            game_over =1;
                            check_screen_updata= 1;
                            printf("Game over. Final score = %d\r\n", score);
        }
                        show_ripestate();}
//Re-render the screen
static void render_game(void)

{   LCD_Fill_Buffer(0);
    soil_generated();
showcurrentmode();
showscore();
show_lefttime();
selectblokcs();
lcdprint();
LCD_Refresh(&cfg0);

}
//buzzer sound 1
static void buzzer_mode_sound(void)
{ buzzer_tone(&buzzer_cfg, BUZZER_MODE_FREQ_HZ, BUZZER_MODE_DURATION_MS);
    HAL_Delay(BUZZER_MODE_DELAY_MS);
    buzzer_off(&buzzer_cfg);
}
//buzzer sound 2
static void buzzer_error_sound(void)
{buzzer_tone(&buzzer_cfg, BUZZER_ERROR_FREQ_HZ, BUZZER_ERROR_DURATION_MS);
    HAL_Delay(BUZZER_ERROR_DELAY_MS);
    buzzer_off(&buzzer_cfg);
}