#ifndef RUNNER_H
#define RUNNER_H
#include "runner_interface.h"
#include <unistd.h>
typedef struct runner_t{
    char name[RUNNER_NAME_MAX_CHARS+1];
    char path[PATH_MAX];
    void* runner_so;
    void* private_data;
    runner_ops ops;
} runner_t;

runner_t* runner_create(const char* path);
void runner_pre_frame(runner_t* runner, GearboyCore *gearboy);
void runner_post_frame(runner_t* runner, GearboyCore *gearboy);
void runner_on_input_poll(runner_t* runner, GearboyCore *gearboy);

#endif