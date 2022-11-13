#include "character_editor.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "game/art.h"
#include "color.h"
#include "core.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "db.h"
#include "game/bmpdlog.h"
#include "debug.h"
#include "draw.h"
#include "game.h"
#include "game_mouse.h"
#include "game_palette.h"
#include "game_sound.h"
#include "interface.h"
#include "item.h"
#include "map.h"
#include "memory.h"
#include "object.h"
#include "palette.h"
#include "perk.h"
#include "proto.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_font.h"
#include "trait.h"
#include "window_manager.h"
#include "word_wrap.h"
#include "worldmap.h"

#define RENDER_ALL_STATS 7

#define EDITOR_WINDOW_X 0
#define EDITOR_WINDOW_Y 0
#define EDITOR_WINDOW_WIDTH 640
#define EDITOR_WINDOW_HEIGHT 480

#define NAME_BUTTON_X 9
#define NAME_BUTTON_Y 0

#define TAG_SKILLS_BUTTON_X 347
#define TAG_SKILLS_BUTTON_Y 26
#define TAG_SKILLS_BUTTON_CODE 536

#define PRINT_BTN_X 363
#define PRINT_BTN_Y 454

#define DONE_BTN_X 475
#define DONE_BTN_Y 454

#define CANCEL_BTN_X 571
#define CANCEL_BTN_Y 454

#define NAME_BTN_CODE 517
#define AGE_BTN_CODE 519
#define SEX_BTN_CODE 520

#define OPTIONAL_TRAITS_LEFT_BTN_X 23
#define OPTIONAL_TRAITS_RIGHT_BTN_X 298
#define OPTIONAL_TRAITS_BTN_Y 352

#define OPTIONAL_TRAITS_BTN_CODE 555

#define OPTIONAL_TRAITS_BTN_SPACE 2

#define SPECIAL_STATS_BTN_X 149

#define PERK_WINDOW_X 33
#define PERK_WINDOW_Y 91
#define PERK_WINDOW_WIDTH 573
#define PERK_WINDOW_HEIGHT 230

#define PERK_WINDOW_LIST_X 45
#define PERK_WINDOW_LIST_Y 43
#define PERK_WINDOW_LIST_WIDTH 192
#define PERK_WINDOW_LIST_HEIGHT 129

#define ANIMATE 0x01
#define RED_NUMBERS 0x02
#define BIG_NUM_WIDTH 14
#define BIG_NUM_HEIGHT 24
#define BIG_NUM_ANIMATION_DELAY 123

// 0x431C40
int gCharacterEditorFrmIds[EDITOR_GRAPHIC_COUNT] = {
    170,
    175,
    176,
    181,
    182,
    183,
    184,
    185,
    186,
    187,
    188,
    189,
    190,
    191,
    192,
    193,
    194,
    195,
    196,
    197,
    198,
    199,
    200,
    8,
    9,
    204,
    205,
    206,
    207,
    208,
    209,
    210,
    211,
    212,
    213,
    214,
    122,
    123,
    124,
    125,
    219,
    220,
    221,
    222,
    178,
    179,
    180,
    38,
    215,
    216,
};

// flags to preload fid
//
// 0x431D08
const unsigned char gCharacterEditorFrmShouldCopy[EDITOR_GRAPHIC_COUNT] = {
    0,
    0,
    1,
    0,
    0,
    0,
    1,
    1,
    0,
    0,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    0,
    0,
};

// graphic ids for derived stats panel
// NOTE: the type originally short
//
// 0x431D3A
const int gCharacterEditorDerivedStatFrmIds[EDITOR_DERIVED_STAT_COUNT] = {
    18,
    19,
    20,
    21,
    22,
    23,
    83,
    24,
    25,
    26,
};

// y offsets for stats +/- buttons
//
// 0x431D50
const int gCharacterEditorPrimaryStatY[7] = {
    37,
    70,
    103,
    136,
    169,
    202,
    235,
};

// stat ids for derived stats panel
// NOTE: the type is originally short
//
// 0x431D6
const int gCharacterEditorDerivedStatsMap[EDITOR_DERIVED_STAT_COUNT] = {
    STAT_ARMOR_CLASS,
    STAT_MAXIMUM_ACTION_POINTS,
    STAT_CARRY_WEIGHT,
    STAT_MELEE_DAMAGE,
    STAT_DAMAGE_RESISTANCE,
    STAT_POISON_RESISTANCE,
    STAT_RADIATION_RESISTANCE,
    STAT_SEQUENCE,
    STAT_HEALING_RATE,
    STAT_CRITICAL_CHANCE,
};

// 0x431D93
char byte_431D93[64];

// 0x431DD4
const int dword_431DD4[7] = {
    1000000,
    100000,
    10000,
    1000,
    100,
    10,
    1,
};

// 0x5016E4
char byte_5016E4[] = "------";

// 0x50170B
const double dbl_50170B = 14.4;

// 0x501713
const double dbl_501713 = 19.2;

// 0x5018F0
const double dbl_5018F0 = 19.2;

// 0x5019BE
const double dbl_5019BE = 14.4;

// 0x518528
bool gCharacterEditorIsoWasEnabled = false;

// 0x51852C
int gCharacterEditorCurrentSkill = 0;

// 0x518534
int gCharacterEditorSkillValueAdjustmentSliderY = 27;

// 0x518538
int gCharacterEditorRemainingCharacterPoints = 0;

// 0x51853C
KarmaEntry* gKarmaEntries = NULL;

// 0x518540
int gKarmaEntriesLength = 0;

// 0x518544
GenericReputationEntry* gGenericReputationEntries = NULL;

// 0x518548
int gGenericReputationEntriesLength = 0;

// 0x51854C
const TownReputationEntry gTownReputationEntries[TOWN_REPUTATION_COUNT] = {
    { GVAR_TOWN_REP_ARROYO, CITY_ARROYO },
    { GVAR_TOWN_REP_KLAMATH, CITY_KLAMATH },
    { GVAR_TOWN_REP_THE_DEN, CITY_DEN },
    { GVAR_TOWN_REP_VAULT_CITY, CITY_VAULT_CITY },
    { GVAR_TOWN_REP_GECKO, CITY_GECKO },
    { GVAR_TOWN_REP_MODOC, CITY_MODOC },
    { GVAR_TOWN_REP_SIERRA_BASE, CITY_SIERRA_ARMY_BASE },
    { GVAR_TOWN_REP_BROKEN_HILLS, CITY_BROKEN_HILLS },
    { GVAR_TOWN_REP_NEW_RENO, CITY_NEW_RENO },
    { GVAR_TOWN_REP_REDDING, CITY_REDDING },
    { GVAR_TOWN_REP_NCR, CITY_NEW_CALIFORNIA_REPUBLIC },
    { GVAR_TOWN_REP_VAULT_13, CITY_VAULT_13 },
    { GVAR_TOWN_REP_SAN_FRANCISCO, CITY_SAN_FRANCISCO },
    { GVAR_TOWN_REP_ABBEY, CITY_ABBEY },
    { GVAR_TOWN_REP_EPA, CITY_ENV_PROTECTION_AGENCY },
    { GVAR_TOWN_REP_PRIMITIVE_TRIBE, CITY_PRIMITIVE_TRIBE },
    { GVAR_TOWN_REP_RAIDERS, CITY_RAIDERS },
    { GVAR_TOWN_REP_VAULT_15, CITY_VAULT_15 },
    { GVAR_TOWN_REP_GHOST_FARM, CITY_MODOC_GHOST_TOWN },
};

// 0x5185E4
const int gAddictionReputationVars[ADDICTION_REPUTATION_COUNT] = {
    GVAR_NUKA_COLA_ADDICT,
    GVAR_BUFF_OUT_ADDICT,
    GVAR_MENTATS_ADDICT,
    GVAR_PSYCHO_ADDICT,
    GVAR_RADAWAY_ADDICT,
    GVAR_ALCOHOL_ADDICT,
    GVAR_ADDICT_JET,
    GVAR_ADDICT_TRAGIC,
};

// 0x518604
const int gAddictionReputationFrmIds[ADDICTION_REPUTATION_COUNT] = {
    142,
    126,
    140,
    144,
    145,
    52,
    136,
    149,
};

// 0x518624
int gCharacterEditorFolderViewScrollUpBtn = -1;

// 0x518628
int gCharacterEditorFolderViewScrollDownBtn = -1;

// 0x56FB60
char gCharacterEditorFolderCardString[256];

// 0x56FC60
int gCharacterEditorSkillsBackup[SKILL_COUNT];

// 0x56FCA8
MessageList gCharacterEditorMessageList;

// 0x56FCB0
PerkDialogOption gPerkDialogOptionList[DIALOG_PICKER_NUM_OPTIONS];

// buttons for selecting traits
//
// 0x5700A8
int gCharacterEditorOptionalTraitBtns[TRAIT_COUNT];

// 0x5700E8
MessageListItem gCharacterEditorMessageListItem;

// 0x5700F8
char gCharacterEditorCardTitle[48];

// 0x570128
char gPerkDialogCardTitle[48];

// buttons for tagging skills
//
// 0x570158
int gCharacterEditorTagSkillBtns[SKILL_COUNT];

// pc name
//
// 0x5701A0
char gCharacterEditorNameBackup[32];

// 0x5701C0
Size gCharacterEditorFrmSize[EDITOR_GRAPHIC_COUNT];

// 0x570350
CacheEntry* gCharacterEditorFrmHandle[EDITOR_GRAPHIC_COUNT];

// 0x570418
unsigned char* gCharacterEditorFrmCopy[EDITOR_GRAPHIC_COUNT];

// 0x5704E0
unsigned char* gCharacterEditorFrmData[EDITOR_GRAPHIC_COUNT];

// 0x5705A8
int gCharacterEditorFolderViewMaxLines;

// 0x5705AC
int gCharacterEditorFolderViewCurrentLine;

// 0x5705B0
int gCharacterEditorFolderCardFrmId;

// 0x5705B4
int gCharacterEditorFolderViewTopLine;

// 0x5705B8
char* gCharacterEditorFolderCardTitle;

// 0x5705BC
char* gCharacterEditorFolderCardSubtitle;

// 0x5705C0
int gCharacterEditorFolderViewOffsetY;

// 0x5705C4
int gCharacterEditorKarmaFolderTopLine;

// 0x5705C8
int gCharacterEditorFolderViewHighlightedLine;

// 0x5705CC
char* gCharacterEditorFolderCardDescription;

// 0x5705D0
int gCharacterEditorFolderViewNextY;

// 0x5705D4
int gCharacterEditorKillsFolderTopLine;

// 0x5705D8
int gCharacterEditorPerkFolderTopLine;

// 0x5705DC
unsigned char* gPerkDialogBackgroundBuffer;

// 0x5705E0
int gPerkDialogWindow;

// 0x5705E4
int gCharacterEditorSliderPlusBtn;

// 0x5705E8
int gCharacterEditorSliderMinusBtn;

// - stats buttons
//
// 0x5705EC
int gCharacterEditorPrimaryStatMinusBtns[7];

// 0x570608
unsigned char* gCharacterEditorWindowBuffer;

// 0x57060C
int gCharacterEditorWindow;

// + stats buttons
//
// 0x570610
int gCharacterEditorPrimaryStatPlusBtns[7];

// 0x57062C
unsigned char* gPerkDialogWindowBuffer;

// 0x570630
CritterProtoData gCharacterEditorDudeDataBackup;

// 0x5707A4
unsigned char* gCharacterEditorWindowBackgroundBuffer;

// 0x5707A8
int gPerkDialogCurrentLine;

// 0x5707AC
int gPerkDialogPreviousCurrentLine;

// unspent skill points
//
// 0x5707B0
int gCharacterEditorUnspentSkillPointsBackup;

// 0x5707B4
int gCharacterEditorLastLevel;

// 0x5707B8
int gCharacterEditorOldFont;

// 0x5707BC
int gCharacterEditorKillsCount;

// character editor background
//
// 0x5707C0
CacheEntry* gCharacterEditorWindowBackgroundHandle;

// current hit points
//
// 0x5707C4
int gCharacterEditorHitPointsBackup;

// 0x5707C8
int gCharacterEditorMouseY; // mouse y

// 0x5707CC
int gCharacterEditorMouseX; // mouse x

// 0x5707D0
int characterEditorSelectedItem;

// 0x5707D4
int characterEditorWindowSelectedFolder;

// 0x5707D8
bool gCharacterEditorCardDrawn;

// 0x5707DC
int gPerkDialogTopLine;

// 0x5707E0
bool gPerkDialogCardDrawn;

// 0x5707E4
int gCharacterEditorPerksBackup[PERK_COUNT];

// 0x5709C0
unsigned int _repFtime;

// 0x5709C4
unsigned int _frame_time;

// 0x5709C8
int gCharacterEditorOldTaggedSkillCount;

// 0x5709CC
int gCharacterEditorLastLevelBackup;

// 0x5709E8
int gPerkDialogCardFrmId;

// 0x5709EC
int gCharacterEditorCardFrmId;

// 0x5709D0
bool gCharacterEditorIsCreationMode;

// 0x5709D4
int gCharacterEditorTaggedSkillsBackup[NUM_TAGGED_SKILLS];

// 0x5709F0
int gCharacterEditorOptionalTraitsBackup[3];

// current index for selecting new trait
//
// 0x5709FC
int gCharacterEditorTempTraitCount;

// 0x570A00
int gPerkDialogOptionCount;

// 0x570A04
int gCharacterEditorTempTraits[3];

// 0x570A10
int gCharacterEditorTaggedSkillCount;

// 0x570A14
int gCharacterEditorTempTaggedSkills[NUM_TAGGED_SKILLS];

// 0x570A28
char gCharacterEditorHasFreePerkBackup;

// 0x570A29
unsigned char gCharacterEditorHasFreePerk;

// 0x570A2A
unsigned char gCharacterEditorIsSkillsFirstDraw;

// 0x431DF8
int characterEditorShow(bool isCreationMode)
{
    char* messageListItemText;
    char line1[128];
    char line2[128];
    const char* lines[] = { line2 };

    gCharacterEditorIsCreationMode = isCreationMode;

    characterEditorSavePlayer();

    if (characterEditorWindowInit() == -1) {
        debugPrint("\n ** Error loading character editor data! **\n");
        return -1;
    }

    if (!gCharacterEditorIsCreationMode) {
        if (characterEditorUpdateLevel()) {
            critterUpdateDerivedStats(gDude);
            characterEditorDrawOptionalTraits();
            characterEditorDrawSkills(0);
            characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
            characterEditorDrawDerivedStats();
            characterEditorDrawCard();
        }
    }

    int rc = -1;
    while (rc == -1) {
        _frame_time = _get_time();
        int keyCode = _get_input();

        bool done = false;
        if (keyCode == 500) {
            done = true;
        }

        if (keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
            done = true;
            soundPlayFile("ib1p1xx1");
        }

        if (done) {
            if (gCharacterEditorIsCreationMode) {
                if (gCharacterEditorRemainingCharacterPoints != 0) {
                    soundPlayFile("iisxxxx1");

                    // You must use all character points
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 118);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 119);
                    strcpy(line2, messageListItemText);

                    dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
                    win_draw(gCharacterEditorWindow);

                    rc = -1;
                    continue;
                }

                if (gCharacterEditorTaggedSkillCount > 0) {
                    soundPlayFile("iisxxxx1");

                    // You must select all tag skills
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 142);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 143);
                    strcpy(line2, messageListItemText);

                    dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
                    win_draw(gCharacterEditorWindow);

                    rc = -1;
                    continue;
                }

                if (_is_supper_bonus()) {
                    soundPlayFile("iisxxxx1");

                    // All stats must be between 1 and 10
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 157);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 158);
                    strcpy(line2, messageListItemText);

                    dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
                    win_draw(gCharacterEditorWindow);

                    rc = -1;
                    continue;
                }

                if (stricmp(critter_name(gDude), "None") == 0) {
                    soundPlayFile("iisxxxx1");

                    // Warning: You haven't changed your player
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 160);
                    strcpy(line1, messageListItemText);

                    // name. Use this character any way?
                    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 161);
                    strcpy(line2, messageListItemText);

                    if (dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_YES_NO) == 0) {
                        win_draw(gCharacterEditorWindow);

                        rc = -1;
                        continue;
                    }
                }
            }

            win_draw(gCharacterEditorWindow);
            rc = 0;
        } else if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            showQuitConfirmationDialog();
            win_draw(gCharacterEditorWindow);
        } else if (keyCode == 502 || keyCode == KEY_ESCAPE || keyCode == KEY_UPPERCASE_C || keyCode == KEY_LOWERCASE_C || _game_user_wants_to_quit != 0) {
            win_draw(gCharacterEditorWindow);
            rc = 1;
        } else if (gCharacterEditorIsCreationMode && (keyCode == 517 || keyCode == KEY_UPPERCASE_N || keyCode == KEY_LOWERCASE_N)) {
            characterEditorEditName();
            win_draw(gCharacterEditorWindow);
        } else if (gCharacterEditorIsCreationMode && (keyCode == 519 || keyCode == KEY_UPPERCASE_A || keyCode == KEY_LOWERCASE_A)) {
            characterEditorEditAge();
            win_draw(gCharacterEditorWindow);
        } else if (gCharacterEditorIsCreationMode && (keyCode == 520 || keyCode == KEY_UPPERCASE_S || keyCode == KEY_LOWERCASE_S)) {
            characterEditorEditGender();
            win_draw(gCharacterEditorWindow);
        } else if (gCharacterEditorIsCreationMode && (keyCode >= 503 && keyCode < 517)) {
            characterEditorAdjustPrimaryStat(keyCode);
            win_draw(gCharacterEditorWindow);
        } else if ((gCharacterEditorIsCreationMode && (keyCode == 501 || keyCode == KEY_UPPERCASE_O || keyCode == KEY_LOWERCASE_O))
            || (!gCharacterEditorIsCreationMode && (keyCode == 501 || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P))) {
            characterEditorShowOptions();
            win_draw(gCharacterEditorWindow);
        } else if (keyCode >= 525 && keyCode < 535) {
            characterEditorHandleInfoButtonPressed(keyCode);
            win_draw(gCharacterEditorWindow);
        } else {
            switch (keyCode) {
            case KEY_TAB:
                if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
                    characterEditorSelectedItem = gCharacterEditorIsCreationMode ? 82 : 7;
                } else if (characterEditorSelectedItem >= 7 && characterEditorSelectedItem < 9) {
                    if (gCharacterEditorIsCreationMode) {
                        characterEditorSelectedItem = 82;
                    } else {
                        characterEditorSelectedItem = 10;
                        characterEditorWindowSelectedFolder = 0;
                    }
                } else if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
                    switch (characterEditorWindowSelectedFolder) {
                    case EDITOR_FOLDER_PERKS:
                        characterEditorSelectedItem = 10;
                        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KARMA;
                        break;
                    case EDITOR_FOLDER_KARMA:
                        characterEditorSelectedItem = 10;
                        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KILLS;
                        break;
                    case EDITOR_FOLDER_KILLS:
                        characterEditorSelectedItem = 43;
                        break;
                    }
                } else if (characterEditorSelectedItem >= 43 && characterEditorSelectedItem < 51) {
                    characterEditorSelectedItem = 51;
                } else if (characterEditorSelectedItem >= 51 && characterEditorSelectedItem < 61) {
                    characterEditorSelectedItem = 61;
                } else if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 82) {
                    characterEditorSelectedItem = 0;
                } else if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
                    characterEditorSelectedItem = 43;
                }
                characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
                characterEditorDrawOptionalTraits();
                characterEditorDrawSkills(0);
                characterEditorDrawPcStats();
                characterEditorDrawFolders();
                characterEditorDrawDerivedStats();
                characterEditorDrawCard();
                win_draw(gCharacterEditorWindow);
                break;
            case KEY_ARROW_LEFT:
            case KEY_MINUS:
            case KEY_UPPERCASE_J:
                if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(gCharacterEditorPrimaryStatMinusBtns[characterEditorSelectedItem]);
                        win_draw(gCharacterEditorWindow);
                    }
                } else if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(gCharacterEditorTagSkillBtns[gCharacterEditorIsCreationMode - 61]);
                        win_draw(gCharacterEditorWindow);
                    } else {
                        characterEditorHandleAdjustSkillButtonPressed(keyCode);
                        win_draw(gCharacterEditorWindow);
                    }
                } else if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(gCharacterEditorOptionalTraitBtns[gCharacterEditorIsCreationMode - 82]);
                        win_draw(gCharacterEditorWindow);
                    }
                }
                break;
            case KEY_ARROW_RIGHT:
            case KEY_PLUS:
            case KEY_UPPERCASE_N:
                if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(gCharacterEditorPrimaryStatPlusBtns[characterEditorSelectedItem]);
                        win_draw(gCharacterEditorWindow);
                    }
                } else if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(gCharacterEditorTagSkillBtns[gCharacterEditorIsCreationMode - 61]);
                        win_draw(gCharacterEditorWindow);
                    } else {
                        characterEditorHandleAdjustSkillButtonPressed(keyCode);
                        win_draw(gCharacterEditorWindow);
                    }
                } else if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(gCharacterEditorOptionalTraitBtns[gCharacterEditorIsCreationMode - 82]);
                        win_draw(gCharacterEditorWindow);
                    }
                }
                break;
            case KEY_ARROW_UP:
                if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
                    if (characterEditorSelectedItem == 10) {
                        if (gCharacterEditorFolderViewTopLine > 0) {
                            characterEditorFolderViewScroll(-1);
                            characterEditorSelectedItem--;
                            characterEditorDrawFolders();
                            characterEditorDrawCard();
                        }
                    } else {
                        characterEditorSelectedItem--;
                        characterEditorDrawFolders();
                        characterEditorDrawCard();
                    }

                    win_draw(gCharacterEditorWindow);
                } else {
                    switch (characterEditorSelectedItem) {
                    case 0:
                        characterEditorSelectedItem = 6;
                        break;
                    case 7:
                        characterEditorSelectedItem = 9;
                        break;
                    case 43:
                        characterEditorSelectedItem = 50;
                        break;
                    case 51:
                        characterEditorSelectedItem = 60;
                        break;
                    case 61:
                        characterEditorSelectedItem = 78;
                        break;
                    case 82:
                        characterEditorSelectedItem = 97;
                        break;
                    default:
                        characterEditorSelectedItem -= 1;
                        break;
                    }

                    if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                        gCharacterEditorCurrentSkill = characterEditorSelectedItem - 61;
                    }

                    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
                    characterEditorDrawOptionalTraits();
                    characterEditorDrawSkills(0);
                    characterEditorDrawPcStats();
                    characterEditorDrawFolders();
                    characterEditorDrawDerivedStats();
                    characterEditorDrawCard();
                    win_draw(gCharacterEditorWindow);
                }
                break;
            case KEY_ARROW_DOWN:
                if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
                    if (characterEditorSelectedItem - 10 < gCharacterEditorFolderViewCurrentLine - gCharacterEditorFolderViewTopLine) {
                        if (characterEditorSelectedItem - 10 == gCharacterEditorFolderViewMaxLines - 1) {
                            characterEditorFolderViewScroll(1);
                        }

                        characterEditorSelectedItem++;

                        characterEditorDrawFolders();
                        characterEditorDrawCard();
                    }

                    win_draw(gCharacterEditorWindow);
                } else {
                    switch (characterEditorSelectedItem) {
                    case 6:
                        characterEditorSelectedItem = 0;
                        break;
                    case 9:
                        characterEditorSelectedItem = 7;
                        break;
                    case 50:
                        characterEditorSelectedItem = 43;
                        break;
                    case 60:
                        characterEditorSelectedItem = 51;
                        break;
                    case 78:
                        characterEditorSelectedItem = 61;
                        break;
                    case 97:
                        characterEditorSelectedItem = 82;
                        break;
                    default:
                        characterEditorSelectedItem += 1;
                        break;
                    }

                    if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                        gCharacterEditorCurrentSkill = characterEditorSelectedItem - 61;
                    }

                    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
                    characterEditorDrawOptionalTraits();
                    characterEditorDrawSkills(0);
                    characterEditorDrawPcStats();
                    characterEditorDrawFolders();
                    characterEditorDrawDerivedStats();
                    characterEditorDrawCard();
                    win_draw(gCharacterEditorWindow);
                }
                break;
            case 521:
            case 523:
                characterEditorHandleAdjustSkillButtonPressed(keyCode);
                win_draw(gCharacterEditorWindow);
                break;
            case 535:
                characterEditorHandleFolderButtonPressed();
                win_draw(gCharacterEditorWindow);
                break;
            case 17000:
                characterEditorFolderViewScroll(-1);
                win_draw(gCharacterEditorWindow);
                break;
            case 17001:
                characterEditorFolderViewScroll(1);
                win_draw(gCharacterEditorWindow);
                break;
            default:
                if (gCharacterEditorIsCreationMode && (keyCode >= 536 && keyCode < 554)) {
                    characterEditorToggleTaggedSkill(keyCode - 536);
                    win_draw(gCharacterEditorWindow);
                } else if (gCharacterEditorIsCreationMode && (keyCode >= 555 && keyCode < 571)) {
                    characterEditorToggleOptionalTrait(keyCode - 555);
                    win_draw(gCharacterEditorWindow);
                } else {
                    if (keyCode == 390) {
                        takeScreenshot();
                    }

                    win_draw(gCharacterEditorWindow);
                }
            }
        }
    }

    if (rc == 0) {
        if (isCreationMode) {
            _proto_dude_update_gender();
            paletteFadeTo(gPaletteBlack);
        }
    }

    characterEditorWindowFree();

    if (rc == 1) {
        characterEditorRestorePlayer();
    }

    if (is_pc_flag(DUDE_STATE_LEVEL_UP_AVAILABLE)) {
        pc_flag_off(DUDE_STATE_LEVEL_UP_AVAILABLE);
    }

    interfaceRenderHitPoints(false);

    return rc;
}

