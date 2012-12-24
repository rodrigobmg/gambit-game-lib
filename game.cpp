#include "game.h"
#include "vector.h"
#include "listlib.h"
#include "memory.h"
#include "particle.h"
#include "rect.h"
#include "controls.h"
#include "agent.h"
#include "steering.h"
#include "tiles.h"
#include "random.h"
#include "game_ui.h"

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

float bomb_delay = 6.0f;
float player_speed = 600;
float player_jump_speed = 1200;
float player_width = 64;
float player_height = 64;
float player_jump_height = 300;
float bomb_max_height = 400;
float bomb_dim = 32;
float player_gravity_accel;
float bomb_gravity_accel;
float throw_speed = 1200;
float ground_level = 100;
float charge_delay = 0.2;
float bomb_explode_start = 0.3;
float bomb_chain_factor = 5.0;
float enemy_speed = 100;
float enemy_dim = 40;

World* world;
struct Random_ rgen;

GO* player_go;
CInput* player_input;

GO* camera;

SpriteAtlas atlas;
SpriteAtlasEntry bomb_entry;

Clock main_clock;

int max_bombs = 50;
int current_n_bombs = 0;

int max_enemies = 20;
int current_n_enemies = 0;

enum Tags {
  TAG_NONE,
  TAG_PERMANENT
};

void camera_relative_enqueue(ColoredRect rect) {
  float dx = floorf(camera->_pos.x);
  float dy = floorf(camera->_pos.y);

  rect->minx -= dx;
  rect->maxx -= dx;
  rect->miny -= dy;
  rect->maxy -= dy;

  filledrect_enqueue_for_screen(rect);
}

void camera_relative_enqueue(Sprite sprite) {
  float dx = floorf(camera->_pos.x);
  float dy = floorf(camera->_pos.y);

  sprite->displayX -= dx;
  sprite->displayY -= dy;
  SpriteList list = frame_spritelist_append(NULL, sprite);
  spritelist_enqueue_for_screen(list);
}

OBJECT_IMPL(CTimer);

CTimer::CTimer()
  : Component(NULL, PRIORITY_THINK), time_remaining(0), expire_payload(NULL)
{}

CTimer::CTimer(GO* go, float time_remaining, void* payload)
  : Component(go, PRIORITY_THINK), time_remaining(time_remaining), expire_payload(payload) {
}

void CTimer::update(float dt) {
  this->time_remaining -= dt;

  if(this->time_remaining <= 0) {
    Message* message = message_make(go, MESSAGE_TIMER_EXPIRED, this->expire_payload);
    // need to add component priorities for this to be effective
    message_postinbox(go, message);
    delete this;
  }
}

enum BombStates {
  BOMB_IDLE,
  BOMB_EXPLODING,
  BOMB_DONE
};

OBJECT_IMPL(CBombBehavior);

CBombBehavior::CBombBehavior()
  : Component(NULL, PRIORITY_ACT), state(BOMB_IDLE) {
}

CBombBehavior::CBombBehavior(GO* go)
  : Component(go, PRIORITY_ACT), state(BOMB_IDLE) {
  new CTimer(go, bomb_delay - bomb_explode_start, NULL);
  this->state = BOMB_IDLE;
}

void CBombBehavior::update(float dt) {
  GO* bomb = this->go;

  // if we're supported then zero our x so that we're sticky
  if(bomb->transform_parent) {
    bomb->_vel.x = 0;
  }

  // nothing to do if a timer hasn't gone off
  int timer_expired = 0;
  bomb->inbox.foreach([&](Message* message) -> int {
      // look for an expired timer message or an explosion
      if(message->kind == MESSAGE_TIMER_EXPIRED
         || message->kind == MESSAGE_EXPLOSION_NEARBY) {
        timer_expired = 1;
        return 1;
      }
      return 0;
    });

  if(!timer_expired) return;

  if(this->state == BOMB_IDLE) {
    // set the next timer and change state
    new CTimer(bomb, bomb_explode_start, NULL);
    this->state = BOMB_EXPLODING;
    Vector_ pos;
    bomb->pos(&pos);
    world_foreach(bomb->world, &pos, bomb_dim * bomb_chain_factor, [&](GO* item) -> int {
        Message* message = message_make(bomb, MESSAGE_EXPLOSION_NEARBY, NULL);
        message_postinbox(item, message);
        return 0;
      });
  } else if(this->state == BOMB_EXPLODING) {
    // destroy the bomb
    this->state = BOMB_DONE;
    agent_send_terminate(bomb, bomb->world);
  }
}

