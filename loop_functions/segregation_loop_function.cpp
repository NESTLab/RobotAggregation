#include <iterator>

#include <loop_functions/segregation_loop_function.h>
#include <loop_functions/cluster_metric.h>
#include <loop_functions/centroid_of_centroids.h>

SegregationLoopFunction::SegregationLoopFunction() : m_rng(nullptr) {}

void SegregationLoopFunction::Init(TConfigurationNode &t_node) {
  try {
    UInt32 seed = 0;
    GetNodeAttribute(t_node, "random_seed", seed);
    GetNodeAttributeOrDefault(t_node, "num_classes", n_classes, 1);
    GetNodeAttributeOrDefault(t_node, "stdev", m_position_stdev, 0.05);
    CRandom::CreateCategory("segregation_loop_function", seed);
    m_rng = CRandom::CreateRNG("segregation_loop_function");

    UInt32 num_placed_robots = 0;

    TConfigurationNodeIterator itDistr;
    for (itDistr = itDistr.begin(&t_node); itDistr != itDistr.end(); ++itDistr) {

      if (itDistr->Value() != "line" && itDistr->Value() != "cluster" && itDistr->Value() != "single") {
        THROW_ARGOSEXCEPTION("Unknown topology \"" << itDistr->Value() << "\"");
      }

      TConfigurationNode &tDistr = *itDistr;

      /* Parse common attributes */
      CVector2 cCenter;
      GetNodeAttribute(tDistr, "center", cCenter);

      UInt32 robot_num;
      GetNodeAttributeOrDefault(tDistr, "robot_num", robot_num, 0u);
      if (itDistr->Value() == "line") {
        /* Distance between the robots */
        Real fDistance;
        GetNodeAttribute(tDistr, "robot_distance", fDistance);
        PlaceLine(cCenter, robot_num, fDistance, num_placed_robots);
        num_placed_robots += robot_num;
      } else if (itDistr->Value() == "cluster") {
        /* Density of the robots */
        Real f_density;
        GetNodeAttribute(tDistr, "robot_density", f_density);
        PlaceCluster(cCenter, robot_num, f_density, num_placed_robots);
        num_placed_robots += robot_num;
      } else if (itDistr->Value() == "single") {
        unsigned long class_id;
        GetNodeAttribute(tDistr, "class", class_id);
        PlaceSingle(cCenter, class_id, num_placed_robots);
        ++num_placed_robots;
      }
    }
  }
  catch (CARGoSException &ex) {
    THROW_ARGOSEXCEPTION_NESTED("Error initializing the loop functions", ex);
  }

  Reset();
}

void SegregationLoopFunction::Reset() {
  m_step = 0;
  m_cost = 0;

  classes_over_time.clear();

  for (const auto &robot_and_initial_pose: m_robots) {
    auto &entity = robot_and_initial_pose.robot->GetEmbodiedEntity();

    bool success;
    auto attempts = 0;
    do {
      CVector3 position_noise{m_rng->Gaussian(m_position_stdev, 0.0), m_rng->Gaussian(m_position_stdev, 0.0), 0};
      auto position = position_noise + robot_and_initial_pose.position;
      success = MoveEntity(entity, position, robot_and_initial_pose.orientation, false);
      ++attempts;
    } while (!success and attempts < 200);
  }
}

void SegregationLoopFunction::PlaceSingle(const CVector2 &center, unsigned long class_id, UInt32 id) {
  try {
    std::ostringstream footbot_id;
    footbot_id.str("");
    footbot_id << id;
    id_string_class_map[footbot_id.str()] = class_id;

    auto position = CVector3{center.GetX(), center.GetY(), 0};
    CQuaternion orientation = CQuaternion{m_rng->Uniform(CRadians::UNSIGNED_RANGE), CVector3::Z};
    auto robot = new CFootBotEntity(footbot_id.str(), XML_CONTROLLER_ID, position, orientation, 100);
    AddEntity(*robot);
    auto &generic_controller = robot->GetControllableEntity().GetController();
    auto controller = &dynamic_cast<SegregationFootbotController &>(generic_controller);
    m_controllers.emplace_back(controller);
    controller->SetClassId(class_id);
    m_robots.emplace_back(RobotAndInitialPose{robot, position, orientation});
  } catch (CARGoSException &ex) {
    THROW_ARGOSEXCEPTION_NESTED("While placing robots in a line", ex);
  }
}

void SegregationLoopFunction::PlaceLine(const CVector2 &center, UInt32 n_robots, Real distance, UInt32 id_start) {
  try {
    std::ostringstream footbot_id;
    int j = -static_cast<int>(n_robots / 2);
    for (size_t i = 0; i < n_robots; ++i, ++j) {
      auto id = i + id_start;
      auto class_id = id % n_classes;
      footbot_id.str("");
      footbot_id << id;
      id_string_class_map[footbot_id.str()] = class_id;

      /* Create the robot in the origin and add it to ARGoS space */
      auto position = CVector3{distance * j + center.GetX(), distance * j + center.GetY(), 0};
      CQuaternion orientation = CQuaternion{m_rng->Uniform(CRadians::UNSIGNED_RANGE), CVector3::Z};
      auto robot = new CFootBotEntity(footbot_id.str(), XML_CONTROLLER_ID, position, orientation, 100);
      AddEntity(*robot);
      auto &generic_controller = robot->GetControllableEntity().GetController();
      auto controller = &dynamic_cast<SegregationFootbotController &>(generic_controller);
      m_controllers.emplace_back(controller);
      controller->SetClassId(class_id);
      m_robots.emplace_back(RobotAndInitialPose{robot, position, orientation});
    }
  }
  catch (CARGoSException &ex) {
    THROW_ARGOSEXCEPTION_NESTED("While placing robots in a line", ex);
  }
}

