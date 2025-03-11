#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "znmsp.h"

#define SAFE_DELETE(ptr) if (ptr) { delete ptr; ptr = 0; }

typedef void* RendererHandle;