OBJECT_IMPL(CEnemyBehavior);

enum EnemyState {
  ENEMY_FALLING,
  ENEMY_LANDED
};

CEnemyBehavior::CEnemyBehavior()
  : Component(NULL, PRIORITY_ACT), state(ENEMY_FALLING) {
}

CEnemyBehavior::CEnemyBehavior(GO* go)
  : Component(go, PRIORITY_ACT), state(ENEMY_FALLING) {
}

void CEnemyBehavior::update(float dt) {
  if(this->state == ENEMY_FALLING && this->go->transform_parent) {
    // when we land, add the left and right behavior
    this->state = ENEMY_LANDED;

    CCollidable* coll = (CCollidable*)go->transform_parent->find_component(&CCollidable::Type);
    this->go->_vel.x = enemy_speed;
    new CLeftAndRight(this->go, -coll->w / 2, coll->w / 2);
  } else if(this->state == ENEMY_LANDED && !this->go->transform_parent) {
    // when our platform disappears, go back to falling
    this->state = ENEMY_FALLING;
    this->go->_vel.x = 0;
    CLeftAndRight* lnr = (CLeftAndRight*)go->find_component(&CLeftAndRight::Type);
    delete lnr;
  }

  // see if a bomb went off
  go->inbox.foreach([this](Message* message) -> int {
      if(message->kind == MESSAGE_EXPLOSION_NEARBY) {
        agent_send_terminate(go, message->source);
        return 1;
      }
      return 0;
    });
}

OBJECT_IMPL(CLeftAndRight);

CLeftAndRight::CLeftAndRight()
  : Component(NULL, PRIORITY_ACT), minx(0), maxx(screen_width) {
}

CLeftAndRight::CLeftAndRight(GO* go, float minx, float maxx)
  : Component(go, PRIORITY_ACT), minx(minx), maxx(maxx) {
}

void CLeftAndRight::update(float dt) {
  if(go->_vel.x > 0) {
    if(go->_pos.x > this->maxx) {
      go->_pos.x = this->maxx;
      go->_vel.x = -go->_vel.x;
    }
  } else {
    if(go->_pos.x < this->minx) {
      go->_pos.x = this->minx;
      go->_vel.x = -go->_vel.x;
    }
  }
}

OBJECT_IMPL(CInput);

CInput::CInput()
  : Component(NULL, PRIORITY_THINK), state(NULL), fire_pressed(0), facing(1) {
}

CInput::CInput(GO* go)
  : Component(go, PRIORITY_THINK) {

  this->state = NULL;
  this->fire_pressed = 0;
  this->facing = 1;
}

void CInput::update(float dt) {
  InputState input = this->state;

  if(input->action2) {
    if(!this->fire_pressed) {
      this->fire_pressed = 1;
    }
  } else if(this->fire_pressed) {
    //fire
    this->fire_pressed = 0;

    if(current_n_bombs < max_bombs) {
      struct Vector_ abs_pos;
      go->pos(&abs_pos);
      abs_pos.y += player_height;

      struct Vector_ abs_vel = {this->facing * throw_speed / 3, throw_speed};

      if(go->transform_parent) {
        struct Vector_ par_vel;
        go->transform_parent->vel(&par_vel);
        vector_add(&abs_vel, &abs_vel, &par_vel);
      }

      bomb_make(&abs_pos, &abs_vel);
    }
  }

  go->_vel.x = input->leftright * player_speed;
  if(fabs(input->leftright) > 0.01) {
    this->facing = SIGN(input->leftright);
  }

  // dangerous conflation? we assume unparented means falling
  if(input->action1 && go->transform_parent) {
    go->_vel.y = player_jump_speed;
  }

  if(!input->action1 && !go->transform_parent) {
    go->_vel.y = MIN(go->_vel.y, 0);
  }
}

OBJECT_IMPL(CTestDisplay);

