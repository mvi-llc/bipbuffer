#pragma once

#include <catch2/catch_test_macros.hpp>

#define REQUIRE_NO_ERROR(result)                                                                   \
  do {                                                                                             \
    auto&& _result = (result);                                                                     \
    if (_result) { FAIL("[ERROR] " << _result->what()); }                                          \
  } while (false)
