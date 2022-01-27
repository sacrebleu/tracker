# tracker
###  Telescope right ascension drive controller software for Arduino Nano rev 3
#### version 0.0.1

#### Overview

`tracker` is the command and control code that is intended to permit an Arduino Nano rev3 drive a stepper motor at a rate that will slew a telescope right ascension drive
at the correct rate to track objects with reasonable accuracy for visual astronomy.

The earth rotates at a rate of 1˚ every four minutes, which means that a telescope that has been correctly [Polar Aligned](https://skyandtelescope.org/astronomy-resources/accurate-polar-alignment/)
is able to track celestial objects by rotating its Right Ascension axis opposite to the direction of Earth's rotation at the same angular rate.

1˚ every four minutes translates to a rotation rate of 0.00416(6) degrees per second on the RA axis, so some configuration of the program is required.

#### Supported architectures

 * Arduino Nano rev 3

#### Configuration

`tracker` requires the following configuration:

`degrees_per_step` - the number of degrees your chosen stepper motor will step per step


