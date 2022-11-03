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
#include <dirent.h>
#include <sys/stat.h>
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
byte *MDA_TEXT_MAP=(byte *)0xB0000000L;
word *PARALEL1=(word *)0x00400008L;		//Paralel port address
word *PARALEL2=(word *)0x0040000AL;
word *PARALEL3=(word *)0x0040000CL;
extern unsigned char *ADPLUG_music_data;
byte running = 1;
byte GRAPHICS_CARD = 2;
byte MCGA = 0;
//ADLIB/SBlaster = 388
//SB16+ = 220 or 240
//opl2lpt = 378, 379 y 37A
int ADLIB_PORT = 0;//0x388;
byte OPL2LPT = 0;
int Sound_Blaster = 0;

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
void WaitVsync_MDA();
void WaitVsync_NOMDA();

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

byte file_number = 2;
byte pos_number = 0;
word selected_cell = 0;
byte control_timer = 0;
byte VIS_ON = 1;
byte v_timer = 0;

byte Color_Selected = 0;
byte Color_Not_Selected = 0;
byte Color_Erase_Bars = 0;
byte Color_Loading = 0;
byte Color_Loaded = 0;
byte Color_Error = 0;

extern unsigned char C_Volume[];

void Set_Tiles(unsigned char *font);
void Set_Map();
void Control_Menu();
byte Read_Dir();
void Print_Dir(byte pos);
void Display_Bars();

void Set_Cell_H(byte val){ //for tandy, set cell height to 8 pixels
	val = val -1;
	asm mov dx,03D4h
	asm mov al,0x09		//Select Max scan Line register
	asm out dx,al
	asm mov dx,03D5h
	asm mov al,val
	asm out dx,al
} 

void test();

void main(){
	//byte load;
	int i;
	Setup();
	Init();
	Set_Map();
	Read_Dir();
	Print_Dir(0);
	//Draw selection
	for (i = 1; i < 26; i+=2) XGA_TEXT_MAP[(160*8)+18+i] = Color_Selected;
	
	while(running){
		Control_Menu();
		if (VIS_ON) Display_Bars();
		if (GRAPHICS_CARD == 1) WaitVsync_MDA();
		else WaitVsync_NOMDA();
	}
	Exit_Dos();
}


//////////////////