CTestDisplay::CTestDisplay()
  : Component(NULL, PRIORITY_SHOW), r(1.0), g(0.0), b(1.0) {
}

CTestDisplay::CTestDisplay(GO* go, float r, float g, float b)
  : Component(go, PRIORITY_SHOW), r(r), g(g), b(b) {
}

void CTestDisplay::update(float dt) {
  CCollidable* coll = (CCollidable*)go->find_component(&CCollidable::Type);

  assert(coll);

  struct ColoredRect_ rect;
  coll->rect(&rect);

  rect.color[0] = r;
  rect.color[1] = g;
  rect.color[2] = b;
  rect.color[3] = 1.0f;

  camera_relative_enqueue(&rect);
}

OBJECT_IMPL(CStaticSprite);

CStaticSprite::CStaticSprite()
  : Component(NULL, PRIORITY_SHOW), entry(NULL) {
}

CStaticSprite::CStaticSprite(GO* go, SpriteAtlasEntry entry)
  : Component(go, PRIORITY_SHOW), entry(entry) {
}

void CStaticSprite::update(float dt) {
  Vector_ pos;
  go->pos(&pos);

  Sprite sprite = frame_make_sprite();
  sprite_fillfromentry(sprite, entry);
  sprite->displayX = pos.x;
  sprite->displayY = pos.y;
  sprite->originX = 0.5;
  sprite->originY = 0.5;
  camera_relative_enqueue(sprite);
}

OBJECT_IMPL(CCameraFocus);

CCameraFocus::CCameraFocus()
  : Component(NULL, PRIORITY_SHOW) {
}

CCameraFocus::CCameraFocus(GO* go, GO* camera)
  : Component(go, PRIORITY_SHOW), camera(camera)  {
}

void CCameraFocus::update(float dt) {
  CInput* input = (CInput*)go->find_component(&CInput::Type);
  float facing_offset = screen_width / 4.0f * input->facing;
  float supported_offset = 0;

  if(input->state->updown) {
    supported_offset = screen_height / 2.0 * input->state->updown;
  } else {
    supported_offset = screen_height / 4.0;
  }

  if(input->state->leftright) {
    facing_offset = screen_width / 3.0f * input->state->leftright;
  }

  Vector_ offset = {screen_width / 2.0f - facing_offset, screen_height / 2.0f - supported_offset};
  Vector_ desired;
  go->pos(&desired);
  vector_sub(&desired, &desired, &offset);

  const float max_v = 600;
  const float max_dx = max_v * dt;

  Vector_ delta;
  vector_sub(&delta, &desired, &camera->_pos);
  float mag = vector_mag(&delta);
  if(mag < max_dx) {
    // snap
    camera->_pos = desired;
    return;
  }

  vector_scale(&delta, &delta, max_dx / mag);
  vector_add(&camera->_pos, &camera->_pos, &delta);

  /*
  Vector_ xv;
  vector_sub(&xv, &desired, &camera->_pos);
  float x = vector_mag(&xv);

  const float mass = 1;
  Vector_ spring_dv;
  const float kfactor = 10;
  vector_scale(&spring_dv, &xv, kfactor / mass * dt);

  Vector_ drag_dv;
  const float dfactor = 9;
  float speed = vector_mag(&camera->_vel);
  vector_scale(&drag_dv, &camera->_vel, -dfactor / mass * dt);

  vector_add(&camera->_vel, &camera->_vel, &spring_dv);
  vector_add(&camera->_vel, &camera->_vel, &drag_dv);
  */
}

GO* platform_make(float x, float y, float w, float h) {
  GO* go = new GO(world);
  go->_pos.x = x;
  go->_pos.y = y;
  go->ttag = TAG_PERMANENT;

  new CTestDisplay(go, 0.0, 0.7, 0.0);
  new CCollidable(go, w, h);
  return go;
}

GO* slidingplatform_make(float x, float y, float w, float h, float speed,
                         float minx, float maxx) {
  GO* go = platform_make(x, y, w, h);
  go->_vel.x = speed;

  new CLeftAndRight(go, minx, maxx);
  return go;
}