// 0x4329EC
int characterEditorWindowInit()
{
    int i;
    char path[MAX_PATH];
    int fid;
    char* str;
    int len;
    int btn;
    int x;
    int y;
    char perks[32];
    char karma[32];
    char kills[32];

    gCharacterEditorOldFont = fontGetCurrent();
    gCharacterEditorOldTaggedSkillCount = 0;
    gCharacterEditorIsoWasEnabled = 0;
    gPerkDialogCardFrmId = -1;
    gCharacterEditorCardFrmId = -1;
    gPerkDialogCardDrawn = false;
    gCharacterEditorCardDrawn = false;
    gCharacterEditorIsSkillsFirstDraw = 1;
    gPerkDialogCardTitle[0] = '\0';
    gCharacterEditorCardTitle[0] = '\0';

    fontSetCurrent(101);

    gCharacterEditorSkillValueAdjustmentSliderY = gCharacterEditorCurrentSkill * (fontGetLineHeight() + 1) + 27;

    // skills
    skillsGetTagged(gCharacterEditorTempTaggedSkills, NUM_TAGGED_SKILLS);

    // NOTE: Uninline.
    gCharacterEditorTaggedSkillCount = tagskl_free();

    // traits
    traitsGetSelected(&(gCharacterEditorTempTraits[0]), &(gCharacterEditorTempTraits[1]));

    // NOTE: Uninline.
    gCharacterEditorTempTraitCount = get_trait_count();

    if (!gCharacterEditorIsCreationMode) {
        gCharacterEditorIsoWasEnabled = isoDisable();
    }

    cycle_disable();
    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    if (!messageListInit(&gCharacterEditorMessageList)) {
        return -1;
    }

    sprintf(path, "%s%s", asc_5186C8, "editor.msg");

    if (!messageListLoad(&gCharacterEditorMessageList, path)) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, (gCharacterEditorIsCreationMode ? 169 : 177), 0, 0, 0);
    gCharacterEditorWindowBackgroundBuffer = art_lock(fid, &gCharacterEditorWindowBackgroundHandle, &(gCharacterEditorFrmSize[0].width), &(gCharacterEditorFrmSize[0].height));
    if (gCharacterEditorWindowBackgroundBuffer == NULL) {
        messageListFree(&gCharacterEditorMessageList);
        return -1;
    }

    if (karmaInit() == -1) {
        return -1;
    }

    if (genericReputationInit() == -1) {
        return -1;
    }

    soundContinueAll();

    for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
        fid = art_id(OBJ_TYPE_INTERFACE, gCharacterEditorFrmIds[i], 0, 0, 0);
        gCharacterEditorFrmData[i] = art_lock(fid, &(gCharacterEditorFrmHandle[i]), &(gCharacterEditorFrmSize[i].width), &(gCharacterEditorFrmSize[i].height));
        if (gCharacterEditorFrmData[i] == NULL) {
            break;
        }
    }

    if (i != EDITOR_GRAPHIC_COUNT) {
        while (--i >= 0) {
            art_ptr_unlock(gCharacterEditorFrmHandle[i]);
        }
        return -1;

        art_ptr_unlock(gCharacterEditorWindowBackgroundHandle);

        messageListFree(&gCharacterEditorMessageList);

        // NOTE: Uninline.
        RstrBckgProc();

        return -1;
    }

    soundContinueAll();

    for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
        if (gCharacterEditorFrmShouldCopy[i]) {
            gCharacterEditorFrmCopy[i] = (unsigned char*)internal_malloc(gCharacterEditorFrmSize[i].width * gCharacterEditorFrmSize[i].height);
            if (gCharacterEditorFrmCopy[i] == NULL) {
                break;
            }
            memcpy(gCharacterEditorFrmCopy[i], gCharacterEditorFrmData[i], gCharacterEditorFrmSize[i].width * gCharacterEditorFrmSize[i].height);
        } else {
            gCharacterEditorFrmCopy[i] = (unsigned char*)-1;
        }
    }

    if (i != EDITOR_GRAPHIC_COUNT) {
        while (--i >= 0) {
            if (gCharacterEditorFrmShouldCopy[i]) {
                internal_free(gCharacterEditorFrmCopy[i]);
            }
        }

        for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
            art_ptr_unlock(gCharacterEditorFrmHandle[i]);
        }

        art_ptr_unlock(gCharacterEditorWindowBackgroundHandle);

        messageListFree(&gCharacterEditorMessageList);

        // NOTE: Uninline.
        RstrBckgProc();

        return -1;
    }

    int editorWindowX = EDITOR_WINDOW_X;
    int editorWindowY = EDITOR_WINDOW_Y;
    gCharacterEditorWindow = windowCreate(editorWindowX,
        editorWindowY,
        EDITOR_WINDOW_WIDTH,
        EDITOR_WINDOW_HEIGHT,
        256,
        WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (gCharacterEditorWindow == -1) {
        for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
            if (gCharacterEditorFrmShouldCopy[i]) {
                internal_free(gCharacterEditorFrmCopy[i]);
            }
            art_ptr_unlock(gCharacterEditorFrmHandle[i]);
        }

        art_ptr_unlock(gCharacterEditorWindowBackgroundHandle);

        messageListFree(&gCharacterEditorMessageList);

        // NOTE: Uninline.
        RstrBckgProc();

        return -1;
    }

    gCharacterEditorWindowBuffer = windowGetBuffer(gCharacterEditorWindow);
    memcpy(gCharacterEditorWindowBuffer, gCharacterEditorWindowBackgroundBuffer, 640 * 480);

    if (gCharacterEditorIsCreationMode) {
        fontSetCurrent(103);

        // CHAR POINTS
        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 116);
        fontDrawText(gCharacterEditorWindowBuffer + (286 * 640) + 14, str, 640, 640, colorTable[18979]);
        characterEditorDrawBigNumber(126, 282, 0, gCharacterEditorRemainingCharacterPoints, 0, gCharacterEditorWindow);

        // OPTIONS
        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 101);
        fontDrawText(gCharacterEditorWindowBuffer + (454 * 640) + 363, str, 640, 640, colorTable[18979]);

        // OPTIONAL TRAITS
        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 139);
        fontDrawText(gCharacterEditorWindowBuffer + (326 * 640) + 52, str, 640, 640, colorTable[18979]);
        characterEditorDrawBigNumber(522, 228, 0, gPerkDialogOptionCount, 0, gCharacterEditorWindow);

        // TAG SKILLS
        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 138);
        fontDrawText(gCharacterEditorWindowBuffer + (233 * 640) + 422, str, 640, 640, colorTable[18979]);
        characterEditorDrawBigNumber(522, 228, 0, gCharacterEditorTaggedSkillCount, 0, gCharacterEditorWindow);
    } else {
        fontSetCurrent(103);

        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 109);
        strcpy(perks, str);

        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 110);
        strcpy(karma, str);

        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 111);
        strcpy(kills, str);

        // perks selected
        len = fontGetStringWidth(perks);
        fontDrawText(
            gCharacterEditorFrmCopy[46] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[18979]);

        len = fontGetStringWidth(karma);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[14723]);

        len = fontGetStringWidth(kills);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[14723]);

        // karma selected
        len = fontGetStringWidth(perks);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[14723]);

        len = fontGetStringWidth(karma);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[18979]);

        len = fontGetStringWidth(kills);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            gCharacterEditorFrmSize[46].width,
            gCharacterEditorFrmSize[46].width,
            colorTable[14723]);

        // kills selected
        len = fontGetStringWidth(perks);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            gCharacterEditorFrmSize[46].width,
            gCharacterEditorFrmSize[46].width,
            colorTable[14723]);

        len = fontGetStringWidth(karma);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            gCharacterEditorFrmSize[46].width,
            gCharacterEditorFrmSize[46].width,
            colorTable[14723]);

        len = fontGetStringWidth(kills);
        fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[18979]);

        characterEditorDrawFolders();

        fontSetCurrent(103);

        // PRINT
        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 103);
        fontDrawText(gCharacterEditorWindowBuffer + (EDITOR_WINDOW_WIDTH * PRINT_BTN_Y) + PRINT_BTN_X, str, EDITOR_WINDOW_WIDTH, EDITOR_WINDOW_WIDTH, colorTable[18979]);

        characterEditorDrawPcStats();
        characterEditorFolderViewInit();
    }

    fontSetCurrent(103);

    // CANCEL
    str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 102);
    fontDrawText(gCharacterEditorWindowBuffer + (EDITOR_WINDOW_WIDTH * CANCEL_BTN_Y) + CANCEL_BTN_X, str, EDITOR_WINDOW_WIDTH, EDITOR_WINDOW_WIDTH, colorTable[18979]);

    // DONE
    str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 100);
    fontDrawText(gCharacterEditorWindowBuffer + (EDITOR_WINDOW_WIDTH * DONE_BTN_Y) + DONE_BTN_X, str, EDITOR_WINDOW_WIDTH, EDITOR_WINDOW_WIDTH, colorTable[18979]);

    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
    characterEditorDrawDerivedStats();

    if (!gCharacterEditorIsCreationMode) {
        gCharacterEditorSliderPlusBtn = buttonCreate(
            gCharacterEditorWindow,
            614,
            20,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
            -1,
            522,
            521,
            522,
            gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
            gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
            0,
            96);
        gCharacterEditorSliderMinusBtn = buttonCreate(
            gCharacterEditorWindow,
            614,
            20 + gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height - 1,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_OFF].height,
            -1,
            524,
            523,
            524,
            gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
            gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
            0,
            96);
        buttonSetCallbacks(gCharacterEditorSliderPlusBtn, _gsound_red_butt_press, NULL);
        buttonSetCallbacks(gCharacterEditorSliderMinusBtn, _gsound_red_butt_press, NULL);
    }

    characterEditorDrawSkills(0);
    characterEditorDrawCard();
    soundContinueAll();
    characterEditorDrawName();
    characterEditorDrawAge();
    characterEditorDrawGender();

    if (gCharacterEditorIsCreationMode) {
        x = NAME_BUTTON_X;
        btn = buttonCreate(
            gCharacterEditorWindow,
            x,
            NAME_BUTTON_Y,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].height,
            -1,
            -1,
            -1,
            NAME_BTN_CODE,
            gCharacterEditorFrmCopy[EDITOR_GRAPHIC_NAME_OFF],
            gCharacterEditorFrmCopy[EDITOR_GRAPHIC_NAME_ON],
            0,
            32);
        if (btn != -1) {
            buttonSetMask(btn, gCharacterEditorFrmData[EDITOR_GRAPHIC_NAME_MASK]);
            buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
        }

        x += gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width;
        btn = buttonCreate(
            gCharacterEditorWindow,
            x,
            NAME_BUTTON_Y,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].height,
            -1,
            -1,
            -1,
            AGE_BTN_CODE,
            gCharacterEditorFrmCopy[EDITOR_GRAPHIC_AGE_OFF],
            gCharacterEditorFrmCopy[EDITOR_GRAPHIC_AGE_ON],
            0,
            32);
        if (btn != -1) {
            buttonSetMask(btn, gCharacterEditorFrmData[EDITOR_GRAPHIC_AGE_MASK]);
            buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
        }

        x += gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].width;
        btn = buttonCreate(
            gCharacterEditorWindow,
            x,
            NAME_BUTTON_Y,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_ON].height,
            -1,
            -1,
            -1,
            SEX_BTN_CODE,
            gCharacterEditorFrmCopy[EDITOR_GRAPHIC_SEX_OFF],
            gCharacterEditorFrmCopy[EDITOR_GRAPHIC_SEX_ON],
            0,
            32);
        if (btn != -1) {
            buttonSetMask(btn, gCharacterEditorFrmData[EDITOR_GRAPHIC_SEX_MASK]);
            buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
        }

        y = TAG_SKILLS_BUTTON_Y;
        for (i = 0; i < SKILL_COUNT; i++) {
            gCharacterEditorTagSkillBtns[i] = buttonCreate(
                gCharacterEditorWindow,
                TAG_SKILLS_BUTTON_X,
                y,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                TAG_SKILLS_BUTTON_CODE + i,
                gCharacterEditorFrmData[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                gCharacterEditorFrmData[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height;
        }

        y = OPTIONAL_TRAITS_BTN_Y;
        for (i = 0; i < TRAIT_COUNT / 2; i++) {
            gCharacterEditorOptionalTraitBtns[i] = buttonCreate(
                gCharacterEditorWindow,
                OPTIONAL_TRAITS_LEFT_BTN_X,
                y,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                OPTIONAL_TRAITS_BTN_CODE + i,
                gCharacterEditorFrmData[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                gCharacterEditorFrmData[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height + OPTIONAL_TRAITS_BTN_SPACE;
        }

        y = OPTIONAL_TRAITS_BTN_Y;
        for (i = TRAIT_COUNT / 2; i < TRAIT_COUNT; i++) {
            gCharacterEditorOptionalTraitBtns[i] = buttonCreate(
                gCharacterEditorWindow,
                OPTIONAL_TRAITS_RIGHT_BTN_X,
                y,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                OPTIONAL_TRAITS_BTN_CODE + i,
                gCharacterEditorFrmData[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                gCharacterEditorFrmData[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += gCharacterEditorFrmSize[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height + OPTIONAL_TRAITS_BTN_SPACE;
        }

        characterEditorDrawOptionalTraits();
    } else {
        x = NAME_BUTTON_X;
        blitBufferToBufferTrans(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_NAME_OFF],
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].height,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width,
            gCharacterEditorWindowBuffer + (EDITOR_WINDOW_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WINDOW_WIDTH);

        x += gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width;
        blitBufferToBufferTrans(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_AGE_OFF],
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].height,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].width,
            gCharacterEditorWindowBuffer + (EDITOR_WINDOW_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WINDOW_WIDTH);

        x += gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].width;
        blitBufferToBufferTrans(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_SEX_OFF],
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_ON].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_ON].height,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_ON].width,
            gCharacterEditorWindowBuffer + (EDITOR_WINDOW_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WINDOW_WIDTH);

        btn = buttonCreate(gCharacterEditorWindow,
            11,
            327,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_FOLDER_MASK].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_FOLDER_MASK].height,
            -1,
            -1,
            -1,
            535,
            NULL,
            NULL,
            NULL,
            BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetMask(btn, gCharacterEditorFrmData[EDITOR_GRAPHIC_FOLDER_MASK]);
        }
    }

    if (gCharacterEditorIsCreationMode) {
        // +/- buttons for stats
        for (i = 0; i < 7; i++) {
            gCharacterEditorPrimaryStatPlusBtns[i] = buttonCreate(gCharacterEditorWindow,
                SPECIAL_STATS_BTN_X,
                gCharacterEditorPrimaryStatY[i],
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
                -1,
                518,
                503 + i,
                518,
                gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
                gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
                NULL,
                32);
            if (gCharacterEditorPrimaryStatPlusBtns[i] != -1) {
                buttonSetCallbacks(gCharacterEditorPrimaryStatPlusBtns[i], _gsound_red_butt_press, NULL);
            }

            gCharacterEditorPrimaryStatMinusBtns[i] = buttonCreate(gCharacterEditorWindow,
                SPECIAL_STATS_BTN_X,
                gCharacterEditorPrimaryStatY[i] + gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height - 1,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height,
                -1,
                518,
                510 + i,
                518,
                gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
                gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
                NULL,
                32);
            if (gCharacterEditorPrimaryStatMinusBtns[i] != -1) {
                buttonSetCallbacks(gCharacterEditorPrimaryStatMinusBtns[i], _gsound_red_butt_press, NULL);
            }
        }
    }

    characterEditorRegisterInfoAreas();
    soundContinueAll();

    btn = buttonCreate(
        gCharacterEditorWindow,
        343,
        454,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        501,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(
        gCharacterEditorWindow,
        552,
        454,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        502,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        0,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(
        gCharacterEditorWindow,
        455,
        454,
        gCharacterEditorFrmSize[23].width,
        gCharacterEditorFrmSize[23].height,
        -1,
        -1,
        -1,
        500,
        gCharacterEditorFrmData[23],
        gCharacterEditorFrmData[24],
        0,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    win_draw(gCharacterEditorWindow);
    indicatorBarHide();

    return 0;
}

// 0x433AA8
void characterEditorWindowFree()
{
    // NOTE: Uninline.
    folder_exit();

    windowDestroy(gCharacterEditorWindow);

    for (int index = 0; index < EDITOR_GRAPHIC_COUNT; index++) {
        art_ptr_unlock(gCharacterEditorFrmHandle[index]);

        if (gCharacterEditorFrmShouldCopy[index]) {
            internal_free(gCharacterEditorFrmCopy[index]);
        }
    }

    art_ptr_unlock(gCharacterEditorWindowBackgroundHandle);

    // NOTE: Uninline.
    genericReputationFree();

    // NOTE: Uninline.
    karmaFree();

    messageListFree(&gCharacterEditorMessageList);

    interfaceBarRefresh();

    // NOTE: Uninline.
    RstrBckgProc();

    fontSetCurrent(gCharacterEditorOldFont);

    if (gCharacterEditorIsCreationMode == 1) {
        skillsSetTagged(gCharacterEditorTempTaggedSkills, 3);
        traitsSetSelected(gCharacterEditorTempTraits[0], gCharacterEditorTempTraits[1]);
        characterEditorSelectedItem = 0;
        critter_adjust_hits(gDude, 1000);
    }

    indicatorBarShow();
}

// NOTE: Inlined.
//
// 0x433BEC
void RstrBckgProc()
{
    if (gCharacterEditorIsoWasEnabled) {
        isoEnable();
    }

    cycle_enable();

    gameMouseSetCursor(MOUSE_CURSOR_ARROW);
}

// CharEditInit
// 0x433C0C
void characterEditorInit()
{
    int i;

    characterEditorSelectedItem = 0;
    gCharacterEditorCurrentSkill = 0;
    gCharacterEditorSkillValueAdjustmentSliderY = 27;
    gCharacterEditorHasFreePerk = 0;
    characterEditorWindowSelectedFolder = EDITOR_FOLDER_PERKS;

    for (i = 0; i < 2; i++) {
        gCharacterEditorTempTraits[i] = -1;
        gCharacterEditorOptionalTraitsBackup[i] = -1;
    }

    gCharacterEditorRemainingCharacterPoints = 5;
    gCharacterEditorLastLevel = 1;
}

// handle name input
int _get_input_str(int win, int cancelKeyCode, char* text, int maxLength, int x, int y, int textColor, int backgroundColor, int flags)
{
    int cursorWidth = fontGetStringWidth("_") - 4;
    int windowWidth = windowGetWidth(win);
    int v60 = fontGetLineHeight();
    unsigned char* windowBuffer = windowGetBuffer(win);
    if (maxLength > 255) {
        maxLength = 255;
    }

    char copy[257];
    strcpy(copy, text);

    int nameLength = strlen(text);
    copy[nameLength] = ' ';
    copy[nameLength + 1] = '\0';

    int nameWidth = fontGetStringWidth(copy);

    bufferFill(windowBuffer + windowWidth * y + x, nameWidth, fontGetLineHeight(), windowWidth, backgroundColor);
    fontDrawText(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);

    win_draw(win);

    int blinkingCounter = 3;
    bool blink = false;

    int rc = 1;
    while (rc == 1) {
        _frame_time = _get_time();

        int keyCode = _get_input();
        if (keyCode == cancelKeyCode) {
            rc = 0;
        } else if (keyCode == KEY_RETURN) {
            soundPlayFile("ib1p1xx1");
            rc = 0;
        } else if (keyCode == KEY_ESCAPE || _game_user_wants_to_quit != 0) {
            rc = -1;
        } else {
            if ((keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE) && nameLength >= 1) {
                bufferFill(windowBuffer + windowWidth * y + x, fontGetStringWidth(copy), v60, windowWidth, backgroundColor);
                copy[nameLength - 1] = ' ';
                copy[nameLength] = '\0';
                fontDrawText(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);
                nameLength--;

                win_draw(win);
            } else if ((keyCode >= KEY_FIRST_INPUT_CHARACTER && keyCode <= KEY_LAST_INPUT_CHARACTER) && nameLength < maxLength) {
                if ((flags & 0x01) != 0) {
                    if (!_isdoschar(keyCode)) {
                        break;
                    }
                }

                bufferFill(windowBuffer + windowWidth * y + x, fontGetStringWidth(copy), v60, windowWidth, backgroundColor);

                copy[nameLength] = keyCode & 0xFF;
                copy[nameLength + 1] = ' ';
                copy[nameLength + 2] = '\0';
                fontDrawText(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);
                nameLength++;

                win_draw(win);
            }
        }

        blinkingCounter -= 1;
        if (blinkingCounter == 0) {
            blinkingCounter = 3;

            int color = blink ? backgroundColor : textColor;
            blink = !blink;

            bufferFill(windowBuffer + windowWidth * y + x + fontGetStringWidth(copy) - cursorWidth, cursorWidth, v60 - 2, windowWidth, color);
        }

        win_draw(win);

        while (getTicksSince(_frame_time) < 1000 / 24) { }
    }

    if (rc == 0 || nameLength > 0) {
        copy[nameLength] = '\0';
        strcpy(text, copy);
    }

    return rc;
}

// 0x434060
bool _isdoschar(int ch)
{
    const char* punctuations = "#@!$`'~^&()-_=[]{}";

    if (isalnum(ch)) {
        return true;
    }

    int length = strlen(punctuations);
    for (int index = 0; index < length; index++) {
        if (punctuations[index] == ch) {
            return true;
        }
    }

    return false;
}

// copy filename replacing extension
//
// 0x4340D0
char* _strmfe(char* dest, const char* name, const char* ext)
{
    char* save = dest;

    while (*name != '\0' && *name != '.') {
        *dest++ = *name++;
    }

    *dest++ = '.';

    strcpy(dest, ext);

    return save;
}

// 0x43410C
void characterEditorDrawFolders()
{
    if (gCharacterEditorIsCreationMode) {
        return;
    }

    blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + (360 * 640) + 34, 280, 120, 640, gCharacterEditorWindowBuffer + (360 * 640) + 34, 640);

    fontSetCurrent(101);

    switch (characterEditorWindowSelectedFolder) {
    case EDITOR_FOLDER_PERKS:
        blitBufferToBuffer(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED],
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorWindowBuffer + (327 * 640) + 11,
            640);
        characterEditorDrawPerksFolder();
        break;
    case EDITOR_FOLDER_KARMA:
        blitBufferToBuffer(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED],
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorWindowBuffer + (327 * 640) + 11,
            640);
        characterEditorDrawKarmaFolder();
        break;
    case EDITOR_FOLDER_KILLS:
        blitBufferToBuffer(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED],
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            gCharacterEditorWindowBuffer + (327 * 640) + 11,
            640);
        gCharacterEditorKillsCount = characterEditorDrawKillsFolder();
        break;
    default:
        debugPrint("\n ** Unknown folder type! **\n");
        break;
    }
}

