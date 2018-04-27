#include<stdio.h>
#include<malloc.h>
#include<string.h>
#include<windows.h>
#include"picedit.h"

#ifdef __cplusplus
#define EXTERNC	extern "C"
#else
#define EXTERNC
#endif

int _wscroll;

typedef struct _Conio2ThreadData {
	HANDLE output;
	HANDLE input;
	int attrib;
	int width;
	int height;
	int _wscroll;
	int charCount;
	int ungetCount;
	int charValue;
	int charFlag;
	int ungetBuf[16];
	int zn;
	int back;
	int newback;
	int w = wstart;
	int h = hstart;
	int defaultwidth = 1;
	int defaultheight = 1;
	int x = xdraw;
	int y = ydraw;
	int draw = 0;
	int *sign;
	char name_of_file[32];

	int origwidth;
	int origheight;
	int origdepth;
	int lastmode;
	DWORD prevOutputMode;
	DWORD prevInputMode;
} Conio2ThreadData;

Conio2ThreadData thData;
char coordx[3];
char coordy[3];

static WORD ToWinAttribs(int attrib) {
	WORD rv = 0;
	if (attrib & 1) rv |= FOREGROUND_BLUE;
	if (attrib & 2) rv |= FOREGROUND_GREEN;
	if (attrib & 4) rv |= FOREGROUND_RED;
	if (attrib & 8) rv |= FOREGROUND_INTENSITY;
	if (attrib & 16) rv |= BACKGROUND_BLUE;
	if (attrib & 32) rv |= BACKGROUND_GREEN;
	if (attrib & 64) rv |= BACKGROUND_RED;
	if (attrib & 128) rv |= BACKGROUND_INTENSITY;
	return rv;
};

