#ifndef _FLUFFY_ALIGN_H
#define _FLUFFY_ALIGN_H

#include <stdalign.h>

#define ALIGN(what, type) ((what) % alignof(type) == 0 ? (what) : (what) + (alignof(type) - ((what) % alignof(type))))

#endif
