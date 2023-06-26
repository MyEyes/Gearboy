#include "../../runner.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define STARTLINE 2
#define P1_START 3
#define P1_END 11
#define P1_SIZE (P1_END-P1_START)

typedef struct{
    FILE *movie_input_file;
    bool key_state[P1_SIZE];
    int curr_frame;
} movie_runner_state;

Gameboy_Keys keymap[] =
{
    Gameboy_Keys::Up_Key,
    Gameboy_Keys::Down_Key,
    Gameboy_Keys::Left_Key,
    Gameboy_Keys::Right_Key,
    Gameboy_Keys::Start_Key,
    Gameboy_Keys::Select_Key,
    Gameboy_Keys::B_Key,
    Gameboy_Keys::A_Key
};

extern "C" int movie_runner_pre_frame(runner_t* runner, GearboyCore *gearboy)
{
    char* line = NULL;
    size_t length = 0;
    movie_runner_state *state = (movie_runner_state*)runner->private_data;
    if(getline(&line,&length,state->movie_input_file)>=0)
    {
        Memory* memory = gearboy->GetMemory();
        uint8_t *RAM = memory->GetMemoryMap();
        uint16_t posx = (RAM[0xC205])+(RAM[0xC206]<<8);
        uint16_t posy = (RAM[0xC207])+(RAM[0xC208]<<8);

        printf("%i: %s", state->curr_frame, line);
        printf("%i: PosX: %d, PosY: %d\n", state->curr_frame, posx, posy);
        for(int i=P1_START; i<P1_END; i++)
        {
            int btn_idx = i-P1_START;
            if(line[i]!='.')//button down
            {
                if(!state->key_state[btn_idx])
                {
                    gearboy->KeyPressed(keymap[btn_idx]);
                    state->key_state[btn_idx] = true;
                }
            }
            else //button up
            {
                if(state->key_state[btn_idx])
                {
                    gearboy->KeyReleased(keymap[btn_idx]);
                    state->key_state[btn_idx] = false;
                }
            }
        }
    }
    if(line)
        free(line);
    return 0;
}

extern "C" int movie_runner_post_frame(runner_t* runner, GearboyCore *gearboy)
{
    movie_runner_state *state = (movie_runner_state*)runner->private_data;
    state->curr_frame++;
    return 0;
}

extern "C" int movie_runner_on_input_poll(runner_t* runner, GearboyCore *gearboy)
{
    printf("polled\n");
    return 0;
}

extern "C" int do_init(runner_t* runner)
{
    runner->ops.init = do_init;
    runner->ops.post_frame = movie_runner_post_frame;
    runner->ops.pre_frame = movie_runner_pre_frame;
    //runner->ops.on_input_poll = movie_runner_on_input_poll;
    runner->private_data = (void*)malloc(sizeof(movie_runner_state));
    if(!runner->private_data)
    {
        perror("Couldn't alloc runner private_data");
        return -1;
    }
    movie_runner_state *state = (movie_runner_state*)runner->private_data;
    state->curr_frame = 0;
    state->movie_input_file = fopen("Input Log.txt", "r");
    memset(state->key_state,0,sizeof(state->key_state));
    if(!state->movie_input_file)
    {
        perror("Couldn't open movie file");
        return -1;
    }
    char *line = (char*)malloc(128);
    //Read away lines we don't care about
    for(int i=0; i<STARTLINE; i++)
    {
        size_t length = 128;
        getline(&line,&length,state->movie_input_file);
    }
    free(line);
    return 0;
}