GO* enemy_make(float x, float y) {
  GO* go = new GO(world);
  go->_pos.x = x;
  go->_pos.y = y;

  new CTestDisplay(go, 0.7, 0.0, 0.5);
  new CCollidable(go, enemy_dim, enemy_dim);
  new CPlatformer(go, player_gravity_accel);
  new CEnemyBehavior(go);

  return go;
}

void player_setup() {
  camera = new GO(world);
  camera->_pos.x = 100;
  camera->_pos.y = 100;
  vector_zero(&camera->_vel);

  player_go = new GO(world);
  player_go->_pos.x = 100;
  player_go->_pos.y = 100;
  player_go->ttag = TAG_PERMANENT;

  new CTestDisplay(player_go, 0.0, 0.0, 1.0);
  new CCollidable(player_go, player_width, player_height);
  new CPlatformer(player_go, player_gravity_accel);
  new CCameraFocus(player_go, camera);

  player_input = new CInput(player_go);
}

GO* bomb_make(Vector pos, Vector vel) {
  GO* go = new GO(world);

  go->_pos = *pos;
  go->_vel = *vel;

  new CStaticSprite(go, bomb_entry);
  new CCollidable(go, bomb_dim, bomb_dim);
  new CPlatformer(go, bomb_gravity_accel);
  new CBombBehavior(go);

  return go;
}

void game_step(long delta, InputState state);

void game_support_init() {
  main_clock = clock_make();

  player_gravity_accel = (player_jump_speed * player_jump_speed) / (2 * player_jump_height);
  bomb_gravity_accel = (throw_speed * throw_speed) / (2 * bomb_max_height);

  atlas = spriteatlas_load("resources/images_default.dat", "resources/images_default.png");
  bomb_entry = spriteatlas_find(atlas, "bomb");

  world = new World();
}

void game_init() {
  game_support_init();
  player_setup();

  platform_make(screen_width / 2, 32, screen_width, 64);
  slidingplatform_make(300, 300, 256, 64, 100, 128, 1024);
  slidingplatform_make(600, 600, 256, 64, 100, 128, 1024);
  slidingplatform_make(300, 900, 256, 64, 100, 128, 1024);
  slidingplatform_make(600, 1200, 256, 64, 100, 128, 1024);
  slidingplatform_make(300, 1500, 256, 64, 100, 128, 1024);
  slidingplatform_make(600, 1800, 256, 64, 100, 128, 1024);
  slidingplatform_make(300, 2100, 256, 64, 100, 128, 1024);
  slidingplatform_make(600, 2400, 256, 64, 100, 128, 1024);

  set_game_step(game_step);
}

const float enemy_period = 1;
float enemy_timer = 1;

typedef enum {
  STATE_START,
  STATE_PLAY,
  STATE_WIN,
  STATE_LOSE
} WinState;

WinState win_state = STATE_START;
const float endgame_delay = 3;
float endgame_timeout;

void game_end(long delta, InputState state) {
  float dt = clock_update(main_clock, delta / 1000.0);
  if(win_state == STATE_WIN) {
    SpriteList text = spritelist_from_string(NULL, atlas, FONT_MEDIUM,
                                             "WINNER", 400, screen_height/2);
    spritelist_enqueue_for_screen(text);
  } else if(win_state == STATE_LOSE) {
    SpriteList text = spritelist_from_string(NULL, atlas, FONT_MEDIUM,
                                             "LOSER", 400, screen_height/2);
    spritelist_enqueue_for_screen(text);
  }

  endgame_timeout -= dt;
  if(endgame_timeout <= 0) {
    win_state = STATE_START;
    struct Vector_ center = {screen_width / 2.0f, screen_height / 2.0f};
    world_foreach(world, &center, INFINITY, [] (GO* go) -> int {
        if(go->ttag != TAG_PERMANENT) {
          agent_send_terminate(go, world);
        }
        return 0;
      });
    set_game_step(game_step);
  }
}

void game_step(long delta, InputState state) {
  float dt = clock_update(main_clock, delta / 1000.0);
  enemy_timer -= dt;
  if(enemy_timer <= 0 && current_n_enemies < max_enemies) {
    enemy_timer = enemy_period;

    enemy_make(screen_width / 2, screen_height * 4);
    // add enemy
  }


  player_input->state = state;
  world_notify_collisions(world);
  world->update(dt);
}

void game_shutdown() {
}
