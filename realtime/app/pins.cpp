#include "pcrincludes.h"

using namespace std;

//devices
const string kSPI0DevicePath {"/dev/spidev1.0"};
const string kSPI1DevicePath {"/dev/spidev2.0"};

//fan
const string kHeatSinkFanControlPWMPath {"/sys/devices/ocp.*/fan_pwm.*"};

//LED control
extern const std::string kLEDGrayscaleClockPWMPath {"/sys/devices/ocp.3/led_pwm.16"};