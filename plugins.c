#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <alloc.h>
#include <mem.h>
#include <conio.h>
#undef outp

#define SB_RESET 0x6
#define SB_READ_DATA 0xA
#define SB_READ_DATA_STATUS 0xE
#define SB_WRITE_DATA 0xC
#define SB_ENABLE_SPEAKER 0xD1
#define SB_DISABLE_SPEAKER 0xD3
#define SB_SET_PLAYBACK_FREQUENCY 0x40
#define SB_SINGLE_CYCLE_8PCM 0x14
#define SB_SINGLE_CYCLE_4ADPCM 0x74
#define SB_SINGLE_CYCLE_3ADPCM 0x76
#define SB_SINGLE_CYCLE_2ADPCM 0x16

#define MASK_REGISTER 0x0A
#define MODE_REGISTER 0x0B
#define MSB_LSB_FLIP_FLOP 0x0C
#define DMA_CHANNEL_0 0x87
#define DMA_CHANNEL_1 0x83
#define DMA_CHANNEL_3 0x82

/* macro to write a word to a port */
#define word_out(port,register,value) \
  outport(port,(((word)value<<8) + register))

#define true 1
#define false 0

#define JUMPMARKER	0x80	// MODULES Orderlist jump marker

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

//TIME interrupt
void interrupt (*old_time_handler)(void);
void interrupt CimfPlayer_update(void);
void interrupt CmodPlayer_update(void);
void interrupt CldsPlayer_update(void);
void /*interrupt*/ Cd00Player_update();
void interrupt CVGM_update(void);
void Exit_Dos();
void Set_Tiles(unsigned char *font);
	
extern int ADLIB_PORT;
char Initial_working_dir[32];
	
extern byte Color_Loading;
extern byte Color_Loaded;
extern byte Color_Error;	
	
	//////////
	//TABLES//
	//////////

unsigned char *ADPLUG_music_data;

const unsigned short CPlayer_note_table[12] =
  {363, 385, 408, 432, 458, 485, 514, 544, 577, 611, 647, 686};

const unsigned short d00_notetable[] =	// D00 note table
	{340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 340,363,385,408,432,458,485,514,544,577,611,647,
	 };
  
int d00_MUL64_DIV63[64][64];// (0 - 64) *(0 - 64) / 63

// SA2 compatible adlib note table
const unsigned short CmodPlayer_sa2_notetable[12] =
  {340,363,385,408,432,458,485,514,544,577,611,647};

// SA2 compatible vibrato rate table
const unsigned char CmodPlayer_vibratotab[32] =
  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

const unsigned char CPlayer_op_table[9] =
  {0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12};

//Channels volume
unsigned char C_Volume[9] = {0,0,0,0,0,0,0,0,0};


	///////////
	//STRUCTS//
	///////////

//IMF
typedef struct{
	byte reg;
	byte val;
	word time;
} m_data;


//MODULES (RAD SA2 AMD)
typedef struct{
    unsigned char data[11],arpstart,arpspeed,arppos,arpspdcnt,misc;
    signed char slide;
} mod_inst;

typedef struct{
    unsigned char note,command,inst,param2,param1;
} mod_tracks;

typedef struct{
    unsigned short freq,nextfreq;
    unsigned char oct,vol1,vol2,inst,fx,info1,info2,key,nextoct,
    note,portainfo,vibinfo1,vibinfo2,arppos,arpspdcnt;
    signed char trigger;
} mod_channel;

enum Flags {
    Standard = 0,
    Decimal = 1 << 0,
    Faust = 1 << 1,
    NoKeyOn = 1 << 2,
    Opl3 = 1 << 3,
    Tremolo = 1 << 4,
    Vibrato = 1 << 5,
    Percussion = 1 << 6
};

//LOUDNESS
int MOD_POS = 0;
unsigned char *MOD_DATA;

typedef struct {
	unsigned char	mod_misc, mod_vol, mod_ad, mod_sr, mod_wave,
    car_misc, car_vol, car_ad, car_sr, car_wave, feedback, keyoff,
    portamento, glide, finetune, vibrato, vibdelay, mod_trem, car_trem,
    tremwait, arpeggio, arp_tab[12];
	unsigned short	start, size;
	unsigned char	fms;
	unsigned short	transp;
	unsigned char	midinst, midvelo, midkey, midtrans, middum1, middum2;
} LDS_SoundBank;

typedef struct {
	unsigned short	gototune, lasttune, packpos;
	unsigned char	finetune, glideto, portspeed, nextvol, volmod, volcar,
    vibwait, vibspeed, vibrate, trmstay, trmwait, trmspeed, trmrate, trmcount,
    trcwait, trcspeed, trcrate, trccount, arp_size, arp_speed, keycount,
    vibcount, arp_pos, arp_count, packwait, arp_tab[12];

	struct {
		unsigned char	chandelay, sound;
		unsigned short	high;
	} chancheat;
} LDS_Channel;

typedef struct {
	unsigned short	patnum;
	unsigned char	transpose;
} LDS_Position;


//EDLIB TRACKER (D00)

typedef struct {
	char id[6];
	unsigned char type,version,speed,subsongs,soundcard;
	char songname[32],author[32],dummy[32];
	unsigned short tpoin,seqptr,instptr,infoptr,spfxptr,endmark;
}d00header ;

typedef struct {
	unsigned char version,speed,subsongs;
	unsigned short tpoin,seqptr,instptr,infoptr,lpulptr,endmark;
}d00header1 ;

typedef struct {
	unsigned short	*order,ordpos,pattpos,del,speed,rhcnt,key,freq,inst,spfx,ispfx,irhcnt;
	signed short	transpose,slide,slideval,vibspeed;
	unsigned char	seqend,vol,vibdepth,fxdel,modvol,cvol,levpuls,frameskip,nextnote,note,ilevpuls,trigger;
}d00channel;

struct Sinsts {
	unsigned char data[11],tunelev,timer,sr,dummy[2];
} *d00_inst;

struct Sspfx {
	unsigned short instnr;
	signed char halfnote;
	unsigned char modlev;
	signed char modlevadd;
	unsigned char duration;
	unsigned short ptr;
} *spfx;

struct Slevpuls {
	unsigned char level;
	signed char voladd;
	unsigned char duration,ptr;
} *levpuls;



	/////////////
	//VARIABLES//
	/////////////

//To select music format
int selected_player = 0;

//IMF
m_data *musicdata = NULL;

byte songend;
word size;
word mfsize;
word songlen;
word pos;
word del = 0;
byte sng_delay = 0;
word sng_loop;
word timer;
word rate;
word start;
word compressed;
unsigned char footer[32];
unsigned char track_name[32];
unsigned char game_name[32];
unsigned char author_name[32];
unsigned char remarks[32];

//RAD AMD SA2
byte MOD_PCM = 0;
byte songend;
word rate;
unsigned char order[128];
unsigned char arplist[256], arpcmd[256], initspeed;
unsigned short tempo, bpm, nop;
word length, restartpos, activechan;
word flags;
unsigned char version,radflags;
char desc[80*22];
unsigned long rw, ord; 
unsigned long nrows = 64; 
unsigned long npats = 64;
unsigned long nchans = 9;
unsigned char speed, mod_del, regbd;
char songname[24],author[24],instname[26][23];
unsigned short trackord[64][9];//[npats][nchans];
mod_tracks **tracks;//[npats*nchans][nrows]; 36864
mod_channel channel[9];//[nchans];
mod_inst inst[250];

byte voiceKeyOn[11];
byte notePitch[11];
int	halfToneOffset[11];
word *fNumFreqPtr[11];
byte noteDIV12[256]; //Store note/12
byte noteMOD12[96];

//Loudness
FILE *modfile;
// Note frequency table (16 notes / octave)
unsigned short lds_frequency[] = {
  343, 344, 345, 347, 348, 349, 350, 352, 353, 354, 356, 357, 358,
  359, 361, 362, 363, 365, 366, 367, 369, 370, 371, 373, 374, 375,
  377, 378, 379, 381, 382, 384, 385, 386, 388, 389, 391, 392, 393,
  395, 396, 398, 399, 401, 402, 403, 405, 406, 408, 409, 411, 412,
  414, 415, 417, 418, 420, 421, 423, 424, 426, 427, 429, 430, 432,
  434, 435, 437, 438, 440, 442, 443, 445, 446, 448, 450, 451, 453,
  454, 456, 458, 459, 461, 463, 464, 466, 468, 469, 471, 473, 475,
  476, 478, 480, 481, 483, 485, 487, 488, 490, 492, 494, 496, 497,
  499, 501, 503, 505, 506, 508, 510, 512, 514, 516, 518, 519, 521,
  523, 525, 527, 529, 531, 533, 535, 537, 538, 540, 542, 544, 546,
  548, 550, 552, 554, 556, 558, 560, 562, 564, 566, 568, 571, 573,
  575, 577, 579, 581, 583, 585, 587, 589, 591, 594, 596, 598, 600,
  602, 604, 607, 609, 611, 613, 615, 618, 620, 622, 624, 627, 629,
  631, 633, 636, 638, 640, 643, 645, 647, 650, 652, 654, 657, 659,
  662, 664, 666, 669, 671, 674, 676, 678, 681, 683
};

// Vibrato (sine) table
unsigned char lds_vibtab[] = {
  0, 13, 25, 37, 50, 62, 74, 86, 98, 109, 120, 131, 142, 152, 162,
  171, 180, 189, 197, 205, 212, 219, 225, 231, 236, 240, 244, 247,
  250, 252, 254, 255, 255, 255, 254, 252, 250, 247, 244, 240, 236,
  231, 225, 219, 212, 205, 197, 189, 180, 171, 162, 152, 142, 131,
  120, 109, 98, 86, 74, 62, 50, 37, 25, 13
};

// Tremolo (sine * sine) table
unsigned char lds_tremtab[] = {
  0, 0, 1, 1, 2, 4, 5, 7, 10, 12, 15, 18, 21, 25, 29, 33, 37, 42, 47,
  52, 57, 62, 67, 73, 79, 85, 90, 97, 103, 109, 115, 121, 128, 134,
  140, 146, 152, 158, 165, 170, 176, 182, 188, 193, 198, 203, 208,
  213, 218, 222, 226, 230, 234, 237, 240, 243, 245, 248, 250, 251,
  253, 254, 254, 255, 255, 255, 254, 254, 253, 251, 250, 248, 245,
  243, 240, 237, 234, 230, 226, 222, 218, 213, 208, 203, 198, 193,
  188, 182, 176, 170, 165, 158, 152, 146, 140, 134, 127, 121, 115,
  109, 103, 97, 90, 85, 79, 73, 67, 62, 57, 52, 47, 42, 37, 33, 29,
  25, 21, 18, 15, 12, 10, 7, 5, 4, 2, 1, 1, 0
};

// 'maxsound' is maximum number of patches (instruments)
// 'maxpos' is maximum number of entries in position list (orderlist)
unsigned short  lds_maxsound = 0x3f,  lds_maxpos = 0xff;
LDS_SoundBank	lds_soundbank[0x3f];
LDS_Channel	*lds_channel;
LDS_Position *lds_positions;
unsigned char fmchip[0xff], jumping, fadeonoff, allvolume, hardfade,
  tempo_now, pattplay, louness_tempo, regbd, chandelay[9], mode, pattlen;
unsigned short	posplay, jumppos, *lds_patterns, lds_speed;
byte playing, songlooped;
unsigned int numpatch, numposi, patterns_size, mainvolume;
int *lds_DIV1216;


//EDLIB TRACKER (D00)
unsigned char instrument_test[11];
unsigned char songend,version;
unsigned char *datainfo;
unsigned short *seqptr;
d00header *header;
d00header1 *header1;
d00channel d00_channel[9];
unsigned char *filedata;

//VGM
word vgm_music_wait = 0;
word vgm_music_offset = 0;
word vgm_data_size = 0;
word vgm_loop = 0;
word vgm_loop_size = 0;

//HERAD
#define HERAD_NUM_VOICES	9
#define HERAD_NUM_NOTES		12
#define HERAD_INST_SIZE		40
#define HERAD_MAX_TRACKS	21
#define HERAD_MAX_SIZE		65535

static const byte slot_offset[HERAD_NUM_VOICES];
static const word FNum[HERAD_NUM_NOTES];
static const byte fine_bend[HERAD_NUM_NOTES + 1];
static const byte coarse_bend[10];

byte songend;
int wTime;
dword ticks_pos;	/* current tick counter */
dword total_ticks;	/* total ticks in song */

byte comp;			/* File compression type (see HERAD_COMP_*) */
byte AGD;			/* Whether this is HERAD AGD (OPL3) */
byte v2;			/* Whether this is HERAD version 2 */
byte nTracks;		/* Number of tracks */
byte nInsts;		/* Number of instruments */

word wLoopStart;	/* Loop starts at this measure (0 = don't loop) */
word wLoopEnd;		/* Loop ends at this measure (0 = don't loop) */
word wLoopCount;	/* Number of times the selected measures will play (0 = loop forever; >0 - play N times) */
word wSpeed;		/* Fixed point value that controls music speed. Value range is 0x0100 - 0x8100 */

typedef struct{
	// stored variables
	word size;			// data size 
	byte *data;			// event data 

	// variables for playback
	word pos;			// data position 
	dword counter;			// tick counter 
	word ticks;			// ticks to wait for next event 
} herad_trk ;

typedef struct{
	byte program;			// current instrument 
	byte playprog;			// current playing instrument (used for keymap) 
	byte note;			// current note 
	byte keyon;				// note is active 
	byte bend;			// current pitch bend 
	byte slide_dur;		// pitch slide duration 
} herad_chn ;

typedef struct {
	char mode;			// instrument mode (see HERAD_INSTMODE_*) 
	byte voice;			// voice number, unused 
	byte mod_ksl;			// modulator: KSL 
	byte mod_mul;			// modulator: frequency multiplier 
	byte feedback;			// feedback 
	byte mod_A;			// modulator: attack 
	byte mod_S;			// modulator: sustain 
	byte mod_eg;			// modulator: envelope gain 
	byte mod_D;			// modulator: decay 
	byte mod_R;			// modulator: release 
	byte mod_out;			// modulator: output level 
	byte mod_am;			// modulator: amplitude modulation (tremolo) 
	byte mod_vib;			// modulator: frequency vibrato 
	byte mod_ksr;			// modulator: key scaling/envelope rate 
	byte con;			// connector 
	byte car_ksl;			// carrier: KSL 
	byte car_mul;			// carrier: frequency multiplier 
	byte pan;			// panning 
	byte car_A;			// carrier: attack 
	byte car_S;			// carrier: sustain 
	byte car_eg;			// carrier: envelope gain 
	byte car_D;			// carrier: decay 
	byte car_R;			// carrier: release 
	byte car_out;			// carrier: output level 
	byte car_am;			// carrier: amplitude modulation (tremolo) 
	byte car_vib;			// carrier: frequency vibrato 
	byte car_ksr;			// carrier: key scaling/envelope rate 
	char mc_fb_at;			// macro: modify feedback with aftertouch 
	byte mod_wave;			// modulator: waveform select 
	byte car_wave;			// carrier: waveform select 
	char mc_mod_out_vel;		// macro: modify modulator output level with note velocity 
	char mc_car_out_vel;		// macro: modify carrier output level with note velocity 
	char mc_fb_vel;			// macro: modify feedback with note velocity 
	byte mc_slide_coarse;	// macro: pitch slide coarse tune 
	byte mc_transpose;		// macro: root note transpose 
	byte mc_slide_dur;		// macro: pitch slide duration (in ticks) 
	char mc_slide_range;		// macro: pitch slide range 
	byte dummy;			// unknown value 
	char mc_mod_out_at;		// macro: modify modulator output level with aftertouch 
	char mc_car_out_at;		// macro: modify carrier output level with aftertouch 
} herad_inst_data;

typedef struct {
	char mode;			// same as herad_inst_data 
	byte voice;			// same as herad_inst_data 
	byte offset;			// keymap start note: 0 => C2 (24), 24 => C4 (48) 
	byte dummy;			// unknown value 
	byte index[HERAD_INST_SIZE-4];	// array of instruments 
} herad_keymap ;

