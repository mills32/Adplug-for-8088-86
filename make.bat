<<<<<<< HEAD
rem required Turbo C installed at C:\TC
rem compile c files
tcc -G -Ic:\tc\include -mc -c player.c plugins.c 
rem compile assembly files
rem tasm /dc lt_sprc.asm
rem create lib
del *.LIB
tlib ad_lib.lib +plugins.obj
rem link
tlink c0c.obj player.obj ad_lib.lib,adplay88.exe,, cc -Lc:\tc\lib
del *.OBJ
del *.MAP
=======
rem required Turbo C installed at C:\TC
rem compile c files
tcc -G -O2 -Ic:\tc\include -mc -c player.c plugins.c 
rem compile assembly files
rem tasm /dc lt_sprc.asm
rem create lib
del *.LIB
tlib ad_lib.lib +plugins.obj
rem link
tlink c0c.obj player.obj ad_lib.lib,adplay88.exe,, cc -Lc:\tc\lib
del *.OBJ
del *.MAP
>>>>>>> 7f575023bf2dcb27edf3351269ef7b7b78484e2a
