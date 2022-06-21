ENG:

Adplug OPL2 Music Player for 8088-86 and 286
--------------------------------------------

Here I go again, the same story, I grew up with an 8086 in which many things would not work 
back in the day, so I decided to compile "adplug" without any 186 instructions.

This is not really adplug, I only took the file readers and players, and then created a simple gui.

If you have a VGA, the program will load custom graphics for the text mode characters from "Font_BIZ.png".

![plot](https://raw.githubusercontent.com/mills32/Adplug-for-8088-86/master/adplay88_003.png)

Conbtrols: 
- Move = UP DOWN LEFT RIGHT arrows
- Select file = ENTER
- Stop music = SPACEBAR
- Enable/Disable visualizer = F1
- Exit = ESC

Requirements:
- CPU 8088 4.77 Mhz
- RAM 256 Kb
- GRAPHICS MDA / Hercules / CGA / Tandy / MCGA / EGA / VGA
- SOUND Adlib or compatible (Sound Blaster, opl2lpt)

Some formats will play slow on 8088 4.77 Mhz, code can be optimized.
Some formats are buggy. 

Supported formats:
- IMF: Id's Music Format - working 100%.
- RAD: Reality Adlib Tracker v1.1 1995 - working 100% (minor bugs caused by my code).

  RAS: channels 0 and 1 are used to send commands to Sound Blaster to play PCM Drums, do not use with adlib.
  Samples are 8 bit unsigned PCM (8192 or 11025Hz). Specify relative route in RAD description (Ctrl-D in RAD tracker).
  - Sample 1: at row 3 column 0 (example: PCM/drum.wav)
  - Sample 2: at row 6 column 0 (example: PCM/snare.wav)

- SA2: Surprise! Adlib Tracker 2 - Some files play at the wrong speed.
- AMD: Amusic Tracker - minor bugs.
- LDS: Loudness Sound System Tracker, the best tracker for adlib - working 100%.
- VGM: Only 3812 recordings (Find more at vgmrips.com)

  VGM files need 44100Hz playback rate, which can't be achieved on old PC, so timming might be a bit off in many songs

- D00: Edlib Tracker, The most complex tracker created for Adlib.
  
  Edlib Tracker format 0 - Minor bugs. 
  
  Edlib Tracker format 1 - not well supported. 

I want to add: Adlib Tracker 2 (A2M OPL2), Twin Tracker (DMO), HSC-Tracker by Electronic Rats (HSC), Sierra games (SCI), and Lucasarts (LAA).

ESP:

Adplug Reproductor de música OPL2 para 8088-86 y 286
--------------------------------------------

Pues mas de lo mismo, quise compilar adplug para que funcionase en 8086.

En realidad no es un port de Adplug, sino que he utilizado el código de los reproductores, y he añadido una interfaz.
Si tienes una VGA, el programa cargará gráficos para sustiruir los caracteres del modo texto desde la imagen "Font_BIZ.png".

![plot](https://raw.githubusercontent.com/mills32/Adplug-for-8088-86/master/adplay88_003.png)

Conbtroles: 
- Mover = Teclas direccionales
- Seleccionar = ENTER
- Detener Música = ESPACIO
- Desactivar/Activar visualizador = F1
- Salir = ESC

Requisitos:
- CPU 8088 4.77 Mhz
- RAM 256 Kb
- GRAFICOS MDA / Hercules / CGA / Tandy / MCGA / EGA / VGA
- SONIDO Adlib o compatible (Sound Blaster, opl2lpt)

Algunos formnatos funcionan lentos en 8088 4.77 Mhz, el código puede ser optimizado.
Algunos formatos tienen problemas.

Formatos soportados:
- IMF: Id's Music Format - funcionando 100%.
- RAD: Reality Adlib Tracker v1.1 1995 - funcionando 100% (Pequeños bugs causados por mi código).

  RAS: los datos de los canales 0 y 1 son utilizados para enviar instrucciones a una Sound Blaster, 
  para que reproduzca sonidos PCM para percusión. No utilizar con Adlib.
  Los archivos de sonido deben ser PCM 8 bit unsigned (8192 o 11025Hz). Especifica la ruta relativa en la descripcion del archivo RAD
  (Pulsando Ctrl-D en el editor RAD, accederas a la descripción).
  - Sonido 1: en fila 3 columna 0 (ejemplo: PCM/drum.wav)
  - Sonido 2: en fila 6 columna 0 (ejemplo: PCM/snare.wav)

- SA2: Surprise! Adlib Tracker 2 - algunas canciones se reproducen a la velocidad incorrecta.
- AMD: Amusic Tracker - pequeños bugs.
- LDS: Loudness Sound System Tracker, el mejor tracker para Adlib - funcionando 100% (Pequeños bugs causados por mi código).
- VGM: Solamente grabaciones del chip 3812 (Encuentra más en vgmrips.com).

  Este formato tiene pequeños desajustes en la velocidad de reproducción, ya que los archivos VGM necesitan reproducirse a 44100Hz,
  cosa que es imposible en estos PC.

- D00: Edlib Tracker, el tracker más complejo creado para Adlib.

  Edlib Tracker format 0 - pequeños bugs. 
  
  Edlib Tracker format 1 - no funcionan bien. 

Además quiero añadir: Adlib Tracker 2 (A2M OPL2), Twin Tracker (DMO), HSC-Tracker (HSC), Sierra (SCI), Lucasarts (LAA).