// 0x434238
void characterEditorDrawPerksFolder()
{
    const char* string;
    char perkName[80];
    int perk;
    int perkLevel;
    bool hasContent = false;

    characterEditorFolderViewClear();

    if (gCharacterEditorTempTraits[0] != -1) {
        // TRAITS
        string = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 156);
        if (characterEditorFolderViewDrawHeading(string)) {
            gCharacterEditorFolderCardFrmId = 54;
            // Optional Traits
            gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 146);
            gCharacterEditorFolderCardSubtitle = NULL;
            // Optional traits describe your character in more detail. All traits will have positive and negative effects. You may choose up to two traits during creation.
            gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 147);
            hasContent = true;
        }

        if (gCharacterEditorTempTraits[0] != -1) {
            string = traitGetName(gCharacterEditorTempTraits[0]);
            if (characterEditorFolderViewDrawString(string)) {
                gCharacterEditorFolderCardFrmId = traitGetFrmId(gCharacterEditorTempTraits[0]);
                gCharacterEditorFolderCardTitle = traitGetName(gCharacterEditorTempTraits[0]);
                gCharacterEditorFolderCardSubtitle = NULL;
                gCharacterEditorFolderCardDescription = traitGetDescription(gCharacterEditorTempTraits[0]);
                hasContent = true;
            }
        }

        if (gCharacterEditorTempTraits[1] != -1) {
            string = traitGetName(gCharacterEditorTempTraits[1]);
            if (characterEditorFolderViewDrawString(string)) {
                gCharacterEditorFolderCardFrmId = traitGetFrmId(gCharacterEditorTempTraits[1]);
                gCharacterEditorFolderCardTitle = traitGetName(gCharacterEditorTempTraits[1]);
                gCharacterEditorFolderCardSubtitle = NULL;
                gCharacterEditorFolderCardDescription = traitGetDescription(gCharacterEditorTempTraits[1]);
                hasContent = true;
            }
        }
    }

    for (perk = 0; perk < PERK_COUNT; perk++) {
        if (perkGetRank(gDude, perk) != 0) {
            break;
        }
    }

    if (perk != PERK_COUNT) {
        // PERKS
        string = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 109);
        characterEditorFolderViewDrawHeading(string);

        for (perk = 0; perk < PERK_COUNT; perk++) {
            perkLevel = perkGetRank(gDude, perk);
            if (perkLevel != 0) {
                string = perkGetName(perk);

                if (perkLevel == 1) {
                    strcpy(perkName, string);
                } else {
                    sprintf(perkName, "%s (%d)", string, perkLevel);
                }

                if (characterEditorFolderViewDrawString(perkName)) {
                    gCharacterEditorFolderCardFrmId = perkGetFrmId(perk);
                    gCharacterEditorFolderCardTitle = perkGetName(perk);
                    gCharacterEditorFolderCardSubtitle = NULL;
                    gCharacterEditorFolderCardDescription = perkGetDescription(perk);
                    hasContent = true;
                }
            }
        }
    }

    if (!hasContent) {
        gCharacterEditorFolderCardFrmId = 71;
        // Perks
        gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 124);
        gCharacterEditorFolderCardSubtitle = NULL;
        // Perks add additional abilities. Every third experience level, you can choose one perk.
        gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 127);
    }
}

// 0x434498
int characterEditorKillsCompare(const void* a1, const void* a2)
{
    const KillInfo* v1 = (const KillInfo*)a1;
    const KillInfo* v2 = (const KillInfo*)a2;
    return stricmp(v1->name, v2->name);
}

// 0x4344A4
int characterEditorDrawKillsFolder()
{
    int i;
    int killsCount;
    KillInfo kills[19];
    int usedKills = 0;
    bool hasContent = false;

    characterEditorFolderViewClear();

    for (i = 0; i < KILL_TYPE_COUNT; i++) {
        killsCount = critter_kill_count(i);
        if (killsCount != 0) {
            KillInfo* killInfo = &(kills[usedKills]);
            killInfo->name = critter_kill_name(i);
            killInfo->killTypeId = i;
            killInfo->kills = killsCount;
            usedKills++;
        }
    }

    if (usedKills != 0) {
        qsort(kills, usedKills, sizeof(*kills), characterEditorKillsCompare);

        for (i = 0; i < usedKills; i++) {
            KillInfo* killInfo = &(kills[i]);
            if (characterEditorFolderViewDrawKillsEntry(killInfo->name, killInfo->kills)) {
                gCharacterEditorFolderCardFrmId = 46;
                gCharacterEditorFolderCardTitle = gCharacterEditorFolderCardString;
                gCharacterEditorFolderCardSubtitle = NULL;
                gCharacterEditorFolderCardDescription = critter_kill_info(kills[i].killTypeId);
                sprintf(gCharacterEditorFolderCardString, "%s %s", killInfo->name, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 126));
                hasContent = true;
            }
        }
    }

    if (!hasContent) {
        gCharacterEditorFolderCardFrmId = 46;
        gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 126);
        gCharacterEditorFolderCardSubtitle = NULL;
        gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 129);
    }

    return usedKills;
}

// 0x4345DC
void characterEditorDrawBigNumber(int x, int y, int flags, int value, int previousValue, int windowHandle)
{
    Rect rect;
    int windowWidth;
    unsigned char* windowBuf;
    int tens;
    int ones;
    unsigned char* tensBufferPtr;
    unsigned char* onesBufferPtr;
    unsigned char* numbersGraphicBufferPtr;

    windowWidth = windowGetWidth(windowHandle);
    windowBuf = windowGetBuffer(windowHandle);

    rect.left = x;
    rect.top = y;
    rect.right = x + BIG_NUM_WIDTH * 2;
    rect.bottom = y + BIG_NUM_HEIGHT;

    numbersGraphicBufferPtr = gCharacterEditorFrmData[0];

    if (flags & RED_NUMBERS) {
        // First half of the bignum.frm is white,
        // second half is red.
        numbersGraphicBufferPtr += gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width / 2;
    }

    tensBufferPtr = windowBuf + windowWidth * y + x;
    onesBufferPtr = tensBufferPtr + BIG_NUM_WIDTH;

    if (value >= 0 && value <= 99 && previousValue >= 0 && previousValue <= 99) {
        tens = value / 10;
        ones = value % 10;

        if (flags & ANIMATE) {
            if (previousValue % 10 != ones) {
                _frame_time = _get_time();
                blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 11,
                    BIG_NUM_WIDTH,
                    BIG_NUM_HEIGHT,
                    gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                    onesBufferPtr,
                    windowWidth);
                win_draw_rect(windowHandle, &rect);
                while (getTicksSince(_frame_time) < BIG_NUM_ANIMATION_DELAY)
                    ;
            }

            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * ones,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                onesBufferPtr,
                windowWidth);
            win_draw_rect(windowHandle, &rect);

            if (previousValue / 10 != tens) {
                _frame_time = _get_time();
                blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 11,
                    BIG_NUM_WIDTH,
                    BIG_NUM_HEIGHT,
                    gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                    tensBufferPtr,
                    windowWidth);
                win_draw_rect(windowHandle, &rect);
                while (getTicksSince(_frame_time) < BIG_NUM_ANIMATION_DELAY)
                    ;
            }

            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * tens,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                tensBufferPtr,
                windowWidth);
            win_draw_rect(windowHandle, &rect);
        } else {
            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * tens,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                tensBufferPtr,
                windowWidth);
            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * ones,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                onesBufferPtr,
                windowWidth);
        }
    } else {

        blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 9,
            BIG_NUM_WIDTH,
            BIG_NUM_HEIGHT,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
            tensBufferPtr,
            windowWidth);
        blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 9,
            BIG_NUM_WIDTH,
            BIG_NUM_HEIGHT,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_BIG_NUMBERS].width,
            onesBufferPtr,
            windowWidth);
    }
}

// 0x434920
void characterEditorDrawPcStats()
{
    int color;
    int y;
    char* formattedValue;
    // NOTE: The length of this buffer is 8 bytes, which is enough to display
    // 999,999 (7 bytes NULL-terminated) experience points. Usually a player
    // will never gain that much during normal gameplay.
    //
    // However it's possible to use one of the F2 modding tools and savegame
    // editors to receive rediculous amount of experience points. Vanilla is
    // able to handle it, because `stringBuffer` acts as continuation of
    // `formattedValueBuffer`. This is not the case with MSVC, where
    // insufficient space for xp greater then 999,999 ruins the stack. In order
    // to fix the `formattedValueBuffer` is expanded to 16 bytes, so it should
    // be possible to store max 32-bit integer (4,294,967,295).
    char formattedValueBuffer[16];
    char stringBuffer[128];

    if (gCharacterEditorIsCreationMode == 1) {
        return;
    }

    fontSetCurrent(101);

    blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + 640 * 280 + 32, 124, 32, 640, gCharacterEditorWindowBuffer + 640 * 280 + 32, 640);

    // LEVEL
    y = 280;
    if (characterEditorSelectedItem != 7) {
        color = colorTable[992];
    } else {
        color = colorTable[32747];
    }

    int level = pcGetStat(PC_STAT_LEVEL);
    sprintf(stringBuffer, "%s %d",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 113),
        level);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 32, stringBuffer, 640, 640, color);

    // EXPERIENCE
    y += fontGetLineHeight() + 1;
    if (characterEditorSelectedItem != 8) {
        color = colorTable[992];
    } else {
        color = colorTable[32747];
    }

    int exp = pcGetStat(PC_STAT_EXPERIENCE);
    sprintf(stringBuffer, "%s %s",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 114),
        _itostndn(exp, formattedValueBuffer));
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 32, stringBuffer, 640, 640, color);

    // EXP NEEDED TO NEXT LEVEL
    y += fontGetLineHeight() + 1;
    if (characterEditorSelectedItem != 9) {
        color = colorTable[992];
    } else {
        color = colorTable[32747];
    }

    int expToNextLevel = pcGetExperienceForNextLevel();
    int expMsgId;
    if (expToNextLevel == -1) {
        expMsgId = 115;
        formattedValue = byte_5016E4;
    } else {
        expMsgId = 115;
        if (expToNextLevel > 999999) {
            expMsgId = 175;
        }
        formattedValue = _itostndn(expToNextLevel, formattedValueBuffer);
    }

    sprintf(stringBuffer, "%s %s",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, expMsgId),
        formattedValue);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 32, stringBuffer, 640, 640, color);
}

// 0x434B38
void characterEditorDrawPrimaryStat(int stat, bool animate, int previousValue)
{
    int off;
    int color;
    const char* description;
    int value;
    int flags;
    int messageListItemId;

    fontSetCurrent(101);

    if (stat == RENDER_ALL_STATS) {
        // NOTE: Original code is different, looks like tail recursion
        // optimization.
        for (stat = 0; stat < 7; stat++) {
            characterEditorDrawPrimaryStat(stat, 0, 0);
        }
        return;
    }

    if (characterEditorSelectedItem == stat) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    off = 640 * (gCharacterEditorPrimaryStatY[stat] + 8) + 103;

    // TODO: The original code is different.
    if (gCharacterEditorIsCreationMode) {
        value = critterGetBaseStatWithTraitModifier(gDude, stat) + critterGetBonusStat(gDude, stat);

        flags = 0;

        if (animate) {
            flags |= ANIMATE;
        }

        if (value > 10) {
            flags |= RED_NUMBERS;
        }

        characterEditorDrawBigNumber(58, gCharacterEditorPrimaryStatY[stat], flags, value, previousValue, gCharacterEditorWindow);

        blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + off, 40, fontGetLineHeight(), 640, gCharacterEditorWindowBuffer + off, 640);

        messageListItemId = critterGetStat(gDude, stat) + 199;
        if (messageListItemId > 210) {
            messageListItemId = 210;
        }

        description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, messageListItemId);
        fontDrawText(gCharacterEditorWindowBuffer + 640 * (gCharacterEditorPrimaryStatY[stat] + 8) + 103, description, 640, 640, color);
    } else {
        value = critterGetStat(gDude, stat);
        characterEditorDrawBigNumber(58, gCharacterEditorPrimaryStatY[stat], 0, value, 0, gCharacterEditorWindow);
        blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + off, 40, fontGetLineHeight(), 640, gCharacterEditorWindowBuffer + off, 640);

        value = critterGetStat(gDude, stat);
        if (value > 10) {
            value = 10;
        }

        description = statGetValueDescription(value);
        fontDrawText(gCharacterEditorWindowBuffer + off, description, 640, 640, color);
    }
}

// 0x434F18
void characterEditorDrawGender()
{
    int gender;
    char* str;
    char text[32];
    int x, width;

    fontSetCurrent(103);

    gender = critterGetStat(gDude, STAT_GENDER);
    str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 107 + gender);

    strcpy(text, str);

    width = gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_ON].width;
    x = (width / 2) - (fontGetStringWidth(text) / 2);

    memcpy(gCharacterEditorFrmCopy[11],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_SEX_ON],
        width * gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_ON].height);
    memcpy(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_SEX_OFF],
        gCharacterEditorFrmData[10],
        width * gCharacterEditorFrmSize[EDITOR_GRAPHIC_SEX_OFF].height);

    x += 6 * width;
    fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_SEX_ON] + x, text, width, width, colorTable[14723]);
    fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_SEX_OFF] + x, text, width, width, colorTable[18979]);
}

// 0x43501C
void characterEditorDrawAge()
{
    int age;
    char* str;
    char text[32];
    int x, width;

    fontSetCurrent(103);

    age = critterGetStat(gDude, STAT_AGE);
    str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 104);

    sprintf(text, "%s %d", str, age);

    width = gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].width;
    x = (width / 2) + 1 - (fontGetStringWidth(text) / 2);

    memcpy(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_AGE_ON],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_AGE_ON],
        width * gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].height);
    memcpy(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_AGE_OFF],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_AGE_OFF],
        width * gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].height);

    x += 6 * width;
    fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_AGE_ON] + x, text, width, width, colorTable[14723]);
    fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_AGE_OFF] + x, text, width, width, colorTable[18979]);
}

// 0x435118
void characterEditorDrawName()
{
    char* str;
    char text[32];
    int x, width;
    char *pch, tmp;
    bool has_space;

    fontSetCurrent(103);

    str = critter_name(gDude);
    strcpy(text, str);

    if (fontGetStringWidth(text) > 100) {
        pch = text;
        has_space = false;
        while (*pch != '\0') {
            tmp = *pch;
            *pch = '\0';
            if (tmp == ' ') {
                has_space = true;
            }

            if (fontGetStringWidth(text) > 100) {
                break;
            }

            *pch = tmp;
            pch++;
        }

        if (has_space) {
            pch = text + strlen(text);
            while (pch != text && *pch != ' ') {
                *pch = '\0';
                pch--;
            }
        }
    }

    width = gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width;
    x = (width / 2) + 3 - (fontGetStringWidth(text) / 2);

    memcpy(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_NAME_ON],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_NAME_ON],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width * gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].height);
    memcpy(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_NAME_OFF],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_NAME_OFF],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_OFF].width * gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_OFF].height);

    x += 6 * width;
    fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_NAME_ON] + x, text, width, width, colorTable[14723]);
    fontDrawText(gCharacterEditorFrmCopy[EDITOR_GRAPHIC_NAME_OFF] + x, text, width, width, colorTable[18979]);
}

// 0x43527C
void characterEditorDrawDerivedStats()
{
    int conditions;
    int color;
    const char* messageListItemText;
    char t[420]; // TODO: Size is wrong.
    int y;

    conditions = gDude->data.critter.combat.results;

    fontSetCurrent(101);

    y = 46;

    blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + 640 * y + 194, 118, 108, 640, gCharacterEditorWindowBuffer + 640 * y + 194, 640);

    // Hit Points
    if (characterEditorSelectedItem == EDITOR_HIT_POINTS) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    int currHp;
    int maxHp;
    if (gCharacterEditorIsCreationMode) {
        maxHp = critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS);
        currHp = maxHp;
    } else {
        maxHp = critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS);
        currHp = critter_get_hits(gDude);
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 300);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d/%d", currHp, maxHp);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 263, t, 640, 640, color);

    // Poisoned
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_POISONED) {
        color = critter_get_poison(gDude) != 0 ? colorTable[32747] : colorTable[15845];
    } else {
        color = critter_get_poison(gDude) != 0 ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 312);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    // Radiated
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_RADIATED) {
        color = critter_get_rads(gDude) != 0 ? colorTable[32747] : colorTable[15845];
    } else {
        color = critter_get_rads(gDude) != 0 ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 313);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    // Eye Damage
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_EYE_DAMAGE) {
        color = (conditions & DAM_BLIND) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_BLIND) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 314);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    // Crippled Right Arm
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_RIGHT_ARM) {
        color = (conditions & DAM_CRIP_ARM_RIGHT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_ARM_RIGHT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 315);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    // Crippled Left Arm
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_LEFT_ARM) {
        color = (conditions & DAM_CRIP_ARM_LEFT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_ARM_LEFT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 316);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    // Crippled Right Leg
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_RIGHT_LEG) {
        color = (conditions & DAM_CRIP_LEG_RIGHT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_LEG_RIGHT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 317);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    // Crippled Left Leg
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_LEFT_LEG) {
        color = (conditions & DAM_CRIP_LEG_LEFT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_LEG_LEFT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 318);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    y = 179;

    blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + 640 * y + 194, 116, 130, 640, gCharacterEditorWindowBuffer + 640 * y + 194, 640);

    // Armor Class
    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_ARMOR_CLASS) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 302);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_ARMOR_CLASS), t, 10);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Action Points
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_ACTION_POINTS) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 301);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_MAXIMUM_ACTION_POINTS), t, 10);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Carry Weight
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_CARRY_WEIGHT) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 311);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_CARRY_WEIGHT), t, 10);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, critterIsOverloaded(gDude) ? colorTable[31744] : color);

    // Melee Damage
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_MELEE_DAMAGE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 304);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_MELEE_DAMAGE), t, 10);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Damage Resistance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_DAMAGE_RESISTANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 305);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_DAMAGE_RESISTANCE));
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Poison Resistance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_POISON_RESISTANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 306);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_POISON_RESISTANCE));
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Radiation Resistance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_RADIATION_RESISTANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 307);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_RADIATION_RESISTANCE));
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Sequence
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_SEQUENCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 308);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_SEQUENCE), t, 10);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Healing Rate
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_HEALING_RATE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 309);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_HEALING_RATE), t, 10);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);

    // Critical Chance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_CRITICAL_CHANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 310);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_CRITICAL_CHANCE));
    fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 288, t, 640, 640, color);
}

// 0x436154
void characterEditorDrawSkills(int a1)
{
    int selectedSkill = -1;
    const char* str;
    int i;
    int color;
    int y;
    int value;
    char valueString[32];

    if (characterEditorSelectedItem >= EDITOR_FIRST_SKILL && characterEditorSelectedItem < 79) {
        selectedSkill = characterEditorSelectedItem - EDITOR_FIRST_SKILL;
    }

    if (gCharacterEditorIsCreationMode == 0 && a1 == 0) {
        buttonDestroy(gCharacterEditorSliderPlusBtn);
        buttonDestroy(gCharacterEditorSliderMinusBtn);
        gCharacterEditorSliderMinusBtn = -1;
        gCharacterEditorSliderPlusBtn = -1;
    }

    blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + 370, 270, 252, 640, gCharacterEditorWindowBuffer + 370, 640);

    fontSetCurrent(103);

    // SKILLS
    str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 117);
    fontDrawText(gCharacterEditorWindowBuffer + 640 * 5 + 380, str, 640, 640, colorTable[18979]);

    if (!gCharacterEditorIsCreationMode) {
        // SKILL POINTS
        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 112);
        fontDrawText(gCharacterEditorWindowBuffer + 640 * 233 + 400, str, 640, 640, colorTable[18979]);

        value = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
        characterEditorDrawBigNumber(522, 228, 0, value, 0, gCharacterEditorWindow);
    } else {
        // TAG SKILLS
        str = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 138);
        fontDrawText(gCharacterEditorWindowBuffer + 640 * 233 + 422, str, 640, 640, colorTable[18979]);

        if (a1 == 2 && !gCharacterEditorIsSkillsFirstDraw) {
            characterEditorDrawBigNumber(522, 228, ANIMATE, gCharacterEditorTaggedSkillCount, gCharacterEditorOldTaggedSkillCount, gCharacterEditorWindow);
        } else {
            characterEditorDrawBigNumber(522, 228, 0, gCharacterEditorTaggedSkillCount, 0, gCharacterEditorWindow);
            gCharacterEditorIsSkillsFirstDraw = 0;
        }
    }

    skillsSetTagged(gCharacterEditorTempTaggedSkills, NUM_TAGGED_SKILLS);

    fontSetCurrent(101);

    y = 27;
    for (i = 0; i < SKILL_COUNT; i++) {
        if (i == selectedSkill) {
            if (i != gCharacterEditorTempTaggedSkills[0] && i != gCharacterEditorTempTaggedSkills[1] && i != gCharacterEditorTempTaggedSkills[2] && i != gCharacterEditorTempTaggedSkills[3]) {
                color = colorTable[32747];
            } else {
                color = colorTable[32767];
            }
        } else {
            if (i != gCharacterEditorTempTaggedSkills[0] && i != gCharacterEditorTempTaggedSkills[1] && i != gCharacterEditorTempTaggedSkills[2] && i != gCharacterEditorTempTaggedSkills[3]) {
                color = colorTable[992];
            } else {
                color = colorTable[21140];
            }
        }

        str = skillGetName(i);
        fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 380, str, 640, 640, color);

        value = skillGetValue(gDude, i);
        sprintf(valueString, "%d%%", value);

        fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 573, valueString, 640, 640, color);

        y += fontGetLineHeight() + 1;
    }

    if (!gCharacterEditorIsCreationMode) {
        y = gCharacterEditorCurrentSkill * (fontGetLineHeight() + 1);
        gCharacterEditorSkillValueAdjustmentSliderY = y + 27;

        blitBufferToBufferTrans(
            gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER],
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER].width,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER].height,
            gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER].width,
            gCharacterEditorWindowBuffer + 640 * (y + 16) + 592,
            640);

        if (a1 == 0) {
            if (gCharacterEditorSliderPlusBtn == -1) {
                gCharacterEditorSliderPlusBtn = buttonCreate(
                    gCharacterEditorWindow,
                    614,
                    gCharacterEditorSkillValueAdjustmentSliderY - 7,
                    gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
                    gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
                    -1,
                    522,
                    521,
                    522,
                    gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
                    gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
                    NULL,
                    96);
                buttonSetCallbacks(gCharacterEditorSliderPlusBtn, _gsound_red_butt_press, NULL);
            }

            if (gCharacterEditorSliderMinusBtn == -1) {
                gCharacterEditorSliderMinusBtn = buttonCreate(
                    gCharacterEditorWindow,
                    614,
                    gCharacterEditorSkillValueAdjustmentSliderY + 4 - 12 + gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height,
                    gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
                    gCharacterEditorFrmSize[EDITOR_GRAPHIC_SLIDER_MINUS_OFF].height,
                    -1,
                    524,
                    523,
                    524,
                    gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
                    gCharacterEditorFrmData[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
                    NULL,
                    96);
                buttonSetCallbacks(gCharacterEditorSliderMinusBtn, _gsound_red_butt_press, NULL);
            }
        }
    }
}

