#include "logo/logo.h"
#include "common/io.h"
#include "common/printing.h"
#include "common/processing.h"
#include "common/textModifier.h"
#include "common/strutil.h"
#include "detection/media/media.h"
#include "detection/os/os.h"
#include "detection/terminalshell/terminalshell.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct FFLogoCachedLine {
    FFstrbuf chars;
    uint32_t width;
} FFLogoCachedLine;

static void logoLineCacheBuild(FFLogoLineCacheState* cache, const char* data, bool doColorReplacement);

static uint8_t getQuadrants(const char* ch) {
    if (strcmp(ch, "▘") == 0) return 1;
    if (strcmp(ch, "▝") == 0) return 2;
    if (strcmp(ch, "▖") == 0) return 4;
    if (strcmp(ch, "▗") == 0) return 8;
    if (strcmp(ch, "▀") == 0) return 1 | 2;
    if (strcmp(ch, "▄") == 0) return 4 | 8;
    if (strcmp(ch, "▌") == 0) return 1 | 4;
    if (strcmp(ch, "▐") == 0) return 2 | 8;
    if (strcmp(ch, "▛") == 0) return 1 | 2 | 4;
    if (strcmp(ch, "▜") == 0) return 1 | 2 | 8;
    if (strcmp(ch, "▙") == 0) return 1 | 4 | 8;
    if (strcmp(ch, "▟") == 0) return 2 | 4 | 8;
    if (strcmp(ch, "█") == 0) return 1 | 2 | 4 | 8;
    return 0;
}

static const char* getCharFromQuadrants(uint8_t q) {
    switch (q) {
        case 1: return "▘";
        case 2: return "▝";
        case 4: return "▖";
        case 8: return "▗";
        case 1|2: return "▀";
        case 4|8: return "▄";
        case 1|4: return "▌";
        case 2|8: return "▐";
        case 1|2|4: return "▛";
        case 1|2|8: return "▜";
        case 1|4|8: return "▙";
        case 2|4|8: return "▟";
        case 1|2|4|8: return "█";
        default: return " ";
    }
}

static uint8_t rotateQuadrants(uint8_t q, double alpha) {
    if (q == 0) return 0;
    if (q == 15) return 15;
    
    double cos_a = cos(alpha);
    double sin_a = sin(alpha);
    uint8_t new_q = 0;
    
    if (q & 1) {
        double rx = -0.5 * cos_a - (-0.5) * sin_a;
        double ry = -0.5 * sin_a + (-0.5) * cos_a;
        if (rx < 0 && ry < 0) new_q |= 1;
        else if (rx >= 0 && ry < 0) new_q |= 2;
        else if (rx < 0 && ry >= 0) new_q |= 4;
        else new_q |= 8;
    }
    if (q & 2) {
        double rx = 0.5 * cos_a - (-0.5) * sin_a;
        double ry = 0.5 * sin_a + (-0.5) * cos_a;
        if (rx < 0 && ry < 0) new_q |= 1;
        else if (rx >= 0 && ry < 0) new_q |= 2;
        else if (rx < 0 && ry >= 0) new_q |= 4;
        else new_q |= 8;
    }
    if (q & 4) {
        double rx = -0.5 * cos_a - 0.5 * sin_a;
        double ry = -0.5 * sin_a + 0.5 * cos_a;
        if (rx < 0 && ry < 0) new_q |= 1;
        else if (rx >= 0 && ry < 0) new_q |= 2;
        else if (rx < 0 && ry >= 0) new_q |= 4;
        else new_q |= 8;
    }
    if (q & 8) {
        double rx = 0.5 * cos_a - 0.5 * sin_a;
        double ry = 0.5 * sin_a + 0.5 * cos_a;
        if (rx < 0 && ry < 0) new_q |= 1;
        else if (rx >= 0 && ry < 0) new_q |= 2;
        else if (rx < 0 && ry >= 0) new_q |= 4;
        else new_q |= 8;
    }
    return new_q;
}

static const char* rotateChar(const char* ch, double alpha) {
    uint8_t q = getQuadrants(ch);
    if (q != 0) {
        return getCharFromQuadrants(rotateQuadrants(q, alpha));
    }

    if (strcmp(ch, "-") == 0 || strcmp(ch, "_") == 0 || strcmp(ch, "~") == 0 || strcmp(ch, "─") == 0) {
        double phi = 0.0 + alpha;
        phi = fmod(phi, M_PI);
        if (phi < 0) phi += M_PI;
        if (phi < M_PI / 8.0 || phi >= 7.0 * M_PI / 8.0) return "─";
        else if (phi < 3.0 * M_PI / 8.0) return "/";
        else if (phi < 5.0 * M_PI / 8.0) return "│";
        else return "\\";
    }
    if (strcmp(ch, "|") == 0 || strcmp(ch, "│") == 0 || strcmp(ch, "i") == 0 || strcmp(ch, "I") == 0 || strcmp(ch, "l") == 0) {
        double phi = M_PI_2 + alpha;
        phi = fmod(phi, M_PI);
        if (phi < 0) phi += M_PI;
        if (phi < M_PI / 8.0 || phi >= 7.0 * M_PI / 8.0) return "─";
        else if (phi < 3.0 * M_PI / 8.0) return "/";
        else if (phi < 5.0 * M_PI / 8.0) return "│";
        else return "\\";
    }
    if (strcmp(ch, "/") == 0) {
        double phi = M_PI / 4.0 + alpha;
        phi = fmod(phi, M_PI);
        if (phi < 0) phi += M_PI;
        if (phi < M_PI / 8.0 || phi >= 7.0 * M_PI / 8.0) return "─";
        else if (phi < 3.0 * M_PI / 8.0) return "/";
        else if (phi < 5.0 * M_PI / 8.0) return "│";
        else return "\\";
    }
    if (strcmp(ch, "\\") == 0) {
        double phi = 3.0 * M_PI / 4.0 + alpha;
        phi = fmod(phi, M_PI);
        if (phi < 0) phi += M_PI;
        if (phi < M_PI / 8.0 || phi >= 7.0 * M_PI / 8.0) return "─";
        else if (phi < 3.0 * M_PI / 8.0) return "/";
        else if (phi < 5.0 * M_PI / 8.0) return "│";
        else return "\\";
    }
    return ch;
}

