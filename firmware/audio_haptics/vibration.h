#pragma once
#include "../shared/packet_format.h"

void initVibration();
void stopVibration();
void triggerVibration(Priority priority, Direction dir, uint8_t intensity);
void testMotors();