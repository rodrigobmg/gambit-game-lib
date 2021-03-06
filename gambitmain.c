#include <stdlib.h>

#define ___VERSION 406006
#include <gambit.h>

#define SCHEME_LIBRARY_LINKER ____20_link__
___BEGIN_C_LINKAGE
extern ___mod_or_lnk SCHEME_LIBRARY_LINKER (___global_state_struct*);
___END_C_LINKAGE

extern int real_main(int argc, char ** argv);
extern void at_exit();

void gambit_cleanup() {
  ___cleanup();
}

int main(int argc, char ** argv) {
  ___setup_params_struct setup_params;
  ___setup_params_reset(&setup_params);
  setup_params.version = ___VERSION;
  setup_params.linker = SCHEME_LIBRARY_LINKER;
  setup_params.min_heap = 5000000;

  ___setup(&setup_params);

  int result = real_main(argc, argv);

  gambit_cleanup();

  return result;
}