static void getLogoDimensions(const char* data, bool doColorReplacement, uint32_t* outWidth, uint32_t* outHeight) {
    uint32_t maxW = 0;
    uint32_t h = 0;
    uint32_t w = 0;
    const char* ptr = data;
    while (*ptr != '\0') {
        if (*ptr == '\n' || (*ptr == '\r' && *(ptr + 1) == '\n')) {
            if (w > maxW) maxW = w;
            w = 0;
            h++;
            if (*ptr == '\r') ptr++;
            ptr++;
            continue;
        }
        if (*ptr == '\t') {
            w += 4;
            ptr++;
            continue;
        }
        if (*ptr == '\e' && *(ptr + 1) == '[') {
            ptr += 2;
            while (ffCharIsDigit(*ptr) || *ptr == ';') ptr++;
            if (isascii(*ptr)) ptr++;
            continue;
        }
        if (doColorReplacement && *ptr == '$') {
            ptr++;
            if (*ptr == '$' || *ptr == '\0') {
                w++;
                if (*ptr != '\0') ptr++;
                continue;
            }
            int index = *ptr - '1';
            if (index >= 0 && index < FASTFETCH_LOGO_MAX_COLORS) {
                ptr++;
                continue;
            }
            w++;
            continue;
        }
        uint8_t charWidth;
        uint8_t bytes = ffUtf8CharLenWidth(ptr, UINT32_MAX, &charWidth);
        w += charWidth;
        ptr += bytes;
    }
    if (w > maxW) maxW = w;
    if (w > 0) h++;
    *outWidth = maxW;
    *outHeight = h;
}

static void populateLogoGrid(const char* data, bool doColorReplacement, LogoCell* grid, uint32_t width, uint32_t height) {
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            LogoCell* cell = &grid[y * width + x];
            strcpy(cell->ch, " ");
            cell->color[0] = '\0';
            cell->width = 1;
        }
    }

    FFOptionsLogo* options = &instance.config.logo;
    char curColor[64] = "";
    if (doColorReplacement && !instance.config.display.pipe) {
        snprintf(curColor, sizeof(curColor), "\e[%sm", options->colors[0].chars);
    }

    uint32_t x = 0;
    uint32_t y = 0;
    const char* ptr = data;
    while (*ptr != '\0' && y < height) {
        if (*ptr == '\n' || (*ptr == '\r' && *(ptr + 1) == '\n')) {
            x = 0;
            y++;
            if (*ptr == '\r') ptr++;
            ptr++;
            continue;
        }
        if (*ptr == '\t') {
            for (int i = 0; i < 4 && x < width; i++) {
                LogoCell* cell = &grid[y * width + x];
                strcpy(cell->ch, " ");
                strcpy(cell->color, curColor);
                cell->width = 1;
                x++;
            }
            ptr++;
            continue;
        }
        if (*ptr == '\e' && *(ptr + 1) == '[') {
            const char* start = ptr;
            ptr += 2;
            while (ffCharIsDigit(*ptr) || *ptr == ';') ptr++;
            if (isascii(*ptr)) ptr++;
            size_t len = ptr - start;
            if (len < sizeof(curColor)) {
                memcpy(curColor, start, len);
                curColor[len] = '\0';
            }
            continue;
        }
        if (doColorReplacement && *ptr == '$') {
            ptr++;
            if (*ptr == '$' || *ptr == '\0') {
                if (x < width) {
                    LogoCell* cell = &grid[y * width + x];
                    strcpy(cell->ch, "$");
                    strcpy(cell->color, curColor);
                    cell->width = 1;
                    x++;
                }
                if (*ptr != '\0') ptr++;
                continue;
            }
            int index = *ptr - '1';
            if (index >= 0 && index < FASTFETCH_LOGO_MAX_COLORS) {
                snprintf(curColor, sizeof(curColor), "\e[%sm", options->colors[index].chars);
                ptr++;
                continue;
            }
            if (x < width) {
                LogoCell* cell = &grid[y * width + x];
                strcpy(cell->ch, "$");
                strcpy(cell->color, curColor);
                cell->width = 1;
                x++;
            }
            continue;
        }
        uint8_t charWidth;
        uint8_t bytes = ffUtf8CharLenWidth(ptr, UINT32_MAX, &charWidth);
        if (x < width) {
            LogoCell* cell = &grid[y * width + x];
            if (bytes < 5) {
                memcpy(cell->ch, ptr, bytes);
                cell->ch[bytes] = '\0';
            } else {
                strcpy(cell->ch, " ");
            }
            strcpy(cell->color, curColor);
            cell->width = charWidth;
            
            for (uint32_t w = 1; w < charWidth && (x + w) < width; w++) {
                LogoCell* nextCell = &grid[y * width + (x + w)];
                nextCell->ch[0] = '\0';
                nextCell->color[0] = '\0';
                nextCell->width = 0;
            }
            x += charWidth;
        }
        ptr += bytes;
    }
}

static void logoLineCacheClear(FFLogoLineCacheState* cache) {
    FF_LIST_FOR_EACH (FFLogoCachedLine, line, cache->lines) {
        ffStrbufDestroy(&line->chars);
    }
    ffListDestroy(&cache->lines);
    cache->nextLine = 0;
    cache->rightOffset = 0;
}

static void logoLineCachePush(const FFstrbuf* chars, uint32_t width, FFLogoLineCacheState* cache) {
    FFLogoCachedLine* line = FF_LIST_ADD(FFLogoCachedLine, cache->lines);
    if (width > 0) {
        ffStrbufInitCopy(&line->chars, chars);
        if (!instance.config.display.pipe) {
            ffStrbufAppendS(&line->chars, FASTFETCH_TEXT_MODIFIER_RESET);
        }
    } else {
        ffStrbufInit(&line->chars);
    }
    line->width = width;
}

