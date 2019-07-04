#pragma once

#include <unordered_map>

#include <argos3/core/utility/math/rng.h>
#include <argos3/core/simulator/loop_functions.h>
#include <argos3/plugins/robots/foot-bot/simulator/footbot_entity.h>

#include <bits/unordered_map.h>

using namespace argos;

using GroupPosMap = std::unordered_map<unsigned long, std::vector<CVector3>>;

class MyLoopFunction : public CLoopFunctions {

 public:

  void Reset() override = 0;

  virtual std::string GetName() = 0;

  virtual void LoadFromFile(const std::string &params_filename) = 0;

  virtual void LoadParameters(size_t n_params, Real const *params) = 0;

  /* Calculates the performance of the robot in a trial */
  virtual Real Cost() = 0;

  std::vector<GroupPosMap> classes_over_time;
};
