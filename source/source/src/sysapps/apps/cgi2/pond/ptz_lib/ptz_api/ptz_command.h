#ifndef  _PTZ_COMMAND_H_
#define  _PTZ_COMMAND_H_

#define SERIALDEV	"/dev/tts/s0"

#define byte 		unsigned char 
#define MaxSize 	64	
// Pelco D Protocol
// Command 1
#define FocusNear 	0x01
#define IrisOpen	0x02
#define IrisClose	0x04
#define CameraOnOff	0x08
#define AutoManualScan	0x10
#define Sense		0x80

// Command 2
#define PanRight 	0x02
#define PanLeft		0x04
#define TiltUp		0x08
#define TiltDown	0x10
#define ZoomTele	0x20
#define ZoomWide	0x40
#define FocusFar	0x80

// Data 1
#define PanSpeedMin	0x00
#define PanSpeedMax	0x3F // Turbo 0xFF

// Data 2
#define TiltSpeedMin	0x00
#define TiltSpeedMax	0x3F // Turbo 0xFF

typedef enum PresetAction{ 
	SET, CLEAR, GOTO
} PresetAction;

typedef enum AuxAction{ 
	Set=0x09, Clear=0x0B
} AuxAction;

typedef enum Action{
	Start, Stop
} Action;

typedef enum LensSpeed{  
	Low=0x00, Medium=0x01, High=0x02, Turbo=0x03
} LensSpeed;

typedef enum PatternAction{ 
	PA_Start, PA_Stop, PA_Run
} PatternAction;

typedef enum SwitchAction{
	SWA_Auto=0x00, SWA_On=0x01, SWA_Off=0x02
} SwitchAction;

typedef enum Switch{
	SW_On=0x01, SW_Off=0x02
} Switch;

typedef enum{
	Near=FocusNear, Far=FocusFar
} Focus;

typedef enum{
	Wide=ZoomWide, Tele=ZoomTele
} Zoom;

typedef enum{
	Up=TiltUp, Down=TiltDown
} Tilt;

typedef enum{
	Left=PanLeft, Right=PanRight
} Pan; 

typedef enum{
	SC_Auto,SC_Manual
} Scan;

typedef enum{
	Open=IrisOpen, Close=IrisClose
} Iris;


typedef struct DPkt_t {
    unsigned char sync;		// always 0xFFH
    unsigned char addr;		// 00 - FF
    unsigned char cmd1;		//
    unsigned char cmd2;		//
    unsigned char data1;
    unsigned char data2;
    unsigned char checksum;	// addr,cmd1,cmd2,data1,data2,XOR sum
} DPkt_t;

typedef struct PPkt_t {
    unsigned char stx;		// always 0xA0 - start transmission
    unsigned char addr;		// 0x00 - 0x1F
    unsigned char data1;	//
    unsigned char data2;	//
    unsigned char data3;	//
    unsigned char data4;	//
    unsigned char etx;		// always 0xAF - end transmission
    unsigned char checksum;	// stx,addr,data1,data2,data3,data4,etx,XOR sum
} PPkt_t;

#endif
