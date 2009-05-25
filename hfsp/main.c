#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hfsp.h"

void
cmd_info (hfsp_info *hfsp, int argc, char *argv[])
{
	
}

int
main (int argc, char *argv[])
{
	char *path = 0;
	unsigned int lba = 0x0;
	char **sub_argv = 0;
	int num_sub_args = 0;
	
	enum param {
		PARAM_NONE = 0,
		PARAM_LBA_OFFSET = 1,
		PARAM_IMAGE_PATH = 2,
		PARAM_CMD_ARG = 3
	} next_param;
	
	next_param = PARAM_NONE;
	
	int i;
	for (i=1;i<argc;i++)
	{
		switch (next_param)
		{
			case PARAM_NONE:
				if (strcmp(argv[i], "-o") == 0)
				{
					next_param = PARAM_LBA_OFFSET;
				} else if (strcmp(argv[i], "-f") == 0) {
					next_param = PARAM_IMAGE_PATH;					
				} else {
					next_param = PARAM_CMD_ARG;
					sub_argv = malloc(sizeof(char*)*(argc-i));					
					i--;
				}
				break;
			case PARAM_LBA_OFFSET:
				lba = strtol(argv[i], 0, 0);
				next_param = PARAM_NONE;
				break;
			case PARAM_IMAGE_PATH:
				path = argv[i];
				next_param = PARAM_NONE;
				break;
			case PARAM_CMD_ARG:
				sub_argv[num_sub_args++] = argv[i];
				break;
		}
	}
	
	FILE* f = fopen(path, "r");
	if (!f)
	{
		printf("fopen: failed opening %s\n", path);
		return 1;
	}

	hfsp_info *hfsp;
	hfsp = hfsp_mount(f, lba);

	if (hfsp)
	{
		if (num_sub_args)
		{
			if (strcmp(sub_argv[0], "info") == 0) {
				cmd_info(hfsp, num_sub_args-1, sub_argv+1);
			} else {
				printf("unknown command: %s!\n", sub_argv[0]);
			}				
		} else {
			printf("no command given\n");
		}

		hfsp_umount(hfsp);		
	}
	
	if (sub_argv) free(sub_argv);

	fflush(f);
	fclose(f);
	
	return 0;
}
