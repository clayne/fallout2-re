#ifndef INTERPRETER_LIB_H
#define INTERPRETER_LIB_H

#include "interpreter.h"
#include "sound.h"

#define INTERPRETER_SOUNDS_LENGTH (32)
#define INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH (256)

typedef struct InterpreterKeyHandlerEntry {
    Program* program;
    int proc;
} InterpreterKeyHandlerEntry;

typedef void (*OFF_59E160)(Program*);

extern Sound* gInterpreterSounds[INTERPRETER_SOUNDS_LENGTH];
extern unsigned char stru_59D650[256 * 3];
extern InterpreterKeyHandlerEntry gInterpreterKeyHandlerEntries[INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH];
extern int dword_59E150;
extern int gIntepreterAnyKeyHandlerProc;
extern int _numCallbacks;
extern Program* gInterpreterAnyKeyHandlerProgram;
extern OFF_59E160* _callbacks;
extern int _sayStartingPosition;
extern char byte_59E168[100];
extern char byte_59E1CC[100];

void opFillWin3x3(Program* program);
void opFormat(Program* program);
void opPrint(Program* program);
void opSelectFileList(Program* program);
void opPrintRect(Program* program);
void opSelect(Program* program);
void opDisplay(Program* program);
void opDisplayRaw(Program* program);
void sub_46222C(unsigned char* a1, unsigned char* a2, int a3, float a4, int a5);
void opFadeIn(Program* program);
void opFadeOut(Program* program);
int _checkMovie(Program* program);
void opSetMovieFlags(Program* program);
void opPlayMovie(Program* program);
void opStopMovie(Program* program);
void opAddRegionProc(Program* program);
void opAddRegionRightProc(Program* program);
void opCreateWin(Program* program);
void opResizeWin(Program* program);
void opScaleWin(Program* program);
void opDeleteWin(Program* program);
void opSayStart(Program* program);
void opDeleteRegion(Program* program);
void opActivateRegion(Program* program);
void opCheckRegion(Program* program);
void opAddRegion(Program* program);
void opSayStartPos(Program* program);
void opSayReplyTitle(Program* program);
void opSayGoToReply(Program* program);
void opSayReply(Program* program);
void opSayOption(Program* program);
int _checkDialog(Program* program);
void opSayEnd(Program* program);
void opSayGetLastPos(Program* program);
void opSayQuit(Program* program);
void opSayMessageTimeout(Program* program);
void opSayMessage(Program* program);
void opGotoXY(Program* program);
void opAddButtonFlag(Program* program);
void opAddRegionFlag(Program* program);
void opAddButton(Program* program);
void opAddButtonText(Program* program);
void opAddButtonGfx(Program* program);
void opAddButtonProc(Program* program);
void opAddButtonRightProc(Program* program);
void opShowWin(Program* program);
void opDeleteButton(Program* program);
void opFillWin(Program* program);
void opFillRect(Program* program);
void opHideMouse(Program* program);
void opShowMouse(Program* program);
void opSetGlobalMouseFunc(Program* Program);
void opLoadPaletteTable(Program* program);
void opAddNamedEvent(Program* program);
void opAddNamedHandler(Program* program);
void opClearNamed(Program* program);
void opSignalNamed(Program* program);
void opAddKey(Program* program);
void opDeleteKey(Program* program);
void opSetFont(Program* program);
void opSetTextFlags(Program* program);
void opSetTextColor(Program* program);
void opSayOptionColor(Program* program);
void opSayReplyColor(Program* program);
void opSetHighlightColor(Program* program);
void opSayReplyWindow(Program* program);
void opSayReplyFlags(Program* program);
void opSayOptionFlags(Program* program);
void opSayBorder(Program* program);
void opSaySetSpacing(Program* program);
void opSayRestart(Program* program);
void interpreterSoundCallback(void* userData, int a2);
int interpreterSoundDelete(int a1);
int interpreterSoundPlay(char* fileName, int mode);
int interpreterSoundPause(int value);
int interpreterSoundRewind(int value);
int interpreterSoundResume(int value);
void opSoundPlay(Program* program);
void opSoundPause(Program* program);
void opSoundResume(Program* program);
void opSoundStop(Program* program);
void opSoundRewind(Program* program);
void opSoundDelete(Program* program);
void opSetOneOptPause(Program* program);
void _updateIntLib();
void _intlibClose();
bool _intLibDoInput(int key);
void _initIntlib();
void _interpretRegisterProgramDeleteCallback(OFF_59E160 fn);
void _removeProgramReferences_(Program* program);

#endif /* INTERPRETER_LIB_H */
