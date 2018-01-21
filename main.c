#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "SDL.h"
//#include <SDL/SDL_ttf.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif
#ifdef __linux
#include <sys/stat.h> 
#endif

int last_got_accel;
float cur_accel_values[3];
float last_accel_values[3];
float accel_per_sec[3];

//debug stuff
int last_time_mouse_down;

float menus_scale=1;
int windows_x_len=1000,windows_y_len=540;
//int windows_x_len=480,windows_y_len=320;
//int windows_x_len=1930,windows_y_len=1000;
//int windows_x_len=100,windows_y_len=100;

int real_windows_x_len,real_windows_y_len;
int pitch;
int *screen_buffer;
Uint8 *blur_buffer;
SDL_Texture* rendering_texture;
SDL_Renderer *renderer;
int x_mouse_pos,y_mouse_pos,mouse_delta_x,mouse_delta_y,button_l=0,button_r=0,have_mouse_focus=0,l_shift,l_ctrl,last_mouse_click;
int old_mouse_x,old_mouse_y;
int last_time_stamp;
int last_delta_check_time;
int last_gesture_time;
int ball_time_begin;
int ball_time_end;
int attractors_no;
int menus_no;
int particles_no=300000;
int gravity_map_changed=1;
int done;
int have_conf;
int last_time_brush;
int have_colors=0;
int use_random_mode=0;
int last_time_random;

//double gravity_constant=0.0673;
float gravity_constant=0.11973;
float border_attenuation=0.3f;
int brush_distance=150;
int brush_force=100000;
int selected_attractor;
Uint8 color_r1=128;
Uint8 color_g1=128;
Uint8 color_b1=128;
Uint8 color_r2=200;
Uint8 color_g2=200;
Uint8 color_b2=200;
//menus
int main_menu_id=0;
int attractors_menu_id=1;
int edit_attractor_menu_id=-1;
int particles_no_slider;
int border_reflect_checkbox;
int liquid_mode_checkbox;
int random_mode_checkbox;
int border_speed_slider;
int brush_distance_slider;
int brush_force_slider;
int gravity_slider;
int attractor_gravity_slider;
int color_r1_slider;
int color_g1_slider;
int color_b1_slider;
int color_r2_slider;
int color_g2_slider;
int color_b2_slider;
int move_attractor_menu_id;
int new_attractor_menu_id;
int exit_menu_id;
int colors_menu_id;
int confirm_remove_menu_id;

int particle_red=40;
int particle_green=50;
int particle_blue=60;
int particle_alpha=60;

float pressure=-1;

char config_path[256];
#define MAX_PARTICLES 1000000
#define MAX_SQRTS 3500000
#define MAX_ATTRACTORS 50
#define MAX_COLORS 20
int colors[MAX_COLORS]={0x01056021,0x02158025,0x03189028,0x041E9C2F,0x0545A533,0x0648AF36,0x074AB23D,0x084CB441,0x094EB944,0x0a51BE47,
                0x0b5AC84C,0x0c64CD50,0x0d67D453,0x0e69da5b,0x0f69e162,0x1069e669,0x1169e66c,0x1269eb70,0x1369ef74,0x1369FF80};


float sqrts[MAX_SQRTS];

#define BORDER_REFLECT 0
#define BORDER_WARP 1
int border_behaviour=BORDER_REFLECT;

int draw_tails=0;

float finger_delta_x,finger_delta_y;
int pixels_finger_delta_x,pixels_finger_delta_y;

int blur_divider=6;

typedef struct
{
    float x;
    float y;

}gravity;

gravity ground_gravity;
gravity* acceleration_map;

typedef struct
{
    int x;
    int y;
    float mass;

}attractor;

attractor attractors[MAX_ATTRACTORS];

typedef struct
{
    int time_begin;
    int time_end;
    int no;
}thread_time;

thread_time thread_times[4];

typedef struct
{
    float x;
    float y;
    float speed_x;
    float speed_y;
}particle;

particle particles[MAX_PARTICLES];

#define MAX_MENUS 10
#define MAX_BUTTONS 5
#define MAX_CHECK_BOXES 10
#define MAX_HORIZONTAL_SLIDERS 10

typedef struct
{
    int in_use;
    int x;
    int y;
    int x_len;
    int y_len;
    int cur_value;
    int min_value;
    int max_value;
}horizontal_slider;

typedef struct
{
    int in_use;
    int x;
    int y;
    int x_len;
    int y_len;
    int selected;
    char text[64];
}checkbox;

typedef struct
{
    int x;
    int y;
    int x_len;
    int y_len;
    char text[32];
    void (*on_click)();

}button;

typedef struct
{
    int in_use;
    int x;
    int y;
    int x_len;
    int y_len;
    void (*on_click)();
    void (*display_menu)();
    char title[128];
    int all_screen;
    button buttons[MAX_BUTTONS];
    checkbox checkboxs[MAX_CHECK_BOXES];
    horizontal_slider horizontal_sliders[MAX_HORIZONTAL_SLIDERS];
}menu;

menu menus[MAX_MENUS];


typedef struct
{
int w;
int h;
SDL_Texture* cur_texture;
}font;

int last_loaded_font;

#define MAX_FONTS 10
font fonts[MAX_FONTS];
static int draw_particles_thread(void* ptr);
static int particle_repulse_by_mouse_thread(void *ptr);
static int build_acceleration_map_thread(void *ptr);
void display_main_menu(int menu_id);
void on_click_main_menu(int menu_id);
void click_ok_main_menu();
void click_cancel_main_menu();
void print_string(char *str, int font, int char_xscreen, int char_yscreen);
void init_particles();
void click_reset_button();
void click_attractors_button();
void display_attractors_menu(int menu_id);
void on_click_attractors_menu(int menu_id);
void click_attractors_done();
void click_attractors_new_attractor();
void on_click_edit_attractor_menu(int menu_id);
void display_edit_attractor_menu(int menu_id);
void click_edit_move();
void click_edit_done();
void click_edit_remove();
void build_acceleration_map();
int add_horizontal_slider_to_menu(int menu_id, int scale, int cur_value, int min_value, int max_value, int x, int y, int x_len, int y_len);
int add_menu(void *on_click, void *display_menu, int x, int y, int x_len, int y_len, int in_use, int all_screen, char *title);
int add_button_to_menu(int menu_id, int scale, void *click_func, int x, int y, int x_len, int y_len, char *text);
void on_click_move_attractors();
void display_move_attractors();
void on_click_new_attractors();
void display_new_attractors();
void on_click_exit_menu(int menu_id);
void display_exit_menu(int menu_id);
void click_yes_exit_menu();
void click_no_exit_menu();
static int build_sqrts_thread(void *ptr);
void click_colors_main_menu();
void display_colors_menu(int menu_id);
void on_click_colors_menu(int menu_id);
void click_colors_ok();
void click_attractors_remove_all();
void display_remove_menu(int menu_id);
void on_click_remove_menu(int menu_id);
void click_yes_remove_menu();
void click_no_remove_menu();
void build_colors(int r1, int g1, int b1, int r2, int g2, int b2);
static int all_particles_attract_thread(void *ptr);


void get_configh_path()
{

#ifdef _WIN32
	SHGetFolderPath(NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL, 0, config_path);
#elif  __ANDROID__
    strcpy(config_path,SDL_AndroidGetInternalStoragePath());
    strcat(config_path,"/");
#else
	strcpy(config_path,getenv("HOME"));
#endif

#ifndef __ANDROID__
	strcat(config_path,"/.million_particles/");

#ifndef _WIN32
		mkdir(config_path, (S_IRWXU|S_IRWXG|S_IRWXO));
#else
		mkdir(config_path);
#endif
#endif //android

}

void save_config()
{
    int i;
    FILE * f;
    void *conf;
    char fn[256];

    sprintf(fn,"%sparticles.cfg",config_path);
    f = fopen(fn, "wb");
    if(!f)return;

    conf=malloc(1000);
    memset(conf,0,250);

    *((Uint32 *)(conf+0))=particles_no;
    *((float *)(conf+4))=gravity_constant;
    *((float *)(conf+8))=border_attenuation;
    *((Uint32 *)(conf+12))=brush_distance;
    *((float *)(conf+16))=brush_force;
    *((Uint32 *)(conf+20))=border_behaviour;
    *((Uint32 *)(conf+24))=draw_tails;
    *((Uint32 *)(conf+30))=attractors_no;
    *((Uint32 *)(conf+34))=have_colors;
    if(have_colors)
    {
        *((Uint8 *)(conf+38))=menus[colors_menu_id].horizontal_sliders[color_r1_slider].cur_value;
        *((Uint8 *)(conf+39))=menus[colors_menu_id].horizontal_sliders[color_g1_slider].cur_value;
        *((Uint8 *)(conf+40))=menus[colors_menu_id].horizontal_sliders[color_b1_slider].cur_value;
        *((Uint8 *)(conf+41))=menus[colors_menu_id].horizontal_sliders[color_r2_slider].cur_value;
        *((Uint8 *)(conf+42))=menus[colors_menu_id].horizontal_sliders[color_g2_slider].cur_value;
        *((Uint8 *)(conf+43))=menus[colors_menu_id].horizontal_sliders[color_b2_slider].cur_value;
    }
    //reserved up to 99

    //write the header and reserved area
    fwrite(conf,100,4,f);

    for(i=0;i<attractors_no;i++)
    {
        *((float *)(conf+400+i*12+0))=attractors[i].mass;
        *((Uint32 *)(conf+400+i*12+4))=attractors[i].x;
        *((Uint32 *)(conf+400+i*12+8))=attractors[i].y;
    }

    //write the objects
    fwrite((conf+400),attractors_no,sizeof(attractor),f);

    fclose(f);
    free(conf);
}

int load_config()
{
    int i;
    FILE * f;
    void *conf;
    char fn[256];

    sprintf(fn,"%sparticles.cfg",config_path);
    f = fopen(fn, "rb");
    if(!f)return 0;

    conf=malloc(1000);

    fread(conf,100,4,f);
    particles_no=*((Uint32 *)(conf+0));
    gravity_constant=*((float *)(conf+4));
    border_attenuation=*((float *)(conf+8));
    brush_distance=*((Uint32 *)(conf+12));
    brush_force=*((float *)(conf+16));
    border_behaviour=*((Uint32 *)(conf+20));
    draw_tails=*((Uint32 *)(conf+24));
    attractors_no=*((Uint32 *)(conf+30));
    have_colors=*((Uint32 *)(conf+34));

    if(have_colors)
    {
        color_r1=*((Uint8 *)(conf+38));
        color_g1=*((Uint8 *)(conf+39));
        color_b1=*((Uint8 *)(conf+40));
        color_r2=*((Uint8 *)(conf+41));
        color_g2=*((Uint8 *)(conf+42));
        color_b2=*((Uint8 *)(conf+43));

        build_colors(color_r1,color_g1,color_b1,color_r2,color_g2,color_b2);
    }

    //reserved up to 99
    fread((conf+400),attractors_no,sizeof(attractor),f);
    for(i=0;i<attractors_no;i++)
    {
        attractors[i].mass=*((float *)(conf+400+i*12+0));
        attractors[i].x=*((Uint32 *)(conf+400+i*12+4));
        attractors[i].y=*((Uint32 *)(conf+400+i*12+8));
    }

    //write the objects

    fclose(f);
    free(conf);
    return 1;
}

