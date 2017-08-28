#pragma once
#include "utils.h"

typedef int (*fileEntryCallback_func) (const char *name, const FILE_ENTRY *entry, void * opaque1, void * opaque2, int includeHidden);

// client can pass "opaque" pointer which will be feeded to the callback function
int traverseDir(const char* directory, fileEntryCallback_func f_callback, void * opaque1, void * opaque2, int includeHidden);



