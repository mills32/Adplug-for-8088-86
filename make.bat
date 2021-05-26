rem required Turbo C installed at C:\TC
rem compile c files
tcc -G -O2 -Ic:\tc\include -ml -c player.c plugins.c 
rem compile assembly files
rem tasm /dc lt_sprc.asm
rem create lib
del *.LIB
tlib ad_lib.lib +plugins.obj
rem link
tlink c0l.obj player.obj ad_lib.lib,adplay88.exe,,emu mathl cl -Lc:\tc\lib
del *.OBJ
del *.MAP