void set_attractor_color(int attractor_id)
{
    if(attractors[attractor_id].mass<0)
    SDL_SetRenderDrawColor(renderer,30,50,255,255);
    else
    if(attractors[attractor_id].mass>0)
    SDL_SetRenderDrawColor(renderer,255,50,30,255);
    else
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
}


void check_buttons(int menu_id)
{
    int i;
    int x_offset;
    int y_offset;

    if(SDL_GetTicks()-last_time_mouse_down>3)return;
    last_time_mouse_down-=10;//to avoid double clicks

    x_offset=menus[menu_id].x;
    y_offset=menus[menu_id].y;
    for(i=0;i<MAX_BUTTONS;i++)
        if(menus[menu_id].buttons[i].on_click)
            if(x_mouse_pos>=menus[menu_id].buttons[i].x+x_offset && x_mouse_pos<=menus[menu_id].buttons[i].x+menus[menu_id].buttons[i].x_len+x_offset && y_mouse_pos>=menus[menu_id].buttons[i].y+y_offset
            && y_mouse_pos<=menus[menu_id].buttons[i].y+menus[menu_id].buttons[i].y_len+y_offset)
                menus[menu_id].buttons[i].on_click();
}

int check_click_on_menu()
{
    int i;

    for(i=0;i<menus_no;i++)
        if(menus[i].in_use)
        {
            if((x_mouse_pos>=menus[i].x && x_mouse_pos<=menus[i].x+menus[i].x_len && y_mouse_pos>=menus[i].y
               && y_mouse_pos<=menus[i].y+menus[i].y_len) || menus[i].all_screen)
               {
                    menus[i].on_click(i);
                    last_time_brush=SDL_GetTicks()+150;//make it not draw the brush right away if we click on the OK, Cancel, ETC. buttons
                    return 1;
               }
        }

    return 0;
}


void click_ok_main_menu()
{
    float diff;
    if(menus[main_menu_id].checkboxs[border_reflect_checkbox].selected)
    border_behaviour=BORDER_REFLECT;
    else
    border_behaviour=BORDER_WARP;

    diff=gravity_constant-menus[main_menu_id].horizontal_sliders[gravity_slider].cur_value/1000.0f;

    draw_tails=menus[main_menu_id].checkboxs[liquid_mode_checkbox].selected;
    use_random_mode=menus[main_menu_id].checkboxs[random_mode_checkbox].selected;
    particles_no=menus[main_menu_id].horizontal_sliders[particles_no_slider].cur_value;
    border_attenuation=menus[main_menu_id].horizontal_sliders[border_speed_slider].cur_value/100.0f;
    brush_distance=menus[main_menu_id].horizontal_sliders[brush_distance_slider].cur_value;
    brush_force=menus[main_menu_id].horizontal_sliders[brush_force_slider].cur_value;
    gravity_constant=menus[main_menu_id].horizontal_sliders[gravity_slider].cur_value/1000.0f;

    if(diff>0.001f || diff<-0.001f || gravity_map_changed)

    build_acceleration_map();
    SDL_Log("gravity_constant:%f gravity_constant_sld:%f diff:%f",gravity_constant,menus[main_menu_id].horizontal_sliders[gravity_slider].cur_value/1000.0f,diff);


    menus[main_menu_id].in_use=0;
}

void click_colors_main_menu()
{
    menus[main_menu_id].in_use=0;
    menus[colors_menu_id].in_use=1;
}

void click_yes_exit_menu()
{
    done=1;
}

void click_no_exit_menu()
{
    menus[exit_menu_id].in_use=0;
    menus[main_menu_id].in_use=1;
}

void click_yes_remove_menu()
{
    menus[confirm_remove_menu_id].in_use=0;
    menus[attractors_menu_id].in_use=1;

    attractors_no=0;
    build_acceleration_map();
}

void click_no_remove_menu()
{
    menus[confirm_remove_menu_id].in_use=0;
    menus[attractors_menu_id].in_use=1;
}
void click_cancel_main_menu()
{
    menus[main_menu_id].in_use=0;
}
void click_attractors_button()
{
    menus[main_menu_id].in_use=0;
    menus[attractors_menu_id].in_use=1;
    gravity_map_changed=0;
}

void click_reset_button()
{
    init_particles();
    //menus[main_menu_id].in_use=0;
}

void click_attractors_done()
{
    menus[attractors_menu_id].in_use=0;
    menus[main_menu_id].in_use=1;
}

void click_attractors_remove_all()
{
    menus[attractors_menu_id].in_use=0;
    menus[confirm_remove_menu_id].in_use=1;
}

void click_attractors_new_attractor()
{
    menus[attractors_menu_id].in_use=0;
    menus[new_attractor_menu_id].in_use=1;
}

void click_edit_move()
{
    menus[edit_attractor_menu_id].in_use=0;
    menus[move_attractor_menu_id].in_use=1;
}

