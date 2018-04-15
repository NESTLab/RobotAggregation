#include <argos3/core/utility/logging/argos_log.h>
#include "GenericFootbotController.h"

#include <cstdio>

GenericFootbotController::GenericFootbotController()
    : m_params{0, 0, 0, 0},
      m_pcWheels(nullptr),
      m_pcRABAct(nullptr),
      m_pcRABSens(nullptr),
      m_pcLEDs(nullptr),
      my_id(0ul),
      my_group(0ul) {
}

void GenericFootbotController::Init(TConfigurationNode &t_node) {
  try {
    m_pcWheels = GetActuator<CCI_DifferentialSteeringActuator>("differential_steering");
    m_pcRABAct = GetActuator<CCI_RangeAndBearingActuator>("range_and_bearing");
    m_pcRABSens = GetSensor<CCI_RangeAndBearingSensor>("range_and_bearing");
    m_pcLEDs = GetActuator<CCI_LEDsActuator>("leds");

  }
  catch (CARGoSException &ex) {
    THROW_ARGOSEXCEPTION_NESTED("Error initializing sensors/actuators", ex);
  }

  std::string params_filename;
  GetNodeAttributeOrDefault(t_node, "parameter_file", params_filename, std::string());

  if (!params_filename.empty()) {
    std::ifstream cIn(params_filename.c_str());
    if (!cIn) {
      THROW_ARGOSEXCEPTION("Cannot open parameter file '" << params_filename << "' for reading");
    }

    // first parameter is the number of real-valued weights
    UInt32 params_length = 0;
    if (!(cIn >> params_length)) {
      THROW_ARGOSEXCEPTION("Cannot read data from file '" << params_filename << "'");
    }

    // check consistency between parameter file and xml declaration
    if (params_length != m_params.size()) {
      THROW_ARGOSEXCEPTION("Number of parameter mismatch: '"
                               << params_filename
                               << "' contains "
                               << params_length
                               << " parameters, while "
                               << m_params.size()
                               << " were expected from the XML configuration file");
    }

    // create weights vector and load it from file
    for (size_t i = 0; i < params_length; ++i) {
      if (!(cIn >> m_params[i])) {
        THROW_ARGOSEXCEPTION("Cannot read data from file '" << params_filename << "'");
      }
    }
  }

  // Parse the ID number
  const std::string my_idstr = GetId();
  size_t pos;
  my_id = std::stoul(my_idstr, &pos);

  // Set group based on ID modulo some number
  UInt8 num_classes;
  GetNodeAttributeOrDefault(t_node, "num_classes", num_classes, 1);
  my_group = my_id % num_classes;

  for (UInt32 led_id = 0; led_id < m_pcLEDs->GetNumLEDs()/2; ++led_id) {
    switch (my_group) {
      case 0: m_pcLEDs->SetSingleColor(led_id, CColor::ORANGE);
        break;
      case 1: m_pcLEDs->SetSingleColor(led_id, CColor::GREEN);
        break;
      case 2: m_pcLEDs->SetSingleColor(led_id, CColor::PURPLE);
        break;
      case 3: m_pcLEDs->SetSingleColor(led_id, CColor::YELLOW);
        break;
      case 4: m_pcLEDs->SetSingleColor(led_id, CColor::BLUE);
        break;
      default: m_pcLEDs->SetSingleColor(led_id, CColor::RED);
        break;
    }
  }

  Reset();
}

void GenericFootbotController::Reset() {
  // not this only supports 256 groups
  m_pcRABAct->SetData(0, (uint8_t) my_group);
}

void GenericFootbotController::SetParameters(const size_t num_params, const Real *params) {
  if (num_params != m_params.size()) {
    THROW_ARGOSEXCEPTION("Number of parameter mismatch: passed "
                             << num_params
                             << " parameters, while GENE_SIZE is "
                             << m_params.size());
  }

  for (size_t i = 0; i < num_params; ++i) {
    LOG << params[i] << ", ";
    m_params[i] = params[i];
  }
  LOG << "\n";

}

GenericFootbotController::SensorState GenericFootbotController::GetKinSensorVal() {
  const CCI_RangeAndBearingSensor::TReadings &tMsgs = m_pcRABSens->GetReadings();

  auto sens_state = SensorState::NOTHING;
  if (!tMsgs.empty()) {
    Real closest_range = INFINITY;

    for (const auto &tMsg : tMsgs) {
      Real bearing = tMsg.HorizontalBearing.GetValue();
      Real range = tMsg.Range;
      if (bearing < kCAM_VIEW_ANG && bearing > -kCAM_VIEW_ANG) {
        if (range < closest_range) {
          closest_range = range;
          uint8_t robot_group = tMsg.Data[0];
          sens_state = (my_group == robot_group) ? SensorState::KIN : SensorState::NONKIN;
        }
      }
    }
  }

  for (UInt32 led_id = static_cast<UInt32>(m_pcLEDs->GetNumLEDs()/2); led_id < m_pcLEDs->GetNumLEDs(); ++led_id) {
    switch (sens_state) {
      case SensorState::KIN: m_pcLEDs->SetSingleColor(led_id, CColor::BLUE);
        break;
      case SensorState::NONKIN: m_pcLEDs->SetSingleColor(led_id, CColor::RED);
        break;
      case SensorState::NOTHING: m_pcLEDs->SetSingleColor(led_id, CColor::WHITE);
        break;
    }
  }

  return sens_state;
}