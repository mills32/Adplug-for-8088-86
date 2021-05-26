// Hristo I. Gueorguiev 2018 (Throwback week)
// Hristogueorguiev.com
/////////////////////////
// ADPLUG for 8088/8086//
/////////////////////////

/*
Here I go again, the same story, I grew up with an 8086 in which many things would not work 
back in the day, so I decided to compile "adplug" without any 186 instructions.

This is not really adplug, I only took the file readers and players, and then created a simple gui. 

REQUIREMENTS

CPU: 8088/NEC V20 at 7MHz
"GPU": CGA or TANDY
RAM: 512 KB
SOUND: of course an adlib (or compatible) card.

An 8088 at 4 MHz can't play some complex formats at full speed (LDS, D00), code needs to be optimized.

I still have to implement an MDA gui 

*/

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <alloc.h>
#include <mem.h>
#include <conio.h>
#undef outp

/* macro to write a word to a port */
#define word_out(port,register,value) \
  outport(port,(((word)value<<8) + register))

#define true 1
#define false 0

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

byte *XGA=(byte *)0xA0000000L;			/* this points to graphics memory. */
byte *XGA_TEXT_MAP=(byte *)0xB8000000L;	/* this points to text MAP - MAP attributes */
extern unsigned char *ADPLUG_music_data;
byte running = 1;
byte GRAPHICS_CARD = 2;
//ADLIB/SBlaster = 388
//SB16+ = 220 or 240
//opl2lpt = 378
int ADLIB_PORT = 0;//0x0388;

//Visualizer bars colors
byte VBar_Colors[] = {
	0,
	BLACK << 4 | LIGHTCYAN,
	BLACK << 4 | CYAN,
	BLACK << 4 | LIGHTBLUE,
	BLACK << 4 | LIGHTBLUE,
	BLACK << 4 | MAGENTA,
	BLACK << 4 | MAGENTA,
	BLACK << 4 | MAGENTA,
	
	/*BLACK << 4 | BLUE,
	BLACK << 4 | MAGENTA,
	BLACK << 4 | LIGHTBLUE,
	BLACK << 4 | CYAN,
	BLACK << 4 | LIGHTCYAN,
	BLACK << 4 | LIGHTRED,
	BLACK << 4 | YELLOW,*/
};

byte VBar_Selector[] = {
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01
};


//MISC
void WaitVsync(){
	asm mov		dx,003dah
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
}

void Clearkb(){
	asm mov ah,00ch
	asm mov al,0
	asm int 21h
}

void Load_Music(char *filename);
void Stop_Music();
void Setup();
void Init();
void Exit_Dos();

byte CimfPlayer_load(char *filename);
byte CxsmPlayer_load(char *filename);
byte CsngPlayer_load(char *filename);
byte CradPlayer_load(char *filename);
byte Csa2Player_load(char *filename);
byte CamdPlayer_load(char *filename);

//GUI
word list_pos = 0;
byte file_list[256*16];
byte file_attr[256];
byte text_size[256];
word nfiles = 0;
word text_pos = 0;
word screen_pos = 0;

byte GUI_MAP_CHAR[];
byte GUI_MAP_COL[];
byte GUI_MAP_CHAR_CGA[];
byte GUI_MAP_COL_CGA[];
byte file_number = 2;
byte pos_number = 0;
word selected_cell = 0;
byte control_timer = 0;

extern unsigned char C_Volume[];

void Set_Tiles(unsigned char *font);
void Set_Map();
void Control_Menu();
byte Read_Dir();
void Print_Dir(byte pos);
void Display_Bars();

void main(){
	//byte load;
	int i;
	Setup();
	Clearkb();
	Init();
	Set_Map();
	Read_Dir();
	Print_Dir(0);
	//Draw selection
	for (i = 1; i < 26; i+=2) XGA_TEXT_MAP[(160*8)+18+i] = GREEN << 4 | WHITE;
	
	while(running){
		Control_Menu();
		Display_Bars();
		WaitVsync();
	}
	Exit_Dos();
}


//////////////////