void build_colors(int r1, int g1, int b1, int r2, int g2, int b2)
{
    int i;
    float red_ratio,green_ratio,blue_ratio,cur_red,cur_green,cur_blue;
    Uint8 *rgb_ptr = (Uint8 *)colors;
    Uint8 red_color[MAX_COLORS];
    Uint8 green_color[MAX_COLORS];
    Uint8 blue_color[MAX_COLORS];

//int colors[MAX_COLORS]={0x01056021,0x02158025,0x03189028,0x041E9C2F,0x0545A533,0x0648AF36,0x074AB23D,0x084CB441,0x094EB944,0x0a51BE47,
//0x0b5AC84C,0x0c64CD50,0x0d67D453,0x0e69da5b,0x0f69e162,0x1069e669,0x1169e66c,0x1269eb70,0x1369ef74,0x1369FF80};

    red_color[0]=r1;
    red_color[1]=r1+((r2-r1)/(float)(MAX_COLORS))*MAX_COLORS/6;
    r1=red_color[1];
    red_color[2]=r1+((r2-r1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    r1=red_color[2];
    red_color[3]=r1+((r2-r1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    r1=red_color[3];
    cur_red=r1;
    red_ratio=(r2-r1)/(float)(MAX_COLORS-2);

    green_color[0]=g1;
    green_color[1]=g1+((g2-g1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    g1=green_color[1];
    green_color[2]=g1+((g2-g1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    g1=green_color[2];
    green_color[3]=g1+((g2-g1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    g1=green_color[3];
    cur_green=g1;
    green_ratio=(g2-g1)/(float)(MAX_COLORS-2);

    blue_color[0]=b1;
    blue_color[1]=b1+((b2-b1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    b1=blue_color[1];
    blue_color[2]=b1+((b2-b1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    b1=blue_color[2];
    blue_color[3]=b1+((b2-b1)/(float)(MAX_COLORS))*MAX_COLORS/8;
    b1=blue_color[3];
    cur_blue=b1;
    blue_ratio=(b2-b1)/(float)(MAX_COLORS-2);

    for(i=4;i<MAX_COLORS;i++)
    {
        cur_red+=red_ratio;
        red_color[i]=cur_red;

        cur_green+=green_ratio;
        green_color[i]=cur_green;

        cur_blue+=blue_ratio;
        blue_color[i]=cur_blue;

    }

    for(i=0;i<MAX_COLORS;i++)
    {
        rgb_ptr[i*4+2]=red_color[i];
        rgb_ptr[i*4+1]=green_color[i];
        rgb_ptr[i*4+0]=blue_color[i];
        rgb_ptr[i*4+3]=i+1;


        SDL_Log("cur_red:%i",red_color[i]);
        SDL_Log("cur_green:%i",green_color[i]);
        SDL_Log("cur_blue:%i",blue_color[i]);

    }

    rgb_ptr[(MAX_COLORS-1)*4+3]=MAX_COLORS-1;//cred
}

void click_colors_ok()
{

    menus[colors_menu_id].in_use=0;
    menus[main_menu_id].in_use=1;
    build_colors(menus[colors_menu_id].horizontal_sliders[color_r1_slider].cur_value,
                 menus[colors_menu_id].horizontal_sliders[color_g1_slider].cur_value,
                 menus[colors_menu_id].horizontal_sliders[color_b1_slider].cur_value,
                 menus[colors_menu_id].horizontal_sliders[color_r2_slider].cur_value,
                 menus[colors_menu_id].horizontal_sliders[color_g2_slider].cur_value,
                 menus[colors_menu_id].horizontal_sliders[color_b2_slider].cur_value);

    have_colors=1;
}

void click_edit_done()
{
    menus[edit_attractor_menu_id].in_use=0;
    menus[attractors_menu_id].in_use=1;
    gravity_map_changed=1;
    attractors[selected_attractor].mass=menus[edit_attractor_menu_id].horizontal_sliders[attractor_gravity_slider].cur_value;
}

void click_edit_remove()
{
    attractors[selected_attractor].mass=attractors[attractors_no-1].mass;
    attractors[selected_attractor].x=attractors[attractors_no-1].x;
    attractors[selected_attractor].y=attractors[attractors_no-1].y;
    attractors_no--;
    menus[edit_attractor_menu_id].in_use=0;
    menus[attractors_menu_id].in_use=1;
    gravity_map_changed=1;

}

void display_horizontal_slider(int menu_id, int slider_id, int font_size, char * str)
{
    SDL_Rect rect;
    int numbers_len;
    int min_no;
    int max_no;
    int cur_no;
    float ratio;

    if(menus_scale<=0.9f)
    font_size=0;
    rect.x=menus[menu_id].horizontal_sliders[slider_id].x+menus[menu_id].x;
    rect.y=menus[menu_id].horizontal_sliders[slider_id].y+menus[menu_id].y+(menus[menu_id].horizontal_sliders[slider_id].y_len-5)/2;
    rect.w=menus[menu_id].horizontal_sliders[slider_id].x_len;
    rect.h=5;

    SDL_SetRenderDrawColor(renderer,160,160,160,255);
    SDL_RenderFillRect(renderer, &rect);

    print_string(str,font_size,rect.x+rect.w+5,rect.y+(rect.h-fonts[font_size].h)/2);

    //draw the cursor
    min_no=menus[menu_id].horizontal_sliders[slider_id].min_value;
    max_no=menus[menu_id].horizontal_sliders[slider_id].max_value;
    cur_no=menus[menu_id].horizontal_sliders[slider_id].cur_value;
    numbers_len=max_no-min_no;
    ratio=(float)numbers_len/(float)menus[menu_id].horizontal_sliders[slider_id].x_len;

    rect.x=(cur_no-min_no)/ratio+menus[menu_id].horizontal_sliders[slider_id].x+menus[menu_id].x-5;
    rect.y=menus[menu_id].horizontal_sliders[slider_id].y+menus[menu_id].y;
    rect.w=10;
    rect.h=menus[menu_id].horizontal_sliders[slider_id].y_len;
    SDL_SetRenderDrawColor(renderer,20,190,255,255);
    SDL_RenderFillRect(renderer, &rect);

}

void display_button(int menu_id,int button_id,int font_size)
{
    SDL_Rect rect;
    int text_x_len;

    if(menus_scale<=0.9)
    font_size=0;

    rect.x=menus[menu_id].buttons[button_id].x+menus[menu_id].x;
    rect.y=menus[menu_id].buttons[button_id].y+menus[menu_id].y;
    rect.w=menus[menu_id].buttons[button_id].x_len;
    rect.h=menus[menu_id].buttons[button_id].y_len;

    SDL_RenderDrawRect(renderer, &rect);
    text_x_len=strlen(menus[menu_id].buttons[button_id].text)*fonts[font_size].w/96;
    print_string(menus[menu_id].buttons[button_id].text,font_size,rect.x+(rect.w-text_x_len)/2,rect.y+(rect.h-fonts[font_size].h)/2);
}

void display_checkboxs(int menu_id,int checkboxs_id,int font_size)
{
    SDL_Rect rect;

    if(menus_scale<=0.9f)
    font_size=0;

    rect.x=menus[menu_id].checkboxs[checkboxs_id].x+menus[menu_id].x;
    rect.y=menus[menu_id].checkboxs[checkboxs_id].y+menus[menu_id].y;
    rect.w=menus[menu_id].checkboxs[checkboxs_id].x_len;
    rect.h=menus[menu_id].checkboxs[checkboxs_id].y_len;

    SDL_RenderDrawRect(renderer, &rect);
    print_string(menus[menu_id].checkboxs[checkboxs_id].text,font_size,rect.x+rect.w+5,rect.y+(rect.h-fonts[font_size].h)/2);

    if(menus[menu_id].checkboxs[checkboxs_id].selected)
    {
        rect.x+=3;
        rect.y+=3;
        rect.w-=6;
        rect.h-=6;
        SDL_RenderFillRect(renderer, &rect);
        }
}


void display_main_menu(int menu_id)
{
    SDL_Rect rect;
//    int font_size=0;
//    int text_x_len;
    char str[128];
    int i;

    rect.x=menus[menu_id].x;
    rect.y=menus[menu_id].y;
    rect.w=menus[menu_id].x_len;
    rect.h=menus[menu_id].y_len;

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderDrawRect(renderer, &rect);

    //text_x_len=strlen(menus[menu_id].title)*fonts[font_size].w/96;
    //print_string(menus[menu_id].title,font_size,rect.x+(rect.w-text_x_len)/2,rect.y);

    //display buttons

    for(i=0;i<MAX_BUTTONS;i++)
    if(menus[menu_id].buttons[i].on_click)
    display_button(menu_id,i,1);

    //display checkboxes
    for(i=0;i<MAX_CHECK_BOXES;i++)
    if(menus[menu_id].checkboxs[i].in_use)
    display_checkboxs(menu_id,i,1);

    //display the sliders
    sprintf(str,"%i particles",menus[menu_id].horizontal_sliders[particles_no_slider].cur_value);
    display_horizontal_slider(0,particles_no_slider,1,str);
    sprintf(str,"%.2f border speed",(float)menus[menu_id].horizontal_sliders[border_speed_slider].cur_value/100.0f);
    display_horizontal_slider(0,border_speed_slider,1,str);
    sprintf(str,"%i size",menus[menu_id].horizontal_sliders[brush_distance_slider].cur_value);
    display_horizontal_slider(0,brush_distance_slider,1,str);
    sprintf(str,"%i force",menus[menu_id].horizontal_sliders[brush_force_slider].cur_value);
    display_horizontal_slider(0,brush_force_slider,1,str);
    sprintf(str,"%i speed",menus[menu_id].horizontal_sliders[gravity_slider].cur_value);
    display_horizontal_slider(0,gravity_slider,1,str);


}

void display_colors_menu(int menu_id)
{
    SDL_Rect rect;
    int font_size=1;
//    int text_x_len;
    char str[128];
    int i;

    if(menus_scale<0.9f)font_size=0;

    rect.x=menus[menu_id].x;
    rect.y=menus[menu_id].y;
    rect.w=menus[menu_id].x_len;
    rect.h=menus[menu_id].y_len;

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderDrawRect(renderer, &rect);

    for(i=0;i<MAX_BUTTONS;i++)
    if(menus[menu_id].buttons[i].on_click)
    display_button(menu_id,i,1);

    str[0]=0;
    //sprintf(str,"%i R1",menus[menu_id].horizontal_sliders[color_r1_slider].cur_value);
    display_horizontal_slider(menu_id,color_r1_slider,1,str);
    rect.x=menus[menu_id].horizontal_sliders[color_r1_slider].x+menus[menu_id].horizontal_sliders[color_r1_slider].x_len+menus[menu_id].x+20;
    rect.y=menus[menu_id].horizontal_sliders[color_r1_slider].y+menus[menu_id].y;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,menus[menu_id].horizontal_sliders[color_r1_slider].cur_value,0,0,255);
    SDL_RenderFillRect(renderer, &rect);

    //sprintf(str,"%i G1",menus[menu_id].horizontal_sliders[color_g1_slider].cur_value);
    display_horizontal_slider(menu_id,color_g1_slider,1,str);
    rect.x=menus[menu_id].horizontal_sliders[color_g1_slider].x+menus[menu_id].horizontal_sliders[color_g1_slider].x_len+menus[menu_id].x+20;
    rect.y=menus[menu_id].horizontal_sliders[color_g1_slider].y+menus[menu_id].y;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,0,menus[menu_id].horizontal_sliders[color_g1_slider].cur_value,0,255);
    SDL_RenderFillRect(renderer, &rect);

    //sprintf(str,"%i B1",menus[menu_id].horizontal_sliders[color_b1_slider].cur_value);
    display_horizontal_slider(menu_id,color_b1_slider,1,str);
    rect.x=menus[menu_id].horizontal_sliders[color_b1_slider].x+menus[menu_id].horizontal_sliders[color_b1_slider].x_len+menus[menu_id].x+20;
    rect.y=menus[menu_id].horizontal_sliders[color_b1_slider].y+menus[menu_id].y;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,0,0,menus[menu_id].horizontal_sliders[color_b1_slider].cur_value,255);
    SDL_RenderFillRect(renderer, &rect);

    //sprintf(str,"%i R2",menus[menu_id].horizontal_sliders[color_r2_slider].cur_value);
    display_horizontal_slider(menu_id,color_r2_slider,1,str);
    rect.x=menus[menu_id].horizontal_sliders[color_r2_slider].x+menus[menu_id].horizontal_sliders[color_r2_slider].x_len+menus[menu_id].x+20;
    rect.y=menus[menu_id].horizontal_sliders[color_r2_slider].y+menus[menu_id].y;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,menus[menu_id].horizontal_sliders[color_r2_slider].cur_value,0,0,255);
    SDL_RenderFillRect(renderer, &rect);

    //sprintf(str,"%i G2",menus[menu_id].horizontal_sliders[color_g2_slider].cur_value);
    display_horizontal_slider(menu_id,color_g2_slider,1,str);
    rect.x=menus[menu_id].horizontal_sliders[color_g2_slider].x+menus[menu_id].horizontal_sliders[color_g2_slider].x_len+menus[menu_id].x+20;
    rect.y=menus[menu_id].horizontal_sliders[color_g2_slider].y+menus[menu_id].y;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,0,menus[menu_id].horizontal_sliders[color_g2_slider].cur_value,0,255);
    SDL_RenderFillRect(renderer, &rect);

    //sprintf(str,"%i B2",menus[menu_id].horizontal_sliders[color_b2_slider].cur_value);
    display_horizontal_slider(menu_id,color_b2_slider,1,str);
    rect.x=menus[menu_id].horizontal_sliders[color_b2_slider].x+menus[menu_id].horizontal_sliders[color_b2_slider].x_len+menus[menu_id].x+20;
    rect.y=menus[menu_id].horizontal_sliders[color_b2_slider].y+menus[menu_id].y;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,0,0,menus[menu_id].horizontal_sliders[color_b2_slider].cur_value,255);
    SDL_RenderFillRect(renderer, &rect);

    rect.x=menus[menu_id].x+15;
    rect.y=menus[menu_id].horizontal_sliders[color_b1_slider].y+menus[menu_id].y+60*menus_scale;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,menus[menu_id].horizontal_sliders[color_r1_slider].cur_value,menus[menu_id].horizontal_sliders[color_g1_slider].cur_value,menus[menu_id].horizontal_sliders[color_b1_slider].cur_value,255);
    SDL_RenderFillRect(renderer, &rect);

    print_string("First color",font_size,rect.x+rect.w+5,rect.y+(rect.h-fonts[font_size].h)/2);

    rect.x=menus[menu_id].horizontal_sliders[color_b2_slider].x+menus[menu_id].x;
    rect.y=menus[menu_id].horizontal_sliders[color_b2_slider].y+menus[menu_id].y+60*menus_scale;
    rect.w=50*menus_scale;
    rect.h=40*menus_scale;
    SDL_SetRenderDrawColor(renderer,menus[menu_id].horizontal_sliders[color_r2_slider].cur_value,menus[menu_id].horizontal_sliders[color_g2_slider].cur_value,menus[menu_id].horizontal_sliders[color_b2_slider].cur_value,255);
    SDL_RenderFillRect(renderer, &rect);

    print_string("Last color",font_size,rect.x+rect.w+5,rect.y+(rect.h-fonts[font_size].h)/2);
}

void display_attractors_menu(int menu_id)
{
    SDL_Rect rect;
    int i;

    for(i=0;i<attractors_no;i++)
    {
        set_attractor_color(i);
        rect.x=((float)real_windows_x_len/(float)windows_x_len)*attractors[i].x-20;
        rect.y=((float)real_windows_y_len/(float)windows_y_len)*attractors[i].y-20;
        rect.w=40;
        rect.h=40;

        SDL_RenderDrawRect(renderer, &rect);
    }

    SDL_SetRenderDrawColor(renderer,0,0,0,255);

    for(i=0;i<MAX_BUTTONS;i++)
    if(menus[menu_id].buttons[i].on_click)
    display_button(menu_id,i,1);


}

void check_checkboxes(int menu_id)
{
    int i;
    int x,y,x_len,y_len;

    if(SDL_GetTicks()-last_time_mouse_down>10)return;

    for(i=0;i<MAX_CHECK_BOXES;i++)
    if(menus[menu_id].checkboxs[i].in_use)
    {
        x=menus[menu_id].checkboxs[i].x+menus[menu_id].x;
        x_len=menus[menu_id].checkboxs[i].x_len;
        y=menus[menu_id].checkboxs[i].y+menus[menu_id].y;
        y_len=menus[menu_id].checkboxs[i].y_len;

        if(x_mouse_pos>=x && x_mouse_pos<=x+x_len && y_mouse_pos>=y && y_mouse_pos<=y_len+y)
        menus[menu_id].checkboxs[i].selected=!menus[menu_id].checkboxs[i].selected;
    }
}

void check_horizontal_sliders(int menu_id)
{
    int i;
    int x,y,x_len,y_len;
    int numbers_len;
    int min_no;
    int max_no;
    int mouse_cursor_len;
    float ratio;

    for(i=0;i<MAX_CHECK_BOXES;i++)
    if(menus[menu_id].horizontal_sliders[i].in_use)
    {
        x=menus[menu_id].horizontal_sliders[i].x+menus[menu_id].x;
        x_len=menus[menu_id].horizontal_sliders[i].x_len;
        y=menus[menu_id].horizontal_sliders[i].y+menus[menu_id].y;
        y_len=menus[menu_id].horizontal_sliders[i].y_len;

        if(x_mouse_pos>=x-10 && x_mouse_pos<=x+x_len+10 && y_mouse_pos>=y && y_mouse_pos<=y_len+y)
        {

            min_no=menus[menu_id].horizontal_sliders[i].min_value;
            max_no=menus[menu_id].horizontal_sliders[i].max_value;
            numbers_len=max_no-min_no;
            ratio=(float)numbers_len/(float)x_len;

            mouse_cursor_len=x_mouse_pos-x;
            if(x_mouse_pos>x_len+x)
            menus[menu_id].horizontal_sliders[i].cur_value=max_no;
            else
            if(x_mouse_pos<x)
            menus[menu_id].horizontal_sliders[i].cur_value=min_no;
            else
            menus[menu_id].horizontal_sliders[i].cur_value=mouse_cursor_len*ratio+min_no;
        }

    }
}

void on_click_edit_attractor_menu(int menu_id)
{
    check_buttons(menu_id);
    check_horizontal_sliders(menu_id);
}

void on_click_main_menu(int menu_id)
{
    check_buttons(menu_id);
    check_checkboxes(menu_id);
    check_horizontal_sliders(menu_id);
}

void on_click_colors_menu(int menu_id)
{
    check_horizontal_sliders(menu_id);
    check_buttons(menu_id);
}

void on_click_remove_menu(int menu_id)
{
    check_buttons(menu_id);
}

void on_click_exit_menu(int menu_id)
{
    check_buttons(menu_id);
}

void on_click_attractors_menu(int menu_id)
{
    int i;
    int x,y,x_len,y_len;

    check_buttons(menu_id);

    x_len=menus[edit_attractor_menu_id].x_len;
    y_len=menus[edit_attractor_menu_id].y_len;

    for(i=0;i<attractors_no;i++)
    {
    x=attractors[i].x*((float)real_windows_x_len/(float)windows_x_len);
    y=attractors[i].y*((float)real_windows_y_len/(float)windows_y_len);
    if(x_mouse_pos>x-20 && y_mouse_pos>y-20 && x_mouse_pos<x+20 && y_mouse_pos<y+20)
    {
        selected_attractor=i;
        if(x+x_len>real_windows_x_len)
            x=real_windows_x_len-x_len;
        if(y+y_len>real_windows_y_len)
            y=real_windows_y_len-y_len;
        {
            menus[edit_attractor_menu_id].x=x;
            menus[edit_attractor_menu_id].y=y;
            menus[edit_attractor_menu_id].in_use=1;
            menus[edit_attractor_menu_id].horizontal_sliders[attractor_gravity_slider].cur_value=attractors[i].mass;
        }

        menus[attractors_menu_id].in_use=0;
        break;
    }
    }

}

void on_click_move_attractors()
{
    attractors[selected_attractor].x=x_mouse_pos*((float)windows_x_len/(float)real_windows_x_len);
    attractors[selected_attractor].y=y_mouse_pos*((float)windows_y_len/(float)real_windows_y_len);

    menus[attractors_menu_id].in_use=1;
    menus[move_attractor_menu_id].in_use=0;

    gravity_map_changed=1;

}

void on_click_new_attractors()
{
    if(last_time_mouse_down!=SDL_GetTicks())return;

    attractors[attractors_no].x=x_mouse_pos*((float)windows_x_len/(float)real_windows_x_len);
    attractors[attractors_no].y=y_mouse_pos*((float)windows_y_len/(float)real_windows_y_len);
    attractors[attractors_no].mass=1000000;
    attractors_no++;

    menus[attractors_menu_id].in_use=1;
    menus[new_attractor_menu_id].in_use=0;

    gravity_map_changed=1;
}

void display_new_attractors()
{
    SDL_Rect rect;
    int i;
    int text_x_len;
    int font_size=1;

    if(menus_scale<0.9f)font_size=0;

    for(i=0;i<attractors_no;i++)
    {
        set_attractor_color(i);
        rect.x=((float)real_windows_x_len/(float)windows_x_len)*attractors[i].x-20;
        rect.y=((float)real_windows_y_len/(float)windows_y_len)*attractors[i].y-20;
        rect.w=40;
        rect.h=40;

        SDL_RenderDrawRect(renderer, &rect);
    }

    text_x_len=strlen(menus[new_attractor_menu_id].title)*fonts[font_size].w/96;
    print_string(menus[new_attractor_menu_id].title,font_size,(real_windows_x_len-text_x_len)/2,10);
}

void display_move_attractors()
{
    SDL_Rect rect;
    int i;
    int text_x_len;
    int font_size=1;

    if(menus_scale<0.9f)font_size=0;


    for(i=0;i<attractors_no;i++)
    {
        if(i==selected_attractor)
        SDL_SetRenderDrawColor(renderer,50,255,50,255);
        else
        set_attractor_color(i);

        rect.x=((float)real_windows_x_len/(float)windows_x_len)*attractors[i].x-20;
        rect.y=((float)real_windows_y_len/(float)windows_y_len)*attractors[i].y-20;
        rect.w=40;
        rect.h=40;

        SDL_RenderDrawRect(renderer, &rect);
    }

    text_x_len=strlen(menus[move_attractor_menu_id].title)*fonts[font_size].w/96;
    print_string(menus[move_attractor_menu_id].title,font_size,(real_windows_x_len-text_x_len)/2,10);
}

int add_button_to_menu(int menu_id, int scale, void *click_func, int x, int y, int x_len, int y_len, char *text)
{
    int i;

    //find an empty space
    for(i=0;i<MAX_BUTTONS;i++)
    if(!menus[menu_id].buttons[i].on_click)
    break;

    menus[menu_id].buttons[i].on_click=click_func;
    menus[menu_id].buttons[i].x=x;
    menus[menu_id].buttons[i].y=y;
    menus[menu_id].buttons[i].x_len=x_len;
    menus[menu_id].buttons[i].y_len=y_len;

    strcpy(menus[menu_id].buttons[i].text,text);

    return i;
}

int add_checkbox_to_menu(int menu_id, int scale, int selected, int x, int y, int x_len, int y_len, char *text)
{
    int i;

    //find an empty space
    for(i=0;i<MAX_CHECK_BOXES;i++)
    if(!menus[menu_id].checkboxs[i].in_use)
    break;

    menus[menu_id].checkboxs[i].in_use=1;
    menus[menu_id].checkboxs[i].selected=selected;
    menus[menu_id].checkboxs[i].x=x;
    menus[menu_id].checkboxs[i].y=y;
    menus[menu_id].checkboxs[i].x_len=x_len;
    menus[menu_id].checkboxs[i].y_len=y_len;

    strcpy(menus[menu_id].checkboxs[i].text,text);

    return i;
}

int add_horizontal_slider_to_menu(int menu_id, int scale, int cur_value, int min_value, int max_value, int x, int y, int x_len, int y_len)
{
    int i;

    //find an empty space
    for(i=0;i<MAX_HORIZONTAL_SLIDERS;i++)
    if(!menus[menu_id].horizontal_sliders[i].in_use)
    break;

    menus[menu_id].horizontal_sliders[i].in_use=1;
    menus[menu_id].horizontal_sliders[i].cur_value=cur_value;
    menus[menu_id].horizontal_sliders[i].min_value=min_value;
    menus[menu_id].horizontal_sliders[i].max_value=max_value;
    menus[menu_id].horizontal_sliders[i].x=x;
    menus[menu_id].horizontal_sliders[i].y=y;
    menus[menu_id].horizontal_sliders[i].x_len=x_len;
    menus[menu_id].horizontal_sliders[i].y_len=y_len;

    return i;
}

int add_menu(void *on_click, void *display_menu, int x, int y, int x_len, int y_len, int in_use, int all_screen, char *title)
{

    menus[menus_no].on_click=on_click;
    menus[menus_no].display_menu=display_menu;
    menus[menus_no].x=x;
    menus[menus_no].y=y;
    menus[menus_no].x_len=x_len;
    menus[menus_no].y_len=y_len;
    menus[menus_no].in_use=in_use;
    menus[menus_no].all_screen=all_screen;
    strcpy(menus[menus_no].title,title);

    menus_no++;

    return menus_no-1;


}

void init_menus()
{

    main_menu_id=add_menu(on_click_main_menu,display_main_menu,(real_windows_x_len-800)/2,(real_windows_y_len-400)/2,
             800,400,1,0,"Main menu");

    attractors_menu_id=add_menu(on_click_attractors_menu,display_attractors_menu,0,0,real_windows_x_len,real_windows_y_len,
             0,1,"Attractors menu");

    move_attractor_menu_id=add_menu(on_click_move_attractors,display_move_attractors,0,0,real_windows_x_len,real_windows_y_len,
             0,1,"Click to place the attractor");
    new_attractor_menu_id=add_menu(on_click_new_attractors,display_new_attractors,0,0,real_windows_x_len,real_windows_y_len,
             0,1,"Click to place the new attractor");
    exit_menu_id=add_menu(on_click_exit_menu,display_exit_menu,(real_windows_x_len-360)/2,(real_windows_y_len-120)/2,
             360,120,0,0,"Exit application?");
    edit_attractor_menu_id=add_menu(on_click_edit_attractor_menu,display_edit_attractor_menu,0,0,370,100,0,0,"Edit attractor");

    colors_menu_id=add_menu(on_click_colors_menu,display_colors_menu,(real_windows_x_len-750)/2,(real_windows_y_len-320)/2,750,320,0,0,"Colors");

    confirm_remove_menu_id=add_menu(on_click_remove_menu,display_remove_menu,(real_windows_x_len-440)/2,(real_windows_y_len-120)/2,
             440,120,0,0,"Remove all attractors?");

    add_button_to_menu(main_menu_id, 1, click_ok_main_menu, 20, menus[main_menu_id].y_len-50, 80, 44, "Ok");
    add_button_to_menu(main_menu_id, 1, click_cancel_main_menu, menus[main_menu_id].x_len-130, menus[main_menu_id].y_len-50,120, 44, "Cancel");
    add_button_to_menu(main_menu_id, 1, click_reset_button, 120, menus[main_menu_id].y_len-50,300, 44, "Reset particles");
    add_button_to_menu(main_menu_id, 1, click_attractors_button, 440, menus[main_menu_id].y_len-50,212, 44, "Attractors");
    add_button_to_menu(main_menu_id, 1, click_colors_main_menu, 20, menus[main_menu_id].y_len-110, 140, 44, "Colors");

    add_button_to_menu(attractors_menu_id,1,click_attractors_done,real_windows_x_len-260,real_windows_y_len-50,80,42,"Done");
    add_button_to_menu(attractors_menu_id,1,click_attractors_new_attractor,real_windows_x_len-260,real_windows_y_len-105,250,42,"New attractor");
    add_button_to_menu(attractors_menu_id,1,click_attractors_remove_all,real_windows_x_len-210,30,200,42,"Remove all");

    add_button_to_menu(exit_menu_id, 1, click_yes_exit_menu, 20, menus[exit_menu_id].y_len-50, 80, 44, "YES");
    add_button_to_menu(exit_menu_id, 1, click_no_exit_menu, menus[exit_menu_id].x_len-95, menus[exit_menu_id].y_len-50,80, 44, "NO");
    add_button_to_menu(edit_attractor_menu_id,1,click_edit_done,3,3,80,45,"Done");
    add_button_to_menu(edit_attractor_menu_id,1,click_edit_move,110,3,100,45,"Move");
    add_button_to_menu(edit_attractor_menu_id,1,click_edit_remove,230,3,130,45,"Remove");

    add_button_to_menu(colors_menu_id,1,click_colors_ok,(menus[colors_menu_id].x_len-80)/2,menus[colors_menu_id].y_len-50,80,42,"Ok");

    add_button_to_menu(confirm_remove_menu_id, 1, click_yes_remove_menu, 20, menus[confirm_remove_menu_id].y_len-50, 80, 44, "YES");
    add_button_to_menu(confirm_remove_menu_id, 1, click_no_remove_menu, menus[confirm_remove_menu_id].x_len-95, menus[confirm_remove_menu_id].y_len-50,80, 44, "NO");

    border_reflect_checkbox=add_checkbox_to_menu(0, 1, (border_behaviour==BORDER_REFLECT) ? 1 : 0, 15, 15, 40, 40, "Borders reflect");
    liquid_mode_checkbox=add_checkbox_to_menu(0, 1, (draw_tails) ? 1 : 0, 540, 15, 40, 40, "Liquid mode");
    random_mode_checkbox=add_checkbox_to_menu(0, 1, (use_random_mode) ? 1 : 0, 360, 15, 40, 40, "Random");

    particles_no_slider=add_horizontal_slider_to_menu(0, 1, particles_no, 10000, 1000000, 10, 70, 400, 40);
    border_speed_slider=add_horizontal_slider_to_menu(0, 1, border_attenuation*100, 1, 100, 10, 120, 400, 40);
    brush_distance_slider=add_horizontal_slider_to_menu(0, 1, brush_distance, 50, 300, 10, 170, 200, 40);
    brush_force_slider=add_horizontal_slider_to_menu(0, 1, brush_force, -200000, 200000, 400, 170, 100, 40);
    gravity_slider=add_horizontal_slider_to_menu(0, 1, gravity_constant*1000, 10, 200, 10, 220, 400, 40);
    attractor_gravity_slider=add_horizontal_slider_to_menu(edit_attractor_menu_id, 1, 0, -3000000, 3000000, 5, 50, 200, 40);

    color_r1_slider=add_horizontal_slider_to_menu(colors_menu_id, 1, color_r1, 0, 255, 15, 15, 255, 40);
    color_g1_slider=add_horizontal_slider_to_menu(colors_menu_id, 1, color_g1, 0, 255, 15, 70, 255, 40);
    color_b1_slider=add_horizontal_slider_to_menu(colors_menu_id, 1, color_b1, 0, 255, 15, 125, 255, 40);

    color_r2_slider=add_horizontal_slider_to_menu(colors_menu_id, 1, color_r2, 0, 255, 380, 15, 255, 40);
    color_g2_slider=add_horizontal_slider_to_menu(colors_menu_id, 1, color_g2, 0, 255, 380, 70, 255, 40);
    color_b2_slider=add_horizontal_slider_to_menu(colors_menu_id, 1, color_b2, 0, 255, 380, 125, 255, 40);


}

void resize_menus(float scale)
{
    int i,j;

    for(i=0;i<menus_no;i++)
    {
        if(!menus[i].all_screen)
        {
            menus[i].x*=scale;
            menus[i].x_len*=scale;
            menus[i].y*=scale;
            menus[i].y_len*=scale;

            //see if it's out of the screen, and if so, center it
            if(menus[i].x<0 || menus[i].y<0 || menus[i].x+menus[i].x_len>real_windows_x_len || menus[i].y+menus[i].y_len>real_windows_y_len)
            {
                menus[i].x=(real_windows_x_len-menus[i].x_len)/2;
                menus[i].y=(real_windows_y_len-menus[i].y_len)/2;
            }

        }

        for(j=0;j<MAX_BUTTONS;j++)
        if(menus[i].buttons[j].on_click)
        {
            if(!menus[i].all_screen)
            {
                menus[i].buttons[j].x*=scale;
                menus[i].buttons[j].y*=scale;
                menus[i].buttons[j].x_len*=scale;
                menus[i].buttons[j].y_len*=scale;
            }
            else
            if(menus_scale<1.0f)
                {
                    menus[i].buttons[j].x_len*=scale;
                    menus[i].buttons[j].y_len*=scale;
                }

        }

        for(j=0;j<MAX_HORIZONTAL_SLIDERS;j++)
        if(menus[i].horizontal_sliders[j].in_use)
        {
            menus[i].horizontal_sliders[j].x*=scale;
            menus[i].horizontal_sliders[j].y*=scale;
            menus[i].horizontal_sliders[j].x_len*=scale;
            menus[i].horizontal_sliders[j].y_len*=scale;
        }

        for(j=0;j<MAX_CHECK_BOXES;j++)
        if(menus[i].checkboxs[j].in_use)
        {
            menus[i].checkboxs[j].x*=scale;
            menus[i].checkboxs[j].y*=scale;
            menus[i].checkboxs[j].x_len*=scale;
            menus[i].checkboxs[j].y_len*=scale;
        }
    }
}

int display_menus()
{
    int i;
    for(i=0;i<menus_no;i++)
        if(menus[i].in_use)
        {
            menus[i].display_menu(i);
            return 1;
        }

    return 0;
}

void display_edit_attractor_menu(int menu_id)
{
    SDL_Rect rect;
    char str[128];
    int i;

    rect.x=menus[menu_id].x;
    rect.y=menus[menu_id].y;
    rect.w=menus[menu_id].x_len;
    rect.h=menus[menu_id].y_len;

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderDrawRect(renderer, &rect);

    for(i=0;i<MAX_BUTTONS;i++)
    if(menus[menu_id].buttons[i].on_click)
    display_button(menu_id,i,1);

    sprintf(str,"%i",menus[menu_id].horizontal_sliders[attractor_gravity_slider].cur_value);
    display_horizontal_slider(menu_id,attractor_gravity_slider,1,str);




}

void display_exit_menu(int menu_id)
{
    SDL_Rect rect;
    int font_size=1;
    int text_x_len;
    int i;

    if(menus_scale<0.9f)font_size=0;

    rect.x=menus[menu_id].x;
    rect.y=menus[menu_id].y;
    rect.w=menus[menu_id].x_len;
    rect.h=menus[menu_id].y_len;

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderDrawRect(renderer, &rect);

    text_x_len=strlen(menus[menu_id].title)*fonts[font_size].w/96;
    print_string(menus[menu_id].title,font_size,rect.x+(rect.w-text_x_len)/2,rect.y+10);

    //display buttons

    for(i=0;i<MAX_BUTTONS;i++)
    if(menus[menu_id].buttons[i].on_click)
    display_button(menu_id,i,1);
}

void display_remove_menu(int menu_id)
{
    SDL_Rect rect;
    int font_size=1;
    int text_x_len;
    int i;

    if(menus_scale<0.9f)font_size=0;

    rect.x=menus[menu_id].x;
    rect.y=menus[menu_id].y;
    rect.w=menus[menu_id].x_len;
    rect.h=menus[menu_id].y_len;

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderDrawRect(renderer, &rect);

    text_x_len=strlen(menus[menu_id].title)*fonts[font_size].w/96;
    print_string(menus[menu_id].title,font_size,rect.x+(rect.w-text_x_len)/2,rect.y+10);

    //display buttons

    for(i=0;i<MAX_BUTTONS;i++)
    if(menus[menu_id].buttons[i].on_click)
    display_button(menu_id,i,1);
}

int blur_thread(void *ptr)
{
    int x1, y1, x2, y2, line_skip;
    int offset;
    int r=0,g=0,b=0;
    int x,y;
    Uint8 *rgb_ptr = (Uint8 *)screen_buffer;
    int *parameters = (int *)ptr;

    x1=parameters[0];
    y1=parameters[1];
    x2=parameters[2];
    y2=parameters[3];
    line_skip=parameters[4];

    for(y=y1;y<y2;y+=line_skip+1)
    {
        offset=y*pitch/4;
        for(x=x1;x<x2;x++)
        {

                r=rgb_ptr[(offset+x)*4+2];
                g=rgb_ptr[(offset+x)*4+1];
                b=rgb_ptr[(offset+x)*4+0];

                r+=rgb_ptr[(offset+x-1)*4+2];
                g+=rgb_ptr[(offset+x-1)*4+1];
                b+=rgb_ptr[(offset+x-1)*4+0];

                r+=rgb_ptr[(offset+x+1)*4+2];
                g+=rgb_ptr[(offset+x+1)*4+1];
                b+=rgb_ptr[(offset+x+1)*4+0];

                r+=rgb_ptr[(offset+x)*4+pitch+2];
                g+=rgb_ptr[(offset+x)*4+pitch+1];
                b+=rgb_ptr[(offset+x)*4+pitch+0];

                r+=rgb_ptr[(offset+x)*4-pitch+2];
                g+=rgb_ptr[(offset+x)*4-pitch+1];
                b+=rgb_ptr[(offset+x)*4-pitch+0];
/*
                r+=rgb_ptr[(offset+x+1)*4+pitch+2];
                g+=rgb_ptr[(offset+x+1)*4+pitch+1];
                b+=rgb_ptr[(offset+x+1)*4+pitch+0];

                r+=rgb_ptr[(offset+x-1)*4+pitch+2];
                g+=rgb_ptr[(offset+x-1)*4+pitch+1];
                b+=rgb_ptr[(offset+x-1)*4+pitch+0];

                r+=rgb_ptr[(offset+x+1)*4-pitch+2];
                g+=rgb_ptr[(offset+x+1)*4-pitch+1];
                b+=rgb_ptr[(offset+x+1)*4-pitch+0];

                r+=rgb_ptr[(offset+x-1)*4-pitch+2];
                g+=rgb_ptr[(offset+x-1)*4-pitch+1];
                b+=rgb_ptr[(offset+x-1)*4-pitch+0];
*/
                //if(r==1)r=255;
                //if(g==1)g=255;
                //if(b==1)b=255;

                blur_buffer[(offset+x)*4+2]=r/blur_divider;
                blur_buffer[(offset+x)*4+1]=g/blur_divider;
                blur_buffer[(offset+x)*4+0]=b/blur_divider;
                blur_buffer[(offset+x)*4+3]=0;

                r=g=b=0;

            }
        }

        return 1;
}

void blur_threaded(int x1,int y1,int x2,int y2)
{
    int parameters_0[5];
    int parameters_1[5];
    int parameters_2[5];
    int parameters_3[5];
    int threadReturnValue;
    SDL_Thread *thread_0;
    SDL_Thread *thread_1;
    SDL_Thread *thread_2;
    SDL_Thread *thread_3;

    parameters_0[0]=x1;
    parameters_0[1]=y1;
    parameters_0[2]=x2;
    parameters_0[3]=y2;
    parameters_0[4]=3;
    thread_0=SDL_CreateThread(blur_thread, "blur_thread", (void *)parameters_0);

    parameters_1[0]=x1;
    parameters_1[1]=y1+1;
    parameters_1[2]=x2;
    parameters_1[3]=y2;
    parameters_1[4]=3;
    thread_1=SDL_CreateThread(blur_thread, "blur_thread", (void *)parameters_1);

    parameters_2[0]=x1;
    parameters_2[1]=y1+2;
    parameters_2[2]=x2;
    parameters_2[3]=y2;
    parameters_2[4]=3;
    thread_2=SDL_CreateThread(blur_thread, "blur_thread", (void *)parameters_2);

    parameters_3[0]=x1;
    parameters_3[1]=y1+3;
    parameters_3[2]=x2;
    parameters_3[3]=y2;
    parameters_3[4]=3;
    thread_3=SDL_CreateThread(blur_thread, "blur_thread", (void *)parameters_3);

    //kill the threads

    SDL_WaitThread(thread_0, &threadReturnValue);
    SDL_WaitThread(thread_1, &threadReturnValue);
    SDL_WaitThread(thread_2, &threadReturnValue);
    SDL_WaitThread(thread_3, &threadReturnValue);

    memcpy(screen_buffer,blur_buffer,pitch*windows_y_len);
    /*
    temp_pointer=(int*)screen_buffer;
    screen_buffer=(int*)blur_buffer;
    blur_buffer=(Uint8*)screen_buffer;
*/

}

static int build_acceleration_map_thread(void *ptr)
{
    float a,x2,y2,dist;
    int i,d,x,y,line_skip,offset;

    int *parameters = (int *)ptr;

    y=parameters[0];
    line_skip=parameters[1];

    for(;y<windows_y_len;y+=line_skip+1)
    {
        offset=y*windows_x_len;
        for(x=0;x<windows_x_len;x++)
        {
            for(i=0;i<attractors_no;i++)
            {
                x2=attractors[i].x;
                y2=attractors[i].y;

                d=(x2-x)*(x2-x)+(y2-y)*(y2-y);
                if(d<32)d=32;
                dist=sqrts[d];
            //dist=sqrt(d);
                a=(gravity_constant*attractors[i].mass)/(float)d;

                acceleration_map[offset].x+=((x2-x)/dist)*a;
                acceleration_map[offset].y+=((y2-y)/dist)*a;
            }
            offset++;
        }
    }

    return 0;
}

void build_acceleration_map()
{
    int parameters_0[2];
    int parameters_1[2];
    int parameters_2[2];
    int parameters_3[2];
    int threadReturnValue;
    SDL_Thread *thread_0;
    SDL_Thread *thread_1;
    SDL_Thread *thread_2;
    SDL_Thread *thread_3;

    memset(acceleration_map,0,windows_x_len*windows_y_len*sizeof(gravity));

    parameters_0[0]=0;
    parameters_0[1]=3;
    thread_0=SDL_CreateThread(build_acceleration_map_thread, "build_acceleration_map_thread", (void *)parameters_0);

    parameters_1[0]=1;
    parameters_1[1]=3;
    thread_1=SDL_CreateThread(build_acceleration_map_thread, "build_acceleration_map_thread", (void *)parameters_1);

    parameters_2[0]=2;
    parameters_2[1]=3;
    thread_2=SDL_CreateThread(build_acceleration_map_thread, "build_acceleration_map_thread", (void *)parameters_2);

    parameters_3[0]=3;
    parameters_3[1]=3;
    thread_3=SDL_CreateThread(build_acceleration_map_thread, "build_acceleration_map_thread", (void *)parameters_3);

    //kill the threads

    SDL_WaitThread(thread_0, &threadReturnValue);
    SDL_WaitThread(thread_1, &threadReturnValue);
    SDL_WaitThread(thread_2, &threadReturnValue);
    SDL_WaitThread(thread_3, &threadReturnValue);

}

static int build_sqrts_thread(void *ptr)
{
    int i;
    int *parameters = (int *)ptr;

    for(i=parameters[0];i<parameters[1];i++)
        sqrts[i]=sqrt(i);

    return 1;
}

void build_sqrts()
{
    int parameters_0[2];
    int parameters_1[2];
    int parameters_2[2];
    int parameters_3[2];
    int threadReturnValue;
    SDL_Thread *thread_0;
    SDL_Thread *thread_1;
    SDL_Thread *thread_2;
    SDL_Thread *thread_3;

    parameters_0[0]=0;
    parameters_0[1]=MAX_SQRTS/4;
    thread_0=SDL_CreateThread(build_sqrts_thread, "build_sqrts_thread", (void *)parameters_0);

    parameters_1[0]=MAX_SQRTS/4;
    parameters_1[1]=MAX_SQRTS/2;
    thread_1=SDL_CreateThread(build_sqrts_thread, "build_sqrts_thread", (void *)parameters_1);

    parameters_2[0]=MAX_SQRTS/2;
    parameters_2[1]=(MAX_SQRTS/4)*3;
    thread_2=SDL_CreateThread(build_sqrts_thread, "build_sqrts_thread", (void *)parameters_2);

    parameters_3[0]=(MAX_SQRTS/4)*3;
    parameters_3[1]=MAX_SQRTS;
    thread_3=SDL_CreateThread(build_sqrts_thread, "build_sqrts_thread", (void *)parameters_3);

    //kill the threads

    SDL_WaitThread(thread_0, &threadReturnValue);
    SDL_WaitThread(thread_1, &threadReturnValue);
    SDL_WaitThread(thread_2, &threadReturnValue);
    SDL_WaitThread(thread_3, &threadReturnValue);

}

static int particle_repulse_by_mouse_thread(void *ptr)
{
    int i;
    int dx;
    int dy;
    int d;
    float dist,a;
    int start_particle;
    int force;
    int* parameters=(int*)ptr;
    float real_force;
    int distance;

    start_particle=parameters[0];
    force=parameters[1];
    distance=parameters[2];

    force/=100;

    real_force=0.21973*force;

    if(pressure>0)real_force*=pressure;

    for(i=start_particle;i<start_particle+particles_no/4;i++)
    {
        dx=x_mouse_pos-particles[i].x;
        dy=y_mouse_pos-particles[i].y;
        d=dx*dx+dy*dy;
        if(d<distance*distance)
        {
            if(d<256)d=256;
            dist=sqrts[d];
            //a=(gravity_constant*force)/(float)d;
            a=real_force/(float)dist;

            particles[i].speed_x+=(dx/dist)*a;//divide by constant instead of dist for interesting effect. 50 is a good number
            particles[i].speed_y+=(dy/dist)*a;

        }
    }

    return 0;
}

void particle_repulse_by_mouse_threaded(int force, int distance)
{
    int parameters_0[5];
    int parameters_1[5];
    int parameters_2[5];
    int parameters_3[5];
    int threadReturnValue;
    SDL_Thread *thread_0;
    SDL_Thread *thread_1;
    SDL_Thread *thread_2;
    SDL_Thread *thread_3;
    int time_now;

    time_now=SDL_GetTicks();
    if(last_time_brush+25>time_now)return;

    last_time_brush=time_now;

    parameters_0[0]=0;
    parameters_0[1]=force;
    parameters_0[2]=distance;
    thread_0=SDL_CreateThread(particle_repulse_by_mouse_thread, "particle_repulse_by_mouse_thread", (void *)parameters_0);

    parameters_1[0]=particles_no/4;
    parameters_1[1]=force;
    parameters_1[2]=distance;
    thread_1=SDL_CreateThread(particle_repulse_by_mouse_thread, "particle_repulse_by_mouse_thread", (void *)parameters_1);


    parameters_2[0]=particles_no/2;
    parameters_2[1]=force;
    parameters_2[2]=distance;
    thread_2=SDL_CreateThread(particle_repulse_by_mouse_thread, "particle_repulse_by_mouse_thread", (void *)parameters_2);

    parameters_3[0]=particles_no/2+particles_no/4;
    parameters_3[1]=force;
    parameters_3[2]=distance;
    thread_3=SDL_CreateThread(particle_repulse_by_mouse_thread, "particle_repulse_by_mouse_thread", (void *)parameters_3);

    //kill the threads

    SDL_WaitThread(thread_0, &threadReturnValue);
    SDL_WaitThread(thread_1, &threadReturnValue);
    SDL_WaitThread(thread_2, &threadReturnValue);
    SDL_WaitThread(thread_3, &threadReturnValue);

}


static int all_particles_attract_thread(void *ptr)
{
    int i;
    int dx;
    int dy;
    int d;
    float dist,a;
    int start_particle;
    float force;
    int* parameters=(int*)ptr;

    start_particle=parameters[0];
    force=parameters[1];

    for(i=start_particle;i<start_particle+particles_no/4;i++)
    {
        dx=x_mouse_pos-particles[i].x;
        dy=y_mouse_pos-particles[i].y;
        d=dx*dx+dy*dy;

        {
            if(d<256)d=256;
            dist=sqrts[d];
            //a=(gravity_constant*force)/(float)d;
            //a=force/(float)dist;
            a=force/100;

            particles[i].speed_x+=(dx/dist)*a;//divide by constant instead of dist for interesting effect. 50 is a good number
            particles[i].speed_y+=(dy/dist)*a;

        }
    }

    return 0;
}

void all_particles_attract_threaded(float force, int distance)
{
    int parameters_0[5];
    int parameters_1[5];
    int parameters_2[5];
    int parameters_3[5];
    int threadReturnValue;
    SDL_Thread *thread_0;
    SDL_Thread *thread_1;
    SDL_Thread *thread_2;
    SDL_Thread *thread_3;
    int time_now;

    time_now=SDL_GetTicks();
    if(last_time_brush+30>time_now)return;

    last_time_brush=time_now;

    parameters_0[0]=0;
    parameters_0[1]=force;
    parameters_0[2]=distance;
    thread_0=SDL_CreateThread(all_particles_attract_thread, "all_particles_attract_thread", (void *)parameters_0);

    parameters_1[0]=particles_no/4;
    parameters_1[1]=force;
    parameters_1[2]=distance;
    thread_1=SDL_CreateThread(all_particles_attract_thread, "all_particles_attract_thread", (void *)parameters_1);


    parameters_2[0]=particles_no/2;
    parameters_2[1]=force;
    parameters_2[2]=distance;
    thread_2=SDL_CreateThread(all_particles_attract_thread, "all_particles_attract_thread", (void *)parameters_2);

    parameters_3[0]=particles_no/2+particles_no/4;
    parameters_3[1]=force;
    parameters_3[2]=distance;
    thread_3=SDL_CreateThread(all_particles_attract_thread, "all_particles_attract_thread", (void *)parameters_3);

    //kill the threads

    SDL_WaitThread(thread_0, &threadReturnValue);
    SDL_WaitThread(thread_1, &threadReturnValue);
    SDL_WaitThread(thread_2, &threadReturnValue);
    SDL_WaitThread(thread_3, &threadReturnValue);

}


void particle_scatter_by_mouse()
{
    int i;
    int dx;
    int dy;

    for(i=0;i<particles_no;i++)
    {
        dx=x_mouse_pos-particles[i].x;
        dy=y_mouse_pos-particles[i].y;

        if(dx*dx+dy*dy<50*50)
        {
            if(particles[i].x<30)
            particles[i].speed_x=(rand()%1000)/10.f;
                else
            if(particles[i].x+30>windows_x_len)
            particles[i].speed_x=-(rand()%1000)/10.f;
                else
            particles[i].speed_x=-100.0f+(rand()%2000)/10.f;

            if(particles[i].y<30)
            particles[i].speed_y=(rand()%1000)/10.f;
                else
            if(particles[i].y+30>windows_y_len)
            particles[i].speed_y=-(rand()%1000)/10.f;
                else
            particles[i].speed_y=-100.0f+(rand()%2000)/10.f;

        }
    }
}

void particle_scatter_thread(int add_movement)
{
    int i;

    if(add_movement)
    for(i=0;i<particles_no;i++)
    {
        particles[i].speed_x+=-10.0f+(rand()%200)/10.f;
        particles[i].speed_y+=-10.0f+(rand()%200)/10.f;
    }
    else
    for(i=0;i<particles_no;i++)
    {
        particles[i].speed_x=-10.0f+(rand()%200)/10.f;
        particles[i].speed_y=-10.0f+(rand()%200)/10.f;
    }
}

void load_fonts()
{
   char str[64];
   int i;
   SDL_Surface* our_surface;
   for(i=0;i<MAX_FONTS;i++)
   {
       sprintf(str,"font%i.bmp",i);
       our_surface=SDL_LoadBMP(str);
       if(our_surface==NULL)
       {
           fprintf(stderr, "Couldn't find font %s",str);
           break;
       }
       last_loaded_font++;

        fonts[i].cur_texture=SDL_CreateTextureFromSurface(renderer, our_surface);
        fonts[i].w=our_surface->w;
        fonts[i].h=our_surface->h;
        SDL_FreeSurface(our_surface);
   }
}

void print_string(char *str, int font, int char_xscreen, int char_yscreen)
{
	int char_x_size;
	int i,cur_char;
	SDL_Rect srcrect;
	SDL_Rect dstrect;

	char_x_size=fonts[font].w/95;
	//char_x_size=7;

    for(i=0;str[i];i++)
    {
        cur_char=str[i]-32;

        srcrect.x = char_x_size*cur_char;
        srcrect.y = 0;
        srcrect.w = char_x_size;
        srcrect.h = fonts[font].h;

        dstrect.x = char_xscreen+char_x_size*i;
        dstrect.y = char_yscreen;
        dstrect.w = char_x_size;
        dstrect.h = fonts[font].h;

        SDL_RenderCopy(renderer,fonts[font].cur_texture,&srcrect,&dstrect);
    }

}


void handle_mouse_movement()
{
#ifndef __ANDROID__
        if(button_l)
        {
            if(check_click_on_menu())
            return;

            //particle_scatter_by_mouse();
            particle_repulse_by_mouse_threaded(brush_force,brush_distance);

        }

        if(button_r)
        {
            //particle_repulse_by_mouse(-100000);
            particle_scatter_by_mouse();
        }
#endif // __ANDROID__
}

void handle_touch_movement()
{
    if(check_click_on_menu())
    return;

    particle_repulse_by_mouse_threaded(brush_force,brush_distance);
}

void init_particles()
{
    int i;
    //int rand_no;
    for(i=0;i<MAX_PARTICLES;i++)
    {
        particles[i].x=rand()%(windows_x_len-1);
        particles[i].y=rand()%(windows_y_len-1);

        particles[i].speed_x=0;
        particles[i].speed_y=0;
    }
}

void init_attractors()
{

    attractors[attractors_no].x=200;
    attractors[attractors_no].y=350;
    attractors[attractors_no].mass=-500000;
    if(menus_scale<0.9f)
    {
        attractors[attractors_no].x/=2;
        attractors[attractors_no].y/=2;
    }
    attractors_no++;

    attractors[attractors_no].x=600;
    attractors[attractors_no].y=300;
    attractors[attractors_no].mass=-600000;
    if(menus_scale<0.9f)
    {
        attractors[attractors_no].x/=2;
        attractors[attractors_no].y/=2;
    }
    attractors_no++;

    attractors[attractors_no].x=400;
    attractors[attractors_no].y=400;
    attractors[attractors_no].mass=2000000;
    if(menus_scale<0.9f)
    {
        attractors[attractors_no].x/=2;
        attractors[attractors_no].y/=2;
    }
    attractors_no++;

    attractors[attractors_no].x=400;
    attractors[attractors_no].y=250;
    attractors[attractors_no].mass=1000000;
    if(menus_scale<0.9f)
    {
        attractors[attractors_no].x/=2;
        attractors[attractors_no].y/=2;
    }
    attractors_no++;
}

static int draw_particles_thread(void* ptr)
{
    double delta_time=1;
    int i,offset;
    float ax;
    float ay;
    int start_particle,thread_id,particles_skip;
    int* parameters=(int*)ptr;
    Uint8 *rgb_ptr = (Uint8 *)screen_buffer;
    float time_squared_halved;

    start_particle=parameters[0];
    thread_id=start_particle;
    particles_skip=parameters[1];

    thread_times[thread_id].no++;

    thread_times[thread_id].time_end=SDL_GetTicks();
    if((thread_times[thread_id].time_end-thread_times[thread_id].time_begin))
    delta_time=(thread_times[thread_id].time_end-thread_times[thread_id].time_begin)/1000.0f;
    thread_times[thread_id].time_begin=thread_times[thread_id].time_end;
    if(delta_time>2)return 0;

    //add some ripples
    if(rand()%7==3)
    delta_time+=(float)(rand()%60)/1000.0f;
    time_squared_halved=delta_time*delta_time/2.0f;

    for(i=start_particle;i<particles_no;i+=particles_skip)
    {

        offset=(int)particles[i].y*windows_x_len+(int)particles[i].x;

        screen_buffer[offset]=colors[rgb_ptr[offset*4+3]];

        ax=acceleration_map[offset].x;
        ay=acceleration_map[offset].y;
        //ax=0;
        //ay=0;

        //x=v0*t+(a*t*t)/2;

        //particles[i].x+=particles[i].speed_x*delta_time+(ax*delta_time*delta_time)/2;
        //particles[i].y+=particles[i].speed_y*delta_time+(ay*delta_time*delta_time)/2;

        //particles[i].x+=particles[i].speed_x*delta_time+ax*time_squared_halved;
        //particles[i].y+=particles[i].speed_y*delta_time+ay*time_squared_halved;

        particles[i].x+=particles[i].speed_x*delta_time;
        particles[i].y+=particles[i].speed_y*delta_time;

        //v=v0+a*t;
        particles[i].speed_x=particles[i].speed_x+ax*delta_time;
        particles[i].speed_y=particles[i].speed_y+ay*delta_time;

        if(particles[i].x>=windows_x_len)
        {
            if(border_behaviour==BORDER_REFLECT)
            {
                particles[i].speed_x*=border_attenuation;
                particles[i].x=windows_x_len-1;
            }
            else
            {
                particles[i].x=0;
                particles[i].speed_x*=border_attenuation;
                particles[i].speed_y*=border_attenuation;
            }
        }
        else
        if(particles[i].x<0)
        {
            if(border_behaviour==BORDER_REFLECT)
            {
                particles[i].speed_x*=border_attenuation;
                particles[i].x=0;
            }
            else
            {
                particles[i].x=windows_x_len-1;
                particles[i].speed_x*=border_attenuation;
                particles[i].speed_y*=border_attenuation;
            }
        }

        if(particles[i].y>=windows_y_len-1)
        {
            if(border_behaviour==BORDER_REFLECT)
            {
                particles[i].speed_y*=border_attenuation;
                particles[i].y=windows_y_len-2;
            }
            else
            {
                particles[i].y=0;
                particles[i].speed_y*=border_attenuation;
                particles[i].speed_x*=border_attenuation;
            }
        }
        else
        if(particles[i].y<0)
        {
            if(border_behaviour==BORDER_REFLECT)
            {
                particles[i].speed_y*=border_attenuation;
                particles[i].y=0;
            }
            else
            {
                particles[i].y=windows_y_len-1;
                particles[i].speed_y*=border_attenuation;
                particles[i].speed_x*=border_attenuation;
            }
        }
    }
    thread_times[thread_id].no--;

    return 0;
}

void draw_threaded()
{
    int number_of_threads=4;

    int parameters_0[5];
    int parameters_1[5];
    int parameters_2[5];
    int parameters_3[5];

    int threadReturnValue;
    SDL_Thread *thread_0;
    SDL_Thread *thread_1;
    SDL_Thread *thread_2;
    SDL_Thread *thread_3;

    parameters_0[0]=0;
    parameters_0[1]=number_of_threads;
    thread_0=SDL_CreateThread(draw_particles_thread, "draw_particles_thread", (void *)parameters_0);

    parameters_1[0]=1;
    parameters_1[1]=number_of_threads;
    thread_1=SDL_CreateThread(draw_particles_thread, "draw_particles_thread", (void *)parameters_1);

    parameters_2[0]=2;
    parameters_2[1]=number_of_threads;
    thread_2=SDL_CreateThread(draw_particles_thread, "draw_particles_thread", (void *)parameters_2);

    parameters_3[0]=3;
    parameters_3[1]=number_of_threads;
    thread_3=SDL_CreateThread(draw_particles_thread, "draw_particles_thread", (void *)parameters_3);

    //kill the threads

    SDL_WaitThread(thread_0, &threadReturnValue);
    SDL_WaitThread(thread_1, &threadReturnValue);
    SDL_WaitThread(thread_2, &threadReturnValue);
    SDL_WaitThread(thread_3, &threadReturnValue);

}

void random_mode_init()
{
     int i;
     int rand_no;

     for(i=0;i<5;i++)
     {
         attractors[i].x=rand()%windows_x_len;
         attractors[i].y=rand()%windows_y_len;
         rand_no=(rand()%800)*1000;
         attractors[i].mass=-300000+rand_no;
     }
     attractors_no=i;

     border_attenuation=0.01f+(rand()%100)*0.01f;

     build_acceleration_map();

}

void random_mode()
{
    int i,rand_no,any_change=0;
    int time_now=SDL_GetTicks();

    if(last_time_random+5000>time_now)return;

    last_time_random=time_now;

    for(i=0;i<attractors_no;i++)
    {
        rand_no=rand()%100;
        if(rand_no<5)
        {
            //remove atractor
            attractors[i].mass=0;
            any_change=1;
        }
        else
        if(rand_no<17)
        {
            //make positive
            attractors[i].mass=700000;
            any_change=1;
        }
        else
        if(rand_no<25)
        {
            //make negative
            attractors[i].mass=-300000;
            any_change=1;
        }
        else
        if(rand_no<50)
        {
            //change location
            attractors[i].x+=-windows_x_len/40+rand()%(windows_x_len/20);
            attractors[i].y+=-windows_y_len/40+rand()%(windows_y_len/20);

            if(attractors[i].x<0)
                attractors[i].x=0;
            if(attractors[i].x>windows_x_len)
                attractors[i].x=windows_x_len;
            if(attractors[i].y<0)
                attractors[i].y=0;
            if(attractors[i].y>windows_y_len)
                attractors[i].y=windows_y_len;
            any_change=1;
        }
        else
        if(rand_no<70)
        {
            //change mass
            int rand_mass;

            rand_mass=(rand()%600)*100;
            attractors[i].mass+=-30000+rand_mass;

            if(attractors[i].mass<-300000)
            attractors[i].mass=-300000;
            if(attractors[i].mass>1200000)
            attractors[i].mass=1200000;
            any_change=1;
        }
        else
        if(rand_no<100)
            continue;
    }

    if(any_change)
    build_acceleration_map();

    if(rand()%100<4) particle_scatter_thread(1);
}

int main(int argc, char *argv[])
{
    SDL_Window *window;
    int cur_time_stamp;
    char str[100];

    int must_draw_menu;
    int delta_acc_time;
    int time_now;

    srand(time(0));
    get_configh_path();
    build_sqrts();

    SDL_Log("We started...");
    ground_gravity.x=0;


    //srand(time(0));
/*
{
    FILE *f = NULL;
    char str[128];
    for(i=32;i<127;i++)
    str[i-32]=i;
  	f=fopen("chars.txt","wb");
  	if(!f)return;

	fwrite(&str,95,1,f);
	fclose(f);
}
*/
#ifndef __ANDROID__
    //if(SDL_CreateWindowAndRenderer(windows_x_len, windows_y_len, 0, &window, &renderer) < 0)
    //exit(2);
    window=SDL_CreateWindow("1 million particles", 100, 100, windows_x_len, windows_y_len, 0);
    renderer=SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    //renderer=SDL_CreateRenderer(window, -1, 0);

#else
    //if(SDL_CreateWindowAndRenderer(0, 0, 0, &window, &renderer) < 0)
    //if(SDL_CreateWindowAndRenderer(windows_x_len, windows_y_len, 0, &window, &renderer) < 0)
    //exit(2);
    window=SDL_CreateWindow("1 million particles", 100, 100, windows_x_len, windows_y_len, 0);
    renderer=SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

#endif
    SDL_GetWindowSize(window, &real_windows_x_len, &real_windows_y_len);

#ifdef __ANDROID__
    if(real_windows_x_len<1281)
    {
        windows_x_len=real_windows_x_len;
        windows_y_len=real_windows_y_len;
    }
#endif // __ANDROID__

    rendering_texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,windows_x_len, windows_y_len);
    SDL_SetTextureBlendMode(rendering_texture,SDL_BLENDMODE_NONE);
    pitch=windows_x_len*4;
    screen_buffer=malloc(pitch*windows_y_len);
    blur_buffer=(Uint8*)malloc(pitch*windows_y_len);
    acceleration_map=malloc(windows_x_len*windows_y_len*sizeof(gravity));

	SDL_Log("Load and init stuff..");
    load_fonts();
    SDL_Log("Loaded fonts..");
    init_particles();
    if(load_config())have_conf=1;

    init_menus();
    if(have_conf)menus[main_menu_id].in_use=0;

    if(real_windows_x_len<800)
    {
        menus_scale=0.5f;
        resize_menus(menus_scale);
    }

    if(real_windows_x_len>=1600 && real_windows_x_len<=1920)
    {
        menus_scale=1.5f;
        resize_menus(menus_scale);
    }
    else
    if(real_windows_x_len>1920)
    {
        menus_scale=2.2f;
        resize_menus(menus_scale);
    }

    if(!have_conf)
    init_attractors();

    SDL_Log("Building acceleration map");
    build_acceleration_map();
    SDL_Log("Finished init...");

    #ifdef __ANDROID__
    Android_JNI_GetAccelerometerValues(last_accel_values);
    last_got_accel=SDL_GetTicks();
    #endif


    /* Main render loop */
    SDL_Event event;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    while(!done)
	{
        /* Check for events */
        while(SDL_PollEvent(&event))
		{
            if(event.type == SDL_QUIT)// || event.type == SDL_FINGERDOWN
			{
                done = 1;
            }
            if(event.type == SDL_KEYDOWN)
            {
                if(event.key.keysym.sym == SDLK_AC_BACK || event.key.keysym.sym ==SDLK_AC_BOOKMARKS)
                {
                    if(menus[main_menu_id].in_use)
                    {
                        menus[main_menu_id].in_use=0;
                        menus[exit_menu_id].in_use=1;
                    }
                    else
                    {
                        menus[main_menu_id].in_use=1;
                        if(edit_attractor_menu_id!=-1)menus[edit_attractor_menu_id].in_use=0;
                        menus[new_attractor_menu_id].in_use=0;
                        menus[attractors_menu_id].in_use=0;
                        menus[move_attractor_menu_id].in_use=0;

                    }
                }

                if(keys[SDL_SCANCODE_LSHIFT])
                {
                    l_shift=1;
                }

                if(keys[SDL_SCANCODE_LCTRL])
                {
                    l_ctrl=1;
                }
                if(keys[SDL_SCANCODE_L])
                {
                    menus[main_menu_id].in_use=1;
                }
                if(keys[SDL_SCANCODE_X])
                {
                    menus[main_menu_id].in_use=0;
                    menus[exit_menu_id].in_use=1;
                }


            }
            if(event.type == SDL_KEYUP)
            {
                if(!keys[SDL_SCANCODE_LSHIFT])
                {
                    l_shift=0;
                }

                if(!keys[SDL_SCANCODE_LCTRL])
                {
                    l_ctrl=0;
                }
            }

            if(event.type == SDL_MOUSEWHEEL)
            {
            }

            if(event.type == SDL_MULTIGESTURE)
            {
                x_mouse_pos=event.mgesture.x*windows_x_len;
                y_mouse_pos=event.mgesture.y*windows_y_len;

                if(last_gesture_time+100<event.mgesture.timestamp)
                {
                    last_gesture_time=event.mgesture.timestamp;
                    all_particles_attract_threaded(event.mgesture.dDist*-600000.0f, 1000);
                }

            }

    if(event.type == SDL_WINDOWEVENT)
    {
        if(event.window.event == SDL_WINDOWEVENT_ENTER)
        {
            have_mouse_focus=1;
            button_l=0;
            button_r=0;
        }

        if(event.window.event == SDL_WINDOWEVENT_LEAVE)
            {
                have_mouse_focus=0;
                button_l=0;
                button_r=0;
            }
    }

    if((event.type==SDL_FINGERMOTION || event.type==SDL_FINGERDOWN) && last_gesture_time+250<SDL_GetTicks())
    {

        if(event.type == SDL_FINGERDOWN)last_time_mouse_down=SDL_GetTicks();

        pressure=event.tfinger.pressure;

        finger_delta_x=event.tfinger.dx;
        finger_delta_y=event.tfinger.dy;

        if(!must_draw_menu)
        {
            x_mouse_pos=event.tfinger.x*windows_x_len;
            y_mouse_pos=event.tfinger.y*windows_y_len;
        }
        else
        {
            x_mouse_pos=event.tfinger.x*real_windows_x_len;
            y_mouse_pos=event.tfinger.y*real_windows_y_len;
        }

        handle_touch_movement();
    }

            if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
            {
                #ifndef __ANDROID__
                if(event.type == SDL_MOUSEBUTTONDOWN)last_time_mouse_down=SDL_GetTicks();
                #endif

                if(have_mouse_focus && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON (SDL_BUTTON_LEFT)))
                button_l++;
                else
                    {
                        button_l = 0;
                    }

                if(have_mouse_focus && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON (SDL_BUTTON_RIGHT)))
                button_r = 1;
                else
                button_r = 0;

                if(event.type == SDL_MOUSEBUTTONDOWN)
                if(have_mouse_focus)
                {
                    last_mouse_click = SDL_GetTicks();
                    handle_mouse_movement();
                }

            }

            if(event.type==SDL_MOUSEMOTION)
            {
                x_mouse_pos = event.motion.x;
                y_mouse_pos = event.motion.y;
                mouse_delta_x = event.motion.xrel;
                mouse_delta_y = event.motion.yrel;

                if(have_mouse_focus)
                handle_mouse_movement();

                if(event.type == SDL_MOUSEBUTTONDOWN)fprintf(stderr, "Mouse down in mouse down in motion check\n");

            }

            }

        time_now=SDL_GetTicks();

        #ifdef __ANDROID__
        Android_JNI_GetAccelerometerValues(cur_accel_values);
        delta_acc_time=time_now-last_got_accel;

        if(delta_acc_time)
        {
            accel_per_sec[0]=(cur_accel_values[0]-last_accel_values[0])*(1000.0f/delta_acc_time);
            accel_per_sec[1]=(cur_accel_values[1]-last_accel_values[1])*(1000.0f/delta_acc_time);
        }

        last_accel_values[0]=cur_accel_values[0];
        last_accel_values[1]=cur_accel_values[1];
        last_accel_values[2]=cur_accel_values[2];

        last_got_accel=time_now;

        if(!must_draw_menu)
        {
            if(accel_per_sec[0]>7.0f)particle_scatter_thread(0);
            else
            if(accel_per_sec[1]>5.0f)particle_scatter_thread(1);
        }
        #endif

        if(use_random_mode==1)
            random_mode();

        //handle_mouse_movement();
        SDL_SetRenderDrawColor(renderer,0,0,0,0);
        SDL_RenderClear(renderer);
        must_draw_menu=display_menus();
        if(!must_draw_menu)
        {
            if(!draw_tails)memset(screen_buffer,0,pitch*windows_y_len);

            draw_threaded();

            if(draw_tails)blur_threaded(1,1,windows_x_len-2,windows_y_len-2);
        }

        if(SDL_GetTicks()-last_delta_check_time>=100)
        {
            old_mouse_x=x_mouse_pos;
            old_mouse_y=y_mouse_pos;
            last_delta_check_time=SDL_GetTicks();
        }
/*
        cur_time_stamp=SDL_GetTicks();
        if(cur_time_stamp-last_time_stamp)
        sprintf(str,"FPS: %i",1000/(cur_time_stamp-last_time_stamp));
        print_string(str,0,windows_x_len-170,10);
        last_time_stamp=cur_time_stamp;
        sprintf(str,"dx: %i,dy: %i",x_mouse_pos-old_mouse_x,y_mouse_pos-old_mouse_y);
        print_string(str,0,windows_x_len-270,50);
*/
        /*sprintf(str,"Finger dx: %i, dy: %i, no_finger: %i",pixels_finger_delta_x,pixels_finger_delta_y,no_finger_motion);
        print_string(str,0,windows_x_len-240,40);*/
        if(!must_draw_menu)
        {
            SDL_UpdateTexture(rendering_texture, NULL, screen_buffer, pitch);
            //SDL_UpdateTexture(rendering_texture, NULL, blur_buffer, pitch);
            SDL_RenderCopy(renderer, rendering_texture, NULL, NULL);
        }

        cur_time_stamp=SDL_GetTicks();

        if(cur_time_stamp-last_time_stamp)
        sprintf(str,"FPS: %i",1000/(cur_time_stamp-last_time_stamp));
        print_string(str,0,real_windows_x_len-120,10);

/*
        sprintf(str,"Pressure: %f",pressure);
        print_string(str,0,real_windows_x_len-160,30);

        sprintf(str,"x: %f y: %f z: %f",cur_accel_values[0],cur_accel_values[1],cur_accel_values[2]);
        print_string(str,0,real_windows_x_len-300,50);

        sprintf(str,"x: %f y: %f z: %f",accel_per_sec[0],accel_per_sec[1],accel_per_sec[2]);
        print_string(str,0,real_windows_x_len-300,70);
*/

        last_time_stamp=cur_time_stamp;

        SDL_RenderPresent(renderer);

#ifndef __ANDROID__
		SDL_Delay(10);
#endif // __ANDROID__
    }

    save_config();
    exit(0);

}
