# Bozzard
Bozzard is a 4-way quiz buzzer system for use in various quiz games.
The schematic for the circuit and the list of components required are published
here. The software is freely available on Github under a
[CC-BY licence](https://creativecommons.org/licenses/by/4.0/).

It was originally designed as a buzzer system for
[FOCAL events](https://focalcountdown.co.uk/), but it can be put to more
general applications.

Bozzard's goal is to provide a cheaper, DIY alternative to commercially
available quiz buzzer sets, as well as offering more functionality and much
greater flexibility in allowing users to modify the provided software or write
their own applications to run on it.

# Hardware
The hardware consists of a central control unit, which contains:
 * An Arduino Nano board, which controls everything
 * A power switch and an internal 9V battery
 * A speaker, which makes the buzzer noise
 * A speaker on/off switch
 * An LCD character display module, 2 rows by 16 columns
 * Four coloured LEDs: red, green, yellow and blue
 * A rotary knob and three general purpose buttons, for use by the quizmaster,
   and whose functions are specific to the application being run
 * Four 3.5mm audio jack sockets for the connection of buzzer units

Each buzzer unit has a 3.5mm audio jack and a large button. Each buzzer unit
is connected to the control unit by means of an ordinary 3.5mm audio cable.
No audio is actually sent down these cables. The buzzer unit's job is to
connect the tip and sleeve of the audio connector together when the button is
pressed.

The four buzzers are coloured red, green, yellow and blue, corresponding to
the colours marked on the control unit's sockets and the colours of the LEDs.

# Software API
The software running on the Arduino Nano board consists of a main loop which
detects events and acts on them, and a set of event-driven applications, one
of which may be running at a time. The running application makes API calls
to register its own callback functions with the main loop. The main loop will
call these callback functions when certain events happen, such as a buzzer
being pressed or a timer expiring.

The API used by the application is detailed in `boz_api.h`.

The Arduino C code provided in this Github repository consists of the main
Bozzard code (`boz.ino`) and a number of applications, not all of which are
used. The file `boz_app_list.ino` lists the applications that are actually
compiled into the image that gets flashed onto the Arduino, which has a limit
of 30,720 bytes for the compiled program.

# Main Menu
When you power on the Bozzard, it starts the Main Menu app. This app presents
to the user a list of applications which can be run. The user scrolls through
the list with the rotary knob, or "steering wheel", and presses the green
"Play" button to start the selected app.

 * Buzzer game - an app with a running clock and a lockout buzzer system. Most
   features in this app are configurable. Press the rotary knob while the clock
   is stopped to view and change settings such as the clock's time limit (if
   any), the buzzer sounds, whether a buzz stops the clock, how long a buzz
   should lock out other buzzers for, and so on.
 * Conundrum - the same as the Buzzer Game app, but with different rules. The
   time limit is 30 seconds, and the clock is stopped when a buzzer is pressed.
   The clock can be resumed with the green "Play" button. Each person may buzz
   only once.
 * Chess clocks - two clocks side by side, one of which is counting down at
   any one time. The buzzers are used to switch which clock is running.
 * Backlight on/off - toggles the state of the backlight and exits back to the
   main menu.
 * Battery info - tells you whether you're running from battery or USB power,
   and if the former, gives you an idea of the battery level.
 * Factory reset - erase the EEPROM, resetting any saved settings in any
   apps.
 * About Bozzard - version and copyright information.