void Setup(){
	int key = 0;
	system("cls");
	printf("\n\n\n\n\n\n");
	printf("                              SELECT GRAPHICS CARD\n\n");
	printf("                             1 for MDA or Hercules\n");
	printf("                             2 for CGA or MCGA\n");
	printf("                             3 for EGA or VGA\n\n\n");
	printf("               Press to select, or any other key for default (CGA)");
	while(!kbhit());
	key = getch();
	GRAPHICS_CARD = key-48;
	if ((GRAPHICS_CARD == 0)||(GRAPHICS_CARD > 3)) GRAPHICS_CARD = 2;
	system("cls");	
	Clearkb();
	if (GRAPHICS_CARD == 3) Set_Tiles("Font_BIZ.png");//Before adding interrupt timer
	printf("\n\n\n\n\n\n");
	printf("                           SELECT PORT FOR OPL2 SOUND\n\n");
	printf("                      1 for ADLIB / Sound Blaster (port 388)\n");
	printf("                      2 for Sound Blaster 16 or newer (port 220)\n");
	printf("                      3 for Sound Blaster 16 or newer (port 240)\n");
	printf("                      4 for opl2lpt (port 378)\n\n\n");
	printf("               Press to select, or any other key for default  (388)");
	while(!kbhit());
	key = getch() -48;
	system("cls");	
	Clearkb();
	if (key == 1) ADLIB_PORT = 0x0388;
	else if (key == 2) ADLIB_PORT = 0x0220;
	else if (key == 3) ADLIB_PORT = 0x0240;
	else if (key == 4) ADLIB_PORT = 0x0378;
	else ADLIB_PORT = 0x0388;
}

//VISUALIZER
byte v_timer = 0;
void Get_Volume();
void Display_Bars(){
	if (v_timer == 2){
	int i,j;
	int map_pos = (160*7)+95;
	int col_pos = 0;
	byte col = 0;
	Get_Volume();
	for (i = 0; i < 9; i++){
		for (j = 8; j > 0; j--){
			if (C_Volume[i]>>3 == j) { col = 1; XGA_TEXT_MAP[map_pos+col_pos] = VBar_Colors[j];}
			else if (!col) XGA_TEXT_MAP[map_pos+col_pos] = 0;
			if (col) XGA_TEXT_MAP[map_pos+col_pos] = VBar_Colors[j];
			col_pos+=160;
		}
		col = 0;
		col_pos = 0;
		map_pos+=4;
	}
	v_timer = 0;
	}
	v_timer++;
}

byte Read_Dir(){
	struct ffblk a;
	int done;
	nfiles = 0;
	text_pos = 0;
	screen_pos = 0;	
	list_pos = 0;
	memset(file_list,0,256*13);
	done = findfirst("*.*", &a, FA_DIREC); // '*' is for all file, all file extensions
	while(!done && (nfiles < 256)){
		text_size[nfiles] = strlen(a.ff_name);
		memcpy(&file_list[list_pos],a.ff_name,text_size[nfiles]);
		if (a.ff_attrib == 16) {
			file_attr[nfiles] = 1; //DIR
		}
		else file_attr[nfiles] = 0;	//FILE
		done = findnext(&a, FA_DIREC);
		list_pos+=16;
		text_size[nfiles] = text_size[nfiles] << 1;
		nfiles++;
	}
	return 1;
}

void Print_Dir(byte pos){
	int i = 0;
	char show_dir[] = "DIR";
	list_pos = pos << 4;
	screen_pos = (6*160)+18;
	while (i < 15){
		int j = 0;
		for (text_pos = 0; text_pos < 26; text_pos+=2) {
			XGA_TEXT_MAP[screen_pos+text_pos] = file_list[list_pos+j];
			j++;
		}
		j = 0;
		if (file_attr[pos+i]){//DIR
			for (text_pos = 28; text_pos < 34; text_pos+=2){
				XGA_TEXT_MAP[screen_pos+text_pos] = show_dir[j];
				j++;
			}
		} else {
			for (text_pos = 28; text_pos < 34; text_pos+=2){
				XGA_TEXT_MAP[screen_pos+text_pos] = ' ';
				j++;
			}
		}
		list_pos+=16;
		screen_pos+=160;
		i++;
	}
}

