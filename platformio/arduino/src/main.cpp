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
