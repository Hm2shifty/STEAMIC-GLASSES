#pragma once
#include "../shared/packet_format.h"

bool      initIMU();
float     readIMU();
bool      isGlassesOn();
Direction getHeadTurnDirection();