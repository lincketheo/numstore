#include "compiler/scanner.hpp"
#include <cassert>
#include <cstddef>
#include <cstdio>

int main(int argc, char** argv) {
  assert(argv[1]);
  FILE* fp = fopen(argv[1], "r");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *buffer = (char*)malloc(len);
  fread(buffer, 1, len, fp);

  fprintf(stdout, "%s\n", buffer);


  result<token_arr> t = scan(buffer, len);
  assert(t.ok);

  token_arr_print(&t.value);

  free(buffer);
  fclose(fp);


  return 0;
}