void Reset_Dir(byte pos){
	int i;
	selected_cell = (160*6)+18+((file_number-pos_number)*160);
	pos_number = 0;
	file_number = pos;
	//for (i = 0; i < 160*12; i+=160) memcpy(&XGA_TEXT_MAP[(160*8)+46+i],del_dir,6);
	Print_Dir(0);
	for (i = 1; i < 26; i+=2) XGA_TEXT_MAP[(160*8)+18+i] = BLACK << 4 | WHITE;
	for (i = 1; i < 26; i+=2) XGA_TEXT_MAP[selected_cell+i] = BLUE << 4 | WHITE;
}

void Control_Menu(){
	int i;
	int j = 0;
	byte dir[13];
	//byte del_dir[] = {'A',BLUE << 4 | WHITE,' ',BLUE << 4 | WHITE,' ',BLUE << 4 | WHITE};
	if (control_timer == 8){
		if (kbhit()){
			byte character = getch();
			switch (character){
				case 13: //Enter
					//Get name
					for (j = 0; j < 13; j++) dir[j] = file_list[(file_number<<4)+j];
					//dir[text_size[file_number]+1] = 0;
					if (file_attr[file_number]) {
						//memcpy(dir,&file_list[(file_number<<4)],13);
						chdir(dir);
						Read_Dir();
						Reset_Dir(2);
					} else {
						for (i = 1; i < 9; i++) C_Volume[i] = 0;
						Load_Music(dir); //LOAD
					}
						
				break;
				case 52:
				case 54: if (file_number > 1) Reset_Dir(1); break; //left //right
				case 56: if (file_number > 1) file_number--; break; //up
				case 50: if (file_number < nfiles-1) file_number++; break; //down
				case 27: running = 0; break; //esc
				case 32: Stop_Music(); break;
				case 0: //Special Keys
					character = getch();
					if (character == 59) ; //F1
					if ((character == 75) || (character == 77)) //left //right
						if (file_number > 1) Reset_Dir(1); 
					if (character == 72) if (file_number > 1) file_number--; //up
					if (character == 80) if (file_number < nfiles-1) file_number++; //down
				break;
				//default: 
			}

			if (file_number > (pos_number+14)){pos_number++; Print_Dir(pos_number);}
			if (file_number < pos_number+1) {pos_number--; Print_Dir(pos_number);}
			selected_cell = (160*6)+18+((file_number-pos_number)*160);
			for (i = 1; i < 26; i+=2){
				XGA_TEXT_MAP[selected_cell-160+i] = BLUE << 4 | WHITE;
				XGA_TEXT_MAP[selected_cell+i] = GREEN << 4 | BLACK;
				XGA_TEXT_MAP[selected_cell+160+i] = BLUE << 4 | WHITE;
			}
			Clearkb();
		}
		control_timer = 0;
	}
	control_timer++;
}


