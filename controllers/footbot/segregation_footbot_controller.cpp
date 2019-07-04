#include <argos3/core/utility/logging/argos_log.h>
#include "segregation_footbot_controller.h"

#include <cstdio>

SegregationFootbotController::SegregationFootbotController() {
  m_rng = CRandom::CreateRNG("segregation_loop_function");
}

void SegregationFootbotController::Init(TConfigurationNode &t_node) {
  try {
    m_pcWheels = GetActuator<CCI_DifferentialSteeringActuator>("differential_steering");
    m_pcRABAct = GetActuator<CCI_RangeAndBearingActuator>("range_and_bearing");
    m_pcRABSens = GetSensor<CCI_RangeAndBearingSensor>("range_and_bearing");
    m_pcLEDs = GetActuator<CCI_LEDsActuator>("leds");
    m_pcPosSens = GetSensor<CCI_PositioningSensor>("positioning");
  }
  catch (CARGoSException &ex) {
    THROW_ARGOSEXCEPTION_NESTED("Error initializing sensors/actuators", ex);
  }

  std::string params_filename;
  std::string sensor_implementation_str;
  double half_beam_angle_deg;
  GetNodeAttributeOrDefault(t_node, "parameter_file", params_filename, std::string());
  GetNodeAttributeOrDefault(t_node, "viz", viz, false);
  GetNodeAttributeOrDefault(t_node, "sensor_length_cm", sensor_length_cm, INFINITY);
  GetNodeAttributeOrDefault(t_node, "kin_nonkin_confusion", kin_nonkin_confusion, 0.);
  GetNodeAttributeOrDefault(t_node, "kin_nothing_confusion", kin_nothing_confusion, 0.);
  GetNodeAttributeOrDefault(t_node, "nonkin_nothing_confusion", nonkin_nothing_confusion, 0.);
  GetNodeAttributeOrDefault(t_node, "half_beam_angle", half_beam_angle_deg, 15.);
  GetNodeAttributeOrDefault(t_node, "sensor_implementation", sensor_implementation_str, std::string());
  half_beam_angle = deg2rad(half_beam_angle_deg);

  if (sensor_implementation_str == "kin_first") {
    sensor_impl = SensorImpl::KIN_FIRST;
  } else if (sensor_implementation_str == "closest_first") {
    sensor_impl = SensorImpl::CLOSEST_FIRST;
  } else if (sensor_implementation_str.empty()) {
    sensor_impl = SensorImpl::CLOSEST_FIRST;
  } else {
    THROW_ARGOSEXCEPTION("sensor implementation must be either \"closest_first\" or \"kin_first\"")
  }

  if (!params_filename.empty()) {
    LoadFromFile(params_filename);
  }

  Reset();
}

void SegregationFootbotController::Reset() {
  // not this only supports 256 classes
  m_pcRABAct->SetData(0, (uint8_t) m_class);
}

void SegregationFootbotController::LoadFromFile(const std::string &params_filename) {
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
                             << " were expected");
  }

  // create weights vector and load it from file
  for (size_t i = 0; i < params_length; ++i) {
    if (!(cIn >> m_params[i])) {
      THROW_ARGOSEXCEPTION("Cannot read data from file '" << params_filename << "'");
    }
  }
}

SegregationFootbotController::SensorState
SegregationFootbotController::ClosestFirstSensor(const CCI_RangeAndBearingSensor::TReadings &tMsgs) {
  auto sens_state = SensorState::NOTHING;
  if (!tMsgs.empty()) {
    float closest_range_cm = sensor_length_cm;
    for (const auto &tMsg : tMsgs) {
      Real bearing = tMsg.HorizontalBearing.GetValue();
      Real range_cm = tMsg.Range;
      if (bearing < half_beam_angle && bearing > -half_beam_angle) {
        if (range_cm < closest_range_cm) {
          closest_range_cm = range_cm;
          uint8_t robot_class = tMsg.Data[0];
          sens_state = (m_class == robot_class) ? SensorState::KIN : SensorState::NONKIN;
        }
      }
    }
  }

  return sens_state;
}

SegregationFootbotController::SensorState
SegregationFootbotController::KinFirstSensor(const CCI_RangeAndBearingSensor::TReadings &tMsgs) {
  auto sens_state = SensorState::NOTHING;
  if (!tMsgs.empty()) {
    for (const auto &tMsg : tMsgs) {
      Real bearing = tMsg.HorizontalBearing.GetValue();
      if (bearing < half_beam_angle && bearing > -half_beam_angle) {
        if (tMsg.Data[0] == m_class) {
          sens_state = SensorState::KIN;
          continue;
        } else {
          sens_state = SensorState::NONKIN;
        }
      }
    }
  }

  return sens_state;
}

