#pragma once
#include "common.h"
#include "server_hander.h"
#include "server.h"

#define BIND_FN(x) boost::bind(&self_type::x, shared_from_this())
#define BIND_FN1(x, y) boost::bind(&self_type::x, shared_from_this(), y)
#define BIND_FN2(x, y, z) boost::bind(&self_type::x, shared_from_this(), y, z)
