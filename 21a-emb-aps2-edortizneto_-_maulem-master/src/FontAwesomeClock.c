/*******************************************************************************
 * Size: 24 px
 * Bpp: 4
 * Opts: 
 ******************************************************************************/
#define LV_LVGL_H_INCLUDE_SIMPLE
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef FONTAWESOMECLOCK
#define FONTAWESOMECLOCK 1
#endif

#if FONTAWESOMECLOCK

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F017 "" */
    0x0, 0x0, 0x0, 0x0, 0x15, 0x68, 0x76, 0x20,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4c,
    0xff, 0xff, 0xff, 0xfd, 0x70, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x1b, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xfe, 0x40, 0x0, 0x0, 0x0, 0x2, 0xef, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0x0, 0x0,
    0x0, 0x1e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x50, 0x0, 0x0, 0xcf, 0xff, 0xff,
    0xff, 0x20, 0xc, 0xff, 0xff, 0xff, 0xf2, 0x0,
    0x6, 0xff, 0xff, 0xff, 0xff, 0x10, 0xc, 0xff,
    0xff, 0xff, 0xfb, 0x0, 0xe, 0xff, 0xff, 0xff,
    0xff, 0x10, 0xc, 0xff, 0xff, 0xff, 0xff, 0x40,
    0x4f, 0xff, 0xff, 0xff, 0xff, 0x10, 0xc, 0xff,
    0xff, 0xff, 0xff, 0x90, 0x8f, 0xff, 0xff, 0xff,
    0xff, 0x10, 0xc, 0xff, 0xff, 0xff, 0xff, 0xe0,
    0xbf, 0xff, 0xff, 0xff, 0xff, 0x10, 0xc, 0xff,
    0xff, 0xff, 0xff, 0xf0, 0xcf, 0xff, 0xff, 0xff,
    0xff, 0x10, 0xb, 0xff, 0xff, 0xff, 0xff, 0xf2,
    0xcf, 0xff, 0xff, 0xff, 0xff, 0x10, 0x3, 0xdf,
    0xff, 0xff, 0xff, 0xf2, 0xbf, 0xff, 0xff, 0xff,
    0xff, 0x30, 0x0, 0x9, 0xff, 0xff, 0xff, 0xf0,
    0x9f, 0xff, 0xff, 0xff, 0xff, 0xe3, 0x0, 0x0,
    0xaf, 0xff, 0xff, 0xe0, 0x5f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x70, 0x1, 0xff, 0xff, 0xff, 0xa0,
    0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x1c,
    0xff, 0xff, 0xff, 0x50, 0x8, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0x0,
    0x0, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xf4, 0x0, 0x0, 0x3f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x70, 0x0,
    0x0, 0x4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xf9, 0x0, 0x0, 0x0, 0x0, 0x2d, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x60, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x7e, 0xff, 0xff, 0xff, 0xff,
    0xa1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x37, 0x9a, 0xa8, 0x50, 0x0, 0x0, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 384, .box_w = 24, .box_h = 24, .ofs_x = 0, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 61463, .range_length = 1, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t FontAwesomeClock = {
#else
lv_font_t FontAwesomeClock = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 24,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0)
    .underline_position = -9,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if FONTAWESOMECLOCK*/