void Setup(){
	int key = 0;
	int i;
	union REGS regs;
	system("cls");
	//Check graphics
	//1 MDA, Hercules; 2 CGA,  MCGA, Tandy, EGA; 3 VGA;
	regs.h.ah=0x1A;
	regs.h.al=0x00;
	regs.h.bl=0x32;
	int86(0x10, &regs, &regs);
	
	if (regs.h.al == 0x1A) {
		printf("\n\n\n\n\n\n");
		printf("                             SELECT GRAPHICS ADAPTER                            \n\n");
		printf("                      1 for VGA\n");
		printf("                      2 for MCGA\n");
		printf("                      3 for bad VGA/MCGA emulator with no VRAM font support");
		while(!kbhit());
		key = getch() -48;
		if (key == 1) GRAPHICS_CARD = 3;
		if (key == 2) {GRAPHICS_CARD = 3; MCGA = 1;}
		if (key == 3) GRAPHICS_CARD = 2;
		Color_Selected = GREEN << 4 | WHITE;
		Color_Not_Selected = BLUE << 4 | WHITE;
		Color_Erase_Bars = BLACK << 4 | WHITE;
		Color_Loading = BLINK | RED << 4 | WHITE;
		Color_Loaded = BLUE << 4 | WHITE;
		Color_Error = RED << 4 | WHITE;
	} else {
		regs.h.ah=0x12;
		regs.h.bh=0x10;
		regs.h.bl=0x10;
		int86(0x10, &regs, &regs);
		if (regs.h.bh == 0) {
			GRAPHICS_CARD = 2;
			Color_Selected = GREEN << 4 | WHITE;
			Color_Not_Selected = BLUE << 4 | WHITE;
			Color_Erase_Bars = BLACK << 4 | WHITE;
			Color_Loading = BLINK | RED << 4 | WHITE;
			Color_Loaded = BLUE << 4 | WHITE;
			Color_Error = RED << 4 | WHITE;
		} else {
			regs.h.ah=0x0F;
			regs.h.bl=0x00;
			int86(0x10, &regs, &regs);
			if (regs.h.al == 0x07) {
				GRAPHICS_CARD = 1;
				Color_Selected = 0x78;
				Color_Not_Selected = 0x10;
				Color_Erase_Bars = 0;
				Color_Loading = 0xF8;
				Color_Loaded = 0x10;
				Color_Error = 0x10;
				for (i = 1; i < 7; i++) VBar_Colors[i] = 0x10;
			} else {
				GRAPHICS_CARD = 2;
				Color_Selected = GREEN << 4 | WHITE;
				Color_Not_Selected = BLUE << 4 | WHITE;
				Color_Erase_Bars = BLACK << 4 | WHITE;
				Color_Loading = BLINK | RED << 4 | WHITE;
				Color_Loaded = BLUE << 4 | WHITE;
				Color_Error = RED << 4 | WHITE;
			}
		}
	}	
	system("cls");
	
	Clearkb();
	if (GRAPHICS_CARD == 2) Set_Cell_H(8);
	if (GRAPHICS_CARD == 3) Set_Tiles("Font_BIZ.png");//Before adding interrupt timer
	printf("\n\n\n\n\n\n");
	printf("                           SELECT PORT FOR OPL2 SOUND\n\n");
	printf("                      1 for ADLIB / Sound Blaster (port 388)\n");
	printf("                      2 for Sound Blaster 16 or newer (port 220)\n");
	printf("                      3 for Sound Blaster 16 or newer (port 240)\n");
	printf("                      4 for opl2 lpt1 \n");
	printf("                      5 for opl2 lpt2 \n\n\n");
	printf("               Press to select, or any other key for default  (388)");
	while(!kbhit());
	key = getch() -48;
	system("cls");	
	Clearkb();
	if (key == 1) {OPL2LPT = 0; ADLIB_PORT = 0x388;}
	else if (key == 2) {OPL2LPT = 0; ADLIB_PORT = 0x220;}
	else if (key == 3) {OPL2LPT = 0; ADLIB_PORT = 0x240;}
	else if (key == 4) {OPL2LPT = 1; ADLIB_PORT = PARALEL1[0];}
	else if (key == 5) {OPL2LPT = 1; ADLIB_PORT = PARALEL2[0];}
	else ADLIB_PORT = 0x388;
	printf("\n\n\n\n\n\n\n\n\n");
	printf("                           SOUND BLASTER DRUMS?   Y/N\n\n");      
	printf("                        Do not use with ADLIB, default = N\n");
	while(!kbhit());
	key = getch();
	if ((key == 89) || (key == 121))Sound_Blaster = 1;
	if ((key == 78) || (key == 110))Sound_Blaster = 0;
	system("cls");	
	Clearkb();
	//asm mov ax, 1h
	//asm int 10h
}