// 0x4365AC
void characterEditorDrawCard()
{
    int graphicId;
    char* title;
    char* description;

    if (characterEditorSelectedItem < 0 || characterEditorSelectedItem >= 98) {
        return;
    }

    blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + (640 * 267) + 345, 277, 170, 640, gCharacterEditorWindowBuffer + (267 * 640) + 345, 640);

    if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
        description = statGetDescription(characterEditorSelectedItem);
        title = statGetName(characterEditorSelectedItem);
        graphicId = statGetFrmId(characterEditorSelectedItem);
        characterEditorDrawCardWithOptions(graphicId, title, NULL, description);
    } else if (characterEditorSelectedItem >= 7 && characterEditorSelectedItem < 10) {
        if (gCharacterEditorIsCreationMode) {
            switch (characterEditorSelectedItem) {
            case 7:
                // Character Points
                description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 121);
                title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 120);
                characterEditorDrawCardWithOptions(7, title, NULL, description);
                break;
            }
        } else {
            switch (characterEditorSelectedItem) {
            case 7:
                description = pcStatGetDescription(PC_STAT_LEVEL);
                title = pcStatGetName(PC_STAT_LEVEL);
                characterEditorDrawCardWithOptions(7, title, NULL, description);
                break;
            case 8:
                description = pcStatGetDescription(PC_STAT_EXPERIENCE);
                title = pcStatGetName(PC_STAT_EXPERIENCE);
                characterEditorDrawCardWithOptions(8, title, NULL, description);
                break;
            case 9:
                // Next Level
                description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 123);
                title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 122);
                characterEditorDrawCardWithOptions(9, title, NULL, description);
                break;
            }
        }
    } else if ((characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) || (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98)) {
        characterEditorDrawCardWithOptions(gCharacterEditorFolderCardFrmId, gCharacterEditorFolderCardTitle, gCharacterEditorFolderCardSubtitle, gCharacterEditorFolderCardDescription);
    } else if (characterEditorSelectedItem >= 43 && characterEditorSelectedItem < 51) {
        switch (characterEditorSelectedItem) {
        case EDITOR_HIT_POINTS:
            description = statGetDescription(STAT_MAXIMUM_HIT_POINTS);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 300);
            graphicId = statGetFrmId(STAT_MAXIMUM_HIT_POINTS);
            characterEditorDrawCardWithOptions(graphicId, title, NULL, description);
            break;
        case EDITOR_POISONED:
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 400);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 312);
            characterEditorDrawCardWithOptions(11, title, NULL, description);
            break;
        case EDITOR_RADIATED:
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 401);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 313);
            characterEditorDrawCardWithOptions(12, title, NULL, description);
            break;
        case EDITOR_EYE_DAMAGE:
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 402);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 314);
            characterEditorDrawCardWithOptions(13, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_RIGHT_ARM:
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 403);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 315);
            characterEditorDrawCardWithOptions(14, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_LEFT_ARM:
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 404);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 316);
            characterEditorDrawCardWithOptions(15, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_RIGHT_LEG:
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 405);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 317);
            characterEditorDrawCardWithOptions(16, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_LEFT_LEG:
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 406);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 318);
            characterEditorDrawCardWithOptions(17, title, NULL, description);
            break;
        }
    } else if (characterEditorSelectedItem >= EDITOR_FIRST_DERIVED_STAT && characterEditorSelectedItem < 61) {
        int derivedStatIndex = characterEditorSelectedItem - 51;
        int stat = gCharacterEditorDerivedStatsMap[derivedStatIndex];
        description = statGetDescription(stat);
        title = statGetName(stat);
        graphicId = gCharacterEditorDerivedStatFrmIds[derivedStatIndex];
        characterEditorDrawCardWithOptions(graphicId, title, NULL, description);
    } else if (characterEditorSelectedItem >= EDITOR_FIRST_SKILL && characterEditorSelectedItem < 79) {
        int skill = characterEditorSelectedItem - 61;
        const char* attributesDescription = skillGetAttributes(skill);

        char formatted[150]; // TODO: Size is probably wrong.
        const char* base = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 137);
        int defaultValue = skillGetDefaultValue(skill);
        sprintf(formatted, "%s %d%% %s", base, defaultValue, attributesDescription);

        graphicId = skillGetFrmId(skill);
        title = skillGetName(skill);
        description = skillGetDescription(skill);
        characterEditorDrawCardWithOptions(graphicId, title, formatted, description);
    } else if (characterEditorSelectedItem >= 79 && characterEditorSelectedItem < 82) {
        switch (characterEditorSelectedItem) {
        case EDITOR_TAG_SKILL:
            if (gCharacterEditorIsCreationMode) {
                // Tag Skill
                description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 145);
                title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 144);
                characterEditorDrawCardWithOptions(27, title, NULL, description);
            } else {
                // Skill Points
                description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 131);
                title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 130);
                characterEditorDrawCardWithOptions(27, title, NULL, description);
            }
            break;
        case EDITOR_SKILLS:
            // Skills
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 151);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 150);
            characterEditorDrawCardWithOptions(27, title, NULL, description);
            break;
        case EDITOR_OPTIONAL_TRAITS:
            // Optional Traits
            description = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 147);
            title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 146);
            characterEditorDrawCardWithOptions(27, title, NULL, description);
            break;
        }
    }
}

// 0x436C4C
int characterEditorEditName()
{
    char* text;

    int windowWidth = gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].width;
    int windowHeight = gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].height;

    int nameWindowX = 17;
    int nameWindowY = 0;
    int win = windowCreate(nameWindowX, nameWindowY, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (win == -1) {
        return -1;
    }

    unsigned char* windowBuf = windowGetBuffer(win);

    // Copy background
    memcpy(windowBuf, gCharacterEditorFrmData[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    blitBufferToBufferTrans(
        gCharacterEditorFrmData[EDITOR_GRAPHIC_NAME_BOX],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_BOX].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_BOX].height,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_BOX].width,
        windowBuf + windowWidth * 13 + 13,
        windowWidth);
    blitBufferToBufferTrans(gCharacterEditorFrmData[EDITOR_GRAPHIC_DONE_BOX],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].height,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 40 + 13,
        windowWidth);

    fontSetCurrent(103);

    text = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 100);
    fontDrawText(windowBuf + windowWidth * 44 + 50, text, windowWidth, windowWidth, colorTable[18979]);

    int doneBtn = buttonCreate(win,
        26,
        44,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    win_draw(win);

    fontSetCurrent(101);

    char name[64];
    strcpy(name, critter_name(gDude));

    if (strcmp(name, "None") == 0) {
        name[0] = '\0';
    }

    // NOTE: I don't understand the nameCopy, not sure what it is used for. It's
    // definitely there, but I just don' get it.
    char nameCopy[64];
    strcpy(nameCopy, name);

    if (_get_input_str(win, 500, nameCopy, 11, 23, 19, colorTable[992], 100, 0) != -1) {
        if (nameCopy[0] != '\0') {
            critter_pc_set_name(nameCopy);
            characterEditorDrawName();
            windowDestroy(win);
            return 0;
        }
    }

    // NOTE: original code is a bit different, the following chunk of code written two times.

    fontSetCurrent(101);
    blitBufferToBuffer(gCharacterEditorFrmData[EDITOR_GRAPHIC_NAME_BOX],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_BOX].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_BOX].height,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_BOX].width,
        windowBuf + gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].width * 13 + 13,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].width);

    _PrintName(windowBuf, gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].width);

    strcpy(nameCopy, name);

    windowDestroy(win);

    return 0;
}

// 0x436F70
void _PrintName(unsigned char* buf, int pitch)
{
    char str[64];
    char* v4;

    memcpy(str, byte_431D93, 64);

    fontSetCurrent(101);

    v4 = critter_name(gDude);

    // TODO: Check.
    strcpy(str, v4);

    fontDrawText(buf + 19 * pitch + 21, str, pitch, pitch, colorTable[992]);
}

// 0x436FEC
int characterEditorEditAge()
{
    int win;
    unsigned char* windowBuf;
    int windowWidth;
    int windowHeight;
    const char* messageListItemText;
    int previousAge;
    int age;
    int doneBtn;
    int prevBtn;
    int nextBtn;
    int keyCode;
    int change;
    int flags;

    int savedAge = critterGetStat(gDude, STAT_AGE);

    windowWidth = gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].width;
    windowHeight = gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].height;

    int ageWindowX = gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width + 9;
    int ageWindowY = 0;
    win = windowCreate(ageWindowX, ageWindowY, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (win == -1) {
        return -1;
    }

    windowBuf = windowGetBuffer(win);

    memcpy(windowBuf, gCharacterEditorFrmData[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    blitBufferToBufferTrans(
        gCharacterEditorFrmData[EDITOR_GRAPHIC_AGE_BOX],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_BOX].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_BOX].height,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_BOX].width,
        windowBuf + windowWidth * 7 + 8,
        windowWidth);
    blitBufferToBufferTrans(
        gCharacterEditorFrmData[EDITOR_GRAPHIC_DONE_BOX],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].height,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 40 + 13,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].width);

    fontSetCurrent(103);

    messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 100);
    fontDrawText(windowBuf + windowWidth * 44 + 50, messageListItemText, windowWidth, windowWidth, colorTable[18979]);

    age = critterGetStat(gDude, STAT_AGE);
    characterEditorDrawBigNumber(55, 10, 0, age, 0, win);

    doneBtn = buttonCreate(win,
        26,
        44,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    nextBtn = buttonCreate(win,
        105,
        13,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LEFT_ARROW_DOWN].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LEFT_ARROW_DOWN].height,
        -1,
        503,
        501,
        503,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_RIGHT_ARROW_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (nextBtn != -1) {
        buttonSetCallbacks(nextBtn, _gsound_med_butt_press, NULL);
    }

    prevBtn = buttonCreate(win,
        19,
        13,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN].height,
        -1,
        504,
        502,
        504,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LEFT_ARROW_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LEFT_ARROW_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (prevBtn != -1) {
        buttonSetCallbacks(prevBtn, _gsound_med_butt_press, NULL);
    }

    while (true) {
        _frame_time = _get_time();
        change = 0;
        flags = 0;
        int v32 = 0;

        keyCode = _get_input();

        if (keyCode == KEY_RETURN || keyCode == 500) {
            if (keyCode != 500) {
                soundPlayFile("ib1p1xx1");
            }

            windowDestroy(win);
            return 0;
        } else if (keyCode == KEY_ESCAPE || _game_user_wants_to_quit != 0) {
            break;
        } else if (keyCode == 501) {
            age = critterGetStat(gDude, STAT_AGE);
            if (age < 35) {
                change = 1;
            }
        } else if (keyCode == 502) {
            age = critterGetStat(gDude, STAT_AGE);
            if (age > 16) {
                change = -1;
            }
        } else if (keyCode == KEY_PLUS || keyCode == KEY_UPPERCASE_N || keyCode == KEY_ARROW_UP) {
            previousAge = critterGetStat(gDude, STAT_AGE);
            if (previousAge < 35) {
                flags = ANIMATE;
                if (critterIncBaseStat(gDude, STAT_AGE) != 0) {
                    flags = 0;
                }
                age = critterGetStat(gDude, STAT_AGE);
                characterEditorDrawBigNumber(55, 10, flags, age, previousAge, win);
            }
        } else if (keyCode == KEY_MINUS || keyCode == KEY_UPPERCASE_J || keyCode == KEY_ARROW_DOWN) {
            previousAge = critterGetStat(gDude, STAT_AGE);
            if (previousAge > 16) {
                flags = ANIMATE;
                if (critterDecBaseStat(gDude, STAT_AGE) != 0) {
                    flags = 0;
                }
                age = critterGetStat(gDude, STAT_AGE);

                characterEditorDrawBigNumber(55, 10, flags, age, previousAge, win);
            }
        }

        if (flags == ANIMATE) {
            characterEditorDrawAge();
            characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
            characterEditorDrawDerivedStats();
            win_draw(gCharacterEditorWindow);
            win_draw(win);
        }

        if (change != 0) {
            int v33 = 0;

            _repFtime = 4;

            while (true) {
                _frame_time = _get_time();

                v33++;

                if ((!v32 && v33 == 1) || (v32 && v33 > dbl_50170B)) {
                    v32 = true;

                    if (v33 > dbl_50170B) {
                        _repFtime++;
                        if (_repFtime > 24) {
                            _repFtime = 24;
                        }
                    }

                    flags = ANIMATE;
                    previousAge = critterGetStat(gDude, STAT_AGE);

                    if (change == 1) {
                        if (previousAge < 35) {
                            if (critterIncBaseStat(gDude, STAT_AGE) != 0) {
                                flags = 0;
                            }
                        }
                    } else {
                        if (previousAge >= 16) {
                            if (critterDecBaseStat(gDude, STAT_AGE) != 0) {
                                flags = 0;
                            }
                        }
                    }

                    age = critterGetStat(gDude, STAT_AGE);
                    characterEditorDrawBigNumber(55, 10, flags, age, previousAge, win);
                    if (flags == ANIMATE) {
                        characterEditorDrawAge();
                        characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
                        characterEditorDrawDerivedStats();
                        win_draw(gCharacterEditorWindow);
                        win_draw(win);
                    }
                }

                if (v33 > dbl_50170B) {
                    while (getTicksSince(_frame_time) < 1000 / _repFtime)
                        ;
                } else {
                    while (getTicksSince(_frame_time) < 1000 / 24)
                        ;
                }

                keyCode = _get_input();
                if (keyCode == 503 || keyCode == 504 || _game_user_wants_to_quit != 0) {
                    break;
                }
            }
        } else {
            win_draw(win);

            while (getTicksSince(_frame_time) < 1000 / 24)
                ;
        }
    }

    critterSetBaseStat(gDude, STAT_AGE, savedAge);
    characterEditorDrawAge();
    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
    characterEditorDrawDerivedStats();
    win_draw(gCharacterEditorWindow);
    win_draw(win);
    windowDestroy(win);
    return 0;
}

// 0x437664
void characterEditorEditGender()
{
    char* text;

    int windowWidth = gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].width;
    int windowHeight = gCharacterEditorFrmSize[EDITOR_GRAPHIC_CHARWIN].height;

    int genderWindowX = 9
        + gCharacterEditorFrmSize[EDITOR_GRAPHIC_NAME_ON].width
        + gCharacterEditorFrmSize[EDITOR_GRAPHIC_AGE_ON].width;
    int genderWindowY = 0;
    int win = windowCreate(genderWindowX, genderWindowY, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);

    if (win == -1) {
        return;
    }

    unsigned char* windowBuf = windowGetBuffer(win);

    // Copy background
    memcpy(windowBuf, gCharacterEditorFrmData[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    blitBufferToBufferTrans(gCharacterEditorFrmData[EDITOR_GRAPHIC_DONE_BOX],
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].height,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 44 + 15,
        windowWidth);

    fontSetCurrent(103);

    text = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 100);
    fontDrawText(windowBuf + windowWidth * 48 + 52, text, windowWidth, windowWidth, colorTable[18979]);

    int doneBtn = buttonCreate(win,
        28,
        48,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    int btns[2];
    btns[0] = buttonCreate(win,
        22,
        2,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_MALE_ON].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_MALE_ON].height,
        -1,
        -1,
        501,
        -1,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_MALE_OFF],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_MALE_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x04 | BUTTON_FLAG_0x02 | BUTTON_FLAG_0x01);
    if (btns[0] != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, NULL);
    }

    btns[1] = buttonCreate(win,
        71,
        3,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_FEMALE_ON].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_FEMALE_ON].height,
        -1,
        -1,
        502,
        -1,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_FEMALE_OFF],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_FEMALE_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x04 | BUTTON_FLAG_0x02 | BUTTON_FLAG_0x01);
    if (btns[1] != -1) {
        _win_group_radio_buttons(2, btns);
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, NULL);
    }

    int savedGender = critterGetStat(gDude, STAT_GENDER);
    _win_set_button_rest_state(btns[savedGender], 1, 0);

    while (true) {
        _frame_time = _get_time();

        int eventCode = _get_input();

        if (eventCode == KEY_RETURN || eventCode == 500) {
            if (eventCode == KEY_RETURN) {
                soundPlayFile("ib1p1xx1");
            }
            break;
        }

        if (eventCode == KEY_ESCAPE || _game_user_wants_to_quit != 0) {
            critterSetBaseStat(gDude, STAT_GENDER, savedGender);
            characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
            characterEditorDrawDerivedStats();
            win_draw(gCharacterEditorWindow);
            break;
        }

        switch (eventCode) {
        case KEY_ARROW_LEFT:
        case KEY_ARROW_RIGHT:
            if (1) {
                bool wasMale = _win_button_down(btns[0]);
                _win_set_button_rest_state(btns[0], !wasMale, 1);
                _win_set_button_rest_state(btns[1], wasMale, 1);
            }
            break;
        case 501:
        case 502:
            // TODO: Original code is slightly different.
            critterSetBaseStat(gDude, STAT_GENDER, eventCode - 501);
            characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
            characterEditorDrawDerivedStats();
            break;
        }

        win_draw(win);

        while (getTicksSince(_frame_time) < 41)
            ;
    }

    characterEditorDrawGender();
    windowDestroy(win);
}

// 0x4379BC
void characterEditorAdjustPrimaryStat(int eventCode)
{
    _repFtime = 4;

    int savedRemainingCharacterPoints = gCharacterEditorRemainingCharacterPoints;

    if (!gCharacterEditorIsCreationMode) {
        return;
    }

    int incrementingStat = eventCode - 503;
    int decrementingStat = eventCode - 510;

    int v11 = 0;

    bool cont = true;
    do {
        _frame_time = _get_time();
        if (v11 <= 19.2) {
            v11++;
        }

        if (v11 == 1 || v11 > 19.2) {
            if (v11 > 19.2) {
                _repFtime++;
                if (_repFtime > 24) {
                    _repFtime = 24;
                }
            }

            if (eventCode >= 510) {
                int previousValue = critterGetStat(gDude, decrementingStat);
                if (critterDecBaseStat(gDude, decrementingStat) == 0) {
                    gCharacterEditorRemainingCharacterPoints++;
                } else {
                    cont = false;
                }

                characterEditorDrawPrimaryStat(decrementingStat, cont ? ANIMATE : 0, previousValue);
                characterEditorDrawBigNumber(126, 282, cont ? ANIMATE : 0, gCharacterEditorRemainingCharacterPoints, savedRemainingCharacterPoints, gCharacterEditorWindow);
                critterUpdateDerivedStats(gDude);
                characterEditorDrawDerivedStats();
                characterEditorDrawSkills(0);
                characterEditorSelectedItem = decrementingStat;
            } else {
                int previousValue = critterGetBaseStatWithTraitModifier(gDude, incrementingStat);
                previousValue += critterGetBonusStat(gDude, incrementingStat);
                if (gCharacterEditorRemainingCharacterPoints > 0 && previousValue < 10 && critterIncBaseStat(gDude, incrementingStat) == 0) {
                    gCharacterEditorRemainingCharacterPoints--;
                } else {
                    cont = false;
                }

                characterEditorDrawPrimaryStat(incrementingStat, cont ? ANIMATE : 0, previousValue);
                characterEditorDrawBigNumber(126, 282, cont ? ANIMATE : 0, gCharacterEditorRemainingCharacterPoints, savedRemainingCharacterPoints, gCharacterEditorWindow);
                critterUpdateDerivedStats(gDude);
                characterEditorDrawDerivedStats();
                characterEditorDrawSkills(0);
                characterEditorSelectedItem = incrementingStat;
            }

            win_draw(gCharacterEditorWindow);
        }

        if (v11 >= 19.2) {
            unsigned int delay = 1000 / _repFtime;
            while (getTicksSince(_frame_time) < delay) {
            }
        } else {
            while (getTicksSince(_frame_time) < 1000 / 24) {
            }
        }
    } while (_get_input() != 518 && cont);

    characterEditorDrawCard();
}

