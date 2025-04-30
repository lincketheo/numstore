#pragma once

#include "compiler/scanner.h"
#include "dev/errors.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "intf/mm.h"
#include "intf/types.h"

#include <arpa/inet.h>
#include <poll.h>

typedef struct
{
  i_file cfd;
  struct sockaddr_in addr;
  socklen_t addr_len;

  bool want_read;
  bool want_write;
  bool want_close;

  cbuffer input;
  cbuffer output;

  u8 _input[10];
  u8 _output[10];

  scanner scanner;
} connection;