//menu 80x25 text mode
void Set_Map(){
	int i = 0;
	int j = 0;
	byte color, character;
	
	if (GRAPHICS_CARD == 3){
		for (i = 0;i <(80*25*2);i+=2){
			//Set Character Map 
			character = GUI_MAP_CHAR[j];
			switch (character){
				case '_': character = 198;break;
				case '-': character = 195;break;
				case '(': character = 197;break;
				case ')': character = 199;break;
				case '[': character = 190;break;
				case ']': character = 191;break;
				case '$': character = 192;break;
				case '%': character = 193;break;
				case '?': character = 194;break;
				case '*': character = 196;break;
				case '.': character = 32;break;
				case '!': character = 200;break;
				case '{': character = 201;break;
				//BUBBLES
				case '#': character = 216;break;
				case '3': character = 217;break;
				case '4': character = 218;break;
				case '5': character = 219;break;
				case '7': character = 220;break;
				case 'a': character = 221;break;
				case '<': character = 222;break;
				case '>': character = 223;break;
			}
			XGA_TEXT_MAP[i] = character;
			//Set Color Map 
			color = GUI_MAP_COL[j];
			switch (color){
				case '0': color = BLACK << 4 | BLACK;break;
				case '1': color = RED << 4 | WHITE;break;
				case '2': color = BLUE << 4 | WHITE;break;
				case '3': color = CYAN << 4 | WHITE;break;
				case '4': color = MAGENTA << 4 | WHITE;break;
				case '5': color = GREEN << 4 | LIGHTGREEN;break;
				case '6': color = GREEN << 4 | WHITE;break;
				case '.':
				case '=': color = BLACK << 4 | WHITE;break;
				case '#': color = BLUE << 4 | BLACK;break;
			}
			XGA_TEXT_MAP[i+1] = color; 
			j++;
		}
		//Place adlib logo arRanging tiles at 6y+72x
		character = 202;//Adlib logo
		for (i = 4;i < 7;i++){
			for (j = 144; j < 150; j+=2){
				XGA_TEXT_MAP[(i*160)+j] = character++;
			}
		}
		for (j = 142; j < 152; j+=2) XGA_TEXT_MAP[(7*160)+j] = character++;
	} 
	if (GRAPHICS_CARD == 2){
		for (i = 0;i <(80*25*2);i+=2){
			//Set Character Map 
			character = GUI_MAP_CHAR_CGA[j];
			switch (character){
				case '+': character = 205;break;
				case '(': character = 201;break;
				case ')': character = 187;break;
				case '!': character = 186;break;
				case '?': character = 200;break;
				case '*': character = 188;break;
				case '.': character = 219;break;
				case '[': character = 223;break;
				case '{': character = 177;break;
			}
			XGA_TEXT_MAP[i] = character;
			//Set Color Map 
			color = GUI_MAP_COL_CGA[j];
			switch (color){
				case '0': color = BLACK << 4 | BLACK;break;
				case '1': color = RED << 4 | WHITE;break;
				case '2': color = BLUE << 4 | WHITE;break;
				case '3': color = CYAN << 4 | WHITE;break;
				case '4': color = MAGENTA << 4 | WHITE;break;
				case '5': color = GREEN << 4 | LIGHTGREEN;break;
				case '6': color = BLACK << 4 | WHITE;break;
				case '.': color = BLACK << 4 | DARKGRAY;break;
				case '#': color = BLUE << 4 | BLACK;break;
			}
			XGA_TEXT_MAP[i+1] = color; 
			j++;
		}
	}
}

byte GUI_MAP_CHAR[] = {"\
.(____________________________________________________________________________).\
.[              ADPLAY for 8088/86 & 286     Not Copyrighted  2021            ].\
.?----------------------------------------------------------------------------*.\
<>.............................................................<>...............\
...........(___________)......................................................<>\
.......(___$SELECT FILE%___).................(___________________)..............\
.......[                   ].....#34.......(_$CHANNEL  VISUALIZER%_)............\
.......[                   ].....57a.......[{{{{{{{{{{{{{{{{{{{{{{{]............\
.......[                   ].........<>....[{{{{{{{{{{{{{{{{{{{{{{{]............\
.......[                   ]...............[{{{{{{{{{{{{{{{{{{{{{{{]............\
34.....[                   ]..............#[{{{{{{{{{{{{{{{{{{{{{{{]............\
7a.....[                   ]..............5[{{{{{{{{{{{{{{{{{{{{{{{]............\
.......[                   ]...............[{{{{{{{{{{{{{{{{{{{{{{{]............\
.......[                   ]<>.............[{{{{{{{{{{{{{{{{{{{{{{{]....#34.....\
.......[                   ]..........#34..[{{{{{{{{{{{{{{{{{{{{{{{]....57a.....\
.......[                   ]..........57a..?-----------------------*............\
.......[                   ].....................(___________).................<\
.......[                   ].........(___________$MUSIC  INFO%___________)......\
.......[                   ].........[FORMAT:                            ]......\
.......[                   ]34.......[NAME:                              ]......\
.......[                   ]7a.......[AUTHOR:                            ]......\
.......[                   ].........?-----------------------------------*......\
.......?-------------------*......................................<>............\
#34..............................<>............................................#\
!! EXIT: ESC !! SELECT: ENTER !! VIS: F1 !! STOP: SPACE !!                    !!\
"                     
};