// handle options dialog
//
// 0x437C08
int characterEditorShowOptions()
{
    int width = gCharacterEditorFrmSize[43].width;
    int height = gCharacterEditorFrmSize[43].height;

    // NOTE: The following is a block of general purpose string buffers used in
    // this function. They are either store path, or strings from .msg files. I
    // don't know if such usage was intentional in the original code or it's a
    // result of some kind of compiler optimization.
    char string1[512];
    char string2[512];
    char string3[512];
    char string4[512];
    char string5[512];

    // Only two of the these blocks are used as a dialog body. Depending on the
    // dialog either 1 or 2 strings used from this array.
    const char* dialogBody[2] = {
        string5,
        string2,
    };

    if (gCharacterEditorIsCreationMode) {
        int optionsWindowX = 238;
        int optionsWindowY = 90;
        int win = windowCreate(optionsWindowX, optionsWindowY, gCharacterEditorFrmSize[41].width, gCharacterEditorFrmSize[41].height, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
        if (win == -1) {
            return -1;
        }

        unsigned char* windowBuffer = windowGetBuffer(win);
        memcpy(windowBuffer, gCharacterEditorFrmData[41], gCharacterEditorFrmSize[41].width * gCharacterEditorFrmSize[41].height);

        fontSetCurrent(103);

        int err = 0;
        unsigned char* down[5];
        unsigned char* up[5];
        int size = width * height;
        int y = 17;
        int index;

        for (index = 0; index < 5; index++) {
            if (err != 0) {
                break;
            }

            do {
                down[index] = (unsigned char*)internal_malloc(size);
                if (down[index] == NULL) {
                    err = 1;
                    break;
                }

                up[index] = (unsigned char*)internal_malloc(size);
                if (up[index] == NULL) {
                    err = 2;
                    break;
                }

                memcpy(down[index], gCharacterEditorFrmData[43], size);
                memcpy(up[index], gCharacterEditorFrmData[42], size);

                strcpy(string4, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 600 + index));

                int offset = width * 7 + width / 2 - fontGetStringWidth(string4) / 2;
                fontDrawText(up[index] + offset, string4, width, width, colorTable[18979]);
                fontDrawText(down[index] + offset, string4, width, width, colorTable[14723]);

                int btn = buttonCreate(win, 13, y, width, height, -1, -1, -1, 500 + index, up[index], down[index], NULL, BUTTON_FLAG_TRANSPARENT);
                if (btn != -1) {
                    buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
                }
            } while (0);

            y += height + 3;
        }

        if (err != 0) {
            if (err == 2) {
                internal_free(down[index]);
            }

            while (--index >= 0) {
                internal_free(up[index]);
                internal_free(down[index]);
            }

            return -1;
        }

        fontSetCurrent(101);

        int rc = 0;
        while (rc == 0) {
            int keyCode = _get_input();

            if (_game_user_wants_to_quit != 0) {
                rc = 2;
            } else if (keyCode == 504) {
                rc = 2;
            } else if (keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
                // DONE
                rc = 2;
                soundPlayFile("ib1p1xx1");
            } else if (keyCode == KEY_ESCAPE) {
                rc = 2;
            } else if (keyCode == 503 || keyCode == KEY_UPPERCASE_E || keyCode == KEY_LOWERCASE_E) {
                // ERASE
                strcpy(string5, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 605));
                strcpy(string2, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 606));

                if (dialog_out(NULL, dialogBody, 2, 169, 126, colorTable[992], NULL, colorTable[992], DIALOG_BOX_YES_NO) != 0) {
                    _ResetPlayer();
                    skillsGetTagged(gCharacterEditorTempTaggedSkills, NUM_TAGGED_SKILLS);

                    // NOTE: Uninline.
                    gCharacterEditorTaggedSkillCount = tagskl_free();

                    traitsGetSelected(&gCharacterEditorTempTraits[0], &gCharacterEditorTempTraits[1]);

                    // NOTE: Uninline.
                    gCharacterEditorTempTraitCount = get_trait_count();
                    critterUpdateDerivedStats(gDude);
                    characterEditorResetScreen();
                }
            } else if (keyCode == 502 || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P) {
                // PRINT TO FILE
                string4[0] = '\0';

                strcat(string4, "*.");
                strcat(string4, "TXT");

                char** fileList;
                int fileListLength = fileNameListInit(string4, &fileList, 0, 0);
                if (fileListLength != -1) {
                    // PRINT
                    strcpy(string1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 616));

                    // PRINT TO FILE
                    strcpy(string4, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 602));

                    if (save_file_dialog(string4, fileList, string1, fileListLength, 168, 80, 0) == 0) {
                        strcat(string1, ".");
                        strcat(string1, "TXT");

                        string4[0] = '\0';
                        strcat(string4, string1);

                        if (!characterFileExists(string4)) {
                            // already exists
                            sprintf(string4,
                                "%s %s",
                                strupr(string1),
                                getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 609));

                            strcpy(string5, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 610));

                            if (dialog_out(string4, dialogBody, 1, 169, 126, colorTable[32328], NULL, colorTable[32328], 0x10) != 0) {
                                rc = 1;
                            } else {
                                rc = 0;
                            }
                        } else {
                            rc = 1;
                        }

                        if (rc != 0) {
                            string4[0] = '\0';
                            strcat(string4, string1);

                            if (characterPrintToFile(string4) == 0) {
                                sprintf(string4,
                                    "%s%s",
                                    strupr(string1),
                                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 607));
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[992], NULL, colorTable[992], 0);
                            } else {
                                soundPlayFile("iisxxxx1");

                                sprintf(string4,
                                    "%s%s%s",
                                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 611),
                                    strupr(string1),
                                    "!");
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[992], 0x01);
                            }
                        }
                    }

                    fileNameListFree(&fileList, 0);
                } else {
                    soundPlayFile("iisxxxx1");

                    strcpy(string4, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 615));
                    dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);

                    rc = 0;
                }
            } else if (keyCode == 501 || keyCode == KEY_UPPERCASE_L || keyCode == KEY_LOWERCASE_L) {
                // LOAD
                string4[0] = '\0';
                strcat(string4, "*.");
                strcat(string4, "GCD");

                char** fileNameList;
                int fileNameListLength = fileNameListInit(string4, &fileNameList, 0, 0);
                if (fileNameListLength != -1) {
                    // NOTE: This value is not copied as in save dialog.
                    char* title = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 601);
                    int loadFileDialogRc = file_dialog(title, fileNameList, string3, fileNameListLength, 168, 80, 0);
                    if (loadFileDialogRc == -1) {
                        fileNameListFree(&fileNameList, 0);
                        // FIXME: This branch ignores cleanup at the end of the loop.
                        return -1;
                    }

                    if (loadFileDialogRc == 0) {
                        string4[0] = '\0';
                        strcat(string4, string3);

                        int oldRemainingCharacterPoints = gCharacterEditorRemainingCharacterPoints;

                        _ResetPlayer();

                        if (pc_load_data(string4) == 0) {
                            // NOTE: Uninline.
                            CheckValidPlayer();

                            skillsGetTagged(gCharacterEditorTempTaggedSkills, 4);

                            // NOTE: Uninline.
                            gCharacterEditorTaggedSkillCount = tagskl_free();

                            traitsGetSelected(&(gCharacterEditorTempTraits[0]), &(gCharacterEditorTempTraits[1]));

                            // NOTE: Uninline.
                            gCharacterEditorTempTraitCount = get_trait_count();

                            critterUpdateDerivedStats(gDude);

                            critter_adjust_hits(gDude, 1000);

                            rc = 1;
                        } else {
                            characterEditorRestorePlayer();
                            gCharacterEditorRemainingCharacterPoints = oldRemainingCharacterPoints;
                            critter_adjust_hits(gDude, 1000);
                            soundPlayFile("iisxxxx1");

                            strcpy(string4, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 612));
                            strcat(string4, string3);
                            strcat(string4, "!");

                            dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);
                        }

                        characterEditorResetScreen();
                    }

                    fileNameListFree(&fileNameList, 0);
                } else {
                    soundPlayFile("iisxxxx1");

                    // Error reading file list!
                    strcpy(string4, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 615));
                    rc = 0;

                    dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);
                }
            } else if (keyCode == 500 || keyCode == KEY_UPPERCASE_S || keyCode == KEY_LOWERCASE_S) {
                // SAVE
                string4[0] = '\0';
                strcat(string4, "*.");
                strcat(string4, "GCD");

                char** fileNameList;
                int fileNameListLength = fileNameListInit(string4, &fileNameList, 0, 0);
                if (fileNameListLength != -1) {
                    strcpy(string1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 617));
                    strcpy(string4, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 600));

                    if (save_file_dialog(string4, fileNameList, string1, fileNameListLength, 168, 80, 0) == 0) {
                        strcat(string1, ".");
                        strcat(string1, "GCD");

                        string4[0] = '\0';
                        strcat(string4, string1);

                        bool shouldSave;
                        if (characterFileExists(string4)) {
                            sprintf(string4, "%s %s",
                                strupr(string1),
                                getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 609));
                            strcpy(string5, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 610));

                            if (dialog_out(string4, dialogBody, 1, 169, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_YES_NO) != 0) {
                                shouldSave = true;
                            } else {
                                shouldSave = false;
                            }
                        } else {
                            shouldSave = true;
                        }

                        if (shouldSave) {
                            skillsSetTagged(gCharacterEditorTempTaggedSkills, 4);
                            traitsSetSelected(gCharacterEditorTempTraits[0], gCharacterEditorTempTraits[1]);

                            string4[0] = '\0';
                            strcat(string4, string1);

                            if (pc_save_data(string4) != 0) {
                                soundPlayFile("iisxxxx1");
                                sprintf(string4, "%s%s!",
                                    strupr(string1),
                                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 611));
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                                rc = 0;
                            } else {
                                sprintf(string4, "%s%s",
                                    strupr(string1),
                                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 607));
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[992], NULL, colorTable[992], DIALOG_BOX_LARGE);
                                rc = 1;
                            }
                        }
                    }

                    fileNameListFree(&fileNameList, 0);
                } else {
                    soundPlayFile("iisxxxx1");

                    // Error reading file list!
                    char* msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 615);
                    dialog_out(msg, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);

                    rc = 0;
                }
            }

            win_draw(win);
        }

        windowDestroy(win);

        for (index = 0; index < 5; index++) {
            internal_free(up[index]);
            internal_free(down[index]);
        }

        return 0;
    }

    // Character Editor is not in creation mode - this button is only for
    // printing character details.

    char pattern[512];
    strcpy(pattern, "*.TXT");

    char** fileNames;
    int filesCount = fileNameListInit(pattern, &fileNames, 0, 0);
    if (filesCount == -1) {
        soundPlayFile("iisxxxx1");

        // Error reading file list!
        strcpy(pattern, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 615));
        dialog_out(pattern, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);
        return 0;
    }

    // PRINT
    char fileName[512];
    strcpy(fileName, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 616));

    char title[512];
    strcpy(title, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 602));

    if (save_file_dialog(title, fileNames, fileName, filesCount, 168, 80, 0) == 0) {
        strcat(fileName, ".TXT");

        title[0] = '\0';
        strcat(title, fileName);

        int v42 = 0;
        if (characterFileExists(title)) {
            sprintf(title,
                "%s %s",
                strupr(fileName),
                getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 609));

            char line2[512];
            strcpy(line2, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 610));

            const char* lines[] = { line2 };
            v42 = dialog_out(title, lines, 1, 169, 126, colorTable[32328], NULL, colorTable[32328], 0x10);
            if (v42) {
                v42 = 1;
            }
        } else {
            v42 = 1;
        }

        if (v42) {
            title[0] = '\0';
            strcpy(title, fileName);

            if (characterPrintToFile(title) != 0) {
                soundPlayFile("iisxxxx1");

                sprintf(title,
                    "%s%s%s",
                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 611),
                    strupr(fileName),
                    "!");
                dialog_out(title, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 1);
            }
        }
    }

    fileNameListFree(&fileNames, 0);

    return 0;
}

// 0x4390B4
bool characterFileExists(const char* fname)
{
    File* stream = fileOpen(fname, "rb");
    if (stream == NULL) {
        return false;
    }

    fileClose(stream);
    return true;
}

// 0x4390D0
int characterPrintToFile(const char* fileName)
{
    File* stream = fileOpen(fileName, "wt");
    if (stream == NULL) {
        return -1;
    }

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    char title1[256];
    char title2[256];
    char title3[256];
    char padding[256];

    // FALLOUT
    strcpy(title1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 620));

    // NOTE: Uninline.
    padding[0] = '\0';
    _AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    fileWriteString(padding, stream);

    // VAULT-13 PERSONNEL RECORD
    strcpy(title1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 621));

    // NOTE: Uninline.
    padding[0] = '\0';
    _AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    fileWriteString(padding, stream);

    int month;
    int day;
    int year;
    gameTimeGetDate(&month, &day, &year);

    sprintf(title1, "%.2d %s %d  %.4d %s",
        day,
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 500 + month - 1),
        year,
        gameTimeGetHour(),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 622));

    // NOTE: Uninline.
    padding[0] = '\0';
    _AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    fileWriteString(padding, stream);

    // Blank line
    fileWriteString("\n", stream);

    // Name
    sprintf(title1,
        "%s %s",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 642),
        critter_name(gDude));

    int paddingLength = 27 - strlen(title1);
    if (paddingLength > 0) {
        // NOTE: Uninline.
        padding[0] = '\0';
        _AddSpaces(padding, paddingLength);

        strcat(title1, padding);
    }

    // Age
    sprintf(title2,
        "%s%s %d",
        title1,
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 643),
        critterGetStat(gDude, STAT_AGE));

    // Gender
    sprintf(title3,
        "%s%s %s",
        title2,
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 644),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 645 + critterGetStat(gDude, STAT_GENDER)));

    fileWriteString(title3, stream);
    fileWriteString("\n", stream);

    sprintf(title1,
        "%s %.2d %s %s ",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 647),
        pcGetStat(PC_STAT_LEVEL),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 648),
        _itostndn(pcGetStat(PC_STAT_EXPERIENCE), title3));

    paddingLength = 12 - strlen(title3);
    if (paddingLength > 0) {
        // NOTE: Uninline.
        padding[0] = '\0';
        _AddSpaces(padding, paddingLength);

        strcat(title1, padding);
    }

    sprintf(title2,
        "%s%s %s",
        title1,
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 649),
        _itostndn(pcGetExperienceForNextLevel(), title3));
    fileWriteString(title2, stream);
    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    // Statistics
    sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 623));

    // Strength / Hit Points / Sequence
    //
    // FIXME: There is bug - it shows strength instead of sequence.
    sprintf(title1,
        "%s %.2d %s %.3d/%.3d %s %.2d",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 624),
        critterGetStat(gDude, STAT_STRENGTH),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 625),
        critter_get_hits(gDude),
        critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 626),
        critterGetStat(gDude, STAT_STRENGTH));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Perception / Armor Class / Healing Rate
    sprintf(title1,
        "%s %.2d %s %.3d %s %.2d",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 627),
        critterGetStat(gDude, STAT_PERCEPTION),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 628),
        critterGetStat(gDude, STAT_ARMOR_CLASS),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 629),
        critterGetStat(gDude, STAT_HEALING_RATE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Endurance / Action Points / Critical Chance
    sprintf(title1,
        "%s %.2d %s %.2d %s %.3d%%",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 630),
        critterGetStat(gDude, STAT_ENDURANCE),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 631),
        critterGetStat(gDude, STAT_MAXIMUM_ACTION_POINTS),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 632),
        critterGetStat(gDude, STAT_CRITICAL_CHANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Charisma / Melee Damage / Carry Weight
    sprintf(title1,
        "%s %.2d %s %.2d %s %.3d lbs.",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 633),
        critterGetStat(gDude, STAT_CHARISMA),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 634),
        critterGetStat(gDude, STAT_MELEE_DAMAGE),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 635),
        critterGetStat(gDude, STAT_CARRY_WEIGHT));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Intelligence / Damage Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 636),
        critterGetStat(gDude, STAT_INTELLIGENCE),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 637),
        critterGetStat(gDude, STAT_DAMAGE_RESISTANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Agility / Radiation Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 638),
        critterGetStat(gDude, STAT_AGILITY),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 639),
        critterGetStat(gDude, STAT_RADIATION_RESISTANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Luck / Poison Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 640),
        critterGetStat(gDude, STAT_LUCK),
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 641),
        critterGetStat(gDude, STAT_POISON_RESISTANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    if (gCharacterEditorTempTraits[0] != -1) {
        // ::: Traits :::
        sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 650));
        fileWriteString(title1, stream);

        // NOTE: The original code does not use loop, or it was optimized away.
        for (int index = 0; index < TRAITS_MAX_SELECTED_COUNT; index++) {
            if (gCharacterEditorTempTraits[index] != -1) {
                sprintf(title1, "  %s", traitGetName(gCharacterEditorTempTraits[index]));
                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        }
    }

    int perk = 0;
    for (; perk < PERK_COUNT; perk++) {
        if (perkGetRank(gDude, perk) != 0) {
            break;
        }
    }

    if (perk < PERK_COUNT) {
        // ::: Perks :::
        sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 651));
        fileWriteString(title1, stream);

        for (perk = 0; perk < PERK_COUNT; perk++) {
            int rank = perkGetRank(gDude, perk);
            if (rank != 0) {
                if (rank == 1) {
                    sprintf(title1, "  %s", perkGetName(perk));
                } else {
                    sprintf(title1, "  %s (%d)", perkGetName(perk), rank);
                }

                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        }
    }

    fileWriteString("\n", stream);

    // ::: Karma :::
    sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 652));
    fileWriteString(title1, stream);

    for (int index = 0; index < gKarmaEntriesLength; index++) {
        KarmaEntry* karmaEntry = &(gKarmaEntries[index]);
        if (karmaEntry->gvar == GVAR_PLAYER_REPUTATION) {
            int reputation = 0;
            for (; reputation < gGenericReputationEntriesLength; reputation++) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);
                if (gGameGlobalVars[GVAR_PLAYER_REPUTATION] >= reputationDescription->threshold) {
                    break;
                }
            }

            if (reputation < gGenericReputationEntriesLength) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);
                sprintf(title1,
                    "  %s: %s (%s)",
                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 125),
                    itoa(gGameGlobalVars[GVAR_PLAYER_REPUTATION], title2, 10),
                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, reputationDescription->name));
                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        } else {
            if (gGameGlobalVars[karmaEntry->gvar] != 0) {
                sprintf(title1, "  %s", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, karmaEntry->name));
                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        }
    }

    bool hasTownReputationHeading = false;
    for (int index = 0; index < TOWN_REPUTATION_COUNT; index++) {
        const TownReputationEntry* pair = &(gTownReputationEntries[index]);
        if (wmAreaIsKnown(pair->city)) {
            if (!hasTownReputationHeading) {
                fileWriteString("\n", stream);

                // ::: Reputation :::
                sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 657));
                fileWriteString(title1, stream);
                hasTownReputationHeading = true;
            }

            wmGetAreaIdxName(pair->city, title2);

            int townReputation = gGameGlobalVars[pair->gvar];

            int townReputationMessageId;

            if (townReputation < -30) {
                townReputationMessageId = 2006; // Vilified
            } else if (townReputation < -15) {
                townReputationMessageId = 2005; // Hated
            } else if (townReputation < 0) {
                townReputationMessageId = 2004; // Antipathy
            } else if (townReputation == 0) {
                townReputationMessageId = 2003; // Neutral
            } else if (townReputation < 15) {
                townReputationMessageId = 2002; // Accepted
            } else if (townReputation < 30) {
                townReputationMessageId = 2001; // Liked
            } else {
                townReputationMessageId = 2000; // Idolized
            }

            sprintf(title1,
                "  %s: %s",
                title2,
                getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, townReputationMessageId));
            fileWriteString(title1, stream);
            fileWriteString("\n", stream);
        }
    }

    bool hasAddictionsHeading = false;
    for (int index = 0; index < ADDICTION_REPUTATION_COUNT; index++) {
        if (gGameGlobalVars[gAddictionReputationVars[index]] != 0) {
            if (!hasAddictionsHeading) {
                fileWriteString("\n", stream);

                // ::: Addictions :::
                sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 656));
                fileWriteString(title1, stream);
                hasAddictionsHeading = true;
            }

            sprintf(title1,
                "  %s",
                getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 1004 + index));
            fileWriteString(title1, stream);
            fileWriteString("\n", stream);
        }
    }

    fileWriteString("\n", stream);

    // ::: Skills ::: / ::: Kills :::
    sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 653));
    fileWriteString(title1, stream);

    int killType = 0;
    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        sprintf(title1, "%s ", skillGetName(skill));

        // NOTE: Uninline.
        _AddDots(title1 + strlen(title1), 16 - strlen(title1));

        bool hasKillType = false;

        for (; killType < KILL_TYPE_COUNT; killType++) {
            int killsCount = critter_kill_count(killType);
            if (killsCount > 0) {
                sprintf(title2, "%s ", critter_kill_name(killType));

                // NOTE: Uninline.
                _AddDots(title2 + strlen(title2), 16 - strlen(title2));

                sprintf(title3,
                    "  %s %.3d%%        %s %.3d\n",
                    title1,
                    skillGetValue(gDude, skill),
                    title2,
                    killsCount);
                hasKillType = true;
                break;
            }
        }

        if (!hasKillType) {
            sprintf(title3,
                "  %s %.3d%%\n",
                title1,
                skillGetValue(gDude, skill));
        }
    }

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    // ::: Inventory :::
    sprintf(title1, "%s\n", getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 654));
    fileWriteString(title1, stream);

    Inventory* inventory = &(gDude->data.inventory);
    for (int index = 0; index < inventory->length; index += 3) {
        title1[0] = '\0';

        for (int column = 0; column < 3; column++) {
            int inventoryItemIndex = index + column;
            if (inventoryItemIndex >= inventory->length) {
                break;
            }

            InventoryItem* inventoryItem = &(inventory->items[inventoryItemIndex]);

            sprintf(title2,
                "  %sx %s",
                _itostndn(inventoryItem->quantity, title3),
                objectGetName(inventoryItem->item));

            int length = 25 - strlen(title2);
            if (length < 0) {
                length = 0;
            }

            _AddSpaces(title2, length);

            strcat(title1, title2);
        }

        strcat(title1, "\n");
        fileWriteString(title1, stream);
    }

    fileWriteString("\n", stream);

    // Total Weight:
    sprintf(title1,
        "%s %d lbs.",
        getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 655),
        objectGetInventoryWeight(gDude));
    fileWriteString(title1, stream);

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);
    fileWriteString("\n", stream);
    fileClose(stream);

    return 0;
}

// 0x43A55C
char* _AddSpaces(char* string, int length)
{
    char* pch = string + strlen(string);

    for (int index = 0; index < length; index++) {
        *pch++ = ' ';
    }

    *pch = '\0';

    return string;
}

// NOTE: Inlined.
//
// 0x43A58C
char* _AddDots(char* string, int length)
{
    char* pch = string + strlen(string);

    for (int index = 0; index < length; index++) {
        *pch++ = '.';
    }

    *pch = '\0';

    return string;
}

// 0x43A4BC
void characterEditorResetScreen()
{
    characterEditorSelectedItem = 0;
    gCharacterEditorCurrentSkill = 0;
    gCharacterEditorSkillValueAdjustmentSliderY = 27;
    characterEditorWindowSelectedFolder = 0;

    if (gCharacterEditorIsCreationMode) {
        characterEditorDrawBigNumber(126, 282, 0, gCharacterEditorRemainingCharacterPoints, 0, gCharacterEditorWindow);
    } else {
        characterEditorDrawFolders();
        characterEditorDrawPcStats();
    }

    characterEditorDrawName();
    characterEditorDrawAge();
    characterEditorDrawGender();
    characterEditorDrawOptionalTraits();
    characterEditorDrawSkills(0);
    characterEditorDrawPrimaryStat(7, 0, 0);
    characterEditorDrawDerivedStats();
    characterEditorDrawCard();
    win_draw(gCharacterEditorWindow);
}

// 0x43A5BC
void characterEditorRegisterInfoAreas()
{
    buttonCreate(gCharacterEditorWindow, 19, 38, 125, 227, -1, -1, 525, -1, NULL, NULL, NULL, 0);
    buttonCreate(gCharacterEditorWindow, 28, 280, 124, 32, -1, -1, 526, -1, NULL, NULL, NULL, 0);

    if (gCharacterEditorIsCreationMode) {
        buttonCreate(gCharacterEditorWindow, 52, 324, 169, 20, -1, -1, 533, -1, NULL, NULL, NULL, 0);
        buttonCreate(gCharacterEditorWindow, 47, 353, 245, 100, -1, -1, 534, -1, NULL, NULL, NULL, 0);
    } else {
        buttonCreate(gCharacterEditorWindow, 28, 363, 283, 105, -1, -1, 527, -1, NULL, NULL, NULL, 0);
    }

    buttonCreate(gCharacterEditorWindow, 191, 41, 122, 110, -1, -1, 528, -1, NULL, NULL, NULL, 0);
    buttonCreate(gCharacterEditorWindow, 191, 175, 122, 135, -1, -1, 529, -1, NULL, NULL, NULL, 0);
    buttonCreate(gCharacterEditorWindow, 376, 5, 223, 20, -1, -1, 530, -1, NULL, NULL, NULL, 0);
    buttonCreate(gCharacterEditorWindow, 370, 27, 223, 195, -1, -1, 531, -1, NULL, NULL, NULL, 0);
    buttonCreate(gCharacterEditorWindow, 396, 228, 171, 25, -1, -1, 532, -1, NULL, NULL, NULL, 0);
}

// NOTE: Inlined.
//
// 0x43A79C
int CheckValidPlayer()
{
    int stat;

    critterUpdateDerivedStats(gDude);
    pcStatsReset();

    for (stat = 0; stat < SAVEABLE_STAT_COUNT; stat++) {
        critterSetBonusStat(gDude, stat, 0);
    }

    perkResetRanks();
    critterUpdateDerivedStats(gDude);

    return 1;
}

