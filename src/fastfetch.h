#pragma once

#include "fastfetch_config.h"

#include <stdint.h>
#include <stdbool.h>

#include "common/arrutil.h"
#include "common/FFstrbuf.h"
#include "common/FFlist.h"
#include "common/FFPlatform.h"
#include "common/unused.h"

#include "options/logo.h"
#include "options/display.h"
#include "options/general.h"

typedef struct FFconfig {
    FFOptionsLogo logo;
    FFOptionsDisplay display;
    FFOptionsGeneral general;
} FFconfig;

typedef struct FFLogoLineCacheState {
    FFlist lines;
    uint32_t nextLine;
    uint32_t rightOffset;
} FFLogoLineCacheState;

typedef struct LogoCell {
    char ch[5];
    char color[64];
    uint8_t width;
} LogoCell;

typedef struct FFstate {
    uint32_t logoWidth;
    uint32_t logoHeight;
    uint32_t keysHeight;
    bool terminalLightTheme;
    bool titleFqdn;
    uint32_t dynamicInterval;
    FFPlatform platform;
    FFLogoLineCacheState logoLineCache;
    LogoCell* logoGrid;
    uint32_t logoGridWidth;
    uint32_t logoGridHeight;
    double logoSpinAngle;
    double cx_orig;
    double cy_orig;
    uint32_t rotLineWidth;
    uint32_t rotLineHeight;
} FFstate;

typedef struct FFinstance {
    FFconfig config;
    FFstate state;
} FFinstance;
extern FFinstance instance; // Defined in `common/init.c`
extern FFModuleBaseInfo** ffModuleInfos[];
