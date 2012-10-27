#ifndef TILES_H
#define TILES_H

#include "spriteatlas.h"
#include "testlib.h"

#include <stdio.h>

/*
 * IT = "In Tiles"
 * IP = "In Pixels"
 */

enum TileSpecMaskEntries {
  TILESPEC_COLLIDABLE = 1,
  TILESPEC_VISIBLE = 2
};

typedef struct TileSpec_ {
  SpriteAtlasEntry image;
  int bitmask;
} *TileSpec;

typedef struct TileMap_ {
  TileSpec tile_specs;
  int width_IT, height_IT;
  int tile_width_IP, tile_height_IP;
  float x_bl, y_bl;
  char tiles[0];
} *TileMap;

typedef struct TilePosition_ {
  int x, y;
} *TilePosition;

TileMap tilemap_make(int width, int height, int tw, int th);
void tilemap_free(TileMap map);

int tilemap_index(TileMap map, TilePosition pos);
void tileposition_tilemap(TilePosition pos, TileMap map, int index);
int tilemap_size(TileMap map);

TileMap tilemap_testmake(SpriteAtlas atlas);
SpriteList tilemap_spritelist(TileMap map, float x_bl, float y_bl, float wpx, float hpx);

typedef struct CharImage_ {
  int w, h;
  char* data;
} *CharImage;

#define charimage_index(img, x, y) (((img)->w * (y)) + x)
#define charimage_set(img, x, y, val) ((img)->data[charimage_index(img, x, y)] = val)
#define charimage_get(img, x, y) ((img)->data[charimage_index(img, x, y)])

void tileposition_charimage(TilePosition pos, CharImage img, int index);

// return true if the floodfill should include the given index
typedef int(*FloodfillCallback)(CharImage img, int index, void* udata);

int charimage_floodfill(CharImage out, CharImage input, TilePosition startpos,
                        char value, FloodfillCallback callback, void* udata);

void charimage_from_tilemap(CharImage img, TileMap map);
void charimage_crosscorrelate(CharImage out, CharImage big, CharImage small);
int charimage_size(CharImage img);
void charimage_replace_value(CharImage img, char from, char to);

void charimage_threshold(CharImage img, char min);

typedef struct LabelEntry_ {
  struct TilePosition_ pos;
  char value;
} *LabelEntry;

typedef void(*LabelCallback)(LabelEntry entries, int nentries, void* udata);

void charimage_label(CharImage img, char* working, LabelCallback callback, void* udata);

void charimage_write(CharImage img, FILE* target);
void charimage_spit(CharImage img, const char* filename);

#endif