static void UpdateWScroll() {
	if (_wscroll != thData._wscroll) {
		if (_wscroll) SetConsoleMode(thData.output, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
		else SetConsoleMode(thData.output, ENABLE_PROCESSED_OUTPUT);
		thData._wscroll = _wscroll;
	};
};

static int FromWinAttribs(WORD attrib) {
	int rv = 0;
	if (attrib & FOREGROUND_BLUE) rv |= 1;
	if (attrib & FOREGROUND_GREEN) rv |= 2;
	if (attrib & FOREGROUND_RED) rv |= 4;
	if (attrib & FOREGROUND_INTENSITY) rv |= 8;
	if (attrib & BACKGROUND_BLUE) rv |= 16;
	if (attrib & BACKGROUND_GREEN) rv |= 32;
	if (attrib & BACKGROUND_RED) rv |= 64;
	if (attrib & BACKGROUND_INTENSITY) rv |= 128;
	return rv;
};

static void GetCP(int *x, int *y) {
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(thData.output, &info);
	if (x) *x = info.dwCursorPosition.X + 1;
	if (y) *y = info.dwCursorPosition.Y + 1;
};

static int HandleKeyEvent(INPUT_RECORD *buf) {
	int ch;
	ch = (int)(buf->Event.KeyEvent.uChar.AsciiChar) & 255;
	if (ch == 0) ch = 0x8000 + buf->Event.KeyEvent.wVirtualKeyCode;
	if (ch == 0x8010 || ch == 0x8011 || ch == 0x8012 || ch == 0x8014
		|| ch == 0x8090 || ch == 0x8091) return 0;
	thData.charCount = buf->Event.KeyEvent.wRepeatCount;
	thData.charFlag = ch & 0x8000 ? 1 : 0;
	if (thData.charFlag) thData.charCount *= 2;
	int alt = (buf->Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
	int ctrl = (buf->Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
	switch (ch) {
	case 0x8000 + 33:	ch = 0x8000 + 73; break;
	case 0x8000 + 34:	ch = 0x8000 + 81; break;
	case 0x8000 + 35:	ch = 0x8000 + 79; break;
	case 0x8000 + 36:	ch = 0x8000 + 71; break;
	case 0x8000 + 37:	if (ctrl) ch = 0x8073;
						else if (alt) ch = 0x809b;
						else ch = 0x8000 + 75; break;
	case 0x8000 + 38:	if (ctrl) ch = 0x808d;
						else if (alt) ch = 0x8098;
						else ch = 0x8000 + 72; break;
	case 0x8000 + 39:	if (ctrl) ch = 0x8074;
						else if (alt) ch = 0x809d;
						else ch = 0x8000 + 77; break;
	case 0x8000 + 40:	if (ctrl) ch = 0x8091;
						else if (alt) ch = 0x80a0;
						else ch = 0x8000 + 80; break;
	case 0x8000 + 46:	ch = 0x8000 + 83; break;
	case 0x8000 + 112:	ch = 0x8000 + 59; break;
	case 0x8000 + 113:	ch = 0x8000 + 60; break;
	case 0x8000 + 114:	ch = 0x8000 + 61; break;
	case 0x8000 + 115:	ch = 0x8000 + 62; break;
	case 0x8000 + 116:	ch = 0x8000 + 63; break;
	case 0x8000 + 117:	ch = 0x8000 + 64; break;
	case 0x8000 + 118:	ch = 0x8000 + 65; break;
	case 0x8000 + 119:	ch = 0x8000 + 66; break;
	case 0x8000 + 120:	ch = 0x8000 + 67; break;
	case 0x8000 + 121:	ch = 0x8000 + 68; break;
	case 0x8000 + 122:	ch = 0x8000 + 133; break;
	case 0x8000 + 123:	ch = 0x8000 + 134; break;
	};
	thData.charValue = ch & 0x7fff;
	return 1;
};

static void ResizeConsole(HANDLE con, int w, int h, int d) {
	int cw, ch;
	COORD s;
	SMALL_RECT r;
	CONSOLE_SCREEN_BUFFER_INFO info;

	BOOL res = GetConsoleScreenBufferInfo(con, &info);
	cw = info.srWindow.Right - info.srWindow.Left + 1;
	ch = info.srWindow.Bottom - info.srWindow.Top + 1;

	if (w < cw || h < ch) {
		r.Top = 0;
		r.Left = 0;
		r.Right = (SHORT)(min(w, cw) - 1);
		r.Bottom = (SHORT)(min(h, ch) - 1);
		SetConsoleWindowInfo(con, TRUE, &r);
	};

	if (d < h) d = h;
	s.X = (SHORT)w;
	s.Y = (SHORT)d;
	SetConsoleScreenBufferSize(con, s);
	r.Top = 0;
	r.Left = 0;
	r.Right = (SHORT)(w - 1);
	r.Bottom = (SHORT)(h - 1);
	SetConsoleWindowInfo(con, TRUE, &r);
};

static void InitConio2(Conio2ThreadData *data) {
	data->output = GetStdHandle(STD_OUTPUT_HANDLE);
	data->input = GetStdHandle(STD_INPUT_HANDLE);
	data->ungetCount = 0;
	data->charCount = 0;

	CONSOLE_SCREEN_BUFFER_INFO info;
	BOOL rc = GetConsoleScreenBufferInfo(data->output, &info);
	if (rc) {
		data->origwidth = info.srWindow.Right - info.srWindow.Left + 1;
		data->origheight = info.srWindow.Bottom - info.srWindow.Top + 1;
		data->origdepth = info.dwSize.Y;
	}
	else {
		data->origwidth = 80;
		data->origheight = 25;
		data->origdepth = 25;
	}

	data->width = data->origwidth;
	data->height = data->origheight;

	data->attrib = 0x07;
	data->lastmode = C80;
	data->_wscroll = -1;
	SetConsoleTextAttribute(thData.output, ToWinAttribs(thData.attrib));
	GetConsoleMode(data->input, &(data->prevInputMode));
	GetConsoleMode(data->output, &(data->prevOutputMode));
	SetConsoleMode(data->input, ENABLE_PROCESSED_INPUT);
	UpdateWScroll();
};


static void ExitConio2(Conio2ThreadData *data) {
	ResizeConsole(data->output, data->origwidth, data->origheight, data->origdepth);
	SetConsoleMode(data->input, data->prevInputMode);
	SetConsoleMode(data->output, data->prevOutputMode);
};

EXTERNC 
void cursor() {
	CONSOLE_CURSOR_INFO inf;
	GetConsoleCursorInfo(thData.output, &inf);
		inf.bVisible = FALSE;
		SetConsoleCursorInfo(thData.output, &inf);
};

EXTERNC
void textbackground(int newcolor) {
	thData.attrib = (thData.attrib & 0x0f) | ((newcolor & 15) << 4);
	SetConsoleTextAttribute(thData.output, ToWinAttribs(thData.attrib));
};

EXTERNC
void textcolor(int newcolor) {
	thData.attrib = (thData.attrib & 0xf0) | (newcolor & 15);
	SetConsoleTextAttribute(thData.output, ToWinAttribs(thData.attrib));
};

EXTERNC
void drawMenu() {
	cursor();
	textbackground(BLACK);
	gotoxy(menux, menuy);
	cputs("ESC = Exit");
	gotoxy(menux, menuy + 1);
	cputs("Arrows = Walking");
	gotoxy(menux, menuy + 2);
	cputs("0-9qwerty = Color choose");
	gotoxy(menux, menuy + 3);
	cputs("F - Fill");
	gotoxy(menux, menuy + 4);
	cputs("I = Load default picture");
	gotoxy(menux, menuy + 5);
	cputs("O = Load picture");
	gotoxy(menux, menuy + 6);
	cputs("S = Save picture");
	gotoxy(menux, menuy + 7);
	cputs("N = Create new file");

};

EXTERNC
void clrscr() {
	int i;
	COORD pos, size;
	SMALL_RECT trg;
	CHAR_INFO *buf;
	textbackground(0);
	buf = (CHAR_INFO *)alloca(sizeof(CHAR_INFO) * thData.width * thData.height);
	for (i = 0; i < thData.width * thData.height; i++) {
		buf[i].Char.UnicodeChar = ' ';
		buf[i].Attributes = thData.attrib;
	};

	pos.X = 0; pos.Y = 0;
	size.X = thData.width; size.Y = thData.height;
	trg.Left = 0; trg.Top = 0;
	trg.Right = thData.width - 1; trg.Bottom = thData.height - 1;
	WriteConsoleOutput(thData.output, buf, size, pos, &trg);
	drawMenu();
};

EXTERNC
int getch() {
	BOOL rv;
	DWORD n;
	INPUT_RECORD buf;

	if (thData.ungetCount > 0) {
		thData.ungetCount--;
		return thData.ungetBuf[thData.ungetCount];
	};

	if (thData.charCount > 0) {
		thData.charCount--;
		if ((thData.charCount & 1) && thData.charFlag) return 0;
		else return thData.charValue;
	};

	while (1) {
		rv = ReadConsoleInput(thData.input, &buf, 1, &n);
		if (!rv) continue;
		if (buf.EventType != KEY_EVENT) continue;
		if (!buf.Event.KeyEvent.bKeyDown) continue;
		if (HandleKeyEvent(&buf)) break;
	};

	thData.charCount--;
	if ((thData.charCount & 1) && thData.charFlag) return 0;
	else return thData.charValue;
};

EXTERNC
int getche() {
	int ch;
	ch = getch();
	putch(ch);
	return ch;
};

EXTERNC
void newFile() {
	textbackground(0);
	gotoxy(menux, menuy +11);
	cputs("Podaj wymiary: ");
	int ch=0, i = 0;
	thData.w = 0;
	putch('\n');
	while (ch != 13) {
		ch = getche();
		if (ch != 13)
		thData.w = thData.w*10 + (ch - 48);
		i++;
	}
	ch = 0, thData.h = 0, i = 0;
	putch('\n');
	while (ch != 13) {
		ch = getch();
		if (ch != 13)
		thData.h = thData.h*10 + (ch - 48);
		i++;		
		putch(ch);
	} 
	clrscr();
	thData.x = xdraw, thData.y = ydraw, thData.back = 0;
	settitle("Adrian Konkol 165181");
	fillEmptyTable();
	gotoxy(menux, menuy + 8);
	cputs("                                    ");
};

EXTERNC
void gotoxy(int x, int y) {
	COORD pos;
	pos.X = x - 1;
	pos.Y = y - 1;
	SetConsoleCursorPosition(thData.output, pos);
};

EXTERNC
int wherex() {
	int x;
	GetCP(&x, NULL);
	return x;
};


EXTERNC
int wherey() {
	int y;
	GetCP(NULL, &y);
	return y;
};


EXTERNC
int cputs(const char *str) {
	DWORD count;
	if (str == NULL) return EOF;
	UpdateWScroll();
	if (WriteConsoleA(thData.output, str, strlen(str), &count, NULL)) return count;
	else return EOF;
};

EXTERNC
int cgets(const char *str) {
	DWORD count;
	if (str == NULL) return EOF;
	UpdateWScroll();
	if (ReadConsole(thData.output, &str, strlen(str), &count, NULL)) return count;
	else return EOF;
};

EXTERNC
int putch(int c) {
	DWORD count;
	UpdateWScroll();
	if (WriteConsoleA(thData.output, &c, 1, &count, NULL)) return c;
	else return EOF;
};

EXTERNC
void settitle(const char *title) {
	SetConsoleTitleA(title);
	gotoxy(menux, menuy + 9);
	textbackground(0);
	cputs(title);

};

EXTERNC
void fillEmptyTable() {
	thData.sign = new int[thData.h*thData.w];
	for (int i = 0; i < thData.h*thData.w; i++)
		thData.sign[i] = 0;
};

//Nie dzia³a
//EXTERNC
//void drawSquare() {
//	int line = 0, xend = 0, yend = 0, xstart, ystart;
//		xstart = thData.x;
//		ystart = thData.y;
//		int ch = getch();
//		thData.draw = 1;
//		while (ch != 0x4b || ch != 0x6b) {
//			ch = getch();
//			if (ch==0) walk();
//			xend = thData.x;
//			yend = thData.y;
//		}
//		thData.draw = 0;
//		gotoxy(xstart, ystart);
//	/*for (int i = ystart; i <= yend; i++)
//	{
//		for (int j = xstart; j <= xend; j++)
//		{
//			if (j == xstart || j == xend || i == ystart || i == yend)
//				putch(' ');
//		}
//		line++;
//		gotoxy(xstart, ystart + line);
//	}*/
//};

EXTERNC
int chooseColor(int back) {
	if (thData.zn == 0x30 || thData.zn == 48) back = 0;
	else if (thData.zn == 0x31 || thData.zn == 49) back = 1;
	else if (thData.zn == 0x32 || thData.zn == 50) back = 2;
	else if (thData.zn == 0x33 || thData.zn == 51) back = 3;
	else if (thData.zn == 0x34 || thData.zn == 52) back = 4;
	else if (thData.zn == 0x35 || thData.zn == 53) back = 5;
	else if (thData.zn == 0x36 || thData.zn == 54) back = 6;
	else if (thData.zn == 0x37 || thData.zn == 55) back = 7;
	else if (thData.zn == 0x38 || thData.zn == 56) back = 8;
	else if (thData.zn == 0x39 || thData.zn == 57) back = 9;
	else if (thData.zn == 0x71 || thData.zn == 0x51 || thData.zn == 58) back = 10;
	else if (thData.zn == 0x77 || thData.zn == 0x57 || thData.zn == 59) back = 11;
	else if (thData.zn == 0x65 || thData.zn == 0x45 || thData.zn == 60) back = 12;
	else if (thData.zn == 0x72 || thData.zn == 0x52 || thData.zn == 61) back = 13;
	else if (thData.zn == 0x74 || thData.zn == 0x54 || thData.zn == 62) back = 14;
	else if (thData.zn == 0x79 || thData.zn == 0x59 || thData.zn == 63) back = 15;
	textbackground(back);
	return back;
};

EXTERNC
void showMyPic() {
	clrscr();
	FILE * myPic = fopen("obrazek.txt", "r");
	fscanf(myPic, "%d:%d\n", & thData.w, & thData.h);
	thData.sign = new int[thData.h*thData.w];
	for (int i = 0; i < thData.h; i++)
	{
		gotoxy(xdraw, ydraw + i);
		for (int j = 0; j < thData.w; j++)
		{
			thData.zn=fgetc(myPic);
			chooseColor(thData.back);
			putch(' ');
			thData.sign[j+i*thData.w] = thData.back;
		}
	}
	fclose(myPic);
	settitle("obrazek");
	thData.x = xdraw, thData.y = ydraw;
};

EXTERNC 
void getName() {
	textbackground(0);
	gotoxy(menux, menuy + 11);
	cputs("Podaj nazwe pliku: ");
	int zn = 0, i = 0;
	while (zn != 13) {
		zn = getche();
		if (zn == 13)
		{
			thData.name_of_file[i] = '.';
			thData.name_of_file[i + 1] = 't';
			thData.name_of_file[i + 2] = 'x';
			thData.name_of_file[i + 3] = 't';
			thData.name_of_file[i + 4] = '\0';
		}
		else
			thData.name_of_file[i] = zn;
		i++;

	}
	gotoxy(menux, menuy + 11);
	cputs("                                   ");
};

EXTERNC
void showYourPic() {
	getName();
	clrscr();
	FILE * myPic = fopen(thData.name_of_file, "r");
	if (!myPic) {
		settitle("Adrian Konkol 165181");
		gotoxy(menux, menuy + 8);
		cputs("Nie ma takiego pliku");
		thData.x = xdraw, thData.y = ydraw;
	}
	else {
		fscanf(myPic, "%d:%d\n", &thData.w, &thData.h);
		thData.sign = new int[thData.h*thData.w];
		for (int i = 0; i < thData.h; i++)
		{
			gotoxy(xdraw, ydraw + i);
			for (int j = 0; j < thData.w; j++)
			{
				thData.zn = fgetc(myPic);
				chooseColor(thData.back);
				putch(' ');
				thData.sign[j + i*thData.w] = thData.back;
			}
		}
		fclose(myPic);
		settitle(thData.name_of_file);
		thData.x = xdraw, thData.y = ydraw;
	}
}

EXTERNC
void saveToFile() {
	getName();
	FILE * saving = fopen(thData.name_of_file, "w");
	fprintf(saving, "%d:%d\n", thData.w, thData.h);
	for (int i = 0; i < thData.h*thData.w; i++)
	{
			fputc(thData.sign[i]+48, saving);
	}
	fclose(saving);
	settitle(thData.name_of_file);
};

EXTERNC
void showCoords() {
	textbackground(0);
	gotoxy(menux, menuy + 10);
	cputs("                                  ");
	gotoxy(menux, menuy + 10);
	_itoa(thData.x-xdraw+1, coordx, 10);
	_itoa(thData.y-ydraw+1, coordy, 10);
	cputs("x = ");
	cputs(coordx);
	cputs(" y = ");
	cputs(coordy);
	cputs(" ");
};

EXTERNC
int fill(int whatcolor, int xpos, int ypos, int towhatcolor) {
	if (xpos - xdraw >= thData.w || xpos - xdraw < 0 || ypos - ydraw >= thData.h || ypos - ydraw < 0) return 0;
	if (thData.sign[xpos - xdraw + (ypos - ydraw)*thData.w] != whatcolor) return 0;
	thData.sign[xpos - xdraw + (ypos - ydraw)*thData.w] = towhatcolor;
		gotoxy(xpos, ypos);
		chooseColor(towhatcolor);
		putch(' ');
		fill(whatcolor, xpos-1, ypos, towhatcolor);
		fill(whatcolor, xpos+1, ypos, towhatcolor);
		fill(whatcolor, xpos, ypos-1, towhatcolor);
		fill(whatcolor, xpos, ypos+1, towhatcolor);
};

EXTERNC
void walk() {
		thData.zn = getch();
		if (thData.zn == 0x48 && thData.y>ydraw) thData.y--;
		else if (thData.zn == 0x50 && thData.y<ydraw + thData.h - 1) thData.y++;
		else if (thData.zn == 0x4b && thData.x>xdraw) thData.x--;
		else if (thData.zn == 0x4d && thData.x<xdraw + thData.w - 1) thData.x++;
		/*else if (thData.zn == 0x73) cputs("ctrl dzia³a");	
		else if (thData.zn == 0x74) cputs("ctrl dzia³a");
		else if (thData.zn == 0x8d) cputs("ctrl dzia³a");
		else if (thData.zn == 0x91) cputs("ctrl dzia³a");*/
};

EXTERNC
void reactToSign() {
	do {
		showCoords();
		thData.back = chooseColor(thData.back);
		textbackground(thData.back);
		thData.sign[thData.x - xdraw + (thData.y - ydraw)*thData.w] = thData.back;
		gotoxy(thData.x, thData.y);
		putch(' ');
		thData.zn = getch();
		if (thData.zn == 0)
			walk();
		else if (thData.zn == 0x69 || thData.zn == 0x49)
			showMyPic();
		else if (thData.zn == 0x6e || thData.zn == 0x4e)
			newFile();
		//else if (thData.draw == 0 && (thData.zn == 0x6b || thData.zn == 0x4b))
			//drawSquare();
		else if (thData.zn == 0x6f || thData.zn == 0x4f)
			showYourPic();
		else if (thData.zn == 0x73 || thData.zn == 0x53)
			saveToFile();
		else if (thData.zn == 0x66 || thData.zn == 0x46) {
			thData.zn = getch();
			thData.newback = chooseColor(thData.newback);
			fill(thData.back, thData.x, thData.y, thData.newback);
		}
	} while (thData.zn != 0x1b);
};

EXTERNC
void loadPicEdit() {
	settitle("Adrian Konkol 165181");
	drawMenu();
	fillEmptyTable();
	reactToSign();
};


void gettextinfo(struct text_info *info) {
	info->curx = wherex();
	info->cury = wherey();
	info->attribute = thData.attrib;
	info->normattr = 0x07;
	info->screenwidth = thData.width;
	info->screenheight = thData.height;
};


void textmode(int mode) {
	int fs = mode & FULLSCREEN;
	mode = mode & (~FULLSCREEN);
	if (mode == C80) {
		thData.width = 80;
		thData.height = 25;
		ResizeConsole(thData.output, 80, 25, 25);
	}
	else if (mode == C4350) {
		thData.width = 80;
		thData.height = 50;
		ResizeConsole(thData.output, 80, 50, 50);
	}
	else if (mode == LASTMODE) {
		textmode(thData.lastmode);
	};
};


static void Conio2_Exit(void) {
	ExitConio2(&thData);
};

int Conio2_Init(void) {
	InitConio2(&thData);
	atexit(Conio2_Exit);
	return 0;
};

#ifdef __cplusplus
static int conio2_init = Conio2_Init();
#endif

