// server.cpp
#pragma once

#include "tcp_server.h"

int main()
{
  tcp_server_impl server_impl;
  server_impl.start();

  return 0;
}