// copy character to editor
//
// 0x43A7DC
void characterEditorSavePlayer()
{
    Proto* proto;
    protoGetProto(gDude->pid, &proto);
    critter_copy(&gCharacterEditorDudeDataBackup, &(proto->critter.data));

    gCharacterEditorHitPointsBackup = critter_get_hits(gDude);

    strncpy(gCharacterEditorNameBackup, critter_name(gDude), 32);

    gCharacterEditorLastLevelBackup = gCharacterEditorLastLevel;

    // NOTE: Uninline.
    push_perks();

    gCharacterEditorHasFreePerkBackup = gCharacterEditorHasFreePerk;

    gCharacterEditorUnspentSkillPointsBackup = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);

    skillsGetTagged(gCharacterEditorTaggedSkillsBackup, NUM_TAGGED_SKILLS);

    traitsGetSelected(&(gCharacterEditorOptionalTraitsBackup[0]), &(gCharacterEditorOptionalTraitsBackup[1]));

    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        gCharacterEditorSkillsBackup[skill] = skillGetValue(gDude, skill);
    }
}

// copy editor to character
//
// 0x43A8BC
void characterEditorRestorePlayer()
{
    Proto* proto;
    int cur_hp;

    _pop_perks();

    protoGetProto(gDude->pid, &proto);
    critter_copy(&(proto->critter.data), &gCharacterEditorDudeDataBackup);

    critter_pc_set_name(gCharacterEditorNameBackup);

    gCharacterEditorLastLevel = gCharacterEditorLastLevelBackup;
    gCharacterEditorHasFreePerk = gCharacterEditorHasFreePerkBackup;

    pcSetStat(PC_STAT_UNSPENT_SKILL_POINTS, gCharacterEditorUnspentSkillPointsBackup);

    skillsSetTagged(gCharacterEditorTaggedSkillsBackup, NUM_TAGGED_SKILLS);

    traitsSetSelected(gCharacterEditorOptionalTraitsBackup[0], gCharacterEditorOptionalTraitsBackup[1]);

    skillsGetTagged(gCharacterEditorTempTaggedSkills, NUM_TAGGED_SKILLS);

    // NOTE: Uninline.
    gCharacterEditorTaggedSkillCount = tagskl_free();

    traitsGetSelected(&(gCharacterEditorTempTraits[0]), &(gCharacterEditorTempTraits[1]));

    // NOTE: Uninline.
    gCharacterEditorTempTraitCount = get_trait_count();

    critterUpdateDerivedStats(gDude);

    cur_hp = critter_get_hits(gDude);
    critter_adjust_hits(gDude, gCharacterEditorHitPointsBackup - cur_hp);
}

// 0x43A9CC
char* _itostndn(int value, char* dest)
{
    int v16[7];
    static_assert(sizeof(v16) == sizeof(dword_431DD4), "wrong size");
    memcpy(v16, dword_431DD4, sizeof(v16));

    char* savedDest = dest;

    if (value != 0) {
        *dest = '\0';

        bool v3 = false;
        for (int index = 0; index < 7; index++) {
            int v18 = value / v16[index];
            if (v18 > 0 || v3) {
                char temp[64]; // TODO: Size is probably wrong.
                itoa(v18, temp, 10);
                strcat(dest, temp);

                v3 = true;

                value -= v16[index] * v18;

                if (index == 0 || index == 3) {
                    strcat(dest, ",");
                }
            }
        }
    } else {
        strcpy(dest, "0");
    }

    return savedDest;
}

// 0x43AAEC
int characterEditorDrawCardWithOptions(int graphicId, const char* name, const char* attributes, char* description)
{
    CacheEntry* graphicHandle;
    Size size;
    int fid;
    unsigned char* buf;
    unsigned char* ptr;
    int v9;
    int x;
    int y;
    short beginnings[WORD_WRAP_MAX_COUNT];
    short beginningsCount;

    fid = art_id(OBJ_TYPE_SKILLDEX, graphicId, 0, 0, 0);
    buf = art_lock(fid, &graphicHandle, &(size.width), &(size.height));
    if (buf == NULL) {
        return -1;
    }

    blitBufferToBuffer(buf, size.width, size.height, size.width, gCharacterEditorWindowBuffer + 640 * 309 + 484, 640);

    v9 = 150;
    ptr = buf;
    for (y = 0; y < size.height; y++) {
        for (x = 0; x < size.width; x++) {
            if (_HighRGB_(*ptr) < 2 && v9 >= x) {
                v9 = x;
            }
            ptr++;
        }
    }

    v9 -= 8;
    if (v9 < 0) {
        v9 = 0;
    }

    fontSetCurrent(102);

    fontDrawText(gCharacterEditorWindowBuffer + 640 * 272 + 348, name, 640, 640, colorTable[0]);
    int nameFontLineHeight = fontGetLineHeight();
    if (attributes != NULL) {
        int nameWidth = fontGetStringWidth(name);

        fontSetCurrent(101);
        int attributesFontLineHeight = fontGetLineHeight();
        fontDrawText(gCharacterEditorWindowBuffer + 640 * (268 + nameFontLineHeight - attributesFontLineHeight) + 348 + nameWidth + 8, attributes, 640, 640, colorTable[0]);
    }

    y = nameFontLineHeight;
    windowDrawLine(gCharacterEditorWindow, 348, y + 272, 613, y + 272, colorTable[0]);
    windowDrawLine(gCharacterEditorWindow, 348, y + 273, 613, y + 273, colorTable[0]);

    fontSetCurrent(101);

    int descriptionFontLineHeight = fontGetLineHeight();

    if (wordWrap(description, v9 + 136, beginnings, &beginningsCount) != 0) {
        // TODO: Leaking graphic handle.
        return -1;
    }

    y = 315;
    for (short i = 0; i < beginningsCount - 1; i++) {
        short beginning = beginnings[i];
        short ending = beginnings[i + 1];
        char c = description[ending];
        description[ending] = '\0';
        fontDrawText(gCharacterEditorWindowBuffer + 640 * y + 348, description + beginning, 640, 640, colorTable[0]);
        description[ending] = c;
        y += descriptionFontLineHeight;
    }

    if (graphicId != gCharacterEditorCardFrmId || strcmp(name, gCharacterEditorCardTitle) != 0) {
        if (gCharacterEditorCardDrawn) {
            soundPlayFile("isdxxxx1");
        }
    }

    strcpy(gCharacterEditorCardTitle, name);
    gCharacterEditorCardFrmId = graphicId;
    gCharacterEditorCardDrawn = true;

    art_ptr_unlock(graphicHandle);

    return 0;
}

// 0x43AE8
void characterEditorHandleFolderButtonPressed()
{
    mouse_get_position(&gCharacterEditorMouseX, &gCharacterEditorMouseY);
    soundPlayFile("ib3p1xx1");

    if (gCharacterEditorMouseX >= 208) {
        characterEditorSelectedItem = 41;
        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KILLS;
    } else if (gCharacterEditorMouseX > 110) {
        characterEditorSelectedItem = 42;
        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KARMA;
    } else {
        characterEditorSelectedItem = 40;
        characterEditorWindowSelectedFolder = EDITOR_FOLDER_PERKS;
    }

    characterEditorDrawFolders();
    characterEditorDrawCard();
}

// 0x43AF40
void characterEditorHandleInfoButtonPressed(int eventCode)
{
    mouse_get_position(&gCharacterEditorMouseX, &gCharacterEditorMouseY);

    switch (eventCode) {
    case 525:
        if (1) {
            // TODO: Original code is slightly different.
            double mouseY = gCharacterEditorMouseY;
            for (int index = 0; index < 7; index++) {
                double buttonTop = gCharacterEditorPrimaryStatY[index];
                double buttonBottom = gCharacterEditorPrimaryStatY[index] + 22;
                double allowance = 5.0 - index * 0.25;
                if (mouseY >= buttonTop - allowance && mouseY <= buttonBottom + allowance) {
                    characterEditorSelectedItem = index;
                    break;
                }
            }
        }
        break;
    case 526:
        if (gCharacterEditorIsCreationMode) {
            characterEditorSelectedItem = 7;
        } else {
            int offset = gCharacterEditorMouseY - 280;
            if (offset < 0) {
                offset = 0;
            }

            characterEditorSelectedItem = offset / 10 + 7;
        }
        break;
    case 527:
        if (!gCharacterEditorIsCreationMode) {
            fontSetCurrent(101);
            int offset = gCharacterEditorMouseY - 364;
            if (offset < 0) {
                offset = 0;
            }
            characterEditorSelectedItem = offset / (fontGetLineHeight() + 1) + 10;
        }
        break;
    case 528:
        if (1) {
            int offset = gCharacterEditorMouseY - 41;
            if (offset < 0) {
                offset = 0;
            }

            characterEditorSelectedItem = offset / 13 + 43;
        }
        break;
    case 529: {
        int offset = gCharacterEditorMouseY - 175;
        if (offset < 0) {
            offset = 0;
        }

        characterEditorSelectedItem = offset / 13 + 51;
        break;
    }
    case 530:
        characterEditorSelectedItem = 80;
        break;
    case 531:
        if (1) {
            int offset = gCharacterEditorMouseY - 27;
            if (offset < 0) {
                offset = 0;
            }

            gCharacterEditorCurrentSkill = (int)(offset * 0.092307694);
            if (gCharacterEditorCurrentSkill >= 18) {
                gCharacterEditorCurrentSkill = 17;
            }

            characterEditorSelectedItem = gCharacterEditorCurrentSkill + 61;
        }
        break;
    case 532:
        characterEditorSelectedItem = 79;
        break;
    case 533:
        characterEditorSelectedItem = 81;
        break;
    case 534:
        if (1) {
            fontSetCurrent(101);

            // TODO: Original code is slightly different.
            double mouseY = gCharacterEditorMouseY;
            double fontLineHeight = fontGetLineHeight();
            double y = 353.0;
            double step = fontGetLineHeight() + 3 + 0.56;
            int index;
            for (index = 0; index < 8; index++) {
                if (mouseY >= y - 4.0 && mouseY <= y + fontLineHeight) {
                    break;
                }
                y += step;
            }

            if (index == 8) {
                index = 7;
            }

            characterEditorSelectedItem = index + 82;
            if (gCharacterEditorMouseX >= 169) {
                characterEditorSelectedItem += 8;
            }
        }
        break;
    }

    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
    characterEditorDrawOptionalTraits();
    characterEditorDrawSkills(0);
    characterEditorDrawPcStats();
    characterEditorDrawFolders();
    characterEditorDrawDerivedStats();
    characterEditorDrawCard();
}

// 0x43B230
void characterEditorHandleAdjustSkillButtonPressed(int keyCode)
{
    if (gCharacterEditorIsCreationMode) {
        return;
    }

    int unspentSp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
    _repFtime = 4;

    bool isUsingKeyboard = false;
    int rc = 0;

    switch (keyCode) {
    case KEY_PLUS:
    case KEY_UPPERCASE_N:
    case KEY_ARROW_RIGHT:
        isUsingKeyboard = true;
        keyCode = 521;
        break;
    case KEY_MINUS:
    case KEY_UPPERCASE_J:
    case KEY_ARROW_LEFT:
        isUsingKeyboard = true;
        keyCode = 523;
        break;
    }

    char title[64];
    char body1[64];
    char body2[64];

    const char* body[] = {
        body1,
        body2,
    };

    int repeatDelay = 0;
    for (;;) {
        _frame_time = _get_time();
        if (repeatDelay <= dbl_5018F0) {
            repeatDelay++;
        }

        if (repeatDelay == 1 || repeatDelay > dbl_5018F0) {
            if (repeatDelay > dbl_5018F0) {
                _repFtime++;
                if (_repFtime > 24) {
                    _repFtime = 24;
                }
            }

            rc = 1;
            if (keyCode == 521) {
                if (pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS) > 0) {
                    if (skillAdd(gDude, gCharacterEditorCurrentSkill) == -3) {
                        soundPlayFile("iisxxxx1");

                        sprintf(title, "%s:", skillGetName(gCharacterEditorCurrentSkill));
                        // At maximum level.
                        strcpy(body1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 132));
                        // Unable to increment it.
                        strcpy(body2, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 133));
                        dialog_out(title, body, 2, 192, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                        rc = -1;
                    }
                } else {
                    soundPlayFile("iisxxxx1");

                    // Not enough skill points available.
                    strcpy(title, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 136));
                    dialog_out(title, NULL, 0, 192, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                    rc = -1;
                }
            } else if (keyCode == 523) {
                if (skillGetValue(gDude, gCharacterEditorCurrentSkill) <= gCharacterEditorSkillsBackup[gCharacterEditorCurrentSkill]) {
                    rc = 0;
                } else {
                    if (skillSub(gDude, gCharacterEditorCurrentSkill) == -2) {
                        rc = 0;
                    }
                }

                if (rc == 0) {
                    soundPlayFile("iisxxxx1");

                    sprintf(title, "%s:", skillGetName(gCharacterEditorCurrentSkill));
                    // At minimum level.
                    strcpy(body1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 134));
                    // Unable to decrement it.
                    strcpy(body2, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 135));
                    dialog_out(title, body, 2, 192, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                    rc = -1;
                }
            }

            characterEditorSelectedItem = gCharacterEditorCurrentSkill + 61;
            characterEditorDrawCard();
            characterEditorDrawSkills(1);

            int flags;
            if (rc == 1) {
                flags = ANIMATE;
            } else {
                flags = 0;
            }

            characterEditorDrawBigNumber(522, 228, flags, pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS), unspentSp, gCharacterEditorWindow);

            win_draw(gCharacterEditorWindow);
        }

        if (!isUsingKeyboard) {
            unspentSp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
            if (repeatDelay >= dbl_5018F0) {
                while (getTicksSince(_frame_time) < 1000 / _repFtime) {
                }
            } else {
                while (getTicksSince(_frame_time) < 1000 / 24) {
                }
            }

            int keyCode = _get_input();
            if (keyCode != 522 && keyCode != 524 && rc != -1) {
                continue;
            }
        }
        return;
    }
}

// 0x43B64C
int tagskl_free()
{
    int taggedSkillCount;
    int index;

    taggedSkillCount = 0;
    for (index = 3; index >= 0; index--) {
        if (gCharacterEditorTempTaggedSkills[index] != -1) {
            break;
        }

        taggedSkillCount++;
    }

    if (gCharacterEditorIsCreationMode == 1) {
        taggedSkillCount--;
    }

    return taggedSkillCount;
}

// 0x43B67C
void characterEditorToggleTaggedSkill(int skill)
{
    int insertionIndex;

    // NOTE: Uninline.
    gCharacterEditorOldTaggedSkillCount = tagskl_free();

    if (skill == gCharacterEditorTempTaggedSkills[0] || skill == gCharacterEditorTempTaggedSkills[1] || skill == gCharacterEditorTempTaggedSkills[2] || skill == gCharacterEditorTempTaggedSkills[3]) {
        if (skill == gCharacterEditorTempTaggedSkills[0]) {
            gCharacterEditorTempTaggedSkills[0] = gCharacterEditorTempTaggedSkills[1];
            gCharacterEditorTempTaggedSkills[1] = gCharacterEditorTempTaggedSkills[2];
            gCharacterEditorTempTaggedSkills[2] = -1;
        } else if (skill == gCharacterEditorTempTaggedSkills[1]) {
            gCharacterEditorTempTaggedSkills[1] = gCharacterEditorTempTaggedSkills[2];
            gCharacterEditorTempTaggedSkills[2] = -1;
        } else {
            gCharacterEditorTempTaggedSkills[2] = -1;
        }
    } else {
        if (gCharacterEditorTaggedSkillCount > 0) {
            insertionIndex = 0;
            for (int index = 0; index < 3; index++) {
                if (gCharacterEditorTempTaggedSkills[index] == -1) {
                    break;
                }
                insertionIndex++;
            }
            gCharacterEditorTempTaggedSkills[insertionIndex] = skill;
        } else {
            soundPlayFile("iisxxxx1");

            char line1[128];
            strcpy(line1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 140));

            char line2[128];
            strcpy(line2, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 141));

            const char* lines[] = { line2 };
            dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
        }
    }

    // NOTE: Uninline.
    gCharacterEditorTaggedSkillCount = tagskl_free();

    characterEditorSelectedItem = skill + 61;
    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
    characterEditorDrawDerivedStats();
    characterEditorDrawSkills(2);
    characterEditorDrawCard();
    win_draw(gCharacterEditorWindow);
}

// 0x43B8A8
void characterEditorDrawOptionalTraits()
{
    int v0 = -1;
    int i;
    int color;
    const char* traitName;
    double step;
    double y;

    if (gCharacterEditorIsCreationMode != 1) {
        return;
    }

    if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
        v0 = characterEditorSelectedItem - 82;
    }

    blitBufferToBuffer(gCharacterEditorWindowBackgroundBuffer + 640 * 353 + 47, 245, 100, 640, gCharacterEditorWindowBuffer + 640 * 353 + 47, 640);

    fontSetCurrent(101);

    traitsSetSelected(gCharacterEditorTempTraits[0], gCharacterEditorTempTraits[1]);

    step = fontGetLineHeight() + 3 + 0.56;
    y = 353;
    for (i = 0; i < 8; i++) {
        if (i == v0) {
            if (i != gCharacterEditorTempTraits[0] && i != gCharacterEditorTempTraits[1]) {
                color = colorTable[32747];
            } else {
                color = colorTable[32767];
            }

            gCharacterEditorFolderCardFrmId = traitGetFrmId(i);
            gCharacterEditorFolderCardTitle = traitGetName(i);
            gCharacterEditorFolderCardSubtitle = NULL;
            gCharacterEditorFolderCardDescription = traitGetDescription(i);
        } else {
            if (i != gCharacterEditorTempTraits[0] && i != gCharacterEditorTempTraits[1]) {
                color = colorTable[992];
            } else {
                color = colorTable[21140];
            }
        }

        traitName = traitGetName(i);
        fontDrawText(gCharacterEditorWindowBuffer + 640 * (int)y + 47, traitName, 640, 640, color);
        y += step;
    }

    y = 353;
    for (i = 8; i < 16; i++) {
        if (i == v0) {
            if (i != gCharacterEditorTempTraits[0] && i != gCharacterEditorTempTraits[1]) {
                color = colorTable[32747];
            } else {
                color = colorTable[32767];
            }

            gCharacterEditorFolderCardFrmId = traitGetFrmId(i);
            gCharacterEditorFolderCardTitle = traitGetName(i);
            gCharacterEditorFolderCardSubtitle = NULL;
            gCharacterEditorFolderCardDescription = traitGetDescription(i);
        } else {
            if (i != gCharacterEditorTempTraits[0] && i != gCharacterEditorTempTraits[1]) {
                color = colorTable[992];
            } else {
                color = colorTable[21140];
            }
        }

        traitName = traitGetName(i);
        fontDrawText(gCharacterEditorWindowBuffer + 640 * (int)y + 199, traitName, 640, 640, color);
        y += step;
    }
}

// NOTE: Inlined.
//
// 0x43BAE8
int get_trait_count()
{
    int traitCount;
    int index;

    traitCount = 0;
    for (index = 1; index >= 0; index--) {
        if (gCharacterEditorTempTraits[index] != -1) {
            break;
        }

        traitCount++;
    }

    return traitCount;
}

// 0x43BB0C
void characterEditorToggleOptionalTrait(int trait)
{
    if (trait == gCharacterEditorTempTraits[0] || trait == gCharacterEditorTempTraits[1]) {
        if (trait == gCharacterEditorTempTraits[0]) {
            gCharacterEditorTempTraits[0] = gCharacterEditorTempTraits[1];
            gCharacterEditorTempTraits[1] = -1;
        } else {
            gCharacterEditorTempTraits[1] = -1;
        }
    } else {
        if (gCharacterEditorTempTraitCount == 0) {
            soundPlayFile("iisxxxx1");

            char line1[128];
            strcpy(line1, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 148));

            char line2[128];
            strcpy(line2, getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 149));

            const char* lines = { line2 };
            dialog_out(line1, &lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
        } else {
            for (int index = 0; index < 2; index++) {
                if (gCharacterEditorTempTraits[index] == -1) {
                    gCharacterEditorTempTraits[index] = trait;
                    break;
                }
            }
        }
    }

    // NOTE: Uninline.
    gCharacterEditorTempTraitCount = get_trait_count();

    characterEditorSelectedItem = trait + EDITOR_FIRST_TRAIT;

    characterEditorDrawOptionalTraits();
    characterEditorDrawSkills(0);
    critterUpdateDerivedStats(gDude);
    characterEditorDrawBigNumber(126, 282, 0, gCharacterEditorRemainingCharacterPoints, 0, gCharacterEditorWindow);
    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, false, 0);
    characterEditorDrawDerivedStats();
    characterEditorDrawCard();
    win_draw(gCharacterEditorWindow);
}

// 0x43BCE0
void characterEditorDrawKarmaFolder()
{
    char* msg;
    char formattedText[256];

    characterEditorFolderViewClear();

    bool hasSelection = false;
    for (int index = 0; index < gKarmaEntriesLength; index++) {
        KarmaEntry* karmaDescription = &(gKarmaEntries[index]);
        if (karmaDescription->gvar == GVAR_PLAYER_REPUTATION) {
            int reputation;
            for (reputation = 0; reputation < gGenericReputationEntriesLength; reputation++) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);
                if (gGameGlobalVars[GVAR_PLAYER_REPUTATION] >= reputationDescription->threshold) {
                    break;
                }
            }

            if (reputation != gGenericReputationEntriesLength) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);

                char reputationValue[32];
                itoa(gGameGlobalVars[GVAR_PLAYER_REPUTATION], reputationValue, 10);

                sprintf(formattedText,
                    "%s: %s (%s)",
                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 125),
                    reputationValue,
                    getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, reputationDescription->name));

                if (characterEditorFolderViewDrawString(formattedText)) {
                    gCharacterEditorFolderCardFrmId = karmaDescription->art_num;
                    gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 125);
                    gCharacterEditorFolderCardSubtitle = NULL;
                    gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, karmaDescription->description);
                    hasSelection = true;
                }
            }
        } else {
            if (gGameGlobalVars[karmaDescription->gvar] != 0) {
                msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, karmaDescription->name);
                if (characterEditorFolderViewDrawString(msg)) {
                    gCharacterEditorFolderCardFrmId = karmaDescription->art_num;
                    gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, karmaDescription->name);
                    gCharacterEditorFolderCardSubtitle = NULL;
                    gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, karmaDescription->description);
                    hasSelection = true;
                }
            }
        }
    }

    bool hasTownReputationHeading = false;
    for (int index = 0; index < TOWN_REPUTATION_COUNT; index++) {
        const TownReputationEntry* pair = &(gTownReputationEntries[index]);
        if (wmAreaIsKnown(pair->city)) {
            if (!hasTownReputationHeading) {
                msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 4000);
                if (characterEditorFolderViewDrawHeading(msg)) {
                    gCharacterEditorFolderCardFrmId = 48;
                    gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 4000);
                    gCharacterEditorFolderCardSubtitle = NULL;
                    gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 4100);
                }
                hasTownReputationHeading = true;
            }

            char cityShortName[40];
            wmGetAreaIdxName(pair->city, cityShortName);

            int townReputation = gGameGlobalVars[pair->gvar];

            int townReputationGraphicId;
            int townReputationBaseMessageId;

            if (townReputation < -30) {
                townReputationGraphicId = 150;
                townReputationBaseMessageId = 2006; // Vilified
            } else if (townReputation < -15) {
                townReputationGraphicId = 153;
                townReputationBaseMessageId = 2005; // Hated
            } else if (townReputation < 0) {
                townReputationGraphicId = 153;
                townReputationBaseMessageId = 2004; // Antipathy
            } else if (townReputation == 0) {
                townReputationGraphicId = 141;
                townReputationBaseMessageId = 2003; // Neutral
            } else if (townReputation < 15) {
                townReputationGraphicId = 137;
                townReputationBaseMessageId = 2002; // Accepted
            } else if (townReputation < 30) {
                townReputationGraphicId = 137;
                townReputationBaseMessageId = 2001; // Liked
            } else {
                townReputationGraphicId = 135;
                townReputationBaseMessageId = 2000; // Idolized
            }

            msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, townReputationBaseMessageId);
            sprintf(formattedText,
                "%s: %s",
                cityShortName,
                msg);

            if (characterEditorFolderViewDrawString(formattedText)) {
                gCharacterEditorFolderCardFrmId = townReputationGraphicId;
                gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, townReputationBaseMessageId);
                gCharacterEditorFolderCardSubtitle = NULL;
                gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, townReputationBaseMessageId + 100);
                hasSelection = 1;
            }
        }
    }

    bool hasAddictionsHeading = false;
    for (int index = 0; index < ADDICTION_REPUTATION_COUNT; index++) {
        if (gGameGlobalVars[gAddictionReputationVars[index]] != 0) {
            if (!hasAddictionsHeading) {
                // Addictions
                msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 4001);
                if (characterEditorFolderViewDrawHeading(msg)) {
                    gCharacterEditorFolderCardFrmId = 53;
                    gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 4001);
                    gCharacterEditorFolderCardSubtitle = NULL;
                    gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 4101);
                    hasSelection = 1;
                }
                hasAddictionsHeading = true;
            }

            msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 1004 + index);
            if (characterEditorFolderViewDrawString(msg)) {
                gCharacterEditorFolderCardFrmId = gAddictionReputationFrmIds[index];
                gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 1004 + index);
                gCharacterEditorFolderCardSubtitle = NULL;
                gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 1104 + index);
                hasSelection = 1;
            }
        }
    }

    if (!hasSelection) {
        gCharacterEditorFolderCardFrmId = 47;
        gCharacterEditorFolderCardTitle = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 125);
        gCharacterEditorFolderCardSubtitle = NULL;
        gCharacterEditorFolderCardDescription = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 128);
    }
}