//VISUALIZER
void Get_Volume();
void Display_Bars(){
	if (v_timer == 2){
	int i,j;
	int map_pos = (160*8)+91;
	int col_pos = 0;
	byte col = 0;
	Get_Volume();
	for (i = 0; i < 11; i++){
		for (j = 7; j > 0; j--){
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
	int done;
	struct dirent *direntry;
	struct stat s;
	DIR *folder = opendir(".");
	nfiles = 0;
	text_pos = 0;
	screen_pos = 0;	
	list_pos = 0;
	memset(file_list,0,256*13);
	memset(file_attr,0,256);
	//Read Folders
	while((direntry = readdir(folder)) != NULL){
		stat(direntry->d_name,&s);
		if (s.st_mode & S_IFDIR){
			file_attr[nfiles] = 1;
			text_size[nfiles] = strlen(direntry->d_name);
			memcpy(&file_list[list_pos],direntry->d_name,text_size[nfiles]);
			list_pos+=16;
			text_size[nfiles] = text_size[nfiles] << 1;
			nfiles++;
		}
	}
	//Read Files
	rewinddir(folder);
	while((direntry = readdir(folder)) != NULL){
		stat(direntry->d_name,&s);
		if (!(s.st_mode & S_IFDIR)) {
			file_attr[nfiles] = 0;
			text_size[nfiles] = strlen(direntry->d_name);
			memcpy(&file_list[list_pos],direntry->d_name,text_size[nfiles]);
			list_pos+=16;
			text_size[nfiles] = text_size[nfiles] << 1;
			nfiles++;
		}
	}
	
	file_attr[0] = 0;
	file_attr[1] = 1;
	
	closedir(folder);
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
	Print_Dir(pos-1);
	for (i = 1; i < 26; i+=2) XGA_TEXT_MAP[(160*8)+18+i] = Color_Erase_Bars;
	for (i = 1; i < 26; i+=2) XGA_TEXT_MAP[selected_cell+i] = Color_Not_Selected;
}

void Control_Menu(){
	int i;
	byte dir[13];
	//byte del_dir[] = {'A',BLUE << 4 | WHITE,' ',BLUE << 4 | WHITE,' ',BLUE << 4 | WHITE};
	if (control_timer == 8){
		if (kbhit()){
			byte character = getch();
			switch (character){
				case 13: //Enter
					//Get name
					//for (j = 0; j < 13; j++) dir[j] = file_list[(file_number<<4)+j];
					memcpy(dir,&file_list[(file_number<<4)],13);
					//dir[text_size[file_number]+1] = 0;
					if (file_attr[file_number]){
						//memcpy(dir,&file_list[(file_number<<4)],13);
						chdir(dir);
						Read_Dir();
						Reset_Dir(1);
					} else {
						Stop_Music();
						if (GRAPHICS_CARD == 1) for (i = 0; i < 16; i++) WaitVsync_MDA();
						else WaitVsync_NOMDA();
						Load_Music(dir); //LOAD
					}
						
				break;
				case 52: if (file_number > 1) Reset_Dir(1); break; //left
				case 54: if (file_number < nfiles-1) Reset_Dir(nfiles-17); break; //right
				case 56: if (file_number > 1) file_number--; break; //up
				case 50: if (file_number < nfiles-1) file_number++; break; //down
				case 27: running = 0; break; //esc
				case 32: Stop_Music(); break;
				case 0: //Special Keys
					character = getch();
					if (character == 59){ //F1
						if (VIS_ON) VIS_ON = 0;
						else VIS_ON = 1;
					}
					if (character == 75) //left
						if (file_number > 1) Reset_Dir(1); 
					if (character == 77) //right
						if (file_number < nfiles-1) Reset_Dir(nfiles-17);
					if (character == 72) if (file_number > 1) file_number--; //up
					if (character == 80) if (file_number < nfiles-1) file_number++; //down
				break;
				//default: 
			}

			if (file_number > (pos_number+14)){pos_number++; Print_Dir(pos_number);}
			if (file_number < pos_number+1) {pos_number--; Print_Dir(pos_number);}
			selected_cell = (160*6)+18+((file_number-pos_number)*160);
			for (i = 1; i < 26; i+=2){
				XGA_TEXT_MAP[selected_cell-160+i] = Color_Not_Selected;
				XGA_TEXT_MAP[selected_cell+i] = Color_Selected;
				XGA_TEXT_MAP[selected_cell+160+i] = Color_Not_Selected;
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
	byte color,character,bkg;
	FILE *MAP;
	
	if (GRAPHICS_CARD == 3) {
		MAP = fopen("VGA_MAP.bin","rb");
		if (!MAP) {printf("\nCan't find VGA_MAP.bin\n"); sleep(1); Exit_Dos();}
		fread(XGA_TEXT_MAP,2,80*25,MAP);
	}
	if (GRAPHICS_CARD == 2) {//CGA/TANDY
		MAP = fopen("CGA_MAP.bin","rb");
		if (!MAP) {printf("\nCan't find CGA_MAP.bin\n"); sleep(1); Exit_Dos();}
		fread(XGA_TEXT_MAP,2,80*25,MAP);
	}
	if (GRAPHICS_CARD == 1) {//MDA
		XGA_TEXT_MAP = MDA_TEXT_MAP;
		MAP = fopen("CGA_MAP.bin","rb");
		if (!MAP) {printf("\nCan't find CGA_MAP.bin\n"); sleep(1); Exit_Dos();}
		fread(XGA_TEXT_MAP,2,80*25,MAP);
		for (i = 0;i <(80*25*2);i+=2){
			character = XGA_TEXT_MAP[i];
			if ((character == 221) || (character == 222)) character = 219;
			XGA_TEXT_MAP[i] = character;
			//MDA Attribute:
			color = XGA_TEXT_MAP[i+1];
			bkg = color >> 4;
			//Bits 0-2: 0 => no underline. //Bit 3 = 1, High intensity.
			if ( (color == 0x2f) || (character == 219) 
			|| (character == 220) || (character == 223)
			|| (bkg == 4)         || (bkg == 5)       ) color = 0x78;//If borders, titles etc
			else color = 0x10; //Normal
			XGA_TEXT_MAP[i+1] = color;
		}
	}
}

// @iport - index port, @iport + 1 - data port
void vga_write_reg(word iport, byte reg, byte val){
	outportb(iport, reg);
	outportb(iport+1, val);
}
#define word_out(port,register,value) \
  outport(port,(((word)value<<8) + register))
//1 bit png, data starts at 0x43
void Set_Tiles(unsigned char *font){
	int x,y,tileline,offset = 0,offset1 = 0;
	unsigned char *data = calloc(1024*4,1);
	unsigned char *data1 = calloc(1024*4,1);
	byte n0 = 0;//MCGA font pages
	byte n1 = 2;
	FILE *f = fopen(font,"rb");
	if (!f){free(data);free(data1);return;}
	fseek(f, 0x43, SEEK_SET);
	fread(data,1,16*256,f);
	fclose(f);
	
	if (!MCGA){
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
		//asm mov ax, 1112h
		//asm xor bl, bl
		//asm int 10h //Cells are 8 pixel tall
		//vga_write_reg(0x3C4, 0x01, 0x01);	//9/8DM -- 9/8 Dot Mode, text cells are 8 pixels wide
		
		//Write tiles
		//Write tiles
		/*outport(0x3C4,0x0402);
		outport(0x3C4,0x0704);
		outport(0x3CE,0x0005);
		outport(0x3CE,0x0406);
		outport(0x3CE,0x0204);
		
		offset1 = 0;
		//It looks like VGA has 8x32 character glyphs 
		for (y = 0; y < 32*256; y+=32){
			memcpy(&XGA[y],&data1[offset1],16);
			offset1+=16;
		}
		outport(0x3C4,0x0302);
		outport(0x3C4,0x0304);
		outport(0x3CE,0x1005);
		outport(0x3CE,0x0E06);
		outport(0x3CE,0x0004);*/
		//vga_write_reg(0x3C4, 0x04, 0x06);
		//vga_write_reg(0x3Ce, 0x04, 0x02);
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
		//vga_write_reg(0x3C4, 0x04, 0x03);
		//vga_write_reg(0x3CE, 0x04, 0x00);
	} else {
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
		offset1 = 0;
		//It looks like MCGA has 8x16 character glyphs 
		for (y = 0; y < 16*256; y+=16){
			memcpy(&XGA[y],&data1[offset1],16);
			offset1+=16;
		}
		
		//MCGA has 4 tables (8kb each), from VRAM A000:0000 to A000:6000
		//Each table has 16 512-byte lists of character codes and bit patterns
		//	(Each list corresponds to one scan line of the characters being defined)
		//Every list has this structure: Char_code,bit_pattern,char_code_bit_pattern...
	
		//To use custom characters, you first select two font pages, then upload tables to vram
		//n0,n1 = font page values
		//Update MCGA Font Pages
		asm mov     ax,1103h	//AH := INT 10H function number
								//AL := 3 (Set Block Specifier)
		asm mov     bl,n1		//n1 BL := value for bits 2-3
		asm shl     bl,1
		asm shl     bl,1		//BL bits 2-3 := n1
		asm or      bl,n0		//n0 BL bits 0-1 := n0
		asm int     10h			//load font pages

		/*
		//Int 10h, 1100h  Load User-Specifed Alpha Font  EGA, MCGA, VGA
		//Changes font to user-defined font in text mode.
		asm push bp
        asm mov AX,1100h
        asm mov BH,16	//Bytes per character
        asm mov BL,0	//Font block to load (EGA/MCGA 0..3 | VGA 0..7)
        asm mov CX,128	//Number of characters
        asm mov DX,0	//Number of first character
		asm xor bp,bp
		//ES:BP	Pointer to character bitmaps
		asm mov	bp,word ptr[data1]
		asm int 10h	//Load font
		asm pop bp*/
	}
	
	free(data);free(data1);
}



//END