SegregationFootbotController::SensorState SegregationFootbotController::GetTrueKinSensorVal() {
  const CCI_RangeAndBearingSensor::TReadings &tMsgs = m_pcRABSens->GetReadings();
  SensorState sens_state;
  switch (sensor_impl) {
    case SensorImpl::KIN_FIRST: {
      sens_state = KinFirstSensor(tMsgs);
      break;
    }
    case SensorImpl::CLOSEST_FIRST: {
      sens_state = ClosestFirstSensor(tMsgs);
      break;
    }
  }

  if (viz) {
    UInt32 led_id = 11;
    switch (sens_state) {
      case SensorState::KIN:
        m_pcLEDs->SetSingleColor(led_id, CColor::BLUE);
        break;
      case SensorState::NONKIN:
        m_pcLEDs->SetSingleColor(led_id, CColor::RED);
        break;
      case SensorState::NOTHING:
        m_pcLEDs->SetSingleColor(led_id, CColor::WHITE);
        break;
    }
  }

  return sens_state;
}

void SegregationFootbotController::ControlStep() {

  if (viz) {
    switch (m_class % 8) {
      case 0:
        m_pcLEDs->SetAllColors(CColor::GREEN);
        break;
      case 1:
        m_pcLEDs->SetAllColors(CColor::RED);
        break;
      case 2:
        m_pcLEDs->SetAllColors(CColor::BLUE);
        break;
      case 3:
        m_pcLEDs->SetAllColors(CColor::WHITE);
        break;
      case 4:
        m_pcLEDs->SetAllColors(CColor::YELLOW);
        break;
      case 5:
        m_pcLEDs->SetAllColors(CColor::CYAN);
        break;
      case 6:
        m_pcLEDs->SetAllColors(CColor::MAGENTA);
        break;
      case 7:
      default:
        m_pcLEDs->SetAllColors(CColor::BLACK);
        break;
    }
  }
  auto true_sensor_state = GetTrueKinSensorVal();

  auto const &posMsgs = m_pcPosSens->GetReading();
  std::cout << m_class
            << ","
            << static_cast<int>(true_sensor_state)
            << ","
            << posMsgs.Position.GetX()
            << ","
            << posMsgs.Position.GetY()
            << ',';

  auto p = m_rng->Uniform(CRange<double>(0., 1.));
  auto sensor_state = true_sensor_state;
  switch (true_sensor_state) {
    case SensorState::NOTHING: {
      if (p < kin_nothing_confusion) {
        sensor_state = SensorState::KIN;
      } else if (p - kin_nothing_confusion < nonkin_nothing_confusion) {
        sensor_state = SensorState::NONKIN;
      }
      break;
    }
    case SensorState::KIN: {
      if (p < kin_nonkin_confusion) {
        sensor_state = SensorState::NONKIN;
      } else if (p - kin_nonkin_confusion < kin_nothing_confusion) {
        sensor_state = SensorState::NOTHING;
      }
      break;
    }
    case SensorState::NONKIN: {
      if (p < kin_nonkin_confusion) {
        sensor_state = SensorState::KIN;
      } else if (p - kin_nonkin_confusion < nonkin_nothing_confusion) {
        sensor_state = SensorState::NOTHING;
      }
      break;
    }
  }

  switch (sensor_state) {
    case SensorState::NOTHING: {
      m_pcWheels->SetLinearVelocity(SCALE * m_params[0], SCALE * m_params[1]);
      break;
    }
    case SensorState::KIN: {
      m_pcWheels->SetLinearVelocity(SCALE * m_params[2], SCALE * m_params[3]);
      break;
    }
    case SensorState::NONKIN: {
      m_pcWheels->SetLinearVelocity(SCALE * m_params[4], SCALE * m_params[5]);
      break;
    }
  }
}

void SegregationFootbotController::SetParameters(const size_t num_params, const Real *params) {
  if (num_params != m_params.size()) {
    THROW_ARGOSEXCEPTION("Number of parameter mismatch: was passed "
                             << num_params
                             << " parameters, while "
                             << m_params.size()
                             << " were expected");
  }

  for (size_t i = 0; i < num_params; ++i) {
    m_params[i] = params[i];
  }
}

void SegregationFootbotController::SetClassId(unsigned long class_id) {
  m_class = class_id;
  m_pcRABAct->SetData(0, (uint8_t) m_class);
}

REGISTER_CONTROLLER(SegregationFootbotController, "footbot_segregation_controller")
