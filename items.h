#ifndef ITEMS_H
#define ITEMS_H

#include "agent.h"
#include "config.h"
#include "updateable.h"

#include <stddef.h>

#define ITEM_MAX_NAME 12

typedef enum {
  HEAT,
  POWER,
  FUEL,
  SPACE,
  MAX_RESOURCE
} Resource;

extern char* resource_names[MAX_RESOURCE];

typedef enum {
  FIRE,
  IMPACT,
  MAX_ACTIVATION
} Activation;

typedef float Resources_[MAX_RESOURCE];
typedef float* Resources;

void resources_zero(Resources attr);

/* update cycle:
 * - consumers request from producers and producers reply until depleted
 * - producers release remaining mandatory productions to the system
 */

struct SystemInstance_;
struct ComponentInstance_;

typedef struct Stats_ {
  Resources_ storage;
  Resources_ max_capacity;
  Resources_ production_rates;
} *Stats;

typedef enum {
  PORT_NORTH,
  PORT_SOUTH,
  PORT_EAST,
  PORT_WEST,
  PORT_TOP,
  PORT_BOTTOM,
  PORT_MAX
} PortDirection;

typedef struct ComponentPort_ {
  struct Class _;
  struct DLLNode_ node;
  PortDirection direction;
  int offsetx;
  int offsety;
  int type;
} *ComponentPort;

void* ComponentPortClass;

struct ComponentClass {
  struct UpdateableClass_ _;

  void(*activate)(void* self, Activation activation);
  int(*push)(void* self, Resources resources);
  int(*pull)(void* self, Resources resources);

  struct DLLNode_ node; // keeps us in the class registry
  LLNode subcomponents; // non-intrusive list of subcomponents
  struct DLL_ ports; // possible ports on this class

  Resources_ requirements;
  struct Stats_ stats;
  float quality;
};

void activate(void* self, Activation activation);
int push(void* self, Resources resources);
int pull(void* self, Resources resources);

void items_init();

struct ComponentClass* componentclass_find(char *name);

struct ComponentInstance_;

typedef struct ComponentInstance_ {
  struct Object _;
  struct DLLNode_ node; // siblings
  struct ComponentInstance_* parent;
  struct DLL_ children;
  struct Stats_ stats;
  struct DLL_ ports; // port instances
  float quality;
} *ComponentInstance;

typedef struct ComponentPortInstance_ {
  struct Object _;
  struct DLLNode_ node;
  ComponentInstance component;
} *ComponentPortInstance;

extern void* ComponentClass;
extern void* ComponentObject;
extern void* ComponentSystemObject;
extern void* ComponentPortClass;
extern void* ComponentPortObject;

void componentinstance_addchild(ComponentInstance parent, ComponentInstance child);
void componentinstance_removechild(ComponentInstance child);
ComponentInstance componentinstance_findchild(ComponentInstance root, char* klass_name);

// temporary XML based file loading. I'll come up with something
// better once I've decided this system is a good idea at all.
void items_load_xml(char* filename);

#endif
