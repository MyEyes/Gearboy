#include "runner.h"
#include <dlfcn.h>
runner_t* runner_create(const char* path)
{
    void* runner_so = dlopen(path, RTLD_NOW);
    if(!runner_so)
    {
        Log("Couldn't open runner .so file");
        return NULL;
    }
    runner_init init_func = (runner_init)dlsym(runner_so, "do_init");
    if(!init_func)
    {
        Log("Couldn't resolve \"do_init\" symbol");
        return NULL;
    }
    runner_t *runner = (runner_t*)malloc(sizeof(runner_t));
    if(!runner)
    {
        Log("Couldn't allocate runner struct");
        return NULL;
    }
    runner->name[0] = 0; //TODO: Set name
    runner->runner_so = runner_so;
    strncpy(runner->path, path, sizeof(runner->path));
    init_func(&runner->ops);
    return runner;
}

void runner_pre_frame(runner_t* runner, GearboyCore *gearboy)
{
    if(runner && runner->ops.pre_frame)
        runner->ops.pre_frame(gearboy);
}

void runner_post_frame(runner_t* runner, GearboyCore *gearboy)
{
    if(runner && runner->ops.post_frame)
        runner->ops.post_frame(gearboy);
}