// 0x43C1B0
int characterEditorSave(File* stream)
{
    if (fileWriteInt32(stream, gCharacterEditorLastLevel) == -1)
        return -1;
    if (fileWriteUInt8(stream, gCharacterEditorHasFreePerk) == -1)
        return -1;

    return 0;
}

// 0x43C1E0
int characterEditorLoad(File* stream)
{
    if (fileReadInt32(stream, &gCharacterEditorLastLevel) == -1)
        return -1;
    if (fileReadUInt8(stream, &gCharacterEditorHasFreePerk) == -1)
        return -1;

    return 0;
}

// 0x43C20C
void characterEditorReset()
{
    gCharacterEditorRemainingCharacterPoints = 5;
    gCharacterEditorLastLevel = 1;
}

// level up if needed
//
// 0x43C228
int characterEditorUpdateLevel()
{
    int level = pcGetStat(PC_STAT_LEVEL);
    if (level != gCharacterEditorLastLevel && level <= PC_LEVEL_MAX) {
        for (int nextLevel = gCharacterEditorLastLevel + 1; nextLevel <= level; nextLevel++) {
            int sp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
            sp += 5;
            sp += critterGetBaseStatWithTraitModifier(gDude, STAT_INTELLIGENCE) * 2;
            sp += perkGetRank(gDude, PERK_EDUCATED) * 2;
            sp += traitIsSelected(TRAIT_SKILLED) * 5;
            if (traitIsSelected(TRAIT_GIFTED)) {
                sp -= 5;
                if (sp < 0) {
                    sp = 0;
                }
            }
            if (sp > 99) {
                sp = 99;
            }

            pcSetStat(PC_STAT_UNSPENT_SKILL_POINTS, sp);

            // NOTE: Uninline.
            int selectedPerksCount = PerkCount();

            if (selectedPerksCount < 37) {
                int progression = 3;
                if (traitIsSelected(TRAIT_SKILLED)) {
                    progression += 1;
                }

                if (nextLevel % progression == 0) {
                    gCharacterEditorHasFreePerk = 1;
                }
            }
        }
    }

    if (gCharacterEditorHasFreePerk != 0) {
        characterEditorWindowSelectedFolder = 0;
        characterEditorDrawFolders();
        win_draw(gCharacterEditorWindow);

        int rc = perkDialogShow();
        if (rc == -1) {
            debugPrint("\n *** Error running perks dialog! ***\n");
            return -1;
        } else if (rc == 0) {
            characterEditorDrawFolders();
        } else if (rc == 1) {
            characterEditorDrawFolders();
            gCharacterEditorHasFreePerk = 0;
        }
    }

    gCharacterEditorLastLevel = level;

    return 1;
}

// 0x43C398
void perkDialogRefreshPerks()
{
    blitBufferToBuffer(
        gPerkDialogBackgroundBuffer + 280,
        293,
        PERK_WINDOW_HEIGHT,
        PERK_WINDOW_WIDTH,
        gPerkDialogWindowBuffer + 280,
        PERK_WINDOW_WIDTH);

    perkDialogDrawPerks();

    // NOTE: Original code is slightly different, but basically does the same thing.
    int perk = gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value;
    int perkFrmId = perkGetFrmId(perk);
    char* perkName = perkGetName(perk);
    char* perkDescription = perkGetDescription(perk);
    char* perkRank = NULL;
    char perkRankBuffer[32];

    int rank = perkGetRank(gDude, perk);
    if (rank != 0) {
        sprintf(perkRankBuffer, "(%d)", rank);
        perkRank = perkRankBuffer;
    }

    perkDialogDrawCard(perkFrmId, perkName, perkRank, perkDescription);

    win_draw(gPerkDialogWindow);
}

// 0x43C4F0
int perkDialogShow()
{
    gPerkDialogTopLine = 0;
    gPerkDialogCurrentLine = 0;
    gPerkDialogCardFrmId = -1;
    gPerkDialogCardTitle[0] = '\0';
    gPerkDialogCardDrawn = false;

    CacheEntry* backgroundFrmHandle;
    int backgroundWidth;
    int backgroundHeight;
    int fid = art_id(OBJ_TYPE_INTERFACE, 86, 0, 0, 0);
    gPerkDialogBackgroundBuffer = art_lock(fid, &backgroundFrmHandle, &backgroundWidth, &backgroundHeight);
    if (gPerkDialogBackgroundBuffer == NULL) {
        debugPrint("\n *** Error running perks dialog window ***\n");
        return -1;
    }

    int perkWindowX = PERK_WINDOW_X;
    int perkWindowY = PERK_WINDOW_Y;
    gPerkDialogWindow = windowCreate(perkWindowX, perkWindowY, PERK_WINDOW_WIDTH, PERK_WINDOW_HEIGHT, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (gPerkDialogWindow == -1) {
        art_ptr_unlock(backgroundFrmHandle);
        debugPrint("\n *** Error running perks dialog window ***\n");
        return -1;
    }

    gPerkDialogWindowBuffer = windowGetBuffer(gPerkDialogWindow);
    memcpy(gPerkDialogWindowBuffer, gPerkDialogBackgroundBuffer, PERK_WINDOW_WIDTH * PERK_WINDOW_HEIGHT);

    int btn;

    btn = buttonCreate(gPerkDialogWindow,
        48,
        186,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(gPerkDialogWindow,
        153,
        186,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        502,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(gPerkDialogWindow,
        25,
        46,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_UP_ARROW_ON].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        -1,
        574,
        572,
        574,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_UP_ARROW_OFF],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_UP_ARROW_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, NULL);
    }

    btn = buttonCreate(gPerkDialogWindow,
        25,
        47 + gCharacterEditorFrmSize[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_UP_ARROW_ON].width,
        gCharacterEditorFrmSize[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        -1,
        575,
        573,
        575,
        gCharacterEditorFrmData[EDITOR_GRAPHIC_DOWN_ARROW_OFF],
        gCharacterEditorFrmData[EDITOR_GRAPHIC_DOWN_ARROW_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, NULL);
    }

    buttonCreate(gPerkDialogWindow,
        PERK_WINDOW_LIST_X,
        PERK_WINDOW_LIST_Y,
        PERK_WINDOW_LIST_WIDTH,
        PERK_WINDOW_LIST_HEIGHT,
        -1,
        -1,
        -1,
        501,
        NULL,
        NULL,
        NULL,
        BUTTON_FLAG_TRANSPARENT);

    fontSetCurrent(103);

    const char* msg;

    // PICK A NEW PERK
    msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 152);
    fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

    // DONE
    msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 100);
    fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 186 + 69, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

    // CANCEL
    msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 102);
    fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 186 + 171, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

    int count = perkDialogDrawPerks();

    // NOTE: Original code is slightly different, but does the same thing.
    int perk = gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value;
    int perkFrmId = perkGetFrmId(perk);
    char* perkName = perkGetName(perk);
    char* perkDescription = perkGetDescription(perk);
    char* perkRank = NULL;
    char perkRankBuffer[32];

    int rank = perkGetRank(gDude, perk);
    if (rank != 0) {
        sprintf(perkRankBuffer, "(%d)", rank);
        perkRank = perkRankBuffer;
    }

    perkDialogDrawCard(perkFrmId, perkName, perkRank, perkDescription);
    win_draw(gPerkDialogWindow);

    int rc = perkDialogHandleInput(count, perkDialogRefreshPerks);

    if (rc == 1) {
        if (perkAdd(gDude, gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value) == -1) {
            debugPrint("\n*** Unable to add perk! ***\n");
            rc = 2;
        }
    }

    rc &= 1;

    if (rc != 0) {
        if (perkGetRank(gDude, PERK_TAG) != 0 && gCharacterEditorPerksBackup[PERK_TAG] == 0) {
            if (!perkDialogHandleTagPerk()) {
                perkRemove(gDude, PERK_TAG);
            }
        } else if (perkGetRank(gDude, PERK_MUTATE) != 0 && gCharacterEditorPerksBackup[PERK_MUTATE] == 0) {
            if (!perkDialogHandleMutatePerk()) {
                perkRemove(gDude, PERK_MUTATE);
            }
        } else if (perkGetRank(gDude, PERK_LIFEGIVER) != gCharacterEditorPerksBackup[PERK_LIFEGIVER]) {
            int maxHp = critterGetBonusStat(gDude, STAT_MAXIMUM_HIT_POINTS);
            critterSetBonusStat(gDude, STAT_MAXIMUM_HIT_POINTS, maxHp + 4);
            critter_adjust_hits(gDude, 4);
        } else if (perkGetRank(gDude, PERK_EDUCATED) != gCharacterEditorPerksBackup[PERK_EDUCATED]) {
            int sp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
            pcSetStat(PC_STAT_UNSPENT_SKILL_POINTS, sp + 2);
        }
    }

    characterEditorDrawSkills(0);
    characterEditorDrawPrimaryStat(RENDER_ALL_STATS, 0, 0);
    characterEditorDrawPcStats();
    characterEditorDrawDerivedStats();
    characterEditorDrawFolders();
    characterEditorDrawCard();
    win_draw(gCharacterEditorWindow);

    art_ptr_unlock(backgroundFrmHandle);

    windowDestroy(gPerkDialogWindow);

    return rc;
}

// 0x43CACC
int perkDialogHandleInput(int count, void (*refreshProc)())
{
    fontSetCurrent(101);

    int v3 = count - 11;

    int height = fontGetLineHeight();
    gPerkDialogPreviousCurrentLine = -2;
    int v16 = height + 2;

    int v7 = 0;

    int rc = 0;
    while (rc == 0) {
        int keyCode = _get_input();
        int v19 = 0;

        if (keyCode == 500) {
            rc = 1;
        } else if (keyCode == KEY_RETURN) {
            soundPlayFile("ib1p1xx1");
            rc = 1;
        } else if (keyCode == 501) {
            mouse_get_position(&gCharacterEditorMouseX, &gCharacterEditorMouseY);
            gPerkDialogCurrentLine = (gCharacterEditorMouseY - (PERK_WINDOW_Y + PERK_WINDOW_LIST_Y)) / v16;
            if (gPerkDialogCurrentLine >= 0) {
                if (count - 1 < gPerkDialogCurrentLine)
                    gPerkDialogCurrentLine = count - 1;
            } else {
                gPerkDialogCurrentLine = 0;
            }

            if (gPerkDialogCurrentLine == gPerkDialogPreviousCurrentLine) {
                soundPlayFile("ib1p1xx1");
                rc = 1;
            }
            gPerkDialogPreviousCurrentLine = gPerkDialogCurrentLine;
            refreshProc();
        } else if (keyCode == 502 || keyCode == KEY_ESCAPE || _game_user_wants_to_quit != 0) {
            rc = 2;
        } else {
            switch (keyCode) {
            case KEY_ARROW_UP:
                gPerkDialogPreviousCurrentLine = -2;

                gPerkDialogTopLine--;
                if (gPerkDialogTopLine < 0) {
                    gPerkDialogTopLine = 0;

                    gPerkDialogCurrentLine--;
                    if (gPerkDialogCurrentLine < 0) {
                        gPerkDialogCurrentLine = 0;
                    }
                }

                refreshProc();
                break;
            case KEY_PAGE_UP:
                gPerkDialogPreviousCurrentLine = -2;

                for (int index = 0; index < 11; index++) {
                    gPerkDialogTopLine--;
                    if (gPerkDialogTopLine < 0) {
                        gPerkDialogTopLine = 0;

                        gPerkDialogCurrentLine--;
                        if (gPerkDialogCurrentLine < 0) {
                            gPerkDialogCurrentLine = 0;
                        }
                    }
                }

                refreshProc();
                break;
            case KEY_ARROW_DOWN:
                gPerkDialogPreviousCurrentLine = -2;

                if (count > 11) {
                    gPerkDialogTopLine++;
                    if (gPerkDialogTopLine > count - 11) {
                        gPerkDialogTopLine = count - 11;

                        gPerkDialogCurrentLine++;
                        if (gPerkDialogCurrentLine > 10) {
                            gPerkDialogCurrentLine = 10;
                        }
                    }
                } else {
                    gPerkDialogCurrentLine++;
                    if (gPerkDialogCurrentLine > count - 1) {
                        gPerkDialogCurrentLine = count - 1;
                    }
                }

                refreshProc();
                break;
            case KEY_PAGE_DOWN:
                gPerkDialogPreviousCurrentLine = -2;

                for (int index = 0; index < 11; index++) {
                    if (count > 11) {
                        gPerkDialogTopLine++;
                        if (gPerkDialogTopLine > count - 11) {
                            gPerkDialogTopLine = count - 11;

                            gPerkDialogCurrentLine++;
                            if (gPerkDialogCurrentLine > 10) {
                                gPerkDialogCurrentLine = 10;
                            }
                        }
                    } else {
                        gPerkDialogCurrentLine++;
                        if (gPerkDialogCurrentLine > count - 1) {
                            gPerkDialogCurrentLine = count - 1;
                        }
                    }
                }

                refreshProc();
                break;
            case 572:
                _repFtime = 4;
                gPerkDialogPreviousCurrentLine = -2;

                do {
                    _frame_time = _get_time();
                    if (v19 <= dbl_5019BE) {
                        v19++;
                    }

                    if (v19 == 1 || v19 > dbl_5019BE) {
                        if (v19 > dbl_5019BE) {
                            _repFtime++;
                            if (_repFtime > 24) {
                                _repFtime = 24;
                            }
                        }

                        gPerkDialogTopLine--;
                        if (gPerkDialogTopLine < 0) {
                            gPerkDialogTopLine = 0;

                            gPerkDialogCurrentLine--;
                            if (gPerkDialogCurrentLine < 0) {
                                gPerkDialogCurrentLine = 0;
                            }
                        }
                        refreshProc();
                    }

                    if (v19 < dbl_5019BE) {
                        while (getTicksSince(_frame_time) < 1000 / 24) {
                        }
                    } else {
                        while (getTicksSince(_frame_time) < 1000 / _repFtime) {
                        }
                    }
                } while (_get_input() != 574);

                break;
            case 573:
                gPerkDialogPreviousCurrentLine = -2;
                _repFtime = 4;

                if (count > 11) {
                    do {
                        _frame_time = _get_time();
                        if (v19 <= dbl_5019BE) {
                            v19++;
                        }

                        if (v19 == 1 || v19 > dbl_5019BE) {
                            if (v19 > dbl_5019BE) {
                                _repFtime++;
                                if (_repFtime > 24) {
                                    _repFtime = 24;
                                }
                            }

                            gPerkDialogTopLine++;
                            if (gPerkDialogTopLine > count - 11) {
                                gPerkDialogTopLine = count - 11;

                                gPerkDialogCurrentLine++;
                                if (gPerkDialogCurrentLine > 10) {
                                    gPerkDialogCurrentLine = 10;
                                }
                            }

                            refreshProc();
                        }

                        if (v19 < dbl_5019BE) {
                            while (getTicksSince(_frame_time) < 1000 / 24) {
                            }
                        } else {
                            while (getTicksSince(_frame_time) < 1000 / _repFtime) {
                            }
                        }
                    } while (_get_input() != 575);
                } else {
                    do {
                        _frame_time = _get_time();
                        if (v19 <= dbl_5019BE) {
                            v19++;
                        }

                        if (v19 == 1 || v19 > dbl_5019BE) {
                            if (v19 > dbl_5019BE) {
                                _repFtime++;
                                if (_repFtime > 24) {
                                    _repFtime = 24;
                                }
                            }

                            gPerkDialogCurrentLine++;
                            if (gPerkDialogCurrentLine > count - 1) {
                                gPerkDialogCurrentLine = count - 1;
                            }

                            refreshProc();
                        }

                        if (v19 < dbl_5019BE) {
                            while (getTicksSince(_frame_time) < 1000 / 24) {
                            }
                        } else {
                            while (getTicksSince(_frame_time) < 1000 / _repFtime) {
                            }
                        }
                    } while (_get_input() != 575);
                }
                break;
            case KEY_HOME:
                gPerkDialogTopLine = 0;
                gPerkDialogCurrentLine = 0;
                gPerkDialogPreviousCurrentLine = -2;
                refreshProc();
                break;
            case KEY_END:
                gPerkDialogPreviousCurrentLine = -2;
                if (count > 11) {
                    gPerkDialogTopLine = count - 11;
                    gPerkDialogCurrentLine = 10;
                } else {
                    gPerkDialogCurrentLine = count - 1;
                }
                refreshProc();
                break;
            default:
                if (getTicksSince(_frame_time) > 700) {
                    _frame_time = _get_time();
                    gPerkDialogPreviousCurrentLine = -2;
                }
                break;
            }
        }
    }

    return rc;
}

// 0x43D0BC
int perkDialogDrawPerks()
{
    blitBufferToBuffer(
        gPerkDialogBackgroundBuffer + PERK_WINDOW_WIDTH * 43 + 45,
        192,
        129,
        PERK_WINDOW_WIDTH,
        gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 43 + 45,
        PERK_WINDOW_WIDTH);

    fontSetCurrent(101);

    int perks[PERK_COUNT];
    int count = perkGetAvailablePerks(gDude, perks);
    if (count == 0) {
        return 0;
    }

    for (int perk = 0; perk < PERK_COUNT; perk++) {
        gPerkDialogOptionList[perk].value = 0;
        gPerkDialogOptionList[perk].name = NULL;
    }

    for (int index = 0; index < count; index++) {
        gPerkDialogOptionList[index].value = perks[index];
        gPerkDialogOptionList[index].name = perkGetName(perks[index]);
    }

    qsort(gPerkDialogOptionList, count, sizeof(*gPerkDialogOptionList), perkDialogOptionCompare);

    int v16 = count - gPerkDialogTopLine;
    if (v16 > 11) {
        v16 = 11;
    }

    v16 += gPerkDialogTopLine;

    int y = 43;
    int yStep = fontGetLineHeight() + 2;
    for (int index = gPerkDialogTopLine; index < v16; index++) {
        int color;
        if (index == gPerkDialogTopLine + gPerkDialogCurrentLine) {
            color = colorTable[32747];
        } else {
            color = colorTable[992];
        }

        fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * y + 45, gPerkDialogOptionList[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);

        if (perkGetRank(gDude, gPerkDialogOptionList[index].value) != 0) {
            char rankString[256];
            sprintf(rankString, "(%d)", perkGetRank(gDude, gPerkDialogOptionList[index].value));
            fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * y + 207, rankString, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
        }

        y += yStep;
    }

    return count;
}

// 0x43D2F8
void perkDialogRefreshTraits()
{
    blitBufferToBuffer(gPerkDialogBackgroundBuffer + 280, 293, PERK_WINDOW_HEIGHT, PERK_WINDOW_WIDTH, gPerkDialogWindowBuffer + 280, PERK_WINDOW_WIDTH);

    perkDialogDrawTraits(gPerkDialogOptionCount);

    char* traitName = gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].name;
    char* tratDescription = traitGetDescription(gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value);
    int frmId = traitGetFrmId(gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value);
    perkDialogDrawCard(frmId, traitName, NULL, tratDescription);

    win_draw(gPerkDialogWindow);
}

// 0x43D38C
bool perkDialogHandleMutatePerk()
{
    gPerkDialogCardFrmId = -1;
    gPerkDialogCardTitle[0] = '\0';
    gPerkDialogCardDrawn = false;

    // NOTE: Uninline.
    gCharacterEditorTempTraitCount = TRAITS_MAX_SELECTED_COUNT - get_trait_count();

    bool result = true;
    if (gCharacterEditorTempTraitCount >= 1) {
        fontSetCurrent(103);

        blitBufferToBuffer(gPerkDialogBackgroundBuffer + PERK_WINDOW_WIDTH * 14 + 49, 206, fontGetLineHeight() + 2, PERK_WINDOW_WIDTH, gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 15 + 49, PERK_WINDOW_WIDTH);

        // LOSE A TRAIT
        char* msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 154);
        fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

        gPerkDialogOptionCount = 0;
        gPerkDialogCurrentLine = 0;
        gPerkDialogTopLine = 0;
        perkDialogRefreshTraits();

        int rc = perkDialogHandleInput(gCharacterEditorTempTraitCount, perkDialogRefreshTraits);
        if (rc == 1) {
            if (gPerkDialogCurrentLine == 0) {
                if (gCharacterEditorTempTraitCount == 1) {
                    gCharacterEditorTempTraits[0] = -1;
                    gCharacterEditorTempTraits[1] = -1;
                } else {
                    if (gPerkDialogOptionList[0].value == gCharacterEditorTempTraits[0]) {
                        gCharacterEditorTempTraits[0] = gCharacterEditorTempTraits[1];
                        gCharacterEditorTempTraits[1] = -1;
                    } else {
                        gCharacterEditorTempTraits[1] = -1;
                    }
                }
            } else {
                if (gPerkDialogOptionList[0].value == gCharacterEditorTempTraits[0]) {
                    gCharacterEditorTempTraits[1] = -1;
                } else {
                    gCharacterEditorTempTraits[0] = gCharacterEditorTempTraits[1];
                    gCharacterEditorTempTraits[1] = -1;
                }
            }
        } else {
            result = false;
        }
    }

    if (result) {
        fontSetCurrent(103);

        blitBufferToBuffer(gPerkDialogBackgroundBuffer + PERK_WINDOW_WIDTH * 14 + 49, 206, fontGetLineHeight() + 2, PERK_WINDOW_WIDTH, gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 15 + 49, PERK_WINDOW_WIDTH);

        // PICK A NEW TRAIT
        char* msg = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 153);
        fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

        gPerkDialogCurrentLine = 0;
        gPerkDialogTopLine = 0;
        gPerkDialogOptionCount = 1;

        perkDialogRefreshTraits();

        int count = 16 - gCharacterEditorTempTraitCount;
        if (count > 16) {
            count = 16;
        }

        int rc = perkDialogHandleInput(count, perkDialogRefreshTraits);
        if (rc == 1) {
            if (gCharacterEditorTempTraitCount != 0) {
                gCharacterEditorTempTraits[1] = gPerkDialogOptionList[gPerkDialogCurrentLine + gPerkDialogTopLine].value;
            } else {
                gCharacterEditorTempTraits[0] = gPerkDialogOptionList[gPerkDialogCurrentLine + gPerkDialogTopLine].value;
                gCharacterEditorTempTraits[1] = -1;
            }

            traitsSetSelected(gCharacterEditorTempTraits[0], gCharacterEditorTempTraits[1]);
        } else {
            result = false;
        }
    }

    if (!result) {
        memcpy(gCharacterEditorTempTraits, gCharacterEditorOptionalTraitsBackup, sizeof(gCharacterEditorTempTraits));
    }

    return result;
}

