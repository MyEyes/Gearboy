#include "../../runner.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "workitem.h"
#include <sys/types.h>
#include <sys/stat.h>

#define FIFO_BASE_PATH "/tmp/gb_worker_"
#define FIFO_MAX_IDX 10
#define FIFO_FORMAT "%s%d_%s"

typedef struct{
    int fifo_in_fd;
    int fifo_out_fd;
    char *read_buf_pos;
    char read_buf[WORKITEM_INPUT_FILE_MAX_LEN];
    int curr_frame;
    workitem_t *curr_work;
} worker_runner_state;

void read_to_buf(worker_runner_state *state)
{
    size_t remaining_space = state->read_buf+WORKITEM_INPUT_FILE_MAX_LEN-state->read_buf_pos;
    size_t read_bytes = read(state->fifo_in_fd, state->read_buf_pos, remaining_space);
    state->read_buf_pos += read_bytes;
}

void consume_from_buf(worker_runner_state *state, size_t num_bytes)
{
    char* copy_start = state->read_buf+num_bytes;
    size_t copy_bytes = copy_start - state->read_buf_pos;
    memcpy(state->read_buf, copy_start, copy_bytes);
    state->read_buf_pos = state->read_buf+copy_bytes;
}

workitem_t* get_workitem(worker_runner_state *state)
{
    char path_buf[WORKITEM_INPUT_FILE_MAX_LEN] = {};
    workitem_t *new_workitem = create_workitem();
    size_t bufsize = state->read_buf_pos-state->read_buf;
    while(!bufsize || !memchr(state->read_buf,0,bufsize))
    {
        read_to_buf(state);
        bufsize = state->read_buf_pos-state->read_buf;
    }
    //We only ever get here if there's a 0 byte in the buffer
    strcpy(path_buf, state->read_buf);
    size_t n_bytes = strlen(path_buf)+1;
    consume_from_buf(state, n_bytes);
    if(load_into_workitem(new_workitem, path_buf)<0)
    {
        free(new_workitem);
        return NULL;
    }
    printf("Finished loading\n");
    return new_workitem;
}

int step_work(worker_runner_state *state, GearboyCore *gearboy)
{
    uint8_t prevInput = 0xcf;
    if(state->curr_frame>0)
    {
        prevInput = state->curr_work->inputs[state->curr_frame-1];
    }
    uint8_t currInput = state->curr_work->inputs[state->curr_frame];

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
    state->curr_frame++;
    return 0;
}

int write_outdata(worker_runner_state *state, GearboyCore *gearboy)
{
    Memory* memory = gearboy->GetMemory();
    uint8_t *RAM = memory->GetMemoryMap();
    for(int i=0; i<state->curr_work->num_outdata; i++)
    {
        uint32_t off = state->curr_work->outdata[i];
        uint8_t val = RAM[off];
        //printf("%x:%x\n", off, val);
        write(state->curr_work->out_fd, &val, 1);
    }
    return 0;
}

extern "C" int worker_runner_pre_frame(runner_t* runner, GearboyCore *gearboy)
{
    worker_runner_state *state = (worker_runner_state*)runner->private_data;
    while(!state->curr_work)
    {
        printf("Getting workitem\n");
        state->curr_work = get_workitem(state);
        state->curr_frame = 0;
    }
    if(state->curr_frame==0)
    {
        if(strlen(state->curr_work->savestate_file)>0)
        {
            gearboy->LoadState(state->curr_work->savestate_file,-1);
        }
    }
    write_outdata(state, gearboy);
    step_work(state, gearboy);

    return 0;
}

extern "C" int worker_runner_post_frame(runner_t* runner, GearboyCore *gearboy)
{
    worker_runner_state *state = (worker_runner_state*)runner->private_data;

    if(state->curr_frame>=state->curr_work->num_inputs)
    {
        printf("Workitem done: %d/%lu\n", state->curr_frame, state->curr_work->num_inputs);
        if(strlen(state->curr_work->output_savestate)>0)
        {
            printf("Writing savestate %s\n", state->curr_work->output_savestate);
            gearboy->SaveState(state->curr_work->output_savestate,-1);
        }
        //Signal that we finished
        write(state->fifo_out_fd,&state->curr_frame,sizeof(state->curr_frame));
        destroy_workitem(state->curr_work);
        state->curr_work = NULL;
    }

    return 0;
}

int init_fifos(worker_runner_state* state)
{
    char path_buf[256] = {0};
    state->fifo_in_fd = -1;
    state->fifo_out_fd = -1;
    int pid = getpid();

    snprintf(path_buf, sizeof(path_buf), FIFO_FORMAT, FIFO_BASE_PATH, pid, "in");
    if(mkfifo(path_buf, 0700)<0)
    {
        perror("Couldn't create in fifo");
        return -1;
    }
    
    printf("Waiting for other end of in pipe to be opened\n");
    state->fifo_in_fd = open(path_buf, O_RDONLY);
    if(state->fifo_in_fd<0)
    {
        remove(path_buf);
        perror("Couldn't open created in fifo");
        return -1;
    }

    //At this point incoming fifo is created and opened
    snprintf(path_buf, sizeof(path_buf), FIFO_FORMAT, FIFO_BASE_PATH, pid, "out");
    if(mkfifo(path_buf, 0700)<0)
    {
        //Cleanup first fifo
        snprintf(path_buf, sizeof(path_buf), FIFO_FORMAT, FIFO_BASE_PATH, pid, "in");
        remove(path_buf);
        perror("Couldn't create out fifo");
        return -1;
    }
    printf("Waiting for other end of out pipe to be opened\n");
    state->fifo_out_fd = open(path_buf, O_WRONLY);
    if(state->fifo_out_fd<0) //We created the fifo but couldn't open it for some reason
    {
        remove(path_buf); //Remove out fifo
        snprintf(path_buf, sizeof(path_buf), FIFO_FORMAT, FIFO_BASE_PATH, pid, "in");
        remove(path_buf); //Remove in fifo
        perror("Couldn't open out fifo");
        return -1;
    }
    //At this point both fifos should be created and opened
    return 0;
}

extern "C" int do_init(runner_t* runner)
{
    runner->ops.init = do_init;
    runner->ops.post_frame = worker_runner_post_frame;
    runner->ops.pre_frame = worker_runner_pre_frame;

    runner->private_data = (void*)malloc(sizeof(worker_runner_state));
    if(!runner->private_data)
    {
        perror("Couldn't alloc runner private_data");
        return -1;
    }
    worker_runner_state *state = (worker_runner_state*)runner->private_data;
    memset(state, 0, sizeof(*state));
    if(init_fifos(state)<0)
    {
        perror("Couldn't init fifos\n");
        return -1;
    }
    state->read_buf_pos = state->read_buf;
    return 0;
}