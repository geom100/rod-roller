// ===-- Debouncer.h - Debouncer class definition --===//
//
//  Part of Rod Roller project.
//  SPDX-License-Identifier: Apache-2.0
//
// Copyright 2024 Mikhail Rumyantsev <dev@furryowl.ru>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ===----------------------------------------------===//

/// \file
/// Debouncer filters repeated events (mainly from encoder) allowing to handle them after timeout or
/// exceeding count of events. Event is uint8_t value. If event is debounced, 0 is returned.   
///

#ifndef RODROLLER_DEBOUNCER_H
#define RODROLLER_DEBOUNCER_H

#include <Arduino.h>

template <uint16_t TIMEOUT, uint8_t COUNT> 
class Debouncer {
public:
  Debouncer() : _counter(0), _timer(millis()) {}

  int8_t event(int8_t dir) {
    if (_counter == 0 && millis() - _timer <= TIMEOUT) {
      _timer = millis();
      return 0;
    } else {
      if (millis() - _timer > TIMEOUT) {
        _counter = 1;
        _timer = millis();
        return 0;
      } else if (++_counter >= COUNT) {
        _counter = 0;
        _timer = millis();
        return dir;
      } else {
        return 0;
      }
    }
  }

private:
  uint8_t _counter;
  uint32_t _timer;
};

#endif