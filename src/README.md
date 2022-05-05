# Video Game
This program creates a simple game that uses the LCD screen,  all four buttons and the buzzer. Buttons can pressed to move the player around the playing field to avoid the asteriods.

## Description

All the buttons (1-4) on the expansion board can be activated by being pressed
down. The objective of the game is to avoid any contact with the asteriods (gray circles). I wanted to be able to have the asteriods move faster as time progressed to change the difficulty.
- Button 1 moves the player to the left.
- Button 2 moves the player forwards.
- Button 3 moves the player backwards.
- Buttn 4 moves the player to the right.

- **buzzer.c** and **buzzer.h** - allow the speaker on the expansion board to play notes.
- **game.c** - the main code to allow the game to run.
- **Makefile** - used to compile and run game.c
## Instructions

To compile type in **"make"** . Next type in **"make load"** to be able to start the program on your MSP430. Once done using the toy type in **"make clean"** to clean and remove program files and reset board. 