typedef struct{
	byte data[HERAD_INST_SIZE];	// array of 8-bit parameters 
	herad_inst_data param;			// structure of parameters 
	herad_keymap keymap;			// keymap structure 
} herad_inst ;

herad_trk track[21];				// event tracks [nTracks] 
herad_chn chn[21];					// active channels [nTracks] 
herad_inst her_inst[32];			// instruments [nInsts] 

dword loop_pos;
word loop_times;
herad_trk loop_data[HERAD_MAX_TRACKS];

	////////////
	//FUNCTIOS//
	//////////// 
	
char empty_str[] = "                           ";	
char empty_str1[] = "                             ";
char Key_Hit[9] = {0,0,0,0,0,0,0,0,0};
void Get_Volume(){
	int i;
	for (i = 0; i < 9; i++) {
		switch (selected_player){
			case 3:
				if (Key_Hit[i]) C_Volume[i] = channel[i].vol1;
				else if (C_Volume[i] > 4) C_Volume[i] -=4;
				Key_Hit[i] = 0;
			break;
			case 4:
				if (Key_Hit[i]) C_Volume[i] = lds_channel[i].volcar;
				else if (C_Volume[i] > 4) C_Volume[i] -=4;
				Key_Hit[i] = 0; 
			break;
			case 0:
			case 5:
			case 6:
				if (Key_Hit[i]) C_Volume[i] = 56;
				else if (C_Volume[i] > 4) C_Volume[i] -=4;
				Key_Hit[i] = 0; 
			break;
		}
	}
}

void Print_Loading(){
	gotoxy(60,25); 
	textattr(Color_Loading);
	cprintf("LOADING...");
}

void Print_Loaded(char *format, char *name, char *author){
	textattr(Color_Loaded); //*
	gotoxy(47,19);
		cprintf("%s",empty_str);
	gotoxy(45,20); 
		cprintf("%s",empty_str1);
	gotoxy(47,21); 
		cprintf("%s",empty_str);
		
	gotoxy(47,19);
		cprintf("%s",format);
	gotoxy(45,20); 
		cprintf("%s",name);
	gotoxy(47,21); 
		cprintf("%s",author);
	gotoxy(60,25); 
	textattr(Color_Error);
	cprintf("LOADED            ");
}

void Print_ERROR(){
	gotoxy(60,25); 
	textattr(Color_Error); //*
	cprintf("ERROR             ");
}

//OPL2

void (*opl2_write)(unsigned char, unsigned char);

void opl2_write0(unsigned char reg, unsigned char data){
	asm mov ah,0
	asm mov dx, ADLIB_PORT
	asm mov al, reg
	asm out dx, al
	
	//Wait at least 3.3 microseconds
	asm mov cx,6
	wait:
		asm in ax,dx
		asm loop wait	//for (i = 0; i < 6; i++) inp(lpt_ctrl);
	
	asm inc dx
	asm mov al, data
	asm out dx, al
	
	// Wait at least 23 microseconds
    //for (i = 0; i < 35; i++) inp(lpt_ctrl);
}

void opl2lpt_write(unsigned char reg, unsigned char data) {  
	// Select OPL2 register
	asm mov dx, ADLIB_PORT	
	asm mov al, reg
	asm out dx, al	//outp(ADLIB_PORT, reg);
	
	asm add dx, 2	//ADLIB_PORT+2 (lpt_ctrl)
    asm mov al, 13
	asm out dx, al	//outp(lpt_ctrl, 13);
	asm mov al, 9
	asm out dx, al	//outp(lpt_ctrl, 9);
	asm mov al, 13
    asm out dx, al	//outp(lpt_ctrl, 13);

    //Wait at least 3.3 microseconds
	asm mov cx,6
	wait:
		asm in ax,dx
		asm loop wait	//for (i = 0; i < 6; i++) inp(lpt_ctrl);

    // Set value
	asm sub dx, 2	//ADLIB_PORT	
	asm mov al, data
	asm out dx, al	//outp(ADLIB_PORT, data);
    
	asm add dx, 2	//ADLIB_PORT+2
    asm mov al, 12
	asm out dx, al	//outp(lpt_ctrl, 12);
	asm mov al, 8
	asm out dx, al	//outp(lpt_ctrl, 8);
	asm mov al, 12
    asm out dx, al	//outp(lpt_ctrl, 12);

    // Wait at least 23 microseconds
    //for (i = 0; i < 35; i++) inp(lpt_ctrl);
}

void Adlib_Detect(){ 
    int status1, status2, i;

    opl2_write(4, 0x60);
    opl2_write(4, 0x80);

    status1 = inp(ADLIB_PORT);
    
    opl2_write(2, 0xFF);
    opl2_write(4, 0x21);

    for (i=100; i>0; i--) inp(ADLIB_PORT);

    status2 = inp(ADLIB_PORT);
    
    opl2_write(4, 0x60);
    opl2_write(4, 0x80);

    if ( ( ( status1 & 0xe0 ) == 0x00 ) && ( ( status2 & 0xe0 ) == 0xc0 ) ){
        unsigned char i;
		for (i=1; i<=0xF5; opl2_write(i++, 0));    //clear all registers
		opl2_write(1, 0x20);  // Set WSE=1
		printf("\nOPL2 compatible card detected.\n");
		sleep(2);
        return;
    } else {
		printf("\nError: OPL2 compatible card not detected\n");
		sleep(2);
		exit(1);
        return;
    }
}

void opl2_clear(void){
	int i, slot1, slot2;
    static unsigned char slotVoice[9][2] = {{0,3},{1,4},{2,5},{6,9},{7,10},{8,11},{12,15},{13,16},{14,17}};
    static unsigned char offsetSlot[18] = {0,1,2,3,4,5,8,9,10,11,12,13,16,17,18,19,20,21};
    
    opl2_write(   1, 0x20);   // Set WSE=1
    opl2_write(   8,    0);   // Set CSM=0 & SEL=0
    opl2_write(0xBD,    0);   // Set AM Depth, VIB depth & Rhythm = 0
    
    for(i=0; i<9; i++)
    {
        slot1 = offsetSlot[slotVoice[i][0]];
        slot2 = offsetSlot[slotVoice[i][1]];
        
        opl2_write(0xB0+i, 0);    //turn note off
        opl2_write(0xA0+i, 0);    //clear frequency

        opl2_write(0xE0+slot1, 0);
        opl2_write(0xE0+slot2, 0);

        opl2_write(0x60+slot1, 0xff);
        opl2_write(0x60+slot2, 0xff);
        opl2_write(0x80+slot1, 0xff);
        opl2_write(0x80+slot2, 0xff);

        opl2_write(0x40+slot1, 0xff);
        opl2_write(0x40+slot2, 0xff);
    }
	for (i = 0; i < 6; i++) inp(0x0388);
    for (i=1; i< 256; opl2_write(i++, 0));    //clear all registers
}

//SOUND BLASTER PCM
short sb_base = 220;
char sb_irq = 7;
char sb_dma = 1;
//void interrupt ( *old_irq )();

unsigned char *dma_buffer;
int page;
int sb_offset;

typedef struct{
	int offset;
	int size;
} audio_sample;

audio_sample sample[16];
int LT_sb_nsamples = 0;
int LT_sb_offset = 0;

void interrupt sb_irq_handler(){
	inportb(sb_base + SB_READ_DATA_STATUS);
	outportb( 0x20, 0x20 );
}

int reset_dsp(short port){
	outportb( port + SB_RESET, 1 );
	delay(10);
	outportb( port + SB_RESET, 0 );
	delay(10);
	if( ((inportb(port + SB_READ_DATA_STATUS) & 0x80) == 0x80) && (inportb(port + SB_READ_DATA) == 0x0AA )) {
		sb_base = port;
		return 1;
	}
	return 0;
}

void write_dsp( unsigned char command ){
	while( (inportb( sb_base + SB_WRITE_DATA ) & 0x80) == 0x80 );
	outportb( sb_base + SB_WRITE_DATA, command );
}

void sb_init(){
	char temp;
	long linear_address;
	// possible values: 210, 220, 230, 240, 250, 260, 280
	for( temp = 1; temp < 9; temp++ ) {
		if( temp != 7 ) if( reset_dsp(0x200 + (temp << 4))) break;
	}
	if(temp == 9) return;
	
	//old_irq = getvect( sb_irq + 8 );
	//setvect( sb_irq + 8, sb_irq_handler );
	//outportb( 0x21, inportb( 0x21 ) & !(1 << sb_irq) );
	
	linear_address = FP_SEG(dma_buffer);
	linear_address = (linear_address << 4)+FP_OFF(dma_buffer);
	
	page = linear_address >> 16;
	sb_offset = linear_address & 0xFFFF;
	
	write_dsp( SB_ENABLE_SPEAKER );
}

void sb_deinit(){
	write_dsp( SB_DISABLE_SPEAKER );
	//setvect( sb_irq + 8, old_irq );
	//outportb( 0x21, inportb( 0x21 ) & (1 << sb_irq) );
	free(dma_buffer);
}

void Clear_Samples(){
	LT_sb_nsamples = 0;
	LT_sb_offset = 0;
	memset(dma_buffer,0x00,16*1024);
}

void sb_load_sample(char *file_name){
	FILE *raw_file;
	if (LT_sb_offset > 31*1024) ;
	sample[LT_sb_nsamples].offset = LT_sb_offset;
	raw_file = fopen(file_name, "rb");
	fseek(raw_file, 0x2C, SEEK_SET);
	sample[LT_sb_nsamples].size = fread(dma_buffer+LT_sb_offset, 1, 16*1024, raw_file);
	LT_sb_offset += sample[LT_sb_nsamples].size;
	LT_sb_nsamples++;
	fclose(raw_file);
}

void sb_play_sample(char sample_number, int freq){
	int pos = sample[sample_number].offset;
	int length = sample[sample_number].size -1;

	//Playback
	outportb( MASK_REGISTER, 4 | 1 /*sb_dma*/ );
	outportb( MSB_LSB_FLIP_FLOP, 0 );
	outportb( MODE_REGISTER, 0x48 | 1 /*sb_dma*/ );
	outportb( DMA_CHANNEL_1, page); 
	
	// program the DMA controller
	outportb( sb_dma << 1, (sb_offset+pos) & 0xFF );
	outportb( sb_dma << 1, (sb_offset+pos) >> 8 );
	outportb((sb_dma << 1) + 1, length & 0xFF );
	outportb((sb_dma << 1) + 1, length >> 8 );
	
	write_dsp(SB_SET_PLAYBACK_FREQUENCY);
	write_dsp(256-1000000/freq);
	
	outportb(MASK_REGISTER, sb_dma);
	write_dsp(SB_SINGLE_CYCLE_8PCM);
	write_dsp(length & 0xFF);
	write_dsp(length >> 8);
}

void sb_setup(){
	unsigned char *temp_buf;
	long linear_address;
	short page1, page2;
	
	//Allocate the first 32 KB of temp data inside a 64KB block for DMA
	temp_buf = farcalloc(16*1024,sizeof(byte));
	linear_address = FP_SEG(temp_buf);
	linear_address = (linear_address << 4)+FP_OFF(temp_buf);
	page1 = linear_address >> 16;
	page2 = (linear_address + 16*1024) >> 16;
	if( page1 != page2 ) {
		dma_buffer = farcalloc(16*1024,sizeof(byte));
		free( temp_buf );
	} else dma_buffer = temp_buf;
	sb_init();
}

void (*MOD_PCM_Drums)(byte);

void MOD_PCM_Drums_OFF(byte chan){};

void MOD_PCM_Drums_ON(byte chan){
	sb_play_sample(chan,11025);
};



//INTERRUPS

//Edlib player is too big to be in an interrupt
//Load it here
void (*Non_Interrupt_Player)();
void Void_function(){};


void Music_Add_Interrupt(int rate){
	//set interrupt and start playing music
	//unsigned long spd = 1193182/rate;
	unsigned long spd;
	if (rate) spd = 1193182/rate;
	else spd = 0;
	opl2_clear();
	asm CLI //disable interrupts
	switch (selected_player){
		//interrupt 1C not available on NEC 9800-series PCs.
		case 0: setvect(0x1C, CimfPlayer_update); break;
		//case 1: setvect(0x1C, ); break;
		//case 2: setvect(0x1C, ); break;
		case 3: setvect(0x1C, CmodPlayer_update); break; //RAD, SA2, AMD, (A2M)
		case 4: setvect(0x1C, CldsPlayer_update); break; //LOUDNESS
		case 5: Non_Interrupt_Player = &Cd00Player_update;/*setvect(0x1C, Cd00Player_update);*/ break; //EDlib
		case 6: setvect(0x1C, CVGM_update); break; //VGM
	}
	outportb(0x43, 0x36);
	outportb(0x40, spd & 0xff00);	//lo-byte
	outportb(0x40, spd >> 8);	//hi-byte	
	asm STI  //enable interrupts
	
	//Enable wave selector
	opl2_write(0x01,0x20);
}

void Music_Remove_Interrupt(){
	asm CLI //disable interrupts
	//reset interrupt
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);	//lo-byte
	outportb(0x40, 0xFF);	//hi-byte
	setvect(0x1C, old_time_handler);
	opl2_clear();
	asm STI  //enable interrupts
	//music_offset = 0;
}

//LOAD FILES

byte file_extension(char *file, char *ext){
	char str_size = strlen(file);
	return strncmp(&file[str_size-3], ext, 3);
}

void Reset_CmodPlayer(){
	int i;
	//int timer = 0;
	Music_Remove_Interrupt();
	
	opl2_clear();
	
	for (i = 1; i < 9; i++) C_Volume[i] = 0;
	
	songend = del = ord = rw = regbd = 0;
	tempo = bpm; speed = initspeed;
	for (i = 0; i < 64*9; i++) memset(tracks[i],0,64*5);
	//memset(ADPLUG_music_data,0,65535);
	memset(channel,0,sizeof(mod_channel)*nchans);
	memset(order,0,128);
	memset(arplist,0,256); 
	memset(arpcmd,0,256);
	for (i = 0; i < 250; i++) memset(inst[i].data,0,11);
	memset(inst,0,250*sizeof(mod_inst));
	memset(trackord,0,64*9);
	memset(songname,0,24);
	memset(author,0,24);
	memset(instname,0,26*23);
	
	Non_Interrupt_Player = &Void_function;
}

void d00_rewind(int subsong);

void Stop_Music(){
	Reset_CmodPlayer();
}

void WaitVsync_MDA(){
	asm mov		dx,003bah
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
	
	Non_Interrupt_Player();
}

void WaitVsync_NOMDA(){
	asm mov		dx,003dah
	WaitNotVsync:
	asm in      al,dx
	asm test    al,08h
	asm jnz		WaitNotVsync
	WaitVsync:
	asm in      al,dx
	asm test    al,08h
	asm jz		WaitVsync
	
	Non_Interrupt_Player();
}

byte CimfPlayer_load(char *filename){
	unsigned int i;
	char	header[5];
    int		version;
	FILE *f = fopen(filename,"rb"); if(!f) return false;
	size = 0;
	pos = 0;
	selected_player = 0;
	//memset(ADPLUG_music_data,0,63*1024);
	musicdata = (m_data*) ADPLUG_music_data;
	
	fseek(f, 0, SEEK_SET);
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	if (fread(header, 1, 2, f) != 2) {
		Print_ERROR();
		fclose(f);
		return false;
	};

	//IMF
	if (header[0] == 0 && header[1] == 0) fseek(f, 0, SEEK_SET);
	else size -=2;
	
	fread(musicdata,1,size,f);
	/*for(i = 0; i < size; i++) {
		fread(&musicdata[i].reg,1,1,f);
		fread(&musicdata[i].val,1,1,f);
		fread(&musicdata[i].time,2,1,f);
	}*/

	fclose(f);
	
	//IMF FILES
	if (!file_extension(filename, "DMF")) rate = 280; //Duke Nukem II
	else if (!file_extension(filename, "IMF")) rate = 560; //Bio Menace, Commander Keen, Cosmo's Cosmic Adventures, Monster Bash, Major Stryker
	else if (!file_extension(filename, "WLF")) rate = 700; //Blake Stone, Operation Body Count, Wolfenstein 3-D, Corridor 7
	else rate = 700; // default speed for unknown files that aren't .IMF or .WLF

	Print_Loaded("Id's Music Format",filename,"unknown");
	Music_Add_Interrupt(rate);

	return true;
}

