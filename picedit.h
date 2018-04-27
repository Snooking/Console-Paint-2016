#ifndef PICEDIT
#define PICEDIT

#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED				4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW			14
#define WHITE			15

#ifndef EOF
#define EOF		-1
#endif

#define LASTMODE	-1
#define C80		3
#define C4350		8
#define FULLSCREEN	0x8000

#define menux 1
#define menuy 1
#define xdraw 35
#define ydraw 1
#define wstart 50
#define hstart 20


extern int _wscroll;

struct text_info {
	unsigned char curx;
	unsigned char cury;
	unsigned short attribute;
	unsigned short normattr;
	unsigned char screenwidth;
	unsigned char screenheight;
};

#ifdef __cplusplus
extern "C" {
#endif
void cursor();
void textbackground(int newcolor);
void textcolor(int newcolor);
void drawMenu();
void clrscr(void);

int getch(void);
int getche(void);
void newFile();

void gotoxy(int x, int y);
int wherex(void);
int wherey(void);

int cputs(const char *str);
int cgets(const char *str);
int putch(int c);

void settitle(const char *title);
void fillEmptyTable();
int chooseColor(int back);
void showMyPic();
void getName();
void showYourPic();
void saveToFile();

void showCoords();
int fill(int whatcolor, int xpos, int ypos, int towhatcolor);
void walk();
void reactToSign();
void loadPicEdit();

#ifdef __cplusplus
}
#endif
#endif
