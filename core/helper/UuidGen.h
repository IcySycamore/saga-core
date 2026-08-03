#pragma once
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

inline boost::uuids::uuid uuidGen() {
  static boost::uuids::random_generator gen;
  return gen();
}