byte CradPlayer_load(char *filename){
	char id[16];
	byte d = 0;
	unsigned char buf,ch,c,b,inp;
	char bufstr[2] = "\0";
	unsigned int i,j;
	unsigned long to;
	unsigned short patofs[32];
	const unsigned char convfx[16] = {255,1,2,3,255,5,255,255,255,255,20,255,17,0xd,255,19};
	FILE *f = fopen(filename,"rb"); if(!f) return false;
	selected_player = 3;
	Print_Loading();

	for (i = 0; i < 256; i++) {arplist[i]=0, arpcmd[i]=0;}
	initspeed = 3;
    activechan = 0xffffffff; flags = 256;/* curchip(opl->getchip()*/
	speed = initspeed;

	// initialize new patterns
	for(i=0;i<npats*nchans;i++) memset(tracks[i],0,sizeof(mod_tracks)*nrows);
	for(i=0;i<npats;i++) memset(trackord[i],0,nchans*2);
	
	// file validation section
	fread(id,1, 16, f); 
	fread(&version,1,1,f);
	if(strncmp(id,"RAD by REALiTY!!",16) || version != 0x10) { 
		Print_ERROR();
		fclose(f); 
		return false; 
	}
	
	// load section
	fread(&radflags,1,1,f);
	speed = radflags & 0x1F;
	if(radflags & 128) {	// description
		memset(desc,0,80*22);
		fread(&buf,1,1,f);
		while(buf){
			if(buf == 1) ;//strcat(desc,"\n");
			else{
				if(buf >= 2 && buf <= 0x1f) for(i=0;i<buf;i++)strcat(desc," ");
				else {
					*bufstr = buf;
					strcat(desc,bufstr);
				}
			}
			d++;
			fread(&buf,1,1,f);
		}
	}
	
	fread(&buf,1,1,f);
	while(buf) {	// instruments
		buf--;
		fread(&inst[buf].data[2],1,1,f); //AVSS CARRIER
		fread(&inst[buf].data[1],1,1,f); //AVSS MODULATOR
		fread(&inst[buf].data[10],1,1,f); //SCAL,VOL CA
		fread(&inst[buf].data[9],1,1,f); //SCAL,VOL MO
		fread(&inst[buf].data[4],1,1,f); //AT,DEC CA
		fread(&inst[buf].data[3],1,1,f); //AT,DEC MO
		fread(&inst[buf].data[6],1,1,f); //SUS,REL CA
		fread(&inst[buf].data[5],1,1,f); //SUS,REL MO
		fread(&inst[buf].data[0],1,1,f); //FEED,ALG
		fread(&inst[buf].data[8],1,1,f); //Wave C
		fread(&inst[buf].data[7],1,1,f); //Wave M
		fread(&buf,1,1,f);
	}
	fread(&length,1,1,f);
	for(i = 0; i < length; i++) fread(&order[i],1,1,f);	// orderlist
	for(i = 0; i < 32; i++) fread(&patofs[i],1,2,f);	// pattern offset table
	for(to=0;to<npats*nchans;to++) trackord[to / nchans][to % nchans] = to + 1;	// patterns
	
	for(i=0;i<32;i++){
		if(patofs[i]) {
			fseek(f,patofs[i],SEEK_SET);
			do {
				fread(&buf,1,1,f); 
				b = buf & 127;
				do {
					fread(&ch,1,1,f); c = ch & 127;
					//
					fread(&inp,1,1,f);
					tracks[i*9+c][b].note = inp & 127;
					tracks[i*9+c][b].inst = (inp & 128) >> 3;
					fread(&inp,1,1,f);
					tracks[i*9+c][b].inst += inp >> 4;
					tracks[i*9+c][b].command = inp & 15;
					if(inp & 15) {
						fread(&inp,1,1,f);
						tracks[i*9+c][b].param1 = inp / 10;
						tracks[i*9+c][b].param2 = inp % 10;
					}

				} while(!(ch & 128));
			} while(!(buf & 128));
		} else memset(&trackord[i],0,9*2);
	}
	fclose(f);

	// convert replay data
	for(i=0;i<32*9;i++) {	// convert patterns
		for(j=0;j<64;j++) {
			if(tracks[i][j].note == 15) tracks[i][j].note = 127;
			if(tracks[i][j].note > 16 && tracks[i][j].note < 127) tracks[i][j].note -= 4 * (tracks[i][j].note >> 4);
			if(tracks[i][j].note && tracks[i][j].note < 126) tracks[i][j].note++;
			tracks[i][j].command = convfx[tracks[i][j].command];
		}
	}
	restartpos = 0; initspeed = radflags & 31;
	bpm = radflags & 64 ? 0 : 50; flags = Decimal;
	
	rate = bpm;//getrate(filename);
	Music_Add_Interrupt(rate);
	desc[29] = 0;
	desc[56] = 0;
	opl2_clear();
	for(i=0;i<9;i++) {
		channel[i].vol2 = 63; channel[i].vol1 = 63;
		channel[i].key = 0;
		opl2_write(0xb0 + i, 0);	// stop old note
	}
	if (!MOD_PCM){
		if (d) Print_Loaded("Reality Adlib Tracker",desc,&desc[30]);
		else Print_Loaded("Reality Adlib Tracker","                             ","                           ");
		
	} else {//Load samples if RAS Reality Adlib + Samples
		desc[80+16] = 0;
		desc[160+16] = 0;
		desc[240+16] = 0;
		if (d) Print_Loaded("Reality Adlib Tracker PCM",desc,&desc[30]);
		else Print_Loaded("Reality Adlib Tracker PCM","                             ","                           ");
		Clear_Samples();
		sb_load_sample(&desc[80]);	//Sample channel 0
		sb_load_sample(&desc[160]);	//Sample channel 1
	}
	
	return true;
}

byte Csa2Player_load(char *filename){
	struct {
		unsigned char data[11],arpstart,arpspeed,arppos,arpspdcnt;
	} insts;
	unsigned char instname[17];
	unsigned char buf;
	int i,j, k, notedis = 0;
	const unsigned char convfx[16] = {0,1,2,3,4,5,6,255,8,255,10,11,12,13,255,15};
	unsigned char sat_type;
	char sadt[4];
	enum SAT_TYPE {
		HAS_ARPEGIOLIST = (1 << 7),
		HAS_V7PATTERNS = (1 << 6),
		HAS_ACTIVECHANNELS = (1 << 5),
		HAS_TRACKORDER = (1 << 4),
		HAS_ARPEGIO = (1 << 3),
		HAS_OLDBPM = (1 << 2),
		HAS_OLDPATTERNS = (1 << 1),
		HAS_UNKNOWN127 = (1 << 0)
	};
	FILE *f = fopen(filename,"rb"); if(!f) return false;
	selected_player = 3;
	Print_Loading();
	
	initspeed = 5;
    activechan = 0xffffffff; flags = 256;/* curchip(opl->getchip()*/
	speed = initspeed;
	
	// read header
	fread(sadt,1,4,f);
	fread(&version,1,1,f);

	// file validation section
	if(strncmp(sadt,"SAdT",4)) { fclose(f); return false; }
	switch(version) {
		case 1:
			notedis = +0x18;
			sat_type = HAS_UNKNOWN127 | HAS_OLDPATTERNS | HAS_OLDBPM;
			break;
		case 2:
			notedis = +0x18;
			sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
			break;
		case 3:
			notedis = +0x0c;
			sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
			break;
		case 4:
			notedis = +0x0c;
			sat_type = HAS_ARPEGIO | HAS_OLDPATTERNS | HAS_OLDBPM;
			break;
		case 5:
			notedis = +0x0c;
			sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
			break;
		case 6:
			sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
			break;
		case 7:
			sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_V7PATTERNS;
			break;
		case 8:
			sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_TRACKORDER;
			break;
		case 9:
			sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_TRACKORDER | HAS_ACTIVECHANNELS;
			break;
		default:	// unknown
			fclose(f);
			return false;
	}

	// load section
	// instruments
	for(i = 0; i < 31; i++) {
		if(sat_type & HAS_ARPEGIO) {
			for(j = 0; j < 11; j++) fread(&insts.data[j],1,1,f);
			fread(&insts.arpstart,1,1,f);
			fread(&insts.arpspeed,1,1,f);
			fread(&insts.arppos,1,1,f);
			fread(&insts.arpspdcnt,1,1,f);
			inst[i].arpstart = insts.arpstart;
			inst[i].arpspeed = insts.arpspeed;
			inst[i].arppos = insts.arppos;
			inst[i].arpspdcnt = insts.arpspdcnt;
		} else {
			for(j = 0; j < 11; j++) fread(&insts.data[j],1,1,f);
			inst[i].arpstart = 0;
			inst[i].arpspeed = 0;
			inst[i].arppos = 0;
			inst[i].arpspdcnt = 0;
		}
		for(j=0;j<11;j++) inst[i].data[j] = insts.data[j];
		inst[i].misc = 0;
		inst[i].slide = 0;
	}

	// instrument names
	for(i = 0; i < 29; i++) fread(&instname[0],1,17,f);
	
	fgetc(f);fgetc(f);fgetc(f); // dummy bytes
  
	for(i = 0; i < 128; i++) fread(&order[i],1,1,f);	// pattern orders
	if(sat_type & HAS_UNKNOWN127) 
		for(i = 0; i < 127; i++) fgetc(f);

	// infos
	fread(&nop,1,2,f); fread(&length,1,1,f); fread(&restartpos,1,1,f);

	// bpm
	fread(&bpm,1,2,f);
	speed = bpm;
	if(sat_type & HAS_OLDBPM) {
		bpm = bpm * 125 / 50;		// cps -> bpm
		speed = bpm;
	}	
	if(sat_type & HAS_ARPEGIOLIST) {
		for(i = 0; i < 256; i++) fread(&arplist[i],1,1,f);	// arpeggio list
		for(i = 0; i < 256; i++) fread(&arpcmd[i],1,1,f);	// arpeggio commands
	}

	for(i=0;i<64;i++) {				// track orders
		for(j=0;j<9;j++) {
			if(sat_type & HAS_TRACKORDER) fread(&trackord[i][j],1,1,f);
			else trackord[i][j] = i * 9 + j;
		}
	}

	if(sat_type & HAS_ACTIVECHANNELS) {
		fread(&activechan,1,2,f);
		activechan = activechan << 16;		// active channels
	}
	// track data
	if(sat_type & HAS_OLDPATTERNS) {
		i = 0;
		while(!feof(f)) {
			for(j=0;j<64;j++) {
				for(k=0;k<9;k++) {
					fread(&buf,1,1,f);
					tracks[i+k][j].note = buf ? (buf + notedis) : 0;
					fread(&tracks[i+k][j].inst,1,1,f);
					fread(&buf,1,1,f);
					tracks[i+k][j].command = convfx[buf & 0xf];
					fread(&tracks[i+k][j].param1,1,1,f);
					fread(&tracks[i+k][j].param2,1,1,f);
				}
			}
			i+=9;
		}
	} else {
		if(sat_type & HAS_V7PATTERNS) {
			i = 0;
			while(!feof(f)) {
				for(j=0;j<64;j++) {
					for(k=0;k<9;k++) {
						fread(&buf,1,1,f);
						tracks[i+k][j].note = buf >> 1;
						tracks[i+k][j].inst = (buf & 1) << 4;
						fread(&buf,1,1,f);
						tracks[i+k][j].inst += buf >> 4;
						tracks[i+k][j].command = convfx[buf & 0x0f];
						fread(&buf,1,1,f);
						tracks[i+k][j].param1 = buf >> 4;
						tracks[i+k][j].param2 = buf & 0x0f;
					}
				}
				i+=9;
			}
		} else {
			i = 0;
			while(!feof(f)) {
				for(j=0;j<64;j++) {
					fread(&buf,1,1,f);
					tracks[i][j].note = buf >> 1;
					tracks[i][j].inst = (buf & 1) << 4;
					fread(&buf,1,1,f);
					tracks[i][j].inst += buf >> 4;
					tracks[i][j].command = convfx[buf & 0x0f];
					fread(&buf,1,1,f);
					tracks[i][j].param1 = buf >> 4;
					tracks[i][j].param2 = buf & 0x0f;
				}
				i++;
			}
		}
	}
	fclose(f);

	// fix instrument names
	/*for(i=0;i<29;i++)
		for(j=0;j<17;j++)
			if(!instname[i][j])
				instname[i][j] = ' ';*/

	rate = 50;//bpm;//getrate(filename);
	Music_Add_Interrupt(rate);
	Print_Loaded("Surprise! Adlib Tracker 2",filename,"     ");
	return true;
}

