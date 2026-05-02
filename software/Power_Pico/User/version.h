#ifndef VERSION_H
#define VERSION_H

#ifdef __cplusplus
extern "C" {
#endif


/***************************
 * CURRENT VERSION OF Power-Pico
 ***************************/
/**
 * total mechanical structure change will show in major
 * hardware change will show in major
 * software change will show in minor and patch
 *
 */
#define VERSION_MAJOR 1
#define VERSION_MINOR 1
#define VERSION_PATCH 2
#define VERSION_INFO "LCD flush optimization, async flush."

/**
 * No-Chicken的意思是no spring chicken any more...
 *
 */
#define VERSION_AUTHOR_ENG_NAME    "No-Chicken"
#define VERSION_AUTHOR_CN_NAME     "油炸鸡开源硬件"
#define VERSION_PROJECT_LINK       "https://github.com/No-Chicken/Power-Pico"
#define VERSION_HW_LINK            "https://oshwhub.com/no_chicken/powerpico"
#define VERSION_BILIBILI_LINK      "https://space.bilibili.com/34154740"


/**
 * Wrapper functions for VERSION macros
 */

static inline int pp_version_major(void)
{
    return VERSION_MAJOR;
}

static inline int pp_version_minor(void)
{
    return VERSION_MINOR;
}

static inline int pp_version_patch(void)
{
    return VERSION_PATCH;
}

static inline const char *pp_version_info(void)
{
    return VERSION_INFO;
}

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