void ffLogoUpdateSpin(void) {
    if (instance.state.logoGrid) {
        instance.state.logoSpinAngle += 0.08;
        logoLineCacheBuild(&instance.state.logoLineCache, NULL, false);
    }
}

static void logoLineCacheBuild(FFLogoLineCacheState* cache, const char* data, bool doColorReplacement) {
    FFOptionsLogo* options = &instance.config.logo;

    if (options->spin && !instance.config.display.pipe && isatty(STDOUT_FILENO)) {
        if (!instance.state.logoGrid && data && *data != '\0') {
            uint32_t gridW = 0, gridH = 0;
            getLogoDimensions(data, doColorReplacement, &gridW, &gridH);
            if (gridW > 0 && gridH > 0) {
                instance.state.logoGridWidth = gridW;
                instance.state.logoGridHeight = gridH;
                instance.state.logoGrid = malloc(gridW * gridH * sizeof(LogoCell));
                populateLogoGrid(data, doColorReplacement, instance.state.logoGrid, gridW, gridH);
                instance.state.logoSpinAngle = 0.0;

                double cx_orig = (gridW - 1) / 2.0;
                double cy_orig = (gridH - 1) / 2.0;
                instance.state.cx_orig = cx_orig;
                instance.state.cy_orig = cy_orig;

                double max_dist = 0;
                for (uint32_t y = 0; y < gridH; y++) {
                    for (uint32_t x = 0; x < gridW; x++) {
                        LogoCell* cell = &instance.state.logoGrid[y * gridW + x];
                        if (strcmp(cell->ch, " ") != 0 && cell->ch[0] != '\0') {
                            double dx = x - cx_orig;
                            double dy = (y - cy_orig) * 2.0;
                            double dist = sqrt(dx*dx + dy*dy);
                            if (dist > max_dist) max_dist = dist;
                        }
                    }
                }
                instance.state.rotLineWidth = (uint32_t)(2 * ceil(max_dist) + 3);
                instance.state.rotLineHeight = (uint32_t)(2 * ceil(max_dist / 2.0) + 2);
            }
        }
    }

    if (instance.state.logoGrid) {
        logoLineCacheClear(cache);
        
        uint32_t maxLineWidth = instance.state.rotLineWidth;
        uint32_t parsedHeight = instance.state.rotLineHeight;
        
        double cx_orig = instance.state.cx_orig;
        double cy_orig = instance.state.cy_orig;
        
        double cx_target = (maxLineWidth - 1) / 2.0;
        double cy_target = (parsedHeight - 1) / 2.0;
        double alpha = instance.state.logoSpinAngle;
        
        for (uint32_t i = 0; i < options->paddingTop; ++i) {
            logoLineCachePush(nullptr, 0, cache);
        }

        for (uint32_t y = 0; y < parsedHeight; y++) {
            FF_STRBUF_AUTO_DESTROY line = ffStrbufCreateA(256);
            if (!instance.config.display.pipe && instance.config.display.brightColor) {
                ffStrbufAppendS(&line, FASTFETCH_TEXT_MODIFIER_BOLT);
            }

            if ((options->position != FF_LOGO_POSITION_RIGHT) && options->paddingLeft > 0) {
                ffStrbufAppendNC(&line, options->paddingLeft, ' ');
            }

            char lastColor[64] = "";
            double dy_target = y - cy_target;
            
            for (uint32_t x = 0; x < maxLineWidth; x++) {
                double dx_target = x - cx_target;
                
                double dx_src = dx_target * cos(alpha) + 2.0 * dy_target * sin(alpha);
                double dy_src = dy_target * cos(alpha) - 0.5 * dx_target * sin(alpha);
                
                double sx_float = cx_orig + dx_src;
                double sy_float = cy_orig + dy_src;
                
                LogoCell cell;
                strcpy(cell.ch, " ");
                cell.color[0] = '\0';
                cell.width = 1;

                if (sx_float >= -2.0 && sx_float < (double)instance.state.logoGridWidth + 2.0 &&
                    sy_float >= -1.0 && sy_float < (double)instance.state.logoGridHeight + 1.0) {
                    
                    int x0 = (int)floor(sx_float);
                    int y0 = (int)floor(sy_float);
                    
                    LogoCell best_cell;
                    strcpy(best_cell.ch, " ");
                    best_cell.color[0] = '\0';
                    best_cell.width = 1;
                    double best_dist_sq = 1e9;
                    
                    for (int ny = y0; ny <= y0 + 1; ny++) {
                        for (int nx = x0 - 1; nx <= x0 + 2; nx++) {
                            if (nx >= 0 && nx < (int)instance.state.logoGridWidth && ny >= 0 && ny < (int)instance.state.logoGridHeight) {
                                LogoCell candidate = instance.state.logoGrid[ny * instance.state.logoGridWidth + nx];
                                if (candidate.width > 0 && strcmp(candidate.ch, " ") != 0 && candidate.ch[0] != '\0') {
                                    double dx_diff = nx - sx_float;
                                    double dy_diff = (ny - sy_float) * 2.0;
                                    double dist_sq = dx_diff * dx_diff + dy_diff * dy_diff;
                                    if (dist_sq < best_dist_sq) {
                                        best_dist_sq = dist_sq;
                                        best_cell = candidate;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (best_dist_sq <= 1.6) {
                        cell = best_cell;
                    } else {
                        int sx = (int)round(sx_float);
                        int sy = (int)round(sy_float);
                        if (sx >= 0 && sx < (int)instance.state.logoGridWidth && sy >= 0 && sy < (int)instance.state.logoGridHeight) {
                            cell = instance.state.logoGrid[sy * instance.state.logoGridWidth + sx];
                        }
                    }
                }
                
                if (cell.width > 0 && strcmp(cell.ch, " ") != 0 && cell.ch[0] != '\0') {
                    const char* rot = rotateChar(cell.ch, alpha);
                    
                    if (cos(alpha) < 0) {
                        if (strcmp(rot, "(") == 0) rot = ")";
                        else if (strcmp(rot, ")") == 0) rot = "(";
                        else if (strcmp(rot, "[") == 0) rot = "]";
                        else if (strcmp(rot, "]") == 0) rot = "[";
                        else if (strcmp(rot, "{") == 0) rot = "}";
                        else if (strcmp(rot, "}") == 0) rot = "{";
                        else if (strcmp(rot, "<") == 0) rot = ">";
                        else if (strcmp(rot, ">") == 0) rot = "<";
                    }
                    
                    if (strcmp(cell.color, lastColor) != 0) {
                        ffStrbufAppendS(&line, cell.color);
                        strcpy(lastColor, cell.color);
                    }
                    ffStrbufAppendS(&line, rot);
                } else {
                    if (lastColor[0] != '\0') {
                        ffStrbufAppendS(&line, FASTFETCH_TEXT_MODIFIER_RESET);
                        lastColor[0] = '\0';
                    }
                    ffStrbufAppendS(&line, " ");
                }
            }
            
            logoLineCachePush(&line, maxLineWidth, cache);
        }

        instance.state.logoHeight = options->paddingTop + parsedHeight;
        if (options->position == FF_LOGO_POSITION_LEFT) {
            instance.state.logoWidth = maxLineWidth + options->paddingRight;
        } else {
            instance.state.logoWidth = 0;
        }

        uint32_t totalLines = instance.state.logoHeight + 1;
        while (cache->lines.length < totalLines) {
            logoLineCachePush(nullptr, 0, cache);
        }

        cache->nextLine = 0;
        cache->rightOffset = maxLineWidth + options->paddingRight - 1;
        return;
    }

    bool keepCarryColor = options->type != FF_LOGO_TYPE_IMAGE_CHAFA;

    logoLineCacheClear(cache);

    // Overrides the auto detected max-width with the configured width, if set.
    // In case we fail to get the actual width of the logo, as we don't use `wcwidth`
    uint32_t maxLineWidth = options->width;
    uint32_t parsedHeight = 0;

    FF_STRBUF_AUTO_DESTROY carryColor = ffStrbufCreate();
    if (keepCarryColor && doColorReplacement && !instance.config.display.pipe) {
        ffStrbufSetF(&carryColor, "\e[%sm", options->colors[0].chars);
    }

    for (uint32_t i = 0; i < options->paddingTop; ++i) {
        logoLineCachePush(nullptr, 0, cache);
    }

    if (*data != '\0') {
        while (true) {
            FF_STRBUF_AUTO_DESTROY line = ffStrbufCreateA(256);
            uint32_t lineWidth = 0;

            if (!instance.config.display.pipe && instance.config.display.brightColor) {
                ffStrbufAppendS(&line, FASTFETCH_TEXT_MODIFIER_BOLT);
            }

            if (keepCarryColor && carryColor.length > 0) {
                ffStrbufAppend(&line, &carryColor);
            }

            if ((options->position != FF_LOGO_POSITION_RIGHT) && options->paddingLeft > 0) {
                ffStrbufAppendNC(&line, options->paddingLeft, ' ');
                lineWidth += options->paddingLeft;
            }

            while (*data != '\0' && *data != '\n' && !(*data == '\r' && *(data + 1) == '\n')) {
                if (*data == '\t') {
                    ffStrbufAppendNC(&line, 4, ' ');
                    lineWidth += 4;
                    ++data;
                    continue;
                }

                if (*data == '\e' && *(data + 1) == '[') {
                    const char* start = data;
                    data += 2;

                    while (ffCharIsDigit(*data) || *data == ';') {
                        ++data;
                    }

                    if (isascii(*data)) {
                        ++data;

                        uint32_t escLen = (uint32_t) (data - start);
                        ffStrbufAppendNS(&line, escLen, start);

                        if (keepCarryColor && start[escLen - 1] == 'm') {
                            ffStrbufSetNS(&carryColor, escLen, start);
                        }
                        continue;
                    }

                    lineWidth += (uint32_t) (data - start - 1);
                }

                if (doColorReplacement && *data == '$') {
                    ++data;

                    if (*data == '$' || *data == '\0') {
                        ffStrbufAppendC(&line, '$');
                        ++lineWidth;
                        ++data;
                        continue;
                    }

                    if (!instance.config.display.pipe) {
                        int index = *data - '1';
                        if (index >= 0 && index < FASTFETCH_LOGO_MAX_COLORS) {
                            if (keepCarryColor) {
                                ffStrbufSetF(&carryColor, "\e[%sm", options->colors[index].chars);
                                ffStrbufAppend(&line, &carryColor);
                            } else {
                                ffStrbufAppendF(&line, "\e[%sm", options->colors[index].chars);
                            }
                            ++data;
                            continue;
                        }

                        ffStrbufAppendC(&line, '$');
                        ++lineWidth;
                    } else {
                        ++data;
                        continue;
                    }
                }

                uint8_t charWidth;
                uint8_t bytes = ffUtf8CharLenWidth(data, UINT32_MAX, &charWidth);
                lineWidth += charWidth;

                for (uint8_t i = 0; i < bytes; ++i) {
                    if (*data == '\0') {
                        break;
                    }

                    ffStrbufAppendC(&line, *data);
                    ++data;
                }
            }

            logoLineCachePush(&line, lineWidth, cache);
            if (lineWidth > maxLineWidth) {
                maxLineWidth = lineWidth;
            }

            if (*data == '\n' || (*data == '\r' && *(data + 1) == '\n')) {
                if (*data == '\r') {
                    ++data;
                }
                ++data;
                ++parsedHeight;
                continue;
            }

            break;
        }
    }

    if (options->type != FF_LOGO_TYPE_IMAGE_CHAFA && options->height > parsedHeight) {
        parsedHeight = options->height;
    }

    instance.state.logoHeight = options->paddingTop + parsedHeight;
    if (options->position == FF_LOGO_POSITION_LEFT) {
        instance.state.logoWidth = maxLineWidth + options->paddingRight;
    } else {
        instance.state.logoWidth = 0;
    }

    uint32_t totalLines = instance.state.logoHeight + 1;
    while (cache->lines.length < totalLines) {
        logoLineCachePush(nullptr, 0, cache);
    }

    cache->nextLine = 0;
    cache->rightOffset = maxLineWidth + options->paddingRight - 1;
}

static bool ffLogoPrintCharsRaw(const char* data, size_t length, bool printError) {
    FFOptionsLogo* options = &instance.config.logo;
    FF_STRBUF_AUTO_DESTROY buf = ffStrbufCreate();

    if (!options->width || !options->height) {
        if (options->position == FF_LOGO_POSITION_LEFT) {
            ffStrbufAppendF(&buf, "\e[2J\e[3J\e[%u;%uH", (unsigned) options->paddingTop + 1, (unsigned) options->paddingLeft + 1);
        } else if (options->position == FF_LOGO_POSITION_TOP) {
            ffStrbufAppendNC(&buf, options->paddingTop, '\n');
            ffStrbufAppendNC(&buf, options->paddingLeft, ' ');
        } else if (options->position == FF_LOGO_POSITION_RIGHT) {
            if (!options->width) {
                if (printError) {
                    fputs("Logo (image-raw): Must set logo width when using position right\n", stderr);
                }
                return false;
            }
            ffStrbufAppendF(&buf, "\e[2J\e[3J\e[%u;9999999H\e[%uD", (unsigned) options->paddingTop + 1, (unsigned) options->paddingRight + options->width);
        }
        ffStrbufAppendNS(&buf, (uint32_t) length, data);
        ffWriteFDBuffer(FFUnixFD2NativeFD(STDOUT_FILENO), &buf);

        if (options->position == FF_LOGO_POSITION_LEFT || options->position == FF_LOGO_POSITION_RIGHT) {
            uint16_t X = 0, Y = 0;
            // Windows Terminal doesn't report `\e` for some reason
            const char* error = ffGetTerminalResponse("\e[6n", 2, "%*[^0-9]%hu;%huR", &Y, &X); // %*[^0-9]: ignore optional \e[
            if (error) {
                if (printError) {
                    fprintf(stderr, "\nLogo (image-raw): fail to query cursor position: %s\n", error);
                }
                return true;
            }
            if (options->position == FF_LOGO_POSITION_LEFT) {
                if (options->width + options->paddingLeft > X) {
                    X = (uint16_t) (options->width + options->paddingLeft);
                }
                instance.state.logoWidth = X + instance.config.logo.paddingRight - 1;
            }
            instance.state.logoHeight = Y;
            fputs("\e[H", stdout);
        } else if (options->position == FF_LOGO_POSITION_TOP) {
            instance.state.logoWidth = instance.state.logoHeight = 0;
            ffPrintCharTimes('\n', options->paddingRight);
        }
    } else {
        ffStrbufAppendNC(&buf, options->paddingTop, '\n');

        if (options->position == FF_LOGO_POSITION_RIGHT) {
            ffStrbufAppendF(&buf, "\e[9999999C\e[%uD", (unsigned) options->paddingRight + options->width);
        } else if (options->paddingLeft) {
            ffStrbufAppendF(&buf, "\e[%uC", (unsigned) options->paddingLeft);
        }

        ffStrbufAppendNS(&buf, (uint32_t) length, data);
        ffStrbufAppendC(&buf, '\n');

        if (options->position == FF_LOGO_POSITION_LEFT) {
            instance.state.logoWidth = options->width + options->paddingLeft + options->paddingRight;
            instance.state.logoHeight = options->paddingTop + options->height;
            ffStrbufAppendF(&buf, "\e[%uA", (unsigned) instance.state.logoHeight);
        } else if (options->position == FF_LOGO_POSITION_TOP) {
            instance.state.logoWidth = instance.state.logoHeight = 0;
            ffStrbufAppendNC(&buf, options->paddingRight, '\n');
        } else if (options->position == FF_LOGO_POSITION_RIGHT) {
            instance.state.logoWidth = instance.state.logoHeight = 0;
            ffStrbufAppendF(&buf, "\e[%uA", (unsigned) options->height);
        }
        ffWriteFDBuffer(FFUnixFD2NativeFD(STDOUT_FILENO), &buf);
    }

    return true;
}

void ffLogoPrintChars(const char* data, bool doColorReplacement) {
    FFOptionsLogo* options = &instance.config.logo;
    FFLogoLineCacheState* cache = &instance.state.logoLineCache;

    logoLineCacheBuild(cache, data, doColorReplacement);

    if (options->position != FF_LOGO_POSITION_TOP) {
        return;
    }

    FF_STRBUF_AUTO_DESTROY result = ffStrbufCreateA(4096);
    FF_LIST_FOR_EACH (FFLogoCachedLine, line, cache->lines) {
        ffStrbufAppend(&result, &line->chars);
        ffStrbufAppendC(&result, '\n');
    }
    ffStrbufAppendNC(&result, options->paddingBottom, '\n');
    ffWriteFDBuffer(FFUnixFD2NativeFD(STDOUT_FILENO), &result);
    instance.state.logoWidth = instance.state.logoHeight = 0;
    logoLineCacheClear(cache);
}

static void logoApplyColors(const FFlogo* logo, bool replacement) {
    if (instance.config.display.colorTitle.length == 0) {
        ffStrbufAppendS(&instance.config.display.colorTitle, logo->colorTitle ?: logo->colors[0]);
    }

    if (instance.config.display.colorKeys.length == 0) {
        ffStrbufAppendS(&instance.config.display.colorKeys, logo->colorKeys ?: logo->colors[1]);
    }

    if (replacement) {
        FFOptionsLogo* options = &instance.config.logo;

        const char* const* colors = logo->colors;
        for (int i = 0; *colors != nullptr && i < FASTFETCH_LOGO_MAX_COLORS; i++, colors++) {
            if (options->colors[i].length == 0) {
                ffStrbufAppendS(&options->colors[i], *colors);
            }
        }
    }
}

static bool logoHasName(const FFlogo* logo, const FFstrbuf* name, bool small) {
    for (
        const char* const* logoName = logo->names;
        *logoName != nullptr && logoName <= &logo->names[FASTFETCH_LOGO_MAX_NAMES];
        ++logoName) {
        if (small) {
            uint32_t logoNameLength = (uint32_t) (strlen(*logoName) - strlen("_small"));
            if (name->length == logoNameLength && strncasecmp(*logoName, name->chars, logoNameLength) == 0) {
                return true;
            }
        }
        if (ffStrbufIgnCaseEqualS(name, *logoName)) {
            return true;
        }
    }

    return false;
}

static const FFlogo* logoGetBuiltin(const FFstrbuf* name, FFLogoSize size) {
    if (name->length == 0 || !isalpha(name->chars[0])) {
        return nullptr;
    }

    for (const FFlogo* logo = ffLogoBuiltins[toupper(name->chars[0]) - 'A']; *logo->names; ++logo) {
        switch (size) {
            // Never use alternate logos
            case FF_LOGO_SIZE_NORMAL:
                if (logo->type != FF_LOGO_LINE_TYPE_NORMAL) {
                    continue;
                }
                break;
            case FF_LOGO_SIZE_SMALL:
                if (logo->type != FF_LOGO_LINE_TYPE_SMALL_BIT) {
                    continue;
                }
                break;
            default:
                break;
        }

        if (logoHasName(logo, name, size == FF_LOGO_SIZE_SMALL)) {
            return logo;
        }
    }

    return nullptr;
}

static const FFlogo* logoGetBuiltinDetected(FFLogoSize size) {
    const FFOSResult* os = ffDetectOS();

    const FFlogo* logo = logoGetBuiltin(&os->id, size);
    if (logo != nullptr) {
        return logo;
    }

    logo = logoGetBuiltin(&os->name, size);
    if (logo != nullptr) {
        return logo;
    }

    if (ffStrbufContainC(&os->idLike, ' ')) {
        FF_STRBUF_AUTO_DESTROY buf = ffStrbufCreate();
        for (
            uint32_t start = 0, end = ffStrbufFirstIndexC(&os->idLike, ' ');
            true;
            start = end + 1, end = ffStrbufNextIndexC(&os->idLike, start, ' ')) {
            ffStrbufSetNS(&buf, end - start, os->idLike.chars + start);
            logo = logoGetBuiltin(&buf, size);
            if (logo != nullptr) {
                return logo;
            }

            if (end >= os->idLike.length) {
                break;
            }
        }
    } else {
        logo = logoGetBuiltin(&os->idLike, size);
        if (logo != nullptr) {
            return logo;
        }
    }

    logo = logoGetBuiltin(&instance.state.platform.sysinfo.name, size);
    if (logo != nullptr) {
        return logo;
    }

    return &ffLogoUnknown;
}

static void logoPrintStruct(const FFlogo* logo) {
    logoApplyColors(logo, true);

    ffLogoPrintChars(logo->lines, true);
}

static void logoPrintNone(void) {
    if (!instance.config.display.pipe) {
        logoApplyColors(logoGetBuiltinDetected(FF_LOGO_SIZE_NORMAL), false);
    }
    instance.state.logoHeight = 0;
    instance.state.logoWidth = 0;
}

static bool logoPrintBuiltinIfExists(const FFstrbuf* name, FFLogoSize size) {
    if (name->chars[0] == '~' || name->chars[0] == '.' || name->chars[0] == '/'
#if _WIN32
        || (ffCharIsEnglishAlphabet(name->chars[0]) && name->chars[1] == ':') // Windows drive letter
#endif
    )
        return false; // Paths

    if (ffStrbufIgnCaseEqualS(name, "none")) {
        logoPrintNone();
        return true;
    }

    const FFlogo* logo = ffLogoGetBuiltinForName(name, size);
    if (logo == nullptr) {
        return false;
    }

    logoPrintStruct(logo);

    return true;
}

void ffLogoPrintDetected(FFLogoSize size) {
    logoPrintStruct(logoGetBuiltinDetected(size));
}

static bool logoPrintData(bool doColorReplacement, FFstrbuf* source) {
    if (source->length == 0) {
        return false;
    }

    logoApplyColors(logoGetBuiltinDetected(FF_LOGO_SIZE_NORMAL), doColorReplacement);
    ffLogoPrintChars(source->chars, doColorReplacement);
    return true;
}

static bool updateLogoPath(void) {
    FFOptionsLogo* options = &instance.config.logo;

    if (ffPathExists(options->source.chars, FF_PATHTYPE_FILE)) {
        return true;
    }

    if (ffStrbufEqualS(&options->source, "-")) { // stdin
        return true;
    }

#if !FF_MODULE_DISABLE_MEDIA
    if (ffStrbufIgnCaseEqualS(&options->source, "media-cover")) {
        const FFMediaResult* media = ffDetectMedia(true);
        if (media->cover.length == 0) {
            return false;
        }
        ffStrbufSet(&options->source, &media->cover);
        return true;
    }
#endif

    FF_STRBUF_AUTO_DESTROY fullPath = ffStrbufCreateA(128);
    if (ffPathExpandEnv(options->source.chars, &fullPath) && ffPathExists(fullPath.chars, FF_PATHTYPE_FILE)) {
        ffStrbufDestroy(&options->source);
        ffStrbufInitMove(&options->source, &fullPath);
        return true;
    }

    FF_LIST_FOR_EACH (FFstrbuf, dataDir, instance.state.platform.dataDirs) {
        // We need to copy it, because multiple threads might be using dataDirs at the same time
        ffStrbufSet(&fullPath, dataDir);
        ffStrbufAppendS(&fullPath, "fastfetch/logos/");
        ffStrbufAppend(&fullPath, &options->source);

        if (ffPathExists(fullPath.chars, FF_PATHTYPE_FILE)) {
            ffStrbufDestroy(&options->source);
            ffStrbufInitMove(&options->source, &fullPath);
            return true;
        }
    }

    return false;
}

static bool logoPrintFileIfExists(bool doColorReplacement, bool raw) {
    FFOptionsLogo* options = &instance.config.logo;

    FF_STRBUF_AUTO_DESTROY content = ffStrbufCreate();

    if (ffStrbufEqualS(&options->source, "-")
            ? !ffAppendFDBuffer(FFUnixFD2NativeFD(STDIN_FILENO), &content)
            : !ffAppendFileBuffer(options->source.chars, &content)) {
        if (instance.config.display.showErrors) {
            fprintf(stderr, "Logo: Failed to load file content from logo source: %s\n", options->source.chars);
        }
        return false;
    }

    logoApplyColors(logoGetBuiltinDetected(FF_LOGO_SIZE_NORMAL), doColorReplacement);
    if (raw) {
        return ffLogoPrintCharsRaw(content.chars, content.length, instance.config.display.showErrors);
    }

    ffLogoPrintChars(content.chars, doColorReplacement);
    return true;
}

static bool logoPrintImageIfExists(FFLogoType logo, bool printError) {
    if (!ffLogoPrintImageIfExists(logo, printError)) {
        return false;
    }

    logoApplyColors(logoGetBuiltinDetected(FF_LOGO_SIZE_NORMAL), false);
    return true;
}

static bool logoTryKnownType(void) {
    FFOptionsLogo* options = &instance.config.logo;

    if (options->type == FF_LOGO_TYPE_NONE) {
        logoPrintNone();
        return true;
    }

    if (options->type == FF_LOGO_TYPE_BUILTIN) {
        return logoPrintBuiltinIfExists(&options->source, FF_LOGO_SIZE_UNKNOWN);
    }

    if (options->type == FF_LOGO_TYPE_SMALL) {
        return logoPrintBuiltinIfExists(&options->source, FF_LOGO_SIZE_SMALL);
    }

    if (options->type == FF_LOGO_TYPE_DATA) {
        return logoPrintData(true, &options->source);
    }

    if (options->type == FF_LOGO_TYPE_DATA_RAW) {
        return logoPrintData(false, &options->source);
    }

    if (options->type == FF_LOGO_TYPE_COMMAND_RAW) {
        FF_STRBUF_AUTO_DESTROY source = ffStrbufCreate();

        const char* error = ffProcessAppendStdOut(&source, (char* const[]){
#ifdef _WIN32
                                                               "cmd.exe", "/c",
#else
                                                               "/bin/sh", "-c",
#endif
                                                               options->source.chars,
                                                               nullptr });

        if (error) {
            if (instance.config.display.showErrors) {
                fprintf(stderr, "Logo: failed to execute command `%s`: %s\n", options->source.chars, error);
            }
            return false;
        }

        return logoPrintData(false, &source);
    }

    // We sure have a file, resolve relative paths
    if (!updateLogoPath()) {
        if (instance.config.display.showErrors) {
            fprintf(stderr, "Logo: Failed to resolve logo source: %s\n", options->source.chars);
        }
        return false;
    }

    if (options->type == FF_LOGO_TYPE_FILE) {
        return logoPrintFileIfExists(true, false);
    }

    if (options->type == FF_LOGO_TYPE_FILE_RAW) {
        return logoPrintFileIfExists(false, false);
    }

    if (options->type == FF_LOGO_TYPE_IMAGE_RAW) {
        return logoPrintFileIfExists(false, true);
    }

    return logoPrintImageIfExists(options->type, instance.config.display.showErrors);
}

void ffLogoPrint(void) {
    const FFOptionsLogo* options = &instance.config.logo;

    if (options->type == FF_LOGO_TYPE_NONE) {
        logoPrintNone();
        return;
    }

    // If the source is not set, we can directly print the detected logo.
    if (options->source.length == 0) {
        ffLogoPrintDetected(options->type == FF_LOGO_TYPE_SMALL ? FF_LOGO_SIZE_SMALL : FF_LOGO_SIZE_NORMAL);
        return;
    }

    // If the source and source type is set to something else than auto, always print with the set type.
    if (options->source.length > 0 && options->type != FF_LOGO_TYPE_AUTO) {
        if (!logoTryKnownType()) {
            if (instance.config.display.showErrors) {
                // Image logo should have been handled
                if (options->type == FF_LOGO_TYPE_BUILTIN || options->type == FF_LOGO_TYPE_SMALL) {
                    fprintf(stderr, "Logo: Failed to load %s logo: %s\n", options->type == FF_LOGO_TYPE_BUILTIN ? "builtin" : "builtin small", options->source.chars);
                }
            }

            ffLogoPrintDetected(FF_LOGO_SIZE_UNKNOWN);
        }
        return;
    }

    // If source matches the name of a builtin logo, print it and return.
    if (logoPrintBuiltinIfExists(&options->source, FF_LOGO_SIZE_UNKNOWN)) {
        return;
    }

    // Make sure the logo path is set correctly.
    if (updateLogoPath()) {
        if (ffStrbufEndsWithIgnCaseS(&options->source, ".raw")) {
            if (logoPrintFileIfExists(false, true)) {
                return;
            }
        }

        if (!ffStrbufEndsWithIgnCaseS(&options->source, ".txt")) {
#if !FF_MODULE_DISABLE_TERMINAL
            const FFTerminalResult* terminal = ffDetectTerminal();

            bool supportsIterm2 = ffStrbufEqualS(&terminal->prettyName, "iTerm");

            if (supportsIterm2 && logoPrintImageIfExists(FF_LOGO_TYPE_IMAGE_ITERM, false)) {
                return;
            }

            // Terminal emulators that support kitty graphics protocol.
            bool supportsKitty =
                ffStrbufIgnCaseEqualS(&terminal->processName, "kitty") ||
                ffStrbufIgnCaseEqualS(&terminal->processName, "konsole") ||
                ffStrbufIgnCaseEqualS(&terminal->processName, "wezterm") ||
                ffStrbufIgnCaseEqualS(&terminal->processName, "wayst") ||
                ffStrbufIgnCaseEqualS(&terminal->processName, "ghostty") ||
    #ifdef __APPLE__
                ffStrbufIgnCaseEqualS(&terminal->processName, "WarpTerminal") ||
    #else
                ffStrbufIgnCaseEqualS(&terminal->processName, "warp") ||
    #endif
                false;
#else
            bool supportsKitty = false;
#endif

            // Try to load the logo as an image. If it succeeds, print it and return.
            if (logoPrintImageIfExists(supportsKitty ? FF_LOGO_TYPE_IMAGE_KITTY : FF_LOGO_TYPE_IMAGE_CHAFA, false)) {
                return;
            }
        }

        // Try to load the logo as a file. If it succeeds, print it and return.
        if (logoPrintFileIfExists(true, false)) {
            return;
        }
    } else {
        if (instance.config.display.showErrors) {
            fprintf(stderr, "Logo: Failed to resolve logo source: %s\n", options->source.chars);
        }
    }

    ffLogoPrintDetected(FF_LOGO_SIZE_UNKNOWN);
}

void ffLogoPrintLine(void) {
    FFLogoLineCacheState* cache = &instance.state.logoLineCache;
    FFOptionsLogo* logo = &instance.config.logo;

    if (cache->lines.length > 0) {
        // Line cache is enabled. Always move cursor with whitespaces to make lolcat happy
        if (cache->nextLine < cache->lines.length) {
            // Print logo line and move cursor
            FFLogoCachedLine* line = FF_LIST_GET(FFLogoCachedLine, cache->lines, cache->nextLine);

            if (logo->position == FF_LOGO_POSITION_RIGHT) {
                printf("\033[9999999C\033[%uD", cache->rightOffset);
                ffStrbufWriteTo(&line->chars, stdout);

                fputs("\033[G", stdout);
            } else {
                ffStrbufWriteTo(&line->chars, stdout);

                uint32_t remaining = instance.state.logoWidth;
                remaining = line->width < remaining ? remaining - line->width : 0;
                ffPrintCharTimes(' ', remaining);
            }
            ++cache->nextLine;
        } else if (logo->position == FF_LOGO_POSITION_LEFT) {
            // Move cursor to the start position
            ffPrintCharTimes(' ', instance.state.logoWidth);
        }
    } else if (instance.state.logoWidth > 0) {
        printf("\033[%uC", instance.state.logoWidth);
    }

    if (instance.state.dynamicInterval > 0 && logo->position == FF_LOGO_POSITION_LEFT) {
        fputs("\033[K", stdout); // Clear to the end of the line
        // Note that we don't clear the line when the logo is on the right, as it will also clear the logo itself
    }

    ++instance.state.keysHeight;
}

void ffLogoPrintRemaining(void) {
    FFLogoLineCacheState* cache = &instance.state.logoLineCache;
    FFOptionsLogo* logo = &instance.config.logo;

    if (cache->lines.length > 0 && (logo->position == FF_LOGO_POSITION_LEFT || logo->position == FF_LOGO_POSITION_RIGHT)) {
        while (cache->nextLine < cache->lines.length) {
            FFLogoCachedLine* line = FF_LIST_GET(FFLogoCachedLine, cache->lines, cache->nextLine);

            if (logo->position == FF_LOGO_POSITION_RIGHT) {
                printf("\033[9999999C\033[%uD", cache->rightOffset);
            }
            ffStrbufPutTo(&line->chars, stdout);

            ++cache->nextLine;
        }

        if (!instance.config.display.pipe) {
            fputs(FASTFETCH_TEXT_MODIFIER_RESET, stdout);
        }

        instance.state.keysHeight = instance.state.logoHeight + 1;
        logoLineCacheClear(cache);
        return;
    }

    if (instance.state.keysHeight <= instance.state.logoHeight) {
        ffPrintCharTimes('\n', instance.state.logoHeight - instance.state.keysHeight + 1);
    }
    instance.state.keysHeight = instance.state.logoHeight + 1;
}

void ffLogoBuiltinPrint(void) {
    FFOptionsLogo* options = &instance.config.logo;
    options->position = FF_LOGO_POSITION_TOP;
    options->paddingRight = 2; // empty line after logo printing
    FF_STRBUF_AUTO_DESTROY buf = ffStrbufCreate();

    for (uint8_t ch = 0; ch < 26; ++ch) {
        for (const FFlogo* logo = ffLogoBuiltins[ch]; *logo->names; ++logo) {
            if (instance.config.display.pipe) {
                ffStrbufSetF(&buf, "%s:\n", logo->names[0]);
            } else {
                ffStrbufSetF(&buf, "\e[%sm%s:\e[0m\n", logo->colors[0], logo->names[0]);
            }
            ffWriteFDBuffer(FFUnixFD2NativeFD(STDOUT_FILENO), &buf);
            logoPrintStruct(logo);

            for (uint8_t i = 0; i < FASTFETCH_LOGO_MAX_COLORS; i++) {
                ffStrbufClear(&options->colors[i]);
            }
        }
    }
}

void ffLogoBuiltinList(void) {
    uint32_t counter = 0;
    for (uint8_t ch = 0; ch < 26; ++ch) {
        for (const FFlogo* logo = ffLogoBuiltins[ch]; *logo->names; ++logo) {
            ++counter;
            printf("%u)%s ", counter, counter < 10 ? " " : "");

            for (
                const char* const* names = logo->names;
                *names != nullptr && names <= &logo->names[FASTFETCH_LOGO_MAX_NAMES];
                ++names) {
                printf("\"%s\" ", *names);
            }

            putchar('\n');
        }
    }
}

void ffLogoBuiltinListAutocompletion(void) {
    for (uint8_t ch = 0; ch < 26; ++ch) {
        for (const FFlogo* logo = ffLogoBuiltins[ch]; *logo->names; ++logo) {
            printf("%s\n", logo->names[0]);
        }
    }
}

const FFlogo* ffLogoGetBuiltinForName(const FFstrbuf* name, FFLogoSize size) {
    return ffStrbufEqualS(name, "?") ? &ffLogoUnknown : logoGetBuiltin(name, size);
}

const FFlogo* ffLogoGetBuiltinDetected(FFLogoSize size) {
    return logoGetBuiltinDetected(size);
}