byte CamdPlayer_load(char *filename){
	struct {
		char id[9];
		unsigned char version;
	} 	header;
	int i, j, k, t, numtrax, maxi = 0;
	word fsize, ii;
	unsigned char buf, buf2, buf3;
	const unsigned char convfx[10] = {0,1,2,9,17,11,13,18,3,14};
	const unsigned char convvol[64] = {
		0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 9, 0xa, 0xa, 0xb,
		0xc, 0xc, 0xd, 0xe, 0xe, 0xf, 0x10, 0x10, 0x11, 0x12, 0x13, 0x14, 0x14,
		0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x21,
		0x22, 0x23, 0x25, 0x26, 0x28, 0x29, 0x2b, 0x2d, 0x2e, 0x30, 0x32, 0x35,
		0x37, 0x3a, 0x3c, 0x3f
	};
	FILE *f = fopen(filename,"rb"); if(!f) return false;
	Print_Loading();
	selected_player = 3;
	initspeed = 6;
    activechan = 0xffffffff; flags = 256;
	speed = initspeed;
	// file validation section
	fseek(f, 0L, SEEK_END);
	fsize = ftell(f);
	if(fsize < 1072) { fclose(f); return false; }
	fseek(f, 1062, SEEK_SET); fread(header.id,1,9,f);
	fread(&header.version,1,1,f);
	if(strncmp(header.id, "<o\xefQU\xeeRoR", 9) && strncmp(header.id, "MaDoKaN96", 9)) { fclose(f); return false; }
	// load section
	memset(inst, 0, sizeof(*inst));
	fseek(f, 0L, SEEK_SET);
	fread(songname, 1, sizeof(songname),f);
	fread(author, 1, sizeof(author),f);
	for(i = 0; i < 26; i++) {
		fread(instname[i],1,23,f);
		for(j = 0; j < 11; j++) fread(&inst[i].data[j],1,1,f);
	}
	fread(&length,1,1,f); fread(&nop,1,1,f); nop+=1;
	for(i=0;i<128;i++) fread(&order[i],1,1,f);
	fseek(f, 10, SEEK_CUR);
	

	if(header.version == 0x10) {	// unpacked module
		maxi = nop * 9;
		for(i=0;i<64*9;i++) trackord[i/9][i%9] = i+1;
		t = 0;
		while(!feof(f)) {
			for(j=0;j<64;j++){
				for(i=t;i<t+9;i++) {
					fread(&buf,1,1,f);
					tracks[i][j].param2 = (buf&127) % 10;
					tracks[i][j].param1 = (buf&127) / 10;
					fread(&buf,1,1,f);
					tracks[i][j].inst = buf >> 4;
					tracks[i][j].command = buf & 0x0f;
					fread(&buf,1,1,f);
					// fix bug in AMD save routine
					if(buf >> 4) tracks[i][j].note = ((buf & 14) >> 1) * 12 + (buf >> 4);
					else tracks[i][j].note = 0;
					tracks[i][j].inst += (buf & 1) << 4;
				}
			}
			t += 9;
		}
	} else {	// packed module
		for(i=0;i<nop;i++){
			for(j=0;j<9;j++){
				fread(&trackord[i][j],1,2,f);
				trackord[i][j]++;
			}
		}
		fread(&numtrax,1,2,f);
		for(k=0;k<numtrax;k++) {
			fread(&i,1,2,f);
			if(i > 575) i = 575;	// fix corrupted modules
			maxi = (i + 1 > maxi ? i + 1 : maxi);
			j = 0;
			do {
				fread(&buf,1,1,f);
				if(buf & 128) {
					for(t = j; t < j + (buf & 127) && t < 64; t++) {
						tracks[i][t].command = 0;
						tracks[i][t].inst = 0;
						tracks[i][t].note = 0;
						tracks[i][t].param1 = 0;
						tracks[i][t].param2 = 0;
					}
					j += buf & 127;
					continue;
				}
				tracks[i][j].param2 = buf % 10;
				tracks[i][j].param1 = buf / 10;
				fread(&buf,1,1,f);
				tracks[i][j].inst = buf >> 4;
				tracks[i][j].command = buf & 0x0f;
				fread(&buf,1,1,f);
				// fix bug in AMD save routine
				if(buf >> 4) tracks[i][j].note = ((buf & 14) >> 1) * 12 + (buf >> 4);
				else tracks[i][j].note = 0;
				tracks[i][j].inst += (buf & 1) << 4;
				j++;
			} while(j<64);
		}
	}
	fclose(f);

	// convert to protracker replay data
	bpm = 50; restartpos = 0; flags = Decimal;
	// convert instruments
	for(i=0;i<26;i++) {	
		buf = inst[i].data[0];
		buf2 = inst[i].data[1];
		inst[i].data[0] = inst[i].data[10];
		inst[i].data[1] = buf;
		buf = inst[i].data[2];
		inst[i].data[2] = inst[i].data[5];
		buf3 = inst[i].data[3];
		inst[i].data[3] = buf;
		buf = inst[i].data[4];
		inst[i].data[4] = inst[i].data[7];
		inst[i].data[5] = buf3;
		buf3 = inst[i].data[6];
		inst[i].data[6] = inst[i].data[8];
		inst[i].data[7] = buf;
		inst[i].data[8] = inst[i].data[9];
		inst[i].data[9] = buf2;
		inst[i].data[10] = buf3;
		for(j=0;j<23;j++){	// convert names
			if(instname[i][j] == '\xff') instname[i][j] = '\x20';
		}
	}
	// convert patterns
	for(i=0;i<maxi;i++){	
		for(j=0;j<64;j++) {
			tracks[i][j].command = convfx[tracks[i][j].command];
			// extended command
			if(tracks[i][j].command == 14) {
				if(tracks[i][j].param1 == 2) {
					tracks[i][j].command = 10;
					tracks[i][j].param1 = tracks[i][j].param2;
					tracks[i][j].param2 = 0;
				}
				if(tracks[i][j].param1 == 3) {
					tracks[i][j].command = 10;
					tracks[i][j].param1 = 0;
				}
			}
		
			// fix volume
			if(tracks[i][j].command == 17) {
				int vol = convvol[tracks[i][j].param1 * 10 + tracks[i][j].param2];
		
				if(vol > 63) vol = 63;
				tracks[i][j].param1 = vol / 10;
				tracks[i][j].param2 = vol % 10;
			}
			if (tracks[i][j].command == 18) { 
				rate = (tracks[i][j].param1 *10) + (tracks[i][j].param2);
				if(rate <= 31 && rate > 0) {speed = rate; rate = 50;}
			}
		}
	}
	//Get tempo
	/*	case 18: // AMD set speed
				if(info <= 31 && info > 0) speed = info;
				if(info > 31 || !info) tempo = info;*/
	//channel[chan].fx
	//bpm;//getrate(filename);
	Music_Add_Interrupt(rate);
	Print_Loaded("AMUSIC Adlib Tracker",songname,author);
	return true;
}

byte CldsPlayer_load(char *filename){
	word	i, j;
	LDS_SoundBank	*sb;
	FILE *f;
	// file validation section (actually just an extension check)
	//if(!file_extension(filename, "LDS")) return false;
	f = fopen(filename,"rb"); if(!f) return false;
	lds_patterns = (unsigned short*) (ADPLUG_music_data + (1024*sizeof(LDS_Position)) + (9*sizeof(LDS_Channel)));
	Print_Loading();
	selected_player = 4;
	opl2_clear();
	// init all with 0
	tempo_now = 3; playing = true; songlooped = false;
	jumping = fadeonoff = allvolume = hardfade = pattplay = posplay = numposi = jumppos = mainvolume = 0;
	for (i = 0; i< 9; i++) memset(&lds_channel[i], 0,sizeof(LDS_Channel));
	memset(chandelay, 64, 9);
	memset(fmchip, 0, sizeof(fmchip));
	memset(lds_positions,0,1024*sizeof(LDS_Position));
	for (i = 0; i< 63; i++) memset(&lds_soundbank[i],0,sizeof(LDS_SoundBank));
	//memset(ADPLUG_music_data,0,63*1024);

	// file load section (header)
	fread(&mode,1,1,f);
	if(mode > 2) { fclose(f); return false; }
	fread(&lds_speed,1,2,f);
	fread(&tempo,1,1,f);
	fread(&pattlen,1,1,f);
	for(i = 0; i < 9; i++) fread(&chandelay[i],1,1,f);
	fread(&regbd,1,1,f);
	
	// load patches
	fread(&numpatch,1,2,f);
	for(i = 0; i < numpatch; i++) {
		sb = &lds_soundbank[i];
		fread(&sb->mod_misc,1,1,f); fread(&sb->mod_vol,1,1,f);
		fread(&sb->mod_ad,1,1,f); fread(&sb->mod_sr,1,1,f);
		fread(&sb->mod_wave,1,1,f); fread(&sb->car_misc,1,1,f);
		fread(&sb->car_vol,1,1,f); fread(&sb->car_ad,1,1,f);
		fread(&sb->car_sr,1,1,f); fread(&sb->car_wave,1,1,f);
		fread(&sb->feedback,1,1,f); fread(&sb->keyoff,1,1,f);
		fread(&sb->portamento,1,1,f); fread(&sb->glide,1,1,f);
		fread(&sb->finetune,1,1,f); fread(&sb->vibrato,1,1,f);
		fread(&sb->vibdelay,1,1,f); fread(&sb->mod_trem,1,1,f);
		fread(&sb->car_trem,1,1,f); fread(&sb->tremwait,1,1,f);
		fread(&sb->arpeggio,1,1,f);
		for(j = 0; j < 12; j++) fread(&sb->arp_tab[j],1,1,f);
		fread(&sb->start,1,2,f); fread(&sb->size,1,2,f);
		fread(&sb->fms,1,1,f); fread(&sb->transp,1,2,f);
		fread(&sb->midinst,1,1,f); fread(&sb->midvelo,1,1,f);
		fread(&sb->midkey,1,1,f); fread(&sb->midtrans,1,1,f);
		fread(&sb->middum1,1,1,f); fread(&sb->middum2,1,1,f);
	}

	// load positions
	fread(&numposi,1,2,f);
	//lds_positions = new Position[9 * numposi];
	for(i = 0; i < numposi; i++)
		for(j = 0; j < 9; j++) {
			//patnum is a pointer inside the pattern space, but patterns are 16bit
			//word fields anyway, so it ought to be an even number (hopefully) and
			//we can just divide it by 2 to get our array index of 16bit words.
			fread(&lds_positions[i * 9 + j].patnum,1,2,f);
			lds_positions[i * 9 + j].patnum = lds_positions[i * 9 + j].patnum/2;
			fread(&lds_positions[i * 9 + j].transpose,1,1,f);
		}

	// load patterns
	fgetc(f);fgetc(f);// ignore # of digital sounds (not played by this player)
	//patterns = new unsigned short[(fp.filesize(f) - f->pos()) / 2 + 1];
	for(i = 0; !feof(f); i++) fread(&lds_patterns[i],1,2,f);
	fclose(f);

	Music_Add_Interrupt(70);
	
	if (!strncmp(filename,"MUSICMAN.LDS",12)){
		Print_Loaded("LOUDNESS Sound System","The Music Man (Loudness Logo)","Andras Molnar");
	}else Print_Loaded("LOUDNESS Sound System",filename,"    ");
	
	return true;
}

byte Cd00Player_load(char *filename){
	d00header checkhead;
	d00header1 ch;
	unsigned long	filesize;
	int		i,ver1=0;
	char	*str;
	FILE	*f = fopen(filename,"rb"); if(!f) return false;
	Print_Loading();
	//memset(ADPLUG_music_data,0,63*1024);
	selected_player = 5;
	opl2_clear();
	// file validation section
	fread((char *)&checkhead,1, sizeof(d00header),f);

	// Check for version 2-4 header
	if(strncmp(checkhead.id,"JCH\x26\x02\x66",6) || checkhead.type ||
	   !checkhead.subsongs || checkhead.soundcard) {
		// Check for version 0 or 1 header (and .d00 file extension)
		fseek(f, 0L, SEEK_SET); fread((char*)&ch,1, sizeof(d00header1),f);
		if(ch.version > 1 || !ch.subsongs) { fclose(f); return false; }
		ver1 = 1;
	}

	// load section
	fseek(f, 0L, SEEK_END);
	filesize = ftell(f); fseek(f, 0L, SEEK_SET);
	filedata = (unsigned char *) ADPLUG_music_data;			// 1 byte is needed for old-style DataInfo block
	fread(filedata,1,filesize,f);
	fclose(f);
	
	if(!ver1) {	// version 2 and above
		header = (d00header *)filedata;
		version = header->version;
		datainfo = (unsigned char *)filedata + header->infoptr;
		d00_inst = (struct Sinsts *)((char *)filedata + header->instptr);
		seqptr = (unsigned short *)((char *)filedata + header->seqptr);
		for(i=31;i>=0;i--)	// erase whitespace
			if(header->songname[i] == ' ')
				header->songname[i] = '\0';
			else
				break;
		for(i=31;i>=0;i--)
			if(header->author[i] == ' ')
				header->author[i] = '\0';
			else
				break;
	} else {	// version 1
		header1 = (d00header1 *)filedata;
		version = header1->version;
		datainfo = (unsigned char *)filedata + header1->infoptr;
		d00_inst = (struct Sinsts *)((char *)filedata + header1->instptr);
		seqptr = (unsigned short *)((char *)filedata + header1->seqptr);
	}
	switch(version) {
	case 0:
		levpuls = 0;
		spfx = 0;
		header1->speed = 70;		// v0 files default to 70Hz
		break;
	case 1:
		levpuls = (struct Slevpuls *)((char *)filedata + header1->lpulptr);
		spfx = 0;
		break;
	case 2:
		levpuls = (struct Slevpuls *)((char *)filedata + header->spfxptr);
		spfx = 0;
		break;
	case 3:
		spfx = 0;
		levpuls = 0;
		break;
	case 4:
		spfx = (struct Sspfx *)((char *)filedata + header->spfxptr);
		levpuls = 0;
		break;
	}
	str = (char*) strstr(datainfo,"\xff\xff");
	if(str)
		while((*str == '\xff' || *str == ' ') && str >= datainfo) {
			*str = '\0'; str--;
		}
	else	// old-style block
		memset((char *)filedata+filesize,0,1);
		
	d00_rewind(0);
	if(!ver1) {
		Print_Loaded("EDlib Tracker 0",header->songname,header->author);
		Music_Add_Interrupt(header->speed);
	} else {
		Print_Loaded("EDlib Tracker 1",filename,"       ");
		Music_Add_Interrupt(header1->speed);
		gotoxy(60,25); 
		textattr(Color_Error);
		cprintf("LOADED   (BUGGY)  ");
	}
	return true;	
}

byte CVGMPlayer_load(char *filename){
	//Header
		//0x00 nn nn nn nn 	file identification (0x56 0x67 0x6d 0x20) "vgm"
		//0x04 nn nn nn nn	eof - 0x04
		//0x14 nn nn nn nn	tags offset - 0x14 (title name...)
		//0x1C nn nn nn nn 	Loop offset 	
		//0x20 nn nn nn nn	Loop samples
		//0x34 nn nn nn nn	data offset - 0x34. if 0x0000000C data at absolute 0x40;
		//0x50 nn nn nn nn	YM3812 clock. if 0, no opl2 data
		
	//Read tags
	char header[5];
	long tag_offset = 0;
	long data_offset = 0;
	long opl_clock = 0;
	long size = 0;
	
	FILE *f = fopen(filename,"rb"); if(!f) return false;
	
	vgm_music_wait = 0;
	vgm_music_offset = 0;
	vgm_data_size = 0;
	vgm_loop = 0;
	vgm_loop_size = 0;
	
	Print_Loading();

	selected_player = 6;
	opl2_clear();
	
	fseek(f, 0, SEEK_SET);
	//Check vgm
	fread(header, 1, 4, f);
	if ((header[0] != 0x56) || (header[1] != 0x67) || (header[2] != 0x6d) || (header[3] != 0x20)){
		Print_ERROR();fclose(f);return false;
	}
	
	//get absolute file size, absolute tags offset
	fread(&size, 1, 4, f);
	size +=4;
	fseek(f, 0x14, SEEK_SET);
	fread(&tag_offset, 1, 4, f);
	tag_offset += 0x14;
	
	//get loop start loop length.
	fseek(f, 0x1C, SEEK_SET);
	fread(&vgm_loop, 1, 2, f);
	fseek(f, 2, SEEK_CUR);
	fread(&vgm_loop_size, 1, 2, f);
	
	//get absolute data offset.
	fseek(f, 0x34, SEEK_SET);
	fread(&data_offset, 1, 4, f);
	data_offset+=0x34;
	
	//calculate data size
	size -= data_offset;
	if (tag_offset != 0) size -= (size - tag_offset);
	if (size > 65535) {Print_ERROR();fclose(f);return false;}

	vgm_data_size = size;

	//Is is an opl2 vgm? (YM3812 clock != 0?)
	fseek(f, 0x50, SEEK_SET);
	fread(&opl_clock, 1, 4, f);
	if (!opl_clock) {Print_ERROR();fclose(f);return false;}
	
	//Read data to ADPLUG_music_data
	fseek(f, data_offset, SEEK_SET);
	fread(ADPLUG_music_data, 1, vgm_data_size, f);
	
	//read tags
	if (tag_offset){
		unsigned char i = 0;
		unsigned char byte = 1;
		unsigned char title[28];
		unsigned char author[26];
		fseek(f, tag_offset+4+4+4, SEEK_SET);
		//Get strings; (end in \0):
		//track, game, system, author. (duplicated (eng\0jap\0):
		//Year, converter
		while (byte !='\0'){//get track eng
			byte = fgetc(f);
			if (i <28) title[i++] = byte;
			fgetc(f);//skip byte
		} 
		Print_Loaded("VGM (YM3812) PC/ARCADE",title," ");
	
	} else Print_Loaded("VGM (YM3812) PC/ARCADE",filename," ");

	fclose(f);
	
	Music_Add_Interrupt(160);
	
	return true;
}

