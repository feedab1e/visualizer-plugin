#include <dlfcn.h>

//
// Created by user on 11/13/23.
//

namespace plugin {
extern void open(const char *, unsigned);
}
int main(){
  int c = 0;
  plugin::open("127.0.0.1", 7576);
  for(;;) ++c;
}