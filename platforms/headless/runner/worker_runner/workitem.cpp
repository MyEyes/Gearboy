#include "workitem.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
extern "C"
{
    workitem_t* create_workitem()
    {
        workitem_t *wi = (workitem_t*)malloc(sizeof(workitem_t));
        memset(wi,0,sizeof(*wi));
        wi->out_fd = -1;
        return wi;
    }

    void destroy_workitem(workitem_t* wi)
    {
        if(wi->out_fd>=0)
            close(wi->out_fd);
        if(wi->inputs)
            free(wi->inputs);
        if(wi->outdata)
            free(wi->outdata);
        free(wi);
    }

    int load_into_workitem(workitem_t* wi, char* path)
    {
        printf("Trying to load workitem from %s\n", path);
        int fd = open(path, O_RDONLY);
        if(fd<0)
        {
            perror("Couldn't open workitem file");
            return -1;
        }
        if(read(fd,wi->savestate_file,sizeof(wi->savestate_file))!=sizeof(wi->savestate_file))
        {
            perror("Couldn't read .savestate_file");
            return -1;
        }
        if(read(fd,wi->output_savestate,sizeof(wi->output_savestate))!=sizeof(wi->output_savestate))
        {
            perror("Couldn't read .output_savestate");
            return -1;
        }
        if(read(fd,wi->output_file,sizeof(wi->output_file))!=sizeof(wi->output_file))
        {
            perror("Couldn't read .output_file");
            return -1;
        }
        if(read(fd,&wi->num_inputs,sizeof(wi->num_inputs))!=sizeof(wi->num_inputs))
        {
            perror("Couldn't read .num_inputs");
            return -1;
        }
        wi->inputs = (uint8_t*) malloc(wi->num_inputs);
        if(!wi->inputs)
        {
            perror("Couldn't allocate .inputs");
            return -1;
        }
        if(read(fd,wi->inputs,wi->num_inputs)!=wi->num_inputs)
        {
            perror("Couldn't read .inputs");
            return -1;
        }

        if(read(fd,&wi->num_outdata,sizeof(wi->num_outdata))!=sizeof(wi->num_outdata))
        {
            perror("Couldn't read .num_outdata");
            return -1;
        }
        wi->outdata = (uint32_t*) calloc(wi->num_outdata,sizeof(uint32_t));
        if(!wi->outdata)
        {
            perror("Couldn't allocate .outdata");
            return -1;
        }
        if(read(fd,wi->outdata,wi->num_outdata*sizeof(uint32_t))!=wi->num_outdata*sizeof(uint32_t))
        {
            perror("Couldn't read .outdata");
            return -1;
        }
        if(strlen(wi->output_file)>0)
        {
            wi->out_fd = open(wi->output_file, O_RDWR|O_CREAT, 0700);
            if(wi->out_fd<0)
            {
                perror("Couldn't open output file");
                return -1;
            }
        }
        else 
        {
            printf("Need to specify output file\n");
            return -1;
        }
        return 0;
    }
}