byte CheradPlayer_load(char *filename){
	word i,size,offset;
	FILE *f = fopen(filename,"rb"); if(!f) return false;

	// file validation
	//	!fp.extension(filename, ".sdb") &&
	//	!fp.extension(filename, ".agd") &&
	//	!fp.extension(filename, ".ha2"))
	//
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	// Read entire file into memory (data)
	fread(ADPLUG_music_data,1,size,f);
	fclose(f);
	// Detect compression
	comp = 0;
	// Process file header
	if (size < 52) goto failure;
	if (size < *(word*)ADPLUG_music_data) goto failure;
	nInsts = (size - *(word *)ADPLUG_music_data) / HERAD_INST_SIZE;
	if ( nInsts == 0 ) goto failure;

	offset = *(word *)(ADPLUG_music_data + 2);
	if ( offset != 0x32 && offset != 0x52 ) goto failure;

	AGD = offset == 0x52;
	wLoopStart = *(word *)(ADPLUG_music_data + 0x2C);
	wLoopEnd = *(word *)(ADPLUG_music_data + 0x2E);
	wLoopCount = *(word *)(ADPLUG_music_data + 0x30);
	wSpeed = *(word *)(ADPLUG_music_data + 0x32);
	if (wSpeed == 0) goto failure;
	nTracks = 0;
	for (i = 0; i < HERAD_MAX_TRACKS; i++){
		if ( *(word *)(ADPLUG_music_data + 2 + i * 2) == 0 ) break;
		nTracks++;
	}
	for (i = 0; i < nTracks; i++){
		word next;
		offset = *(word *)(ADPLUG_music_data + 2 + i * 2) + 2;
		next = (i < HERAD_MAX_TRACKS - 1 ? *(word *)(ADPLUG_music_data + 2 + (i + 1) * 2) + 2 : *(word *)ADPLUG_music_data);
		if (next <= 2) next = *(word *)ADPLUG_music_data;
		track[i].size = next - offset;
		//printf("size %i\n",track[i].size);
		//track[i].data = new uint8_t[track[i].size];
		memcpy(track[i].data, ADPLUG_music_data + offset, track[i].size);
	}
	//inst = new herad_inst[nInsts];
	offset = *(word *)ADPLUG_music_data;
	v2 = true;
	for (i = 0; i < nInsts; i++){
		memcpy(her_inst[i].data, ADPLUG_music_data + offset + i * HERAD_INST_SIZE, HERAD_INST_SIZE);
		if (v2 && her_inst[i].param.mode == 0)v2 = false; // 0 1 HERAD_INSTMODE_SDB1)
	}
	//delete[] data;
	goto good;

failure:
	//delete[] data;
	return false;

good:
	//rewind(0);

	return true;
}

//LOADERS TO DO:
// CMF HSC DMO
//MAYBE
//A2M? Sierra?, Lucasarts?

void Load_Music(char *filename){
	//CimfPlayer_load("KICKPANT.IMF");
	if (!file_extension(filename,"IMF") || !file_extension(filename,"DMF") || !file_extension(filename,"WLF")) CimfPlayer_load(filename);
	else if (!file_extension(filename,"RAD")) {MOD_PCM = 0; CradPlayer_load(filename);}
	else if (!file_extension(filename,"RAS")) {MOD_PCM = 1; CradPlayer_load(filename);}
	else if (!file_extension(filename,"SA2")) {MOD_PCM = 0; Csa2Player_load(filename);}
	else if (!file_extension(filename,"AMD")) {MOD_PCM = 0; CamdPlayer_load(filename);}
	else if (!file_extension(filename,"LDS")) CldsPlayer_load(filename);
	else if (!file_extension(filename,"D00")) Cd00Player_load(filename);
	else if (!file_extension(filename,"VGM")) CVGMPlayer_load(filename);
	else if (!file_extension(filename,"SDB")) CheradPlayer_load(filename);
	else {
		gotoxy(59,25);
		textattr(Color_Error);
		cprintf(" NOT SUPPORTED  ");
	}
}

//INIT RAM AND etc
extern byte OPL2LPT;
void Init(){
	int i,j;
	asm CLI
	old_time_handler = getvect(0x1C);
	asm STI
	for (i = 0; i < 256; i++) noteDIV12[i] = i/12;
	//Get currect directory
	getcwd(Initial_working_dir,32);
	//Allocate modules data 185KB
	if ((tracks = calloc(64*9,5)) == NULL) {printf("Not enough RAM"); exit(1);}
	for (i = 0; i < 64*9; i++) {
		if ((tracks[i] = calloc(64,5)) == NULL) {printf("Not enough RAM"); exit(1);}
	}
	
	//To save ram I use allocated tracks data. I think this avoids overwritting tracks pointers
	ADPLUG_music_data = (unsigned char*) (tracks+36864); 
	/*if ((ADPLUG_music_data = farcalloc(65535,1)) == NULL) {
		printf("Not enough RAM");
		exit(1);
	}*/
	
	//if ((lds_positions = calloc(1024,sizeof(LDS_Position))) == NULL) {printf("Not enough RAM"); exit(1);}
	//if ((lds_channel = calloc(9,sizeof(LDS_Channel))) == NULL) {printf("Not enough RAM"); exit(1);}
	lds_positions = (LDS_Position*) ADPLUG_music_data;
	lds_channel = (LDS_Channel*) (ADPLUG_music_data + (1024*sizeof(LDS_Position)));
	lds_DIV1216 = (int*) calloc(2048,2);
	//precalculate lds value/(12 * 16) - 1
	for (i = 0; i < 2048; i++) lds_DIV1216[i] = i/(12 * 16) - 1;
	//Precalculate EDLIB incredibly messy and slow volume format (it kills 8088 and 8086 CPUs)
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) d00_MUL64_DIV63[i][j] = (int)i*j/63;
	}
	
	if (OPL2LPT == 1) opl2_write = &opl2lpt_write;
	if (OPL2LPT == 0) opl2_write = &opl2_write0;
	
	
	//Herad
	for (i = 0; i < 21; i++) track[i].data = calloc(8192,1);
	MOD_DATA = calloc(65535,1);
	
	//If Sound Blaster
	sb_setup();	

	Non_Interrupt_Player = &Void_function;
	
	Adlib_Detect();
}

void Exit_Dos(){
	int i;

	sb_deinit();
	
	Music_Remove_Interrupt();
	opl2_clear();

	for (i = 0; i < 21; i++) free(track[i].data);
	for (i = 0; i < 64; i++) free(tracks[i]);
	free(tracks);
	free(lds_DIV1216);
	system("cls");
	chdir(Initial_working_dir);
	
	//CREATE MOD FILE
	//Only a few effects will be lost, but we can recreate them with looped samples
	/*modfile = fopen("test.mod","wb");
	fwrite("Loudness to MOD",1,15,modfile);
	fputc(0,modfile);
	fseek(modfile, 950, SEEK_SET);
	fputc(63,modfile); fputc(0,modfile); //size 1
	for (i = 0; i < 64; i++) fputc(i,modfile);//pattern
	fseek(modfile, 1080, SEEK_SET);
	fwrite("9CHN",1,4,modfile);//9 channels
	fwrite(MOD_DATA,1,65535,modfile);
	fclose(modfile);

	free(MOD_DATA);*/
	
	//Reset text mode
	asm mov ax, 3h
	asm int 10h
	
	exit(1);
}


	///////////
	//PLAYERS//
	///////////


//CUSTOM PLAYERS	
void interrupt CimfPlayer_update(void){
	asm CLI
	//byte *ost = music_sdata + music_offset;
	while (!del && pos < size) {
		byte reg = musicdata[pos].reg;
        opl2_write(reg, musicdata[pos].val);
		del = musicdata[pos].time;
		//Detect key hits
		switch (reg){
			case 0xB0:
			case 0xB1:
			case 0xB2:
			case 0xB3:
			case 0xB4:
			case 0xB5:
			case 0xB6:
			case 0xB7:
			case 0xB8: 
				Key_Hit[reg - 0xB0] = 1;
			break;
		}
		
		pos++;
	}
	del--;
	if(pos >= size) {
		pos = 0;
		songend = true;
	}
	//else timer = rate / (float)del;
	//return !songend;
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
	asm STI //enable interrupts
}

//PROTRACKER PLAYER
int getrate(char *filename){
	if(filename);
	/*if(db) {	// Database available
		f->seek(0, binio::Set);
		CClockRecord *record = (CClockRecord *)db->search(CAdPlugDatabase::CKey(*f));
		if (record && record->type == CAdPlugDatabase::CRecord::ClockSpeed)
		return record->clock;
	}*/

	// Otherwise the database is either unavailable, or there's no entry for this file
	return 0;
}

void setfreq(unsigned char chan){
	opl2_write(0xa0 + chan, channel[chan].freq & 255);
	if(channel[chan].key) opl2_write(0xb0 + chan, ((channel[chan].freq & 768) >> 8) + (channel[chan].oct << 2) | 32);
	else opl2_write(0xb0 + chan, ((channel[chan].freq & 768) >> 8) + (channel[chan].oct << 2));
}

void setvolume_alt(unsigned char chan){
	unsigned char ivol2 = inst[channel[chan].inst].data[9] & 63;
	unsigned char ivol1 = inst[channel[chan].inst].data[10] & 63;

	opl2_write(0x40 + CPlayer_op_table[chan], (((63 - (channel[chan].vol2 & 63)) + ivol2) >> 1) + (inst[channel[chan].inst].data[9] & 192));
	opl2_write(0x43 + CPlayer_op_table[chan], (((63 - (channel[chan].vol1 & 63)) + ivol1) >> 1) + (inst[channel[chan].inst].data[10] & 192));
}

void setvolume(unsigned char chan){
	if(flags & Faust) setvolume_alt(chan);
	else {
		opl2_write(0x40 + CPlayer_op_table[chan], 63-channel[chan].vol2 + (inst[channel[chan].inst].data[9] & 192));
		opl2_write(0x43 + CPlayer_op_table[chan], 63-channel[chan].vol1 + (inst[channel[chan].inst].data[10] & 192));
	}
}

void setnote(unsigned char chan, int note){
	if(note > 96) {
		if(note == 127) {	// key off
			channel[chan].key = 0;
			setfreq(chan);
			return;
		} else
		note = 96;
	}

	if(note < 13) channel[chan].freq = CmodPlayer_sa2_notetable[note - 1];
	else
		if(note % 12 > 0) channel[chan].freq = CmodPlayer_sa2_notetable[(note % 12) - 1];
		else channel[chan].freq = CmodPlayer_sa2_notetable[11];
		channel[chan].oct = noteDIV12[note-1];
	/*
	* 2/24
	*/
	channel[chan].freq += inst[channel[chan].inst].slide;	// apply pre-slide
}

void playnote(unsigned char chan){
	Key_Hit[chan] = 1;//For visualizer bars
	if (MOD_PCM && chan < 2){
		sb_play_sample(chan,11050);
	} else {
		unsigned char op = CPlayer_op_table[chan], insnr = channel[chan].inst;
		unsigned char *data = (char*) &inst[insnr].data;
		if(!(flags & NoKeyOn)) opl2_write(0xb0 + chan, 0);	// stop old note
		opl2_write(0x01,0x20);
		//opl2_write(0x08,0x8F);
		// set instrument data
		opl2_write(0x20 + op, data[1]);
		opl2_write(0x23 + op, data[2]);
		opl2_write(0x60 + op, data[3]);
		opl2_write(0x63 + op, data[4]);
		opl2_write(0x80 + op, data[5]);
		opl2_write(0x83 + op, data[6]);
		opl2_write(0xe0 + op, data[7]);
		opl2_write(0xe3 + op, data[8]);
		opl2_write(0xc0 + chan, data[0]);
		//opl2_write(0xbd, inst[insnr].data[0]);	// set misc. register //inst[insnr].misc
	
		// set frequency, volume & play
		channel[chan].key = 1;
		setfreq(chan);
		
		if (flags & Faust) {
			channel[chan].vol2 = 63;
			channel[chan].vol1 = 63;
		}
		
		setvolume(chan);
	}
}

byte resolve_order(){
	if(ord < length) {
		while(order[ord] >= JUMPMARKER) {	// jump to order
		unsigned long neword = order[ord] - JUMPMARKER;
			if(neword <= ord) songend = 1;
			if(neword == ord) return false;
			ord = neword;
		}
	} else {
		songend = 1;
		ord = restartpos;
	}
	return true;
}

void slide_down(unsigned char chan, word amount){
	channel[chan].freq -= amount;
	if(channel[chan].freq <= 342) {
		if(channel[chan].oct) {
			channel[chan].oct--;
			channel[chan].freq <<= 1;
		} else channel[chan].freq = 342;
	}
}

void slide_up(unsigned char chan, word amount){
	channel[chan].freq += amount;
	if(channel[chan].freq >= 686) {
		if(channel[chan].oct < 7) {
			channel[chan].oct++;
			channel[chan].freq >>= 1;
		} else channel[chan].freq = 686;
	}
}

void tone_portamento(unsigned char chan, unsigned char info){
	if((channel[chan].freq + (channel[chan].oct << 10)) < (channel[chan].nextfreq + (channel[chan].nextoct << 10))) {
		slide_up(chan,info);
		if((channel[chan].freq + (channel[chan].oct << 10)) > (channel[chan].nextfreq + (channel[chan].nextoct << 10))) {
			channel[chan].freq = channel[chan].nextfreq;
			channel[chan].oct = channel[chan].nextoct;
		}
	}
	if((channel[chan].freq + (channel[chan].oct << 10)) > (channel[chan].nextfreq + (channel[chan].nextoct << 10))) {
		slide_down(chan,info);
		if((channel[chan].freq + (channel[chan].oct << 10)) < (channel[chan].nextfreq + (channel[chan].nextoct << 10))) {
			channel[chan].freq = channel[chan].nextfreq;
			channel[chan].oct = channel[chan].nextoct;
		}
	}
	setfreq(chan);
}

void vibrato(unsigned char chan, unsigned char speed, unsigned char depth){
	int i;
	
	if(!speed || !depth) return;
	if(depth > 14) depth = 14;
	
	for(i=0;i<speed;i++) {
		channel[chan].trigger++;
		while(channel[chan].trigger >= 64) channel[chan].trigger -= 64;
		if(channel[chan].trigger >= 16 && channel[chan].trigger < 48) slide_down(chan,CmodPlayer_vibratotab[channel[chan].trigger - 16] / (16-depth));
		if(channel[chan].trigger < 16) slide_up(chan,CmodPlayer_vibratotab[channel[chan].trigger + 16] / (16-depth));
		if(channel[chan].trigger >= 48) slide_up(chan,CmodPlayer_vibratotab[channel[chan].trigger - 48] / (16-depth));
	}
	setfreq(chan);
}

void vol_up(unsigned char chan, int amount){
	if(channel[chan].vol1 + amount < 63) channel[chan].vol1 += amount;
	else channel[chan].vol1 = 63;
	if(channel[chan].vol2 + amount < 63) channel[chan].vol2 += amount;
	else channel[chan].vol2 = 63;
}

void vol_down(unsigned char chan, int amount){
	if(channel[chan].vol1 - amount > 0) channel[chan].vol1 -= amount;
	else channel[chan].vol1 = 0;
	if(channel[chan].vol2 - amount > 0) channel[chan].vol2 -= amount;
	else channel[chan].vol2 = 0;
}

void vol_up_alt(unsigned char chan, int amount){
	if(channel[chan].vol1 + amount < 63) channel[chan].vol1 += amount;
	else channel[chan].vol1 = 63;
	if(inst[channel[chan].inst].data[0] & 1) {
		if(channel[chan].vol2 + amount < 63) channel[chan].vol2 += amount;
		else channel[chan].vol2 = 63;
	}
}

void vol_down_alt(unsigned char chan, int amount){
	if(channel[chan].vol1 - amount > 0) channel[chan].vol1 -= amount;
	else channel[chan].vol1 = 0;
	if(inst[channel[chan].inst].data[0] & 1) {
		if(channel[chan].vol2 - amount > 0)channel[chan].vol2 -= amount;
		else channel[chan].vol2 = 0;
	}
}