void SegregationLoopFunction::PlaceCluster(const CVector2 &center, UInt32 n_robots, Real density, UInt32 id_start) {
  try {
    Real fHalfSide = Sqrt((M_PI * Square(0.085036758f) * n_robots) / density) / 2.0f;
    CRange<Real> area_range(-fHalfSide, fHalfSide);

    std::ostringstream footbot_id;

    for (size_t i = 0; i < n_robots; ++i) {
      auto id = i + id_start;
      auto class_id = id % n_classes;
      footbot_id.str("");
      footbot_id << id;
      id_string_class_map[footbot_id.str()] = class_id;

      /* Create the robot in the origin and add it to ARGoS space */
      CQuaternion orientation = CQuaternion{m_rng->Uniform(CRadians::UNSIGNED_RANGE), CVector3::Z};
      CVector3 position = CVector3::ZERO;
      auto robot = new CFootBotEntity(footbot_id.str(), XML_CONTROLLER_ID, position, orientation, 100);
      AddEntity(*robot);
      auto &generic_controller = robot->GetControllableEntity().GetController();
      auto controller = &dynamic_cast<SegregationFootbotController &>(generic_controller);
      controller->SetClassId(class_id);
      m_controllers.emplace_back(controller);

      /* Try to place it in the arena */
      auto attempts = 0;
      bool bDone;
      do {
        /* Choose a random position and orientation */
        ++attempts;
        position =
            CVector3(m_rng->Uniform(area_range) + center.GetX(), m_rng->Uniform(area_range) + center.GetY(), 0.f);
        orientation = CQuaternion{m_rng->Uniform(CRadians::UNSIGNED_RANGE), CVector3::Z};
        bDone = MoveEntity(robot->GetEmbodiedEntity(), position, orientation);
      } while (!bDone && attempts < 200);
      if (!bDone) {
        THROW_ARGOSEXCEPTION("Can't place " << footbot_id.str());
      }

      m_robots.emplace_back(RobotAndInitialPose{robot, position, orientation});
    }
  }
  catch (CARGoSException &ex) {
    THROW_ARGOSEXCEPTION_NESTED("While placing robots in a cluster", ex);
  }
}

void SegregationLoopFunction::ConfigureFromGenome(const Real *genome) {
  for (const auto &robot_controller: m_controllers) {
    robot_controller->SetParameters(SegregationFootbotController::GENOME_SIZE, genome);
  }
}

void SegregationLoopFunction::LoadFromFile(const std::string &params_filename) {
  for (const auto &robot_controller: m_controllers) {
    robot_controller->LoadFromFile(params_filename);
  }
}

void SegregationLoopFunction::LoadParameters(const size_t n_params, const Real *params) {
  for (const auto &robot_controller: m_controllers) {
    robot_controller->SetParameters(n_params, params);
  }
}

Real SegregationLoopFunction::Cost() {
  return m_cost;
}

void SegregationLoopFunction::PostStep() {
  try {
    CSpace::TMapPerType &robot_map = GetSpace().GetEntitiesByType("foot-bot");

    // split up the map into containers for each robot class
    GroupPosMap classes;
    for (const auto &p : robot_map) {
      auto id = p.first;
      auto class_id = id_string_class_map.at(id);
      auto robot = any_cast<CFootBotEntity *>(p.second);
      auto pos = robot->GetEmbodiedEntity().GetOriginAnchor().Position;
      classes[class_id].emplace_back(pos);
    }

    GroupPosMap class_pos;
    for (auto const &class_i : classes) {
      std::vector<CVector3> positions;
      for (auto const &robot : class_i.second) {
        auto robot_position = robot;
        positions.emplace_back(robot_position);
      }
      class_pos[class_i.first] = positions;
    }
    classes_over_time.push_back(class_pos);

    // Centroid of centroid metric -- NOT USED IN OUR PAPER!
    // m_cost += m_step * centroid_of_centroids(class_pos);

    // Not-Time-Penalized Cluster Metric, doesn't penalize time -- NOT USED IN OUR PAPER!
    // m_cost += cluster_metric(class_pos);

    // Time-Penalized Clust Metric
     m_cost += m_step * cluster_metric(class_pos);
  } catch (argos::CARGoSException &e) {
    argos::LOG << e.what() << std::endl;
    m_cost = -999;
  }

  ++m_step;
}

std::string SegregationLoopFunction::GetName() {
  return "SegregationLoopFunction";
}

extern "C" MyLoopFunction *create() {
  return new SegregationLoopFunction();
}

char loop_function_label[] = LABEL_NAME;
REGISTER_LOOP_FUNCTIONS(SegregationLoopFunction, LABEL_NAME)
