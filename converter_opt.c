
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "converter-src/toolbox.h"
#include "cmdutils.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>/*
#include "libavformat/avformat.h"

#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mathematics.h"
#include "libavutil/random_seed.h"
#include "libavutil/rational.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
*/
#include <stdarg.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#if HAVE_POLL_H
#include <poll.h>
#endif
#include <errno.h>
#include <time.h>
#include <signal.h>

#define WEB_API_LARGE_STRING 1024


const char program_name[] = "converter";
const int program_birth_year = 2017;

static int no_launch;

char source_filename[2048] = "";
char dest_filename[2048] = "";

static void opt_debug(void)
{  printf("opt_debug");
}
void show_help_default(const char *opt, const char *arg)
{
	printf("usage: converter -i input -o output\n");
    printf("\n");
}