void interrupt CmodPlayer_update(void){
	unsigned char	pattbreak = 0;
	unsigned char	donote, pattnr, chan, info1, info2, info, pattern_delay;
	unsigned short	track;
	unsigned long	row;
	int note;
	mod_tracks *mtrack;
	asm CLI //disable interrupts
	if(!speed) return;// song full stop
		
	// effect handling (timer dependant)
	for(chan = 0; chan < nchans; chan++) {
		mod_channel *cchan = &channel[chan];
		
		if(arplist && arpcmd && inst[cchan->inst].arpstart) {// special arpeggio
			if(cchan->arpspdcnt) cchan->arpspdcnt--;
			else { 
				if(arpcmd[cchan->arppos] != 255) {
					switch(arpcmd[cchan->arppos]) {
						case 252: cchan->vol1 = arplist[cchan->arppos];	// set volume
							if(cchan->vol1 > 63)	// ?????
							cchan->vol1 = 63;
							cchan->vol2 = cchan->vol1;
							setvolume(chan);
						break;
						case 253: cchan->key = 0; setfreq(chan); break;	// release sustaining note
						case 254: cchan->arppos = arplist[cchan->arppos]; break; // arpeggio loop
						default: if(arpcmd[cchan->arppos]) {
							int a = arpcmd[cchan->arppos] / 10;
							int b = arpcmd[cchan->arppos] % 10;
							if(a) opl2_write(0xe3 + CPlayer_op_table[chan], a - 1);
							if(b) opl2_write(0xe0 + CPlayer_op_table[chan], b - 1);
							if(arpcmd[cchan->arppos] < 10)	// ?????
							opl2_write(0xe0 + CPlayer_op_table[chan], arpcmd[cchan->arppos] - 1);
						}
					}
					if(arpcmd[cchan->arppos] != 252) {
						if(arplist[cchan->arppos] <= 96) setnote(chan,cchan->note + arplist[cchan->arppos]);
						if(arplist[cchan->arppos] >= 100) setnote(chan,arplist[cchan->arppos] - 100);
					} else setnote(chan,cchan->note);
					setfreq(chan);
					if(arpcmd[cchan->arppos] != 255) cchan->arppos++;
					cchan->arpspdcnt = inst[cchan->inst].arpspeed - 1;
				}
			}
		}
	
		info1 = cchan->info1;
		info2 = cchan->info2;
		if(flags & Decimal) info = cchan->info1 * 10 + cchan->info2;
		else info = (cchan->info1 << 4) + cchan->info2;
		switch(cchan->fx) {
			case 0:	// arpeggio
				if(info) {	
					if(cchan->trigger < 2) cchan->trigger++;
					else cchan->trigger = 0;
					switch(cchan->trigger) {
						case 0: setnote(chan,cchan->note); break;
						case 1: setnote(chan,cchan->note + info1); break;
						case 2: setnote(chan,cchan->note + info2); //break;
					}
					setfreq(chan);
				}
			break;
			case 1: slide_up(chan,info); setfreq(chan); break;	// slide up
			case 2: slide_down(chan,info); setfreq(chan); break;	// slide down
			case 3: tone_portamento(chan,cchan->portainfo); break;	// tone portamento
			case 4: vibrato(chan,cchan->vibinfo1,cchan->vibinfo2); break;	// vibrato
			case 5:	// tone portamento & volume slide
			case 6: // vibrato & volume slide
				if(cchan->fx == 5) tone_portamento(chan,cchan->portainfo);
				else vibrato(chan,cchan->vibinfo1,cchan->vibinfo2);
			case 10: // SA2 volume slide
				if(mod_del & 3) break;	
				if(info1) vol_up(chan,info1);
				else vol_down(chan,info2);
				setvolume(chan);
			break;
			case 14: // retrig note
				//if(info1 == 3)	if(!(mod_del % (info2+1))) playnote(chan);
			break;
			case 16: // AMD volume slide
				if(mod_del & 3) break;	
				if(info1) vol_up_alt(chan,info1);
				else vol_down_alt(chan,info2);
				setvolume(chan);
			break;
			case 20: 
				if(info < 50) vol_down_alt(chan,info); // RAD volume slide
				else vol_up_alt(chan,info - 50);
				setvolume(chan);
			break;
			case 26: // volume slide
				if(info1) vol_up(chan,info1);
				else vol_down(chan,info2);
				setvolume(chan);
			break;
			case 28:
				if (info1) slide_up(chan,1); cchan->info1--;
				if (info2) slide_down(chan,1); cchan->info2--;
				setfreq(chan);
			break;
		}
	}
	// speed compensation
	if(mod_del) {mod_del--; return;} //!songend;}

	// arrangement handling
	if(!resolve_order()) return;// !songend;
	pattnr = order[ord];
	if(!rw) ;//AdPlug_LogWrite("\nCmodPlayer::update(): Pattern: %d, Order: %d\n", pattnr, ord);
	// play row
	pattern_delay = 0;
	row = rw;
	for(chan = 0; chan < nchans; chan++) {
		mod_channel *cchan = &channel[chan];
		// channel active?
		//if(!((activechan >> (31 - chan)) & 1)) {  continue;} //printf("Continue %i\n",chan);
		// resolve track
		if(!(track = trackord[pattnr][chan])) {	 continue;}//AdPlug_LogWrite("------------|");
		else track--;
		
		//AdPlug_LogWrite("%3d%3d%2X%2X%2X|", tracks[track][row].note,tracks[track][row].inst, tracks[track][row].command,tracks[track][row].param1, tracks[track][row].param2);
		mtrack = &tracks[track][row];
		donote = 0;
		if(mtrack->inst) {
			cchan->inst = mtrack->inst - 1;
			if (!(flags & Faust)) {
				cchan->vol1 = 63 - (inst[cchan->inst].data[10] & 63);
				cchan->vol2 = 63 - (inst[cchan->inst].data[9] & 63);
				setvolume(chan);
			}
		}
		note = mtrack->note;
		if(note && mtrack->command != 3) {	// no tone portamento
			cchan->note = note;
			setnote(chan,note);
			cchan->nextfreq = cchan->freq;
			cchan->nextoct = cchan->oct;
			cchan->arppos = inst[cchan->inst].arpstart;
			cchan->arpspdcnt = 0;
			// handle key off
			if(note != 127) donote = 1;
		}
		cchan->fx = mtrack->command;
		cchan->info1 = mtrack->param1;
		cchan->info2 = mtrack->param2;
		
		if(donote) playnote(chan);
	
		// command handling (row dependant)
		info1 = cchan->info1;
		info2 = cchan->info2;
		if(flags & Decimal)info = cchan->info1 * 10 + cchan->info2;
		else info = (cchan->info1 << 4) + cchan->info2;
		switch(cchan->fx) {
			case 3: // tone portamento
				if(note) {
				if(note < 13) cchan->nextfreq = CmodPlayer_sa2_notetable[note - 1];
				else {
					if(note % 12 > 0) cchan->nextfreq = CmodPlayer_sa2_notetable[(note % 12) - 1];
					else cchan->nextfreq = CmodPlayer_sa2_notetable[11];
					cchan->nextoct = noteDIV12[note - 1];//(tracks[track][row].note - 1) / 12;
					if(note == 127) { // handle key off
						cchan->nextfreq = cchan->freq;
						cchan->nextoct = cchan->oct;
					}
				}
				}
				// remember vars
				if(info)cchan->portainfo = info;
			break;
			case 4: // vibrato (remember vars)
				if(info) {
					cchan->vibinfo1 = info1;
					cchan->vibinfo2 = info2;
				}
			break;
			case 7: tempo = info; break;	// set tempo
			case 8: cchan->key = 0; setfreq(chan); break;	// release sustaining note
			case 9: // set carrier/modulator volume
				if(info1) cchan->vol1 = info1<<3 -info1;
				else cchan->vol2 = info2<<3 -info2;
				setvolume(chan);
			break;
			case 11: // position jump
				pattbreak = 1; rw = 0; 
				if(info < ord) songend = 1; 
				ord = info; 
			break;
			case 12: // set volume
				cchan->vol1 = info;
				cchan->vol2 = info;
				if(cchan->vol1 > 63)cchan->vol1 = 63;
				if(cchan->vol2 > 63)cchan->vol2 = 63;
				setvolume(chan);
			break;
			case 13: // pattern break
				if(!pattbreak) { pattbreak = 1; rw = info; ord++; } 
			break;
			case 14: // extended command
			switch(info1) {
				case 0: // define cell-tremolo
					if(info2) regbd |= 128;
					else regbd &= 127;
					opl2_write(0xbd,regbd);
				break;
				case 1: // define cell-vibrato
					if(info2) regbd |= 64;
					else regbd &= 191;
					opl2_write(0xbd,regbd);
				break;
				case 4: // increase volume fine
					vol_up_alt(chan,info2);
					setvolume(chan);
				break;
				case 5: // decrease volume fine
					vol_down_alt(chan,info2);
					setvolume(chan);
				break;
				case 6: // manual slide up
					slide_up(chan,info2);
					setfreq(chan);
				break;
				case 7: // manual slide down
					slide_down(chan,info2);
					setfreq(chan);
				break;
				case 8: // pattern delay (rows)
					pattern_delay = info2 * speed;
				break;
			}
			break;
		
			case 15: // SA2 set speed
				if(info <= 0x1f) speed = info;
				if(info >= 0x32) tempo = info;
				if(!info) songend = 1;
			break;
			case 17: // alternate set volume
				cchan->vol1 = info;
				if(cchan->vol1 > 63) cchan->vol1 = 63;
				if(inst[cchan->inst].data[0] & 1) {
					cchan->vol2 = info;
					if(cchan->vol2 > 63) cchan->vol2 = 63;
				}
				setvolume(chan);
			break;
			case 18: // AMD set speed
				if(info <= 31 && info > 0) speed = info;
				if(info > 31 || !info) tempo = info;
			break;
			case 19: // RAD/A2M set speed
				speed = (info ? info : info + 1);
			break;
			case 21: // set modulator volume
				if(info <= 63)cchan->vol2 = info;
				else cchan->vol2 = 63;
				setvolume(chan);
			break;
			case 22: // set carrier volume
				if(info <= 63) cchan->vol1 = info;
				else cchan->vol1 = 63;
				setvolume(chan);
			break;
			case 23: // fine frequency slide up
				slide_up(chan,info);
				setfreq(chan);
			break;
			case 24: // fine frequency slide down
				slide_down(chan,info);
				setfreq(chan);
			break;
			case 25: // set carrier/modulator waveform
				if(info1 != 0x0f) opl2_write(0xe3 + CPlayer_op_table[chan],info1);
				if(info2 != 0x0f) opl2_write(0xe0 + CPlayer_op_table[chan],info2);
			break;
			case 27: // set chip tremolo/vibrato
				if(info1) regbd |= 128;
				else regbd &= 127;
				if(info2) regbd |= 64;
				else regbd &= 191;
				opl2_write(0xbd,regbd);
			break;
			case 29: // pattern delay (frames)
				pattern_delay = info;
			break;
		}
	}

	// speed compensation
	mod_del = speed - 1 + pattern_delay;
	
	if(!pattbreak) {	// next row (only if no manual advance)
		rw++;
		if(rw >= nrows) {
			rw = 0;
			ord++;
		}
	}

	resolve_order();	// so we can report songend right away
	
	//Return
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
	asm STI //enable interrupts
	//return !songend;
}

//LOUDNESS PLAYER
void lds_setregs(unsigned char reg, unsigned char val){
	if(fmchip[reg] == val) return;
	fmchip[reg] = val;
	opl2_write(reg, val);
}

void lds_setregs_adv(unsigned char reg, unsigned char mask,unsigned char val){
	lds_setregs(reg, (fmchip[reg] & mask) | val);
}

void lds_playsound(int inst_number, int channel_number, int tunehigh){
	unsigned int		regnum = CPlayer_op_table[channel_number];	// channel's OPL2 register
	unsigned char		volcalc, octave, remainder_192;
	unsigned short	freq;
	LDS_Channel		*c = &lds_channel[channel_number];		// current channel
	LDS_SoundBank		*i = &lds_soundbank[inst_number];		// current instrument
	Key_Hit[channel_number] = 1; //For visualizer bars
	// set fine tune
	tunehigh += ((i->finetune + c->finetune + 0x80) & 0xff) - 0x80;

	// arpeggio handling
	if(!i->arpeggio) {
		unsigned short	arpcalc = i->arp_tab[0] << 4;
		if(arpcalc > 0x800) tunehigh = tunehigh - (arpcalc ^ 0xff0) - 16;
		else tunehigh += arpcalc;
	}

	// glide handling
	if(c->glideto != 0) {
		c->gototune = tunehigh;
		c->portspeed = c->glideto;
		c->glideto = c->finetune = 0;
		return;
	}

	// set modulator registers
	lds_setregs(0x20 + regnum, i->mod_misc);
	volcalc = i->mod_vol;
	if(!c->nextvol || !(i->feedback & 1))c->volmod = volcalc;
	else c->volmod = (volcalc & 0xc0) | ((((volcalc & 0x3f) * c->nextvol) >> 6));

	if((i->feedback & 1) == 1 && allvolume != 0)
		lds_setregs(0x40 + regnum, ((c->volmod & 0xc0) | (((c->volmod & 0x3f) * allvolume) >> 8)) ^ 0x3f);
	else lds_setregs(0x40 + regnum, c->volmod ^ 0x3f);
	lds_setregs(0x60 + regnum, i->mod_ad);
	lds_setregs(0x80 + regnum, i->mod_sr);
	lds_setregs(0xe0 + regnum, i->mod_wave);

	// Set carrier registers
	lds_setregs(0x23 + regnum, i->car_misc);
	volcalc = i->car_vol;
	if(!c->nextvol) c->volcar = volcalc;
	else c->volcar = (volcalc & 0xc0) | ((((volcalc & 0x3f) * c->nextvol) >> 6));

	if(allvolume) lds_setregs(0x43 + regnum, ((c->volcar & 0xc0) | (((c->volcar & 0x3f) * allvolume) >> 8)) ^ 0x3f);
	else lds_setregs(0x43 + regnum, c->volcar ^ 0x3f);
	lds_setregs(0x63 + regnum, i->car_ad);
	lds_setregs(0x83 + regnum, i->car_sr);
	lds_setregs(0xe3 + regnum, i->car_wave);
	lds_setregs(0xc0 + channel_number, i->feedback);
	lds_setregs_adv(0xb0 + channel_number, 0xdf, 0);		// key off

	freq = lds_frequency[tunehigh % (12 * 16)];
	octave = tunehigh / (12 * 16) - 1;
	
	if(!i->glide) {
		if(!i->portamento || !c->lasttune) {
			lds_setregs(0xa0 + channel_number, freq & 0xff);
			lds_setregs(0xb0 + channel_number, (octave << 2) + 0x20 + (freq >> 8));
			c->lasttune = c->gototune = tunehigh;
		} else {
			c->gototune = tunehigh;
			c->portspeed = i->portamento;
			lds_setregs_adv(0xb0 + channel_number, 0xdf, 0x20);	// key on
		}
	} else {
		lds_setregs(0xa0 + channel_number, freq & 0xff);
		lds_setregs(0xb0 + channel_number, (octave << 2) + 0x20 + (freq >> 8));
		c->lasttune = tunehigh;
		c->gototune = tunehigh + ((i->glide + 0x80) & 0xff) - 0x80;	// set destination
		c->portspeed = i->portamento;
	}

	if(!i->vibrato) c->vibwait = c->vibspeed = c->vibrate = 0;
	else {
		c->vibwait = i->vibdelay;
		// PASCAL:    c->vibspeed = ((i->vibrato >> 4) & 15) + 1;
		c->vibspeed = (i->vibrato >> 4) + 2;
		c->vibrate = (i->vibrato & 15) + 1;
	}

	if(!(c->trmstay & 0xf0)) {
		c->trmwait = (i->tremwait & 0xf0) >> 3;
		// PASCAL:    c->trmspeed = (i->mod_trem >> 4) & 15;
		c->trmspeed = i->mod_trem >> 4;
		c->trmrate = i->mod_trem & 15;
		c->trmcount = 0;
	}

	if(!(c->trmstay & 0x0f)) {
		c->trcwait = (i->tremwait & 15) << 1;
		// PASCAL:    c->trcspeed = (i->car_trem >> 4) & 15;
		c->trcspeed = i->car_trem >> 4;
		c->trcrate = i->car_trem & 15;
		c->trccount = 0;
	}

	c->arp_size = i->arpeggio & 15;
	c->arp_speed = i->arpeggio >> 4;
	memcpy(c->arp_tab, i->arp_tab, 12);
	c->keycount = i->keyoff;
	c->nextvol = c->glideto = c->finetune = c->vibcount = c->arp_pos = c->arp_count = 0;
}