// 0x43D668
void perkDialogRefreshSkills()
{
    blitBufferToBuffer(gPerkDialogBackgroundBuffer + 280, 293, PERK_WINDOW_HEIGHT, PERK_WINDOW_WIDTH, gPerkDialogWindowBuffer + 280, PERK_WINDOW_WIDTH);

    perkDialogDrawSkills();

    char* name = gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].name;
    char* description = skillGetDescription(gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value);
    int frmId = skillGetFrmId(gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value);
    perkDialogDrawCard(frmId, name, NULL, description);

    win_draw(gPerkDialogWindow);
}

// 0x43D6F8
bool perkDialogHandleTagPerk()
{
    fontSetCurrent(103);

    blitBufferToBuffer(gPerkDialogBackgroundBuffer + 573 * 14 + 49, 206, fontGetLineHeight() + 2, 573, gPerkDialogWindowBuffer + 573 * 15 + 49, 573);

    // PICK A NEW TAG SKILL
    char* messageListItemText = getmsg(&gCharacterEditorMessageList, &gCharacterEditorMessageListItem, 155);
    fontDrawText(gPerkDialogWindowBuffer + 573 * 16 + 49, messageListItemText, 573, 573, colorTable[18979]);

    gPerkDialogCurrentLine = 0;
    gPerkDialogTopLine = 0;
    gPerkDialogCardFrmId = -1;
    gPerkDialogCardTitle[0] = '\0';
    gPerkDialogCardDrawn = false;
    perkDialogRefreshSkills();

    int rc = perkDialogHandleInput(gPerkDialogOptionCount, perkDialogRefreshSkills);
    if (rc != 1) {
        memcpy(gCharacterEditorTempTaggedSkills, gCharacterEditorTaggedSkillsBackup, sizeof(gCharacterEditorTempTaggedSkills));
        skillsSetTagged(gCharacterEditorTaggedSkillsBackup, NUM_TAGGED_SKILLS);
        return false;
    }

    gCharacterEditorTempTaggedSkills[3] = gPerkDialogOptionList[gPerkDialogTopLine + gPerkDialogCurrentLine].value;
    skillsSetTagged(gCharacterEditorTempTaggedSkills, NUM_TAGGED_SKILLS);

    return true;
}

// 0x43D81C
void perkDialogDrawSkills()
{
    blitBufferToBuffer(gPerkDialogBackgroundBuffer + PERK_WINDOW_WIDTH * 43 + 45, 192, 129, PERK_WINDOW_WIDTH, gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 43 + 45, PERK_WINDOW_WIDTH);

    fontSetCurrent(101);

    gPerkDialogOptionCount = 0;

    int y = 43;
    int yStep = fontGetLineHeight() + 2;

    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        if (skill != gCharacterEditorTempTaggedSkills[0] && skill != gCharacterEditorTempTaggedSkills[1] && skill != gCharacterEditorTempTaggedSkills[2] && skill != gCharacterEditorTempTaggedSkills[3]) {
            gPerkDialogOptionList[gPerkDialogOptionCount].value = skill;
            gPerkDialogOptionList[gPerkDialogOptionCount].name = skillGetName(skill);
            gPerkDialogOptionCount++;
        }
    }

    qsort(gPerkDialogOptionList, gPerkDialogOptionCount, sizeof(*gPerkDialogOptionList), perkDialogOptionCompare);

    for (int index = gPerkDialogTopLine; index < gPerkDialogTopLine + 11; index++) {
        int color;
        if (index == gPerkDialogCurrentLine + gPerkDialogTopLine) {
            color = colorTable[32747];
        } else {
            color = colorTable[992];
        }

        fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * y + 45, gPerkDialogOptionList[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
        y += yStep;
    }
}

// 0x43D960
int perkDialogDrawTraits(int a1)
{
    blitBufferToBuffer(gPerkDialogBackgroundBuffer + PERK_WINDOW_WIDTH * 43 + 45, 192, 129, PERK_WINDOW_WIDTH, gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 43 + 45, PERK_WINDOW_WIDTH);

    fontSetCurrent(101);

    int y = 43;
    int yStep = fontGetLineHeight() + 2;

    if (a1 != 0) {
        int count = 0;
        for (int trait = 0; trait < TRAIT_COUNT; trait++) {
            if (trait != gCharacterEditorOptionalTraitsBackup[0] && trait != gCharacterEditorOptionalTraitsBackup[1]) {
                gPerkDialogOptionList[count].value = trait;
                gPerkDialogOptionList[count].name = traitGetName(trait);
                count++;
            }
        }

        qsort(gPerkDialogOptionList, count, sizeof(*gPerkDialogOptionList), perkDialogOptionCompare);

        for (int index = gPerkDialogTopLine; index < gPerkDialogTopLine + 11; index++) {
            int color;
            if (index == gPerkDialogCurrentLine + gPerkDialogTopLine) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * y + 45, gPerkDialogOptionList[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
            y += yStep;
        }
    } else {
        // NOTE: Original code does not use loop.
        for (int index = 0; index < TRAITS_MAX_SELECTED_COUNT; index++) {
            gPerkDialogOptionList[index].value = gCharacterEditorTempTraits[index];
            gPerkDialogOptionList[index].name = traitGetName(gCharacterEditorTempTraits[index]);
        }

        if (gCharacterEditorTempTraitCount > 1) {
            qsort(gPerkDialogOptionList, gCharacterEditorTempTraitCount, sizeof(*gPerkDialogOptionList), perkDialogOptionCompare);
        }

        for (int index = 0; index < gCharacterEditorTempTraitCount; index++) {
            int color;
            if (index == gPerkDialogCurrentLine) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * y + 45, gPerkDialogOptionList[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
            y += yStep;
        }
    }
    return 0;
}

// 0x43DB48
int perkDialogOptionCompare(const void* a1, const void* a2)
{
    PerkDialogOption* v1 = (PerkDialogOption*)a1;
    PerkDialogOption* v2 = (PerkDialogOption*)a2;
    return strcmp(v1->name, v2->name);
}

// 0x43DB54
int perkDialogDrawCard(int frmId, const char* name, const char* rank, char* description)
{
    int fid = art_id(OBJ_TYPE_SKILLDEX, frmId, 0, 0, 0);

    CacheEntry* handle;
    int width;
    int height;
    unsigned char* data = art_lock(fid, &handle, &width, &height);
    if (data == NULL) {
        return -1;
    }

    blitBufferToBuffer(data, width, height, width, gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 64 + 413, PERK_WINDOW_WIDTH);

    // Calculate width of transparent pixels on the left side of the image. This
    // space will be occupied by description (in addition to fixed width).
    int extraDescriptionWidth = 150;
    for (int y = 0; y < height; y++) {
        unsigned char* stride = data;
        for (int x = 0; x < width; x++) {
            if (_HighRGB_(*stride) < 2) {
                if (extraDescriptionWidth > x) {
                    extraDescriptionWidth = x;
                }
            }
            stride++;
        }
        data += width;
    }

    // Add gap between description and image.
    extraDescriptionWidth -= 8;
    if (extraDescriptionWidth < 0) {
        extraDescriptionWidth = 0;
    }

    fontSetCurrent(102);
    int nameHeight = fontGetLineHeight();

    fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * 27 + 280, name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[0]);

    if (rank != NULL) {
        int rankX = fontGetStringWidth(name) + 280 + 8;
        fontSetCurrent(101);

        int rankHeight = fontGetLineHeight();
        fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * (23 + nameHeight - rankHeight) + rankX, rank, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[0]);
    }

    windowDrawLine(gPerkDialogWindow, 280, 27 + nameHeight, 545, 27 + nameHeight, colorTable[0]);
    windowDrawLine(gPerkDialogWindow, 280, 28 + nameHeight, 545, 28 + nameHeight, colorTable[0]);

    fontSetCurrent(101);

    int yStep = fontGetLineHeight() + 1;
    int y = 70;

    short beginnings[WORD_WRAP_MAX_COUNT];
    short count;
    if (wordWrap(description, 133 + extraDescriptionWidth, beginnings, &count) != 0) {
        // FIXME: Leaks handle.
        return -1;
    }

    for (int index = 0; index < count - 1; index++) {
        char* beginning = description + beginnings[index];
        char* ending = description + beginnings[index + 1];

        char ch = *ending;
        *ending = '\0';

        fontDrawText(gPerkDialogWindowBuffer + PERK_WINDOW_WIDTH * y + 280, beginning, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[0]);

        *ending = ch;

        y += yStep;
    }

    if (frmId != gPerkDialogCardFrmId || strcmp(gPerkDialogCardTitle, name) != 0) {
        if (gPerkDialogCardDrawn) {
            soundPlayFile("isdxxxx1");
        }
    }

    strcpy(gPerkDialogCardTitle, name);
    gPerkDialogCardFrmId = frmId;
    gPerkDialogCardDrawn = true;

    art_ptr_unlock(handle);

    return 0;
}

// 0x43DE94
void push_perks()
{
    int perk;

    for (perk = 0; perk < PERK_COUNT; perk++) {
        gCharacterEditorPerksBackup[perk] = perkGetRank(gDude, perk);
    }
}

// copy editor perks to character
//
// 0x43DEBC
void _pop_perks()
{
    for (int perk = 0; perk < PERK_COUNT; perk++) {
        for (;;) {
            int rank = perkGetRank(gDude, perk);
            if (rank <= gCharacterEditorPerksBackup[perk]) {
                break;
            }

            perkRemove(gDude, perk);
        }
    }

    for (int i = 0; i < PERK_COUNT; i++) {
        for (;;) {
            int rank = perkGetRank(gDude, i);
            if (rank >= gCharacterEditorPerksBackup[i]) {
                break;
            }

            perkAdd(gDude, i);
        }
    }
}

// NOTE: Inlined.
//
// 0x43DF24
int PerkCount()
{
    int perk;
    int perkCount;

    perkCount = 0;
    for (perk = 0; perk < PERK_COUNT; perk++) {
        if (perkGetRank(gDude, perk) > 0) {
            perkCount++;
            if (perkCount >= 37) {
                break;
            }
        }
    }

    return perkCount;
}

// validate SPECIAL stats are <= 10
//
// 0x43DF50
int _is_supper_bonus()
{
    for (int stat = 0; stat < 7; stat++) {
        int v1 = critterGetBaseStatWithTraitModifier(gDude, stat);
        int v2 = critterGetBonusStat(gDude, stat);
        if (v1 + v2 > 10) {
            return 1;
        }
    }

    return 0;
}

// 0x43DF8C
int characterEditorFolderViewInit()
{
    gCharacterEditorKarmaFolderTopLine = 0;
    gCharacterEditorPerkFolderTopLine = 0;
    gCharacterEditorKillsFolderTopLine = 0;

    if (gCharacterEditorFolderViewScrollUpBtn == -1) {
        gCharacterEditorFolderViewScrollUpBtn = buttonCreate(gCharacterEditorWindow, 317, 364, gCharacterEditorFrmSize[22].width, gCharacterEditorFrmSize[22].height, -1, -1, -1, 17000, gCharacterEditorFrmData[21], gCharacterEditorFrmData[22], NULL, 32);
        if (gCharacterEditorFolderViewScrollUpBtn == -1) {
            return -1;
        }

        buttonSetCallbacks(gCharacterEditorFolderViewScrollUpBtn, _gsound_red_butt_press, NULL);
    }

    if (gCharacterEditorFolderViewScrollDownBtn == -1) {
        gCharacterEditorFolderViewScrollDownBtn = buttonCreate(gCharacterEditorWindow,
            317,
            365 + gCharacterEditorFrmSize[22].height,
            gCharacterEditorFrmSize[4].width,
            gCharacterEditorFrmSize[4].height,
            gCharacterEditorFolderViewScrollDownBtn,
            gCharacterEditorFolderViewScrollDownBtn,
            gCharacterEditorFolderViewScrollDownBtn,
            17001,
            gCharacterEditorFrmData[3],
            gCharacterEditorFrmData[4],
            0,
            32);
        if (gCharacterEditorFolderViewScrollDownBtn == -1) {
            buttonDestroy(gCharacterEditorFolderViewScrollUpBtn);
            return -1;
        }

        buttonSetCallbacks(gCharacterEditorFolderViewScrollDownBtn, _gsound_red_butt_press, NULL);
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x43E090
void folder_exit()
{
    if (gCharacterEditorFolderViewScrollDownBtn != -1) {
        buttonDestroy(gCharacterEditorFolderViewScrollDownBtn);
        gCharacterEditorFolderViewScrollDownBtn = -1;
    }

    if (gCharacterEditorFolderViewScrollUpBtn != -1) {
        buttonDestroy(gCharacterEditorFolderViewScrollUpBtn);
        gCharacterEditorFolderViewScrollUpBtn = -1;
    }
}

// 0x43E0D4
void characterEditorFolderViewScroll(int direction)
{
    int* v1;

    switch (characterEditorWindowSelectedFolder) {
    case EDITOR_FOLDER_PERKS:
        v1 = &gCharacterEditorPerkFolderTopLine;
        break;
    case EDITOR_FOLDER_KARMA:
        v1 = &gCharacterEditorKarmaFolderTopLine;
        break;
    case EDITOR_FOLDER_KILLS:
        v1 = &gCharacterEditorKillsFolderTopLine;
        break;
    default:
        return;
    }

    if (direction >= 0) {
        if (gCharacterEditorFolderViewMaxLines + gCharacterEditorFolderViewTopLine <= gCharacterEditorFolderViewCurrentLine) {
            gCharacterEditorFolderViewTopLine++;
            if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43 && characterEditorSelectedItem != 10) {
                characterEditorSelectedItem--;
            }
        }
    } else {
        if (gCharacterEditorFolderViewTopLine > 0) {
            gCharacterEditorFolderViewTopLine--;
            if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43 && gCharacterEditorFolderViewMaxLines + 9 > characterEditorSelectedItem) {
                characterEditorSelectedItem++;
            }
        }
    }

    *v1 = gCharacterEditorFolderViewTopLine;
    characterEditorDrawFolders();

    if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
        blitBufferToBuffer(
            gCharacterEditorWindowBackgroundBuffer + 640 * 267 + 345,
            277,
            170,
            640,
            gCharacterEditorWindowBuffer + 640 * 267 + 345,
            640);
        characterEditorDrawCardWithOptions(gCharacterEditorFolderCardFrmId, gCharacterEditorFolderCardTitle, gCharacterEditorFolderCardSubtitle, gCharacterEditorFolderCardDescription);
    }
}

// 0x43E200
void characterEditorFolderViewClear()
{
    int v0;

    gCharacterEditorFolderViewCurrentLine = 0;
    gCharacterEditorFolderViewNextY = 364;

    v0 = fontGetLineHeight();

    gCharacterEditorFolderViewMaxLines = 9;
    gCharacterEditorFolderViewOffsetY = v0 + 1;

    if (characterEditorSelectedItem < 10 || characterEditorSelectedItem >= 43)
        gCharacterEditorFolderViewHighlightedLine = -1;
    else
        gCharacterEditorFolderViewHighlightedLine = characterEditorSelectedItem - 10;

    if (characterEditorWindowSelectedFolder < 1) {
        if (characterEditorWindowSelectedFolder)
            return;

        gCharacterEditorFolderViewTopLine = gCharacterEditorPerkFolderTopLine;
    } else if (characterEditorWindowSelectedFolder == 1) {
        gCharacterEditorFolderViewTopLine = gCharacterEditorKarmaFolderTopLine;
    } else if (characterEditorWindowSelectedFolder == 2) {
        gCharacterEditorFolderViewTopLine = gCharacterEditorKillsFolderTopLine;
    }
}

// render heading string with line
//
// 0x43E28C
int characterEditorFolderViewDrawHeading(const char* string)
{
    int lineHeight;
    int x;
    int y;
    int lineLen;
    int gap;
    int v8 = 0;

    if (gCharacterEditorFolderViewMaxLines + gCharacterEditorFolderViewTopLine > gCharacterEditorFolderViewCurrentLine) {
        if (gCharacterEditorFolderViewCurrentLine >= gCharacterEditorFolderViewTopLine) {
            if (gCharacterEditorFolderViewCurrentLine - gCharacterEditorFolderViewTopLine == gCharacterEditorFolderViewHighlightedLine) {
                v8 = 1;
            }
            lineHeight = fontGetLineHeight();
            x = 280;
            y = gCharacterEditorFolderViewNextY + lineHeight / 2;
            if (string != NULL) {
                gap = fontGetLetterSpacing();
                // TODO: Not sure about this.
                lineLen = fontGetStringWidth(string) + gap * 4;
                x = (x - lineLen) / 2;
                fontDrawText(gCharacterEditorWindowBuffer + 640 * gCharacterEditorFolderViewNextY + 34 + x + gap * 2, string, 640, 640, colorTable[992]);
                windowDrawLine(gCharacterEditorWindow, 34 + x + lineLen, y, 34 + 280, y, colorTable[992]);
            }
            windowDrawLine(gCharacterEditorWindow, 34, y, 34 + x, y, colorTable[992]);
            gCharacterEditorFolderViewNextY += gCharacterEditorFolderViewOffsetY;
        }
        gCharacterEditorFolderViewCurrentLine++;
        return v8;
    } else {
        return 0;
    }
}

// 0x43E3D8
bool characterEditorFolderViewDrawString(const char* string)
{
    bool success = false;
    int color;

    if (gCharacterEditorFolderViewMaxLines + gCharacterEditorFolderViewTopLine > gCharacterEditorFolderViewCurrentLine) {
        if (gCharacterEditorFolderViewCurrentLine >= gCharacterEditorFolderViewTopLine) {
            if (gCharacterEditorFolderViewCurrentLine - gCharacterEditorFolderViewTopLine == gCharacterEditorFolderViewHighlightedLine) {
                success = true;
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            fontDrawText(gCharacterEditorWindowBuffer + 640 * gCharacterEditorFolderViewNextY + 34, string, 640, 640, color);
            gCharacterEditorFolderViewNextY += gCharacterEditorFolderViewOffsetY;
        }

        gCharacterEditorFolderViewCurrentLine++;
    }

    return success;
}

// 0x43E470
bool characterEditorFolderViewDrawKillsEntry(const char* name, int kills)
{
    char killsString[8];
    int color;
    int gap;

    bool success = false;
    if (gCharacterEditorFolderViewMaxLines + gCharacterEditorFolderViewTopLine > gCharacterEditorFolderViewCurrentLine) {
        if (gCharacterEditorFolderViewCurrentLine >= gCharacterEditorFolderViewTopLine) {
            if (gCharacterEditorFolderViewCurrentLine - gCharacterEditorFolderViewTopLine == gCharacterEditorFolderViewHighlightedLine) {
                color = colorTable[32747];
                success = true;
            } else {
                color = colorTable[992];
            }

            itoa(kills, killsString, 10);
            int v6 = fontGetStringWidth(killsString);

            // TODO: Check.
            gap = fontGetLetterSpacing();
            int v11 = gCharacterEditorFolderViewNextY + fontGetLineHeight() / 2;

            fontDrawText(gCharacterEditorWindowBuffer + 640 * gCharacterEditorFolderViewNextY + 34, name, 640, 640, color);

            int v12 = fontGetStringWidth(name);
            windowDrawLine(gCharacterEditorWindow, 34 + v12 + gap, v11, 314 - v6 - gap, v11, color);

            fontDrawText(gCharacterEditorWindowBuffer + 640 * gCharacterEditorFolderViewNextY + 314 - v6, killsString, 640, 640, color);
            gCharacterEditorFolderViewNextY += gCharacterEditorFolderViewOffsetY;
        }

        gCharacterEditorFolderViewCurrentLine++;
    }

    return success;
}

// 0x43E5C4
int karmaInit()
{
    const char* delim = " \t,";

    if (gKarmaEntries != NULL) {
        internal_free(gKarmaEntries);
        gKarmaEntries = NULL;
    }

    gKarmaEntriesLength = 0;

    File* stream = fileOpen("data\\karmavar.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (fileReadString(string, 256, stream)) {
        KarmaEntry entry;

        char* pch = string;
        while (isspace(*pch & 0xFF)) {
            pch++;
        }

        if (*pch == '#') {
            continue;
        }

        char* tok = strtok(pch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.art_num = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.name = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.description = atoi(tok);

        KarmaEntry* entries = (KarmaEntry*)internal_realloc(gKarmaEntries, sizeof(*entries) * (gKarmaEntriesLength + 1));
        if (entries == NULL) {
            fileClose(stream);

            return -1;
        }

        memcpy(&(entries[gKarmaEntriesLength]), &entry, sizeof(entry));

        gKarmaEntries = entries;
        gKarmaEntriesLength++;
    }

    qsort(gKarmaEntries, gKarmaEntriesLength, sizeof(*gKarmaEntries), karmaEntryCompare);

    fileClose(stream);

    return 0;
}

// NOTE: Inlined.
//
// 0x43E764
void karmaFree()
{
    if (gKarmaEntries != NULL) {
        internal_free(gKarmaEntries);
        gKarmaEntries = NULL;
    }

    gKarmaEntriesLength = 0;
}

// 0x43E78C
int karmaEntryCompare(const void* a1, const void* a2)
{
    KarmaEntry* v1 = (KarmaEntry*)a1;
    KarmaEntry* v2 = (KarmaEntry*)a2;
    return v1->gvar - v2->gvar;
}

// 0x43E798
int genericReputationInit()
{
    const char* delim = " \t,";

    if (gGenericReputationEntries != NULL) {
        internal_free(gGenericReputationEntries);
        gGenericReputationEntries = NULL;
    }

    gGenericReputationEntriesLength = 0;

    File* stream = fileOpen("data\\genrep.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (fileReadString(string, 256, stream)) {
        GenericReputationEntry entry;

        char* pch = string;
        while (isspace(*pch & 0xFF)) {
            pch++;
        }

        if (*pch == '#') {
            continue;
        }

        char* tok = strtok(pch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.threshold = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.name = atoi(tok);

        GenericReputationEntry* entries = (GenericReputationEntry*)internal_realloc(gGenericReputationEntries, sizeof(*entries) * (gGenericReputationEntriesLength + 1));
        if (entries == NULL) {
            fileClose(stream);

            return -1;
        }

        memcpy(&(entries[gGenericReputationEntriesLength]), &entry, sizeof(entry));

        gGenericReputationEntries = entries;
        gGenericReputationEntriesLength++;
    }

    qsort(gGenericReputationEntries, gGenericReputationEntriesLength, sizeof(*gGenericReputationEntries), genericReputationCompare);

    fileClose(stream);

    return 0;
}

// NOTE: Inlined.
//
// 0x43E914
void genericReputationFree()
{
    if (gGenericReputationEntries != NULL) {
        internal_free(gGenericReputationEntries);
        gGenericReputationEntries = NULL;
    }

    gGenericReputationEntriesLength = 0;
}

// 0x43E93C
int genericReputationCompare(const void* a1, const void* a2)
{
    GenericReputationEntry* v1 = (GenericReputationEntry*)a1;
    GenericReputationEntry* v2 = (GenericReputationEntry*)a2;

    if (v2->threshold > v1->threshold) {
        return 1;
    } else if (v2->threshold < v1->threshold) {
        return -1;
    }
    return 0;
}
