// ===-- main.cpp - Arduino starter for rod roller --===//
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

#include <RodRoller.h>

RodRoller roller;

void setup() {
  // put your setup code here, to run once:

  roller.init();

}

void loop() {
  // put your main code here, to run repeatedly:

  roller.tick();
}
