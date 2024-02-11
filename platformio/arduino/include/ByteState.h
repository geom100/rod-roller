// ===-- ByteState.h - ByteState class definition --===//
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

#ifndef RODROLLER_BYTESTATE_H
#define RODROLLER_BYTESTATE_H

#include <stdint.h>

class ByteState {
public:
  ByteState() : _state(0) {}
  ByteState(uint8_t state) : _state(state) {}
  ByteState(const ByteState &state) : _state(state._state) {}

  void on(uint8_t bit) { _state |= (1 << bit); }

  void off(uint8_t bit) { _state &= ~(1 << bit); }

  void toggle(uint8_t bit) { _state ^= (1 << bit); }

  bool is(uint8_t bit) { return (_state & (1 << bit)); }

private:
  uint8_t _state = 0;
};

#endif