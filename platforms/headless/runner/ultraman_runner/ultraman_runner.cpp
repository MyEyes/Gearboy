#include "../../runner.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define ULTRAMAN_FORM_ADDR (0xC201)
#define ULTRAMAN_POSX_ADDR (0xC205)
#define ULTRAMAN_POSY_ADDR (0xC207)
#define ULTRAMAN_VELX_ADDR (0xC209)
#define ULTRAMAN_VELY_ADDR (0xC20A)
#define ULTRAMAN_LVL_TIMER_ADDR (0xC109)
#define ULTRAMAN_PLAYER_LIVES_ADDR (0xC108)
#define ULTRAMAN_PLAYER_HEARTS_ADDR (0xC10D)
#define ULTRAMAN_GAME_STATE_ADDR (0xC101)
#define ULTRAMAN_BLOCK_INPUTS_ADDR (0xC115)

#define ULTRAMAN_GAMESTATE_INTRO1 (1)
#define ULTRAMAN_GAMESTATE_INTRO2 (2)
#define ULTRAMAN_GAMESTATE_MAINMENU (3)
#define ULTRAMAN_GAMESTATE_LEVEL (5)
#define ULTRAMAN_GAMESTATE_LEVEL_PAUSED (6)
#define ULTRAMAN_GAMESTATE_LEVEL_FINISHED (9)

#define ATTEMPT_MAX_LEN (60*30)
#define GENERATION_NUM_ATTEMPTS (4096)
#define GENERATION_KEEP_TOP (32)

typedef struct{
    uint16_t x;
    uint16_t y;
} pos_t;

static pos_t level_exits[] =
{
    {.x = 12279, .y = 3658}
};

typedef struct{
    uint8_t game_state;
    uint8_t form;
    uint16_t posx;
    uint16_t posy;
    int16_t velx;
    int16_t vely;
    uint16_t lvl_timer;
    uint8_t player_lives;
    uint8_t player_hearts;
    uint8_t block_inputs;
} ultraman_game_state;

typedef struct{
    int level_index;
    int savestate_index;
    size_t currFrame;
    uint8_t inputs[ATTEMPT_MAX_LEN];
    uint64_t score;
} attempt_t;

typedef struct{
    int num_gen;
    int curr_attempt;
    attempt_t attempts[GENERATION_NUM_ATTEMPTS];
} generation_t;

typedef struct{
    int level_savestate_index;
    int level_index;
    uint8_t curr_game_state;
    bool pressed_pause;
    bool released_pause;
    ultraman_game_state* state;
    generation_t *curr_gen;
    attempt_t *curr_attempt;
    attempt_t *best_attempt;
} ultraman_runner_state;

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

generation_t *create_generation(generation_t* old_gen, int savestate, int level_index);

attempt_t *create_attempt(int savestate)
{
    printf("UMB Runner: Started new attempt\n");
    attempt_t *attempt = (attempt_t*)malloc(sizeof(attempt_t));
    memset(attempt, 0, sizeof(attempt_t));
    attempt->currFrame = 0;
    attempt->level_index = 0;
    attempt->savestate_index = savestate;
    attempt->score = 0;
    return attempt;
}

uint64_t score_attempt(attempt_t *attempt, ultraman_game_state* gstate)
{
    const uint64_t base_score = 0x1000000000000000;
    const uint64_t completion_bonus = 2*base_score;
    uint64_t result = base_score;
    uint64_t dx = abs(gstate->posx-level_exits[attempt->level_index].x);
    uint64_t dy = abs(gstate->posy-level_exits[attempt->level_index].y);
    result -= dx*dx+dy*dy;
    return result;
}

void run_main_menu(ultraman_game_state* state, GearboyCore *gearboy)
{
    if(!state->block_inputs)
    {
        gearboy->KeyPressed(Gameboy_Keys::Start_Key);
    }
    else
    {
        gearboy->KeyReleased(Gameboy_Keys::Start_Key);
    }
}

void gen_random_attempt(attempt_t* attempt)
{
    uint8_t prevInput = 0xcf;
    uint8_t wasPausing = 0;
    for(int i=0; i<ATTEMPT_MAX_LEN; i++)
    {
        uint8_t currInput = rand()&0xCF;
        if(currInput&(1<<7)) //Mark that was just paused
        {
            wasPausing = true;
        }
        else if(wasPausing && prevInput&(1<<7)) //We just paused and haven't released start yet
        {
            currInput &= 0x3F;
        }
        else if(wasPausing) //We just paused and have already released the start button, so we press again to unpause
        {
            currInput |= (1<<7); //Make sure to unpause
            wasPausing = false;
        }
        attempt->inputs[i] = currInput;
        prevInput = currInput;
    }
}

void step_attempt(ultraman_runner_state* state, attempt_t* attempt, GearboyCore *gearboy)
{
    if(attempt->currFrame==ATTEMPT_MAX_LEN)
        return;
    ultraman_game_state *gstate = state->state;
    uint8_t prevInput = 0xcf;
    if(attempt->currFrame>0)
    {
        prevInput = attempt->inputs[attempt->currFrame-1];
    }
    uint8_t currInput = attempt->inputs[attempt->currFrame];

    for(int i=0; i<8; i++)
    {
        uint8_t btnMask = 1<<i;
        if((prevInput&btnMask) && !(currInput&btnMask))
        {
            gearboy->KeyReleased((Gameboy_Keys)i);
        }
        else if(!(prevInput&btnMask) && (currInput&btnMask))
        {
            gearboy->KeyPressed((Gameboy_Keys)i);
        }
    }
    attempt->inputs[attempt->currFrame] = currInput;
    attempt->currFrame++;
}

