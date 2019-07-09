#pragma once

#include <unordered_map>

#include <argos3/core/utility/math/rng.h>
#include <argos3/core/simulator/loop_functions.h>
#include <argos3/plugins/robots/foot-bot/simulator/footbot_entity.h>

#include <loop_functions/my_loop_function.h>
#include <controllers/footbot/segregation_footbot_controller.h>
#include <unordered_map>

using namespace argos;

class SegregationLoopFunction : public MyLoopFunction {

public:

  static constexpr auto XML_CONTROLLER_ID = "fb_segregation";

  SegregationLoopFunction();

  /** This was causing bugs so we explicitly delete it **/
  SegregationLoopFunction(SegregationLoopFunction &/*other*/) = delete;

  void Init(TConfigurationNode &t_node) override;

  void Reset() override;

  std::string GetName() override;

  /* Configures the robot controller from the genome */
  void ConfigureFromGenome(const Real *pf_genome);

  void LoadFromFile(const std::string &params_filename) override;

  void LoadParameters(const size_t n_params, const Real *params) override;

  /**
   * Executes user-defined logic right after a control step is executed.
   */
  void PostStep() override;

  /* Calculates the performance of the robot in a trial */
  Real Cost() override;

protected:
  std::unordered_map<std::string, unsigned long> id_string_class_map;

private:

  void PlaceSingle(const CVector2 &center, unsigned long class_id, UInt32 id);

  void PlaceLine(const CVector2 &center, UInt32 un_robots, Real distance, UInt32 un_id_start);

  void PlaceCluster(const CVector2 &center, UInt32 un_robots, Real density, UInt32 un_id_start);

  struct RobotAndInitialPose {
    CFootBotEntity *robot;
    CVector3 position;
    CQuaternion orientation;
  };

  CRandom::CRNG *m_rng;
  std::vector<SegregationFootbotController *> m_controllers;
  std::vector<RobotAndInitialPose> m_robots;
  Real m_cost = 0;
  Real m_position_stdev = 0;
  unsigned long m_step = 0;

  UInt8 n_classes;
};

#define LABEL_NAME "segregation_loop_function"
extern "C" char loop_function_label[];