byte GUI_MAP_COL[] = {"\
.==============================================================================.\
.=4444444444444444444444444444444444444444444444444444444444444444444444444444=.\
.==============================================================================.\
................................................................................\
...........=============........................................................\
.......=====33333333333=====...............=========================............\
.......=2222222222222222222=...............===3333333333333333333===............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=00000000000000000000000=............\
.......=2222222222222222222=...............=========================............\
.......=2222222222222222222=.....................=============..................\
.......=2222222222222222222=.........=============33333333333=============......\
.......=2222222222222222222=.........=22222222222222222222222222222222222=......\
.......=2222222222222222222=.........=22222222222222222222222222222222222=......\
.......=2222222222222222222=.........=22222222222222222222222222222222222=......\
.......=2222222222222222222=.........=====================================......\
.......=====================....................................................\
................................................................................\
66111111222226611111111222222266111112222661111112222222661111111111111111111166\
"
};

byte GUI_MAP_CHAR_CGA[] = {"\
.(++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++).\
.!              ADPLAY for 8088/86 & 286  -  Not Copyrighted  2021            !.\
.?++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*.\
................................................................................\
................................................................................\
.......     SELECT FILE     ....................................................\
.......!                   !...............   CHANNEL  VISUALIZER   ............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............!{{{{{{{{{{{{{{{{{{{{{{{!............\
.......!                   !...............?+++++++++++++++++++++++*............\
.......!                   !....................................................\
.......!                   !.........             MUSIC  INFO             ......\
.......!                   !.........!FORMAT:                            !......\
.......!                   !.........!NAME:                              !......\
.......!                   !.........!AUTHOR:                            !......\
.......?+++++++++++++++++++*.........?+++++++++++++++++++++++++++++++++++*......\
................................................................................\
................................................................................\
[[ EXIT: ESC [[ SELECT: ENTER [[ VIS: F1 [[ STOP: SPACE [[                    [[\
"                     
};

byte GUI_MAP_COL_CGA[] = {"\
.444444444444444444444444444444444444444444444444444444444444444444444444444444.\
.444444444444444444444444444444444444444444444444444444444444444444444444444444#\
.444444444444444444444444444444444444444444444444444444444444444444444444444444#\
..##############################################################################\
................................................................................\
.......333333333333333333333....................................................\
.......222222222222222222222#..............3333333333333333333333333............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6000000000000000000000006............\
.......222222222222222222222#..............6666666666666666666666666............\
.......222222222222222222222#...................................................\
.......222222222222222222222#........3333333333333333333333333333333333333......\
.......222222222222222222222#........2222222222222222222222222222222222222#.....\
.......222222222222222222222#........2222222222222222222222222222222222222#.....\
.......222222222222222222222#........2222222222222222222222222222222222222#.....\
.......222222222222222222222#........2222222222222222222222222222222222222#.....\
........#####################.........#####################################.....\
................................................................................\
55111111222225511111111222222255111112222551111112222222551111111111111111111155\
"
};


// @iport - index port, @iport + 1 - data port
void vga_write_reg(word iport, byte reg, byte val){
	outportb(iport, reg);
	outportb(iport+1, val);
}

//1 bit png, data starts at 0x43
void Set_Tiles(unsigned char *font){
	int x,y,tileline,offset = 0,offset1 = 0;
	unsigned char *data = calloc(1024*4,1);
	unsigned char *data1 = calloc(1024*4,1);
	FILE *f = fopen(font,"rb");
	if (!f){free(data);free(data1);return;}
	fseek(f, 0x43, SEEK_SET);
	fread(data,1,16*256,f);
	fclose(f);
	
	// 1 bit png, each line is stored in 16 bytes + 1 extra at the end
	for (y = 0; y < 16*(16*17); y+=(16*17)){
		for (x = 0; x < 16; x++){
			offset = y + x;
			for (tileline = 0; tileline < 16*17; tileline +=17){
				data1[offset1] = data[offset+tileline];
				offset1++;
			}
		}
	}

	vga_write_reg(0x3C4, 0x04, 0x06);
	vga_write_reg(0x3Ce, 0x04, 0x02);
	asm{
		push bp
		mov	ax,1100h
		mov	bx,1000h
		
		mov	cx,256			// we'll change 256 tiles
		mov	dx,0			// Tile 0 
		mov	bh,16			// 16 bytes per char RAM block
		xor	bl,bl			
		mov	bp,word ptr[data1]
		int	10h				// video interrupt
		
		pop bp
	}
	vga_write_reg(0x3C4, 0x04, 0x03);
	vga_write_reg(0x3CE, 0x04, 0x00);
	free(data);free(data1);
}



//END