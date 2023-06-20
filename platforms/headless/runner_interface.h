#ifndef RUNNER_INTERFACE_H
#define RUNNER_INTERFACE_H

#include "../../src/gearboy.h"

#define RUNNER_NAME_MAX_CHARS 31
typedef struct runner_ops runner_ops;

typedef int (*runner_init)(runner_ops* op_struct);
typedef int (*runner_pre_frame_func)(GearboyCore *gearboy);
typedef int (*runner_post_frame_func)(GearboyCore *gearboy);
typedef int (*runner_destructor)();

typedef struct runner_ops{
    runner_init init;
    runner_pre_frame_func pre_frame;
    runner_post_frame_func post_frame;
    runner_destructor destroy;
} runner_ops;
#endif