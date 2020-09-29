
#ifndef INPUT_H_
#define INPUT_H_

extern void keyrepeat(bool on, int mask = ~0);

enum
{
    TextInput_Console = 1<<0,
    TextInput_GUI     = 1<<1,
};

extern void textinput(bool on, int mask = ~0);

extern bool grabinput, minimized;

extern void checkinput();
extern void ignoremousemotion();
extern void inputgrab(bool on);
extern bool interceptkey(int sym);
extern void pushevent(const SDL_Event &e);
#endif
