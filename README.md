Adplug for 8088-86 and 286
--------------------------------------------
Here I go again, the same story, I grew up with an 8086 in which many things would not work 
back in the day, so I decided to compile "adplug" without any 186 instructions.

This is not really adplug, I only took the file readers and players, and then created a simple gui.

If you choose EGA or VGA, the program will load custom graphics for the text mode characters from "Font_BIZ.png".
The program will restore the original font from "Font_VGA.png" when you go back to dos. 

Conbtrols: 
- Move = UP DOWN LEFT RIGHT arrows.
- Select file = ENTER
- Stop music = SPACEBAR
- Exit = ESC

Requirements:
- CPU 8088 4.77 Mhz
- RAM 512 Kb.
- GRAPHICS CGA
- SOUND Adlib or compatible (Sound Blaster, opl2lpt).

Some formats will play slow on 8088 4.77 Mhz, code needa to be optimized.

Supported formats:
- IMF: Id's Music Format - working 100%.
- RAD: Reality Adlib Tracker - working 100% (minor bugs caused by my code).
- SA2: Surprise! Adlib Tracker 2 - Some files play at the wrong speed.
- AMD: Amusic Tracker - minor bugs.
- LDS: Loudness Sound System Tracker, the best tracker for adlib - working 100% (minor bugs caused by my code).
- D00: Edlib Tracker, The most complex tracker created for Adlib
  Edlib Tracker format 0 - Minor bugs. 
  Edlib Tracker format 1 - not well supported. 

I want to add: Adlib Tracker 2 (A2M OPL2), Twin Tracker (DMO), Sierra games (SCI), and Lucasarts (LAA).
Also I want to add a custom RAD format (RAP) in which channels 0 and 1 are used to send commands to Sound Blaster to play PCM Drums.

