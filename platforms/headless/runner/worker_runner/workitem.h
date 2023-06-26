#ifndef WORKITEM_H
#define WORKITEM_H

#include <stdint.h>
#include <stdlib.h>
#define WORKITEM_SAVESTATE_FILE_MAX_LEN 128
#define WORKITEM_OUTPUT_FILE_MAX_LEN 256
#define WORKITEM_INPUT_FILE_MAX_LEN 256

typedef struct {
    char savestate_file[WORKITEM_SAVESTATE_FILE_MAX_LEN];
    char output_file[WORKITEM_OUTPUT_FILE_MAX_LEN];
    char output_savestate[WORKITEM_SAVESTATE_FILE_MAX_LEN];
    int out_fd;
    size_t num_inputs;
    uint8_t *inputs;
    size_t num_outdata;
    uint32_t *outdata;
} workitem_t;
extern "C"
{
    workitem_t* create_workitem();
    void destroy_workitem(workitem_t* wi);
    int load_into_workitem(workitem_t* wi, char* file);
}

#endif