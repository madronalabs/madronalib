//
// MLT3D.h
// madronalib
//

#pragma once

// default port for t3d plugin communication. Plugins may be receiving on different ports.
const int kDefaultUDPPort = 3123;

// maximum number of ports from kDefaultUDPPort to (kDefaultUDPPort + kNumUDPPorts - 1)
const int kNumUDPPorts = 16;

// Soundplane app input port for Kyma and other config messages
const int kDefaultUDPReceivePort = 3122;

