#pragma once

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4100)
#pragma warning(disable: 4505)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

#pragma warning(pop)