void interrupt CldsPlayer_update(void){
	unsigned short	comword, freq, octave, chan, tune, wibc, tremc, arpreg;
	byte			vbreak;
	unsigned char		level, regnum, comhi, comlo;
	int			i;
	LDS_Channel		*c;
	
	asm CLI

	if(!playing) return ;//false;

	// handle fading
	if(fadeonoff){
		if(fadeonoff <= 128) {
			if(allvolume > fadeonoff || allvolume == 0) allvolume -= fadeonoff;
			else {
				allvolume = 1;
				fadeonoff = 0;
				if(hardfade != 0) {
					playing = false;
					hardfade = 0;
					for(i = 0; i < 9; i++) lds_channel[i].keycount = 1;
				}
			}
		} else {
			if(((allvolume + (0x100 - fadeonoff)) & 0xff) <= mainvolume) allvolume += 0x100 - fadeonoff;
			else {
				allvolume = mainvolume;
				fadeonoff = 0;
			}
		}
	}
	// handle channel delay
	for(chan = 0; chan < 9; chan++) {
		c = &lds_channel[chan];
		if(c->chancheat.chandelay){
			if(!(--c->chancheat.chandelay)) lds_playsound(c->chancheat.sound, chan, c->chancheat.high);
		}
	}

	// handle notes
	if(!tempo_now) {
		vbreak = false;
		for(chan = 0; chan < 9; chan++) {
			c = &lds_channel[chan];
			if(!c->packwait) {
				unsigned short	patnum = lds_positions[posplay * 9 + chan].patnum;
				unsigned char	transpose = lds_positions[posplay * 9 + chan].transpose;
				comword = lds_patterns[patnum + c->packpos];
				comhi = comword >> 8; comlo = comword & 0xff;
				//printf("0 ");
				if(comword){
					if(comhi == 0x80) c->packwait = comlo;
					else {
						if(comhi >= 0x80) {
							switch(comhi) {
								case 0xff:
									c->volcar = (((c->volcar & 0x3f) * comlo) >> 6) & 0x3f;
									if(fmchip[0xc0 + chan] & 1) c->volmod = (((c->volmod & 0x3f) * comlo) >> 6) & 0x3f;
								break;
								case 0xfe: tempo = comword & 0x3f; break;
								case 0xfd: c->nextvol = comlo; break;
								//full keyoff here
								case 0xfc: playing = false; /*lds_setregs_adv(0xb0 + chan, 0xdf, 0);*/ break;
								case 0xfb: c->keycount = 1; break;
								case 0xfa:
									vbreak = true;
									jumppos = (posplay + 1) & lds_maxpos;
								break;
								case 0xf9:
									vbreak = true;
									jumppos = comlo & lds_maxpos;
									jumping = 1;
									if(jumppos < posplay) songlooped = true;
								break;
								case 0xf8: c->lasttune = 0; break;
								case 0xf7:
									c->vibwait = 0;
									// PASCAL: c->vibspeed = ((comlo >> 4) & 15) + 2;
									c->vibspeed = (comlo >> 4) + 2;
									c->vibrate = (comlo & 15) + 1;
								break;
								case 0xf6: c->glideto = comlo; break;
								case 0xf5: c->finetune = comlo; break;
								case 0xf4:
									if(!hardfade) {
										allvolume = mainvolume = comlo;
										fadeonoff = 0;
									}
								break;
								case 0xf3: if(!hardfade) fadeonoff = comlo; break;
								case 0xf2: c->trmstay = comlo; break;
								case 0xf1:	// panorama
								case 0xf0:	// progch
									// MIDI commands (unhandled)
								break;
								default:
									if(comhi < 0xa0) c->glideto = comhi & 0x1f;
									else ;
								break;
							}
						} else {
							unsigned char	sound;
							unsigned short	high;
							signed char	transp = transpose & 127;
					
							//Originally, in assembler code, the player first shifted
							//logically left the transpose byte by 1 and then shifted
							//arithmetically right the same byte to achieve the final,
							//signed transpose value. Since we can't do arithmetic shifts
							//in C, we just duplicate the 7th bit into the 8th one and
							//discard the 8th one completely.
					
							if(transpose & 64) transp |= 128;
					
							if(transpose & 128) {
								sound = (comlo + transp) & lds_maxsound;
								high = comhi << 4;
							} else { sound = comlo & lds_maxsound; high = (comhi + transp) << 4;}
					
							//PASCAL:
							//  sound = comlo & maxsound;
							//  high = (comhi + (((transpose + 0x24) & 0xff) - 0x24)) << 4;
					
							if(!chandelay[chan]) lds_playsound(sound, chan, high);
							else {
								c->chancheat.chandelay = chandelay[chan];
								c->chancheat.sound = sound;
								c->chancheat.high = high;
							}
						}
					}
				}
				//Writte pattrens
				/*
				MOD_DATA[offset] = (instrument_number & 0xF0) + (note >> 8); //byte 1
				MOD_DATA[offset+1] = note & 0xFF; //byte 2
				MOD_DATA[offset+2] = ((instrument_number & 0x0F) << 4) + (effect & 0x0F);//byte 3
				MOD_DATA[offset+3] = command; //byte 4
				
				//for (i = 0; i < chan; i++) printf(" -");
				printf("%i %i\n",c->packpos,chan);
				MOD_DATA[(c->packpos*9*4) + (chan*4)] = 6;*/
				c->packpos++;
				
			} else c->packwait--;
		}
		
		tempo_now = tempo;

		//The continue table is updated here, but this is only used in the
		//original player, which can be paused in the middle of a song and then
		//unpaused. Since AdPlug does all this for us automatically, we don't
		//have a continue table here. The continue table update code is noted
		//here for reference only.
	
		//if(!pattplay) {
		//  conttab[speed & maxcont].position = posplay & 0xff;
		//  conttab[speed & maxcont].tempo = tempo;
    

		pattplay++;
		if(vbreak) {
			pattplay = 0;
			for(i = 0; i < 9; i++) lds_channel[i].packpos = lds_channel[i].packwait = 0;
			posplay = jumppos;
		} else {
			if(pattplay >= pattlen) {
				pattplay = 0;
				for(i = 0; i < 9; i++) lds_channel[i].packpos = lds_channel[i].packwait = 0;
				posplay = (posplay + 1) & lds_maxpos;
			}
		}
	} else tempo_now--;

	// make effects
	for(chan = 0; chan < 9; chan++) {
		c = &lds_channel[chan];
		regnum = CPlayer_op_table[chan];
		if(c->keycount > 0) {
			if(c->keycount == 1) lds_setregs_adv(0xb0 + chan, 0xdf, 0);
			c->keycount--;
		}

		// arpeggio
		if(c->arp_size == 0) arpreg = 0;
		else {
			arpreg = c->arp_tab[c->arp_pos] << 4;
			if(arpreg == 0x800) {
				if(c->arp_pos > 0) c->arp_tab[0] = c->arp_tab[c->arp_pos - 1];
				c->arp_size = 1; c->arp_pos = 0;
				arpreg = c->arp_tab[0] << 4;
			}

			if(c->arp_count == c->arp_speed) {
				c->arp_pos++;
				if(c->arp_pos >= c->arp_size) c->arp_pos = 0;
				c->arp_count = 0;
			} else c->arp_count++;
		}

		// glide & portamento
		if(c->lasttune && (c->lasttune != c->gototune)) {
			if(c->lasttune > c->gototune) {
				if(c->lasttune - c->gototune < c->portspeed) c->lasttune = c->gototune;
				else c->lasttune -= c->portspeed;
			} else {
				if(c->gototune - c->lasttune < c->portspeed) c->lasttune = c->gototune;
				else c->lasttune += c->portspeed;
			}
		
			if(arpreg >= 0x800) arpreg = c->lasttune - (arpreg ^ 0xff0) - 16;
			else arpreg += c->lasttune;
			freq = lds_frequency[arpreg % (12 * 16)];
			octave = lds_DIV1216[arpreg];//arpreg / (12 * 16) - 1;
			lds_setregs(0xa0 + chan, freq & 0xff);
			lds_setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
		} else { // vibrato
			if(!c->vibwait) {
				if(c->vibrate) {
					wibc = lds_vibtab[c->vibcount & 0x3f] * c->vibrate;
				
					if((c->vibcount & 0x40) == 0) tune = c->lasttune + (wibc >> 8);
					else tune = c->lasttune - (wibc >> 8);
				
					if(arpreg >= 0x800) tune = tune - (arpreg ^ 0xff0) - 16;
					else tune += arpreg;
				
					freq = lds_frequency[tune % (12 * 16)];
					octave = lds_DIV1216[tune];//tune / (12 * 16) - 1;
					lds_setregs(0xa0 + chan, freq & 0xff);
					lds_setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
					c->vibcount += c->vibspeed;
				} else {
					if(c->arp_size != 0) {	// no vibrato, just arpeggio
						if(arpreg >= 0x800) tune = c->lasttune - (arpreg ^ 0xff0) - 16;
						else tune = c->lasttune + arpreg;
				
						freq = lds_frequency[tune % (12 * 16)];
						octave = lds_DIV1216[tune];//tune / (12 * 16) - 1;
						lds_setregs(0xa0 + chan, freq & 0xff);
						lds_setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
					}
				}
			} else { // no vibrato, just arpeggio
				c->vibwait--;

				if(c->arp_size != 0) {
					if(arpreg >= 0x800) tune = c->lasttune - (arpreg ^ 0xff0) - 16;
					else tune = c->lasttune + arpreg;
				
					freq = lds_frequency[tune % (12 * 16)];
					octave = lds_DIV1216[tune];//tune / (12 * 16) - 1;
					lds_setregs(0xa0 + chan, freq & 0xff);
					lds_setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
				}
			}
		}

		// tremolo (modulator)
		if(!c->trmwait) {
			if(c->trmrate) {
				tremc = lds_tremtab[c->trmcount & 0x7f] * c->trmrate;
				if((tremc >> 8) <= (c->volmod & 0x3f)) level = (c->volmod & 0x3f) - (tremc >> 8);
				else level = 0;

				if(allvolume != 0 && (fmchip[0xc0 + chan] & 1))
					lds_setregs_adv(0x40 + regnum, 0xc0, ((level * allvolume) >> 8) ^ 0x3f);
				else lds_setregs_adv(0x40 + regnum, 0xc0, level ^ 0x3f);

				c->trmcount += c->trmspeed;
			} else {
				if(allvolume != 0 && (fmchip[0xc0 + chan] & 1))
					lds_setregs_adv(0x40 + regnum, 0xc0, ((((c->volmod & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
				else
					lds_setregs_adv(0x40 + regnum, 0xc0, (c->volmod ^ 0x3f) & 0x3f);
			}
		} else {
			c->trmwait--;
			if(allvolume != 0 && (fmchip[0xc0 + chan] & 1))
				lds_setregs_adv(0x40 + regnum, 0xc0, ((((c->volmod & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
		}

		// tremolo (carrier)
		if(!c->trcwait) {
			if(c->trcrate) {
				tremc = lds_tremtab[c->trccount & 0x7f] * c->trcrate;
				if((tremc >> 8) <= (c->volcar & 0x3f)) level = (c->volcar & 0x3f) - (tremc >> 8);
				else level = 0;

				if(allvolume != 0)
					lds_setregs_adv(0x43 + regnum, 0xc0, ((level * allvolume) >> 8) ^ 0x3f);
				else lds_setregs_adv(0x43 + regnum, 0xc0, level ^ 0x3f);
				c->trccount += c->trcspeed;
			} else {
				if(allvolume != 0)
					lds_setregs_adv(0x43 + regnum, 0xc0, ((((c->volcar & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
				else
					lds_setregs_adv(0x43 + regnum, 0xc0, (c->volcar ^ 0x3f) & 0x3f);
			}
		} else {
			c->trcwait--;
			if(allvolume != 0) lds_setregs_adv(0x43 + regnum, 0xc0, ((((c->volcar & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
		}
	}
	
	//Return
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
	asm STI //enable interrupts
	return;// (!playing || songlooped) ? false : true;
}


//EDLIB PLAYER
void d00_rewind(int subsong){
	struct Stpoin {
		unsigned short ptr[9];
		unsigned char volume[9],dummy[5];
	} *tpoin;
	int i;
	Music_Remove_Interrupt();
	if(version > 1) {	// do nothing if subsong > number of subsongs
		if(subsong >= header->subsongs)
			return;
	} else
		if(subsong >= header1->subsongs)
			return;

	memset(&d00_channel[0],0,sizeof(d00_channel));
	if(version > 1)
		tpoin = (struct Stpoin *)((char *)filedata + header->tpoin);
	else
		tpoin = (struct Stpoin *)((char *)filedata + header1->tpoin);
	
	for(i=0;i<9;i++) {
		if(tpoin[subsong].ptr[i]) {	// track enabled
			d00_channel[i].speed = *((unsigned short *)((char *)filedata + tpoin[subsong].ptr[i]));
			d00_channel[i].order = (unsigned short *)((char *)filedata + tpoin[subsong].ptr[i] + 2);
		} else {					// track disabled
			d00_channel[i].speed = 0;
			d00_channel[i].order = 0;
		}
		
		d00_channel[i].ispfx = 0xffff; d00_channel[i].spfx = 0xffff;	// no SpFX
		d00_channel[i].ilevpuls = 0xff; d00_channel[i].levpuls = 0xff;	// no LevelPuls
		d00_channel[i].cvol = tpoin[subsong].volume[i] & 0x7f;			// our player may savely ignore bit 7
		d00_channel[i].vol = d00_channel[i].cvol;						// initialize volume
	}
	songend = 0;
	opl2_clear(); opl2_write(1,32);	// reset OPL chip
}

unsigned int d00_getsubsongs(){
	if(version <= 1) return header1->subsongs;	// return number of subsongs	
	else return header->subsongs;
}

//60.558
void d00_setvolume(unsigned char chan){
	unsigned char	op = CPlayer_op_table[chan];
	unsigned short	insnr = d00_channel[chan].inst;
	struct Sinsts *inst = &d00_inst[insnr];
	d00channel *channel = &d00_channel[chan];
	unsigned char chvol = 63 - channel->vol;
	
// 1 - Original
	//byte a = (int)(63-((63-(d00_inst[insnr].data[2] & 63))/63.0)*(63-d00_channel[chan].vol)) + (d00_inst[insnr].data[2] & 192);
	//byte b = (int)(63-((63-d00_channel[chan].modvol)/63.0)*(63-d00_channel[chan].vol)) + (d00_inst[insnr].data[7] & 192);
	
// 2 - Optimized for integer operations. by Miguel Angel from RetroPC telegram Group.
	//byte a = 63-((63-(inst->data[2] & 63))*chvol)/63 + (inst->data[2] & 192);
	//byte b = 63-((63-((63-channel->modvol))*chvol)/63) + (inst->data[7] & 192);
	
// 3 - Precalculated division and multiplication (uses 8k)
	byte a = 63- d00_MUL64_DIV63[63-(inst->data[2] & 63)][chvol] + (inst->data[2] & 192);
	byte b = 63- /*d00_MUL64_DIV63[63-((63-channel->modvol))][0]*/ + (inst->data[7] & 192);
	
	opl2_write(0x43 + op, a);
	if(inst->data[10] & 1) opl2_write(0x40 + op,b);
	else opl2_write(0x40 + op,channel->modvol + (inst->data[7] & 192));
}

void d00_setfreq(unsigned char chan){
	d00channel *channel = &d00_channel[chan];
	unsigned short freq = channel->freq;
	if(version == 4) freq += d00_inst[channel->inst].tunelev;	// v4: apply instrument finetune

	freq += channel->slideval;
	opl2_write(0xa0 + chan, freq & 255);
	if(channel->key)
		opl2_write(0xb0 + chan, ((freq >> 8) & 31) | 32);
	else
		opl2_write(0xb0 + chan, (freq >> 8) & 31);
}

void d00_setinst(unsigned char chan){
	unsigned char	op = CPlayer_op_table[chan];
	unsigned short	insnr = d00_channel[chan].inst;
	byte *data = (byte*) d00_inst[insnr].data;
	byte tune = d00_inst[insnr].tunelev & 1;
	// set instrument data
	
	asm push ds
	asm push di
	asm push si
	
	asm mov dx,ADLIB_PORT
	asm mov al,0x01
	asm out dx,al
	asm inc dx
	asm mov al,0x20
	asm out dx,al
	
	//Set inst
	asm les bx,[data]
	asm mov dx,ADLIB_PORT
	asm mov ah,op
	
	asm mov al,0x63
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	asm dec dx
	asm inc bx
	asm mov al,0x83
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	asm dec dx
	asm add bx,2
	asm mov al,0x23
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	asm dec dx
	asm inc bx
	asm mov al,0xe3
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	asm dec dx
	asm inc bx
	asm mov al,0x60
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	asm dec dx
	asm inc bx
	asm mov al,0x80
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	asm dec dx
	asm add bx,2
	asm mov al,0x20
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	asm dec dx
	asm inc bx
	asm mov al,0xe0
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]
	asm out dx,al
	
	
	asm mov	ax,word ptr [version] //if (version == 0)
	asm jne noversion
	
	asm dec dx
	asm inc bx
	asm mov ah,chan
	asm mov al,0xC0
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]//10
	asm	shl al,1
	asm mov	ah,tune
	asm add al,ah
	asm out dx,al
	asm jmp end
	
noversion:
	
	asm dec dx
	asm inc bx
	asm mov ah,chan
	asm mov al,0xC0
	asm add al,ah
	asm out dx,al
	asm inc dx
	asm mov al,byte ptr es:[bx]//10
	asm out dx,al

end:
	
	asm pop si
	asm pop di
	asm pop ds
	
	
	/*if(version)
		opl2_write(0xc0 + chan, data[10]);
	else
		opl2_write(0xc0 + chan, (data[10] << 1) + tune);*/
}

void d00_playnote(unsigned char chan){
	// set misc vars & play
	Key_Hit[chan] = 1;
	opl2_write(0xb0 + chan, 0);	// stop old note
	d00_setinst(chan);
	d00_channel[chan].key = 1;
	d00_setfreq(chan);
	d00_setvolume(chan);
}

void d00_vibrato(unsigned char chan){
	d00channel *channel = &d00_channel[chan];
	if(!channel->vibdepth)return;

	if(channel->trigger) channel->trigger--;
	else {
		channel->trigger = channel->vibdepth;
		channel->vibspeed = -channel->vibspeed;
	}
	channel->freq += channel->vibspeed;
	d00_setfreq(chan);
}

//60.366
void /*interrupt*/ Cd00Player_update() {
	unsigned char	c,cnt,trackend=0,fx,note;
	unsigned short	ord,*patt,buf,fxop;
	
	asm CLI
	
	// effect handling (timer dependant)
	for(c=0;c<9;c++) {
		d00channel *channel = &d00_channel[c];
		struct Sinsts *inst = &d00_inst[channel->inst];
		channel->slideval += channel->slide; d00_setfreq(c);	// sliding
		d00_vibrato(c);	// d00_vibrato

		if(channel->spfx != 0xffff) {	// SpFX
			struct Sspfx *spfx_chan = &spfx[channel->spfx];
			if(channel->fxdel) channel->fxdel--;
			else {
				channel->spfx = spfx_chan->ptr;
				channel->fxdel = spfx_chan->duration;
				channel->inst = spfx_chan->instnr & 0xfff;
				spfx_chan = &spfx[channel->spfx];
				if(spfx_chan->modlev != 0xff) channel->modvol = spfx_chan->modlev;
				d00_setinst(c);
				if(spfx_chan->instnr & 0x8000) note = spfx_chan->halfnote; // locked frequency
				else note = spfx_chan->halfnote + channel->note;			// unlocked frequency
				channel->freq = d00_notetable[note] + (noteDIV12[note] << 10);
				d00_setfreq(c);
			}
			channel->modvol += spfx_chan->modlevadd; channel->modvol &= 63;
			d00_setvolume(c);
		}

		if(channel->levpuls != 0xff){	// Levelpuls
			if(channel->frameskip) channel->frameskip--;
			else {
				channel->frameskip = inst->timer;
				if(channel->fxdel) channel->fxdel--;
				else {
					channel->levpuls = levpuls[channel->levpuls].ptr - 1;
					channel->fxdel = levpuls[channel->levpuls].duration;
					if(levpuls[channel->levpuls].level != 0xff) channel->modvol = levpuls[channel->levpuls].level;
				}
				channel->modvol += levpuls[channel->levpuls].voladd; channel->modvol &= 63;
				d00_setvolume(c);
			}
		}
	// song handling
		if(version < 3 ? channel->del : channel->del <= 0x7f) {
			if(version == 4)	// v4: hard restart SR
				if(channel->del == inst->timer)
					if(channel->nextnote) opl2_write(0x83 + CPlayer_op_table[c], inst->sr);
			if(version < 3) channel->del--;
			else
				if(channel->speed) channel->del += channel->speed;
				else { channel->seqend = 1; continue;}
		} else {
			if(channel->speed) {
				if(version < 3) channel->del = channel->speed;
				else {
					channel->del &= 0x7f;
					channel->del += channel->speed;
				}
			} else { channel->seqend = 1; continue;}
			if(channel->rhcnt) { channel->rhcnt--; continue;}	// process pending REST/HOLD events
readorder:	// process arrangement (orderlist)
			ord = channel->order[channel->ordpos];
			switch(ord) {
				case 0xfffe: channel->seqend = 1; continue;	// end of arrangement stream
				case 0xffff:										// jump to order
					channel->ordpos = channel->order[channel->ordpos+1];
					channel->seqend = 1;
					goto readorder;
				default:
					if(ord >= 0x9000) {		// set speed
						channel->speed = ord & 0xff;
						ord = channel->order[channel->ordpos - 1];
						channel->ordpos++;
					} else
						if(ord >= 0x8000) {	// transpose track
							channel->transpose = ord & 0xff;
							if(ord & 0x100)
								channel->transpose = -channel->transpose;
							ord = channel->order[++channel->ordpos];
						}
					patt = (unsigned short *)((char *)filedata + seqptr[ord]);
					break;
			}
readseq:	// process sequence (pattern)
			if(!version) channel->rhcnt = channel->irhcnt;	// v0: always initialize rhcnt
			if(patt[channel->pattpos] == 0xffff) {	// pattern ended?
				channel->pattpos = 0;
				channel->ordpos++;
				goto readorder;
			}
			cnt = patt[channel->pattpos] >> 8;
			note = patt[channel->pattpos] & 0xff;
			fx = patt[channel->pattpos] >> 12;
			fxop = patt[channel->pattpos] & 0x0fff;
			channel->pattpos++;
			channel->nextnote = patt[channel->pattpos] & 0x7f;//& 0xff) & 0x7f;
			if(version ? cnt < 0x40 : !fx) {	// note event
				
				switch(note) {
					case 0:						// REST event
					case 0x80:
						if(!note || version) {
							channel->key = 0;
							d00_setfreq(c);
						}
						// fall through...
					case 0x7e:					// HOLD event
						if(version)
							channel->rhcnt = cnt;
						channel->nextnote = 0;
						break;
					default:					// play note
						channel->slideval = 0; channel->slide = 0; channel->vibdepth = 0;	// restart fx
	
						if(version) {	// note handling for v1 and above
							if(note > 0x80)	// locked note (no d00_channel transpose)
								note -= 0x80;
							else			// unlocked note
								note += channel->transpose;
							channel->note = note;	// remember note for SpFX
	
							if(channel->ispfx != 0xffff && cnt < 0x20) {	// reset SpFX
								struct Sspfx *spfx_chan;
								channel->spfx = channel->ispfx;
								spfx_chan = &spfx[channel->spfx];
								
								if(spfx_chan->instnr & 0x8000)	// locked frequency
									note = spfx_chan->halfnote;
								else												// unlocked frequency
									note += spfx_chan->halfnote;
								channel->inst = spfx_chan->instnr & 0xfff;
								channel->fxdel = spfx_chan->duration;
								if(spfx_chan->modlev != 0xff)
									channel->modvol = spfx_chan->modlev;
								else
									channel->modvol = inst->data[7] & 63;
							}
	
							if(channel->ilevpuls != 0xff && cnt < 0x20) {	// reset LevelPuls
								channel->levpuls = channel->ilevpuls;
								channel->fxdel = levpuls[channel->levpuls].duration;
								channel->frameskip = inst->timer;
								if(levpuls[channel->levpuls].level != 0xff)
									channel->modvol = levpuls[channel->levpuls].level;
								else
									channel->modvol = inst->data[7] & 63;
							}
	
							channel->freq = d00_notetable[note] + (noteDIV12[note] << 10);
							if(cnt < 0x20)	// normal note
								d00_playnote(c);
							else {			// tienote
								d00_setfreq(c);
								cnt -= 0x20;	// make count proper
							}
							channel->rhcnt = cnt;
						} else {	// note handling for v0
							if(cnt < 2)	// unlocked note
								note += channel->transpose;
							channel->note = note;
	
							channel->freq = d00_notetable[note] + (noteDIV12[note] << 10);
							if(cnt == 1) d00_setfreq(c);	// tienote
							else d00_playnote(c);			// normal note
								
						}
					break;
				}
				continue;	// event is complete
			} else {		// effect event
				struct Sinsts *sinst = &d00_inst[fxop];
				switch(fx) {
					case 6:		// Cut/Stop Voice
						buf = channel->inst;
						channel->inst = 0;
						d00_playnote(c);
						channel->inst = buf;
						channel->rhcnt = fxop;
						continue;	// no note follows this event
					case 7:		// d00_vibrato
						channel->vibspeed = fxop & 0xff;
						channel->vibdepth = fxop >> 8;
						channel->trigger = fxop >> 9;
					break;
					case 8:		// v0: Duration
						if(!version) channel->irhcnt = fxop;
					break;
					case 9:		// New Level
						channel->vol = fxop & 63;
						if(channel->vol + channel->cvol < 63) channel->vol += channel->cvol; // apply d00_channel volume
						else channel->vol = 63;
						d00_setvolume(c);
					break;
					case 0xb:	// v4: Set SpFX
						if(version == 4) channel->ispfx = fxop;
					break;
					case 0xc:	// Set Instrument
						channel->ispfx = 0xffff;
						channel->spfx = 0xffff;
						channel->inst = fxop;
						channel->modvol = sinst->data[7] & 63;
						if(version < 3 && version && sinst->tunelev) channel->ilevpuls = sinst->tunelev - 1; // Set LevelPuls
						else {
							channel->ilevpuls = 0xff;
							channel->levpuls = 0xff;
						}
					break;
					case 0xd: channel->slide = fxop; break; // Slide up
					case 0xe: channel->slide = -fxop;	break;// Slide down	
				}
				goto readseq;	// event is incomplete, note follows
			}
		}
	}
	for(c=0;c<9;c++) if(d00_channel[c].seqend) trackend++;
	if(trackend == 9) songend = 1;

	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
	asm STI //enable interrupts
	return ;//!songend;
}


//VGM Player
void interrupt CVGM_update(void){
	asm CLI
	while (!vgm_music_wait){
		byte command,reg,val,byte3,data_type;
		long data_size;
		command = ADPLUG_music_data[vgm_music_offset++];
		switch (command){
			case 0x5a: //read reg, val
				reg = ADPLUG_music_data[vgm_music_offset++];
				val = ADPLUG_music_data[vgm_music_offset++];
				//cprintf("%02X %02X\n",reg,val);
			break;
			case 0x61: //delay
				vgm_music_offset++;
				vgm_music_wait = (word) ADPLUG_music_data[vgm_music_offset++]; //delay
				//vgm_music_wait /=256;
			break;
			case 0x62: //delay
				vgm_music_wait = 735/256;
			break;
			case 0x63: //delay
				vgm_music_wait = 882/256;
			break;
			case 0x66: //end
				vgm_music_wait = 0;
				vgm_music_offset = 0;
			break;
			case 0x67: //data block. pcm sample?
				//tt ss ss ss ss (data)
				data_type = ADPLUG_music_data[vgm_music_offset++]; //data type (I will only play 8 bit unsigned pcm)
				data_size = ADPLUG_music_data[vgm_music_offset+=4]; //data size to read 
				vgm_music_offset+=data_size;//jump data block (for the moment)
			break;
			case 0x90:
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95: //nothing yet, (PCM play)
			break;
			case 0xAA: //nothing (dual chip)
			break;
			default:
				//Short delay commands, too short to be used in pc xt
				if (command > 0x76) vgm_music_wait = 1;//Looks like this fixes most songs
				/*if (command >= 0x70 && command <= 0x7F){
					vgm_music_wait = (command & 0x0F) + 1;
				}
				//if (vgm_music_wait && (vgm_music_wait < 40)) vgm_music_wait = 0; // skip too short pauses
				//Other skip commands
				switch(command & 0xF0){
					case 0x30:
						vgm_music_offset+=1;
					break;
					case 0x40:
					case 0x50:
					case 0xA0:
					case 0xB0:
						vgm_music_offset+=2;
					break;
					case 0xC0:
					case 0xD0:
						vgm_music_offset+=3;
					break;
					case 0xE0:
					case 0xF0:
						vgm_music_offset+=4;
					break;
				}
				*/
			break;
		}
		opl2_write(reg, val);
		//Detect key hits
		switch (reg){
			case 0xB0:
			case 0xB1:
			case 0xB2:
			case 0xB3:
			case 0xB4:
			case 0xB5:
			case 0xB6:
			case 0xB7:
			case 0xB8: 
				Key_Hit[reg - 0xB0] = 1;
			break;
		}
		//if (vgm_music_offset > vgm_data_size) {vgm_music_offset = 0; vgm_music_wait = 0;}
	}
	vgm_music_wait--;
	
	asm mov al,020h
	asm mov dx,020h
	asm out dx, al	//PIC, EOI
	asm STI //enable interrupts
	
}

//HERAD PLAYER
void CheradPlayer_rewind(int subsong){
/*	dword j;
	wTime = 0;
	songend = false;

	ticks_pos = -1; // there's always 1 excess tick at start
	total_ticks = 0;
	loop_pos = -1;
	loop_times = 1;

	for (int i = 0; i < nTracks; i++)
	{
		track[i].pos = 0;
		j = 0;
		while (track[i].pos < track[i].size)
		{
			j += GetTicks(i);
			switch (track[i].data[track[i].pos++] & 0xF0)
			{
			case 0x80:	// Note Off
				track[i].pos += (v2 ? 1 : 2);
				break;
			case 0x90:	// Note On
			case 0xA0:	// Unused
			case 0xB0:	// Unused
				track[i].pos += 2;
				break;
			case 0xC0:	// Program Change
			case 0xD0:	// Aftertouch
			case 0xE0:	// Pitch Bend
				track[i].pos++;
				break;
			default:
				track[i].pos = track[i].size;
				break;
			}
		}
		if (j > total_ticks)
			total_ticks = j;
		track[i].pos = 0;
		track[i].counter = 0;
		track[i].ticks = 0;
		chn[i].program = 0;
		chn[i].playprog = 0;
		chn[i].note = 0;
		chn[i].keyon = false;
		chn[i].bend = HERAD_BEND_CENTER;
		chn[i].slide_dur = 0;
	}
	if (v2)
	{
		if (!wLoopStart || wLoopCount) wLoopStart = 1; // if loop not specified, start from beginning
		if (!wLoopEnd || wLoopCount) wLoopEnd = getpatterns() + 1; // till the end
		if (wLoopCount) wLoopCount = 0; // repeats forever
	}

	opl->init();
	opl->write(1, 32); // Enable Waveform Select
	opl->write(0xBD, 0); // Disable Percussion Mode
	opl->write(8, 64); // Enable Note-Sel
	if (AGD)
	{
		opl->setchip(1);
		opl->write(5, 1); // Enable OPL3
		opl->write(4, 0); // Disable 4OP Mode
		opl->setchip(0);
	}*/
}




