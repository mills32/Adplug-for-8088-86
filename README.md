ENG:

Adplug OPL2 Music Player for 8088-86 and 286
--------------------------------------------

Here I go again, the same story, I grew up with an 8086 in which many things would not work 
back in the day, so I decided to compile "adplug" without any 186 instructions.

This is not really adplug, I only took the file readers and players, and then created a simple gui.

If you choose EGA or VGA, the program will load custom graphics for the text mode characters from "Font_BIZ.png".
The program will restore the original font from "Font_VGA.png" when you go back to dos. 

![plot](https://raw.githubusercontent.com/mills32/Adplug-for-8088-86/master/adplay88_003.png)

Conbtrols: 
- Move = UP DOWN LEFT RIGHT arrows
- Select file = ENTER
- Stop music = SPACEBAR
- Enable/Disable visualizer = F1
- Exit = ESC

Requirements:
- CPU 8088 4.77 Mhz
- RAM 512 Kb
- GRAPHICS CGA
- SOUND Adlib or compatible (Sound Blaster, opl2lpt)

Some formats will play slow on 8088 4.77 Mhz, code needs to be optimized.
For the moment it is highly recommended an 8088 at 9 Mhz or an 8086 at 8Mhz. 

Supported formats:
- IMF: Id's Music Format - working 100%.
- RAD: Reality Adlib Tracker - working 100% (minor bugs caused by my code).
- SA2: Surprise! Adlib Tracker 2 - Some files play at the wrong speed.
- AMD: Amusic Tracker - minor bugs.
- LDS: Loudness Sound System Tracker, the best tracker for adlib - working 100% (minor bugs caused by my code).
- D00: Edlib Tracker, The most complex tracker created for Adlib.
  Edlib Tracker format 0 - Minor bugs. 
  Edlib Tracker format 1 - not well supported. 

I want to add: Adlib Tracker 2 (A2M OPL2), Twin Tracker (DMO), HSC-Tracker by Electronic Rats (HSC), Sierra games (SCI), and Lucasarts (LAA).
Also I want to add a custom RAD format I created (RAP), in which channels 0 and 1 are used to send commands to Sound Blaster to play PCM Drums.

ESP:

Adplug Reproductor de música OPL2 para 8088-86 y 286
--------------------------------------------

Pues mas de lo mismo, quise compilar adplug para que funcionase en 8086.

En realidad no es un port de Adplug, sino que he utilizado el código de los reproductores, y he añadido una interfaz.
Si selecciones EGA o VGA en el inicio, el programa cargará gráficos para sustiruir los caracteres del modo texto de EGA 
o VGA desde la imagen "Font_BIZ.png". Al salir a msdos, restaurará los gráficos originales de la imagen "Font_VGA.png".

![plot](https://raw.githubusercontent.com/mills32/Adplug-for-8088-86/master/adplay88_003.png)

Conbtroles: 
- Mover = Teclas direccionales
- Seleccionar = ENTER
- Detener Música = ESPACIO
- Desactivar/Activar visualizador = F1
- Salir = ESC

Requisitos:
- CPU 8088 4.77 Mhz
- RAM 512 Kb
- GRAFICOS CGA
- SONIDO Adlib o compatible (Sound Blaster, opl2lpt)

Algunos formnatos funcionan lentos en 8088 4.77 Mhz, el código necesita ser optimizado.
De momento es mejor utilizar un 8088 a 9 Mhz, o un 8086 a 8 Mhz.

Formatos soportados:
- IMF: Id's Music Format - funcionando 100%.
- RAD: Reality Adlib Tracker - funcionando 100% (Pequeños bugs causados por mi código).
- SA2: Surprise! Adlib Tracker 2 - algunas canciones se reproducen a la velocidad incorrecta.
- AMD: Amusic Tracker - pequeños bugs.
- LDS: Loudness Sound System Tracker, el mejor tracker para Adlib - funcionando 100% (Pequeños bugs causados por mi código).
- D00: Edlib Tracker, el tracker más complejo creado para Adlib.
  Edlib Tracker format 0 - pequeños bugs. 
  Edlib Tracker format 1 - no funcionan bien. 

Además quiero añadir: Adlib Tracker 2 (A2M OPL2), Twin Tracker (DMO), HSC-Tracker (HSC), Sierra (SCI), Lucasarts (LAA).
También quiero añadir un formato especial creado por mi (RAP), en el cual, los datos de los canales 0 y 1 son utilizados para enviar 
instrucciones a una Sound Blaster, para que reproduzca sonidos PCM para percusión.