void run_level(ultraman_runner_state* state, GearboyCore *gearboy)
{
    ultraman_game_state *gstate = state->state;
    if(state->level_savestate_index < 0)
    {
        state->level_savestate_index = 0;
        gearboy->SaveState(state->level_savestate_index);
        printf("UMB Runner: Found Level Start. Setting Savestate\n");
    }
    if(!state->curr_gen)
    {
        state->curr_gen = create_generation(state->curr_gen, state->level_savestate_index, state->level_index);
    }
    if(!state->curr_attempt || state->curr_attempt->currFrame==ATTEMPT_MAX_LEN)
    {
        if(state->curr_attempt)
        {
            state->curr_attempt->score = score_attempt(state->curr_attempt, gstate);
            printf("G %d - A %d: Score %lx\n",state->curr_gen->num_gen, state->curr_gen->curr_attempt, state->curr_attempt->score);
            state->curr_gen->curr_attempt++;
        }
        state->curr_attempt = &state->curr_gen->attempts[state->curr_gen->curr_attempt];
    }

    step_attempt(state, state->curr_attempt, gearboy);

    //printf("P: %d, R: %d\n", keyPressed, keyReleased);
    //printf("Pos X: %d Pos Y: %d Vel X: %d Vel Y: %d Form: %d\n", gstate->posx, gstate->posy, gstate->velx, gstate->vely, gstate->form);
}

void update_state(ultraman_game_state* state, GearboyCore *gearboy)
{
    Memory* memory = gearboy->GetMemory();
    uint8_t *RAM = memory->GetMemoryMap();
    state->posx = *(uint16_t*)(RAM+ULTRAMAN_POSX_ADDR);
    state->posy = *(uint16_t*)(RAM+ULTRAMAN_POSY_ADDR);
    state->velx = *(int16_t*)(RAM+ULTRAMAN_VELX_ADDR);
    state->vely = *(int16_t*)(RAM+ULTRAMAN_VELY_ADDR);
    state->form = RAM[ULTRAMAN_FORM_ADDR];
    state->game_state = RAM[ULTRAMAN_GAME_STATE_ADDR];
    state->block_inputs = RAM[ULTRAMAN_BLOCK_INPUTS_ADDR];
}


generation_t *create_generation(generation_t* old_gen, int savestate, int level_index)
{
    generation_t *gen = (generation_t*) malloc(sizeof(generation_t));
    memset(gen, 0, sizeof(*gen));
    if(!old_gen)
    {
        printf("UMB Runner: Seeding Gen 0\n");
        gen->num_gen = 0;
        gen->curr_attempt = 0;
        for(int i=0; i<GENERATION_NUM_ATTEMPTS; i++)
        {
            gen->attempts[i].level_index = level_index;
            gen->attempts[i].savestate_index = savestate;
            gen_random_attempt(&gen->attempts[i]);
        }
        printf("UMB Runner: Finished Seeding Gen 0\n");
    }
    return gen;
}

extern "C" int ultraman_runner_pre_frame(runner_t* runner, GearboyCore *gearboy)
{
    ultraman_runner_state *state = (ultraman_runner_state*)runner->private_data;
    update_state(state->state,gearboy);
    if(state->state->game_state != state->curr_game_state)
    {
        //printf("UMB Runner: Game state changed: %d->%d\n", state->curr_game_state, state->state->game_state);
        state->curr_game_state = state->state->game_state;
    }
    switch(state->curr_game_state)
    {
        case ULTRAMAN_GAMESTATE_MAINMENU:
        case ULTRAMAN_GAMESTATE_INTRO1:
        case ULTRAMAN_GAMESTATE_INTRO2:
            run_main_menu(state->state, gearboy);
            break;
        case ULTRAMAN_GAMESTATE_LEVEL:
        case ULTRAMAN_GAMESTATE_LEVEL_PAUSED:
            run_level(state, gearboy);
            break;
        case ULTRAMAN_GAMESTATE_LEVEL_FINISHED:
            printf("Finished Level");
            exit(0);
            break;
    }
    return 0;
}

extern "C" int ultraman_runner_post_frame(runner_t* runner, GearboyCore *gearboy)
{
    ultraman_runner_state *state = (ultraman_runner_state*)runner->private_data;
    return 0;
}

extern "C" int do_init(runner_t* runner)
{
    runner->ops.init = do_init;
    runner->ops.post_frame = ultraman_runner_post_frame;
    runner->ops.pre_frame = ultraman_runner_pre_frame;

    runner->private_data = (void*)malloc(sizeof(ultraman_runner_state));
    if(!runner->private_data)
    {
        perror("Couldn't alloc runner private_data");
        return -1;
    }
    ultraman_runner_state *state = (ultraman_runner_state*)runner->private_data;
    memset(state, 0, sizeof(*state));
    state->level_savestate_index = -1;
    state->state = (ultraman_game_state*)malloc(sizeof(ultraman_game_state));
    if(!state->state)
    {
        perror("Couldn't alloc game state");
        return -1;
    }
    return 0;
}