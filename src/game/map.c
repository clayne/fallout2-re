#include "game/map.h"

#include <direct.h>
#include <stdio.h>
#include <string.h>

#include "game/anim.h"
#include "game/automap.h"
#include "game/editor.h"
#include "color.h"
#include "game/combat.h"
#include "core.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "debug.h"
#include "draw.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/light.h"
#include "game/loadsave.h"
#include "memory.h"
#include "object.h"
#include "palette.h"
#include "pipboy.h"
#include "proto.h"
#include "proto_instance.h"
#include "queue.h"
#include "random.h"
#include "scripts.h"
#include "text_object.h"
#include "tile.h"
#include "window_manager.h"
#include "window_manager_private.h"
#include "worldmap.h"

static void map_display_draw(Rect* rect);
static void map_scroll_refresh_game(Rect* rect);
static void map_scroll_refresh_mapper(Rect* rect);
static int map_allocate_global_vars(int count);
static void map_free_global_vars();
static int map_load_global_vars(File* stream);
static int map_allocate_local_vars(int count);
static void map_free_local_vars();
static void map_place_dude_and_mouse();
static void square_reset();
static int square_load(File* stream, int a2);
static int map_write_MapData(MapHeader* ptr, File* stream);
static int map_read_MapData(MapHeader* ptr, File* stream);

// 0x50B058
char byte_50B058[] = "";

// 0x50B30C
char _aErrorF2[] = "ERROR! F2";

// 0x519540
static IsoWindowRefreshProc* map_scroll_refresh = map_scroll_refresh_game;

// 0x519544
static int map_data_elev_flags[ELEVATION_COUNT] = {
    2,
    4,
    8,
};

// 0x519550
static unsigned int map_last_scroll_time = 0;

// 0x519554
static bool map_bk_enabled = false;

// 0x519558
static int mapEntranceElevation = 0;

// 0x51955C
static int mapEntranceTileNum = -1;

// 0x519560
static int mapEntranceRotation = ROTATION_NE;

// 0x519564
int map_script_id = -1;

// local_vars
// 0x519568
int* map_local_vars = NULL;

// map_vars
// 0x51956C
int* map_global_vars = NULL;

// local_vars_num
// 0x519570
int num_map_local_vars = 0;

// map_vars_num
// 0x519574
int num_map_global_vars = 0;

// Current elevation.
//
// 0x519578
int map_elevation = 0;

// 0x519584
static int wmMapIdx = -1;

// 0x614868
TileData square_data[ELEVATION_COUNT];

// 0x631D28
static MapTransition map_state;

// 0x631D38
static Rect map_display_rect;

// map.msg
//
// map_msg_file
// 0x631D48
MessageList map_msg_file;

// 0x631D50
static unsigned char* display_buf;

// 0x631D54
MapHeader map_data;

// 0x631E40
TileData* square[ELEVATION_COUNT];

// 0x631E4C
int display_win;

// iso_init
// 0x481CA0
int iso_init()
{
    tileScrollLimitingDisable();
    tileScrollBlockingDisable();

    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        square[elevation] = &(square_data[elevation]);
    }

    display_win = windowCreate(0, 0, _scr_size.right - _scr_size.left + 1, _scr_size.bottom - _scr_size.top - 99, 256, 10);
    if (display_win == -1) {
        debugPrint("win_add failed in iso_init\n");
        return -1;
    }

    display_buf = windowGetBuffer(display_win);
    if (display_buf == NULL) {
        debugPrint("win_get_buf failed in iso_init\n");
        return -1;
    }

    if (win_get_rect(display_win, &map_display_rect) != 0) {
        debugPrint("win_get_rect failed in iso_init\n");
        return -1;
    }

    if (art_init() != 0) {
        debugPrint("art_init failed in iso_init\n");
        return -1;
    }

    debugPrint(">art_init\t\t");

    if (tileInit(square, SQUARE_GRID_WIDTH, SQUARE_GRID_HEIGHT, HEX_GRID_WIDTH, HEX_GRID_HEIGHT, display_buf, _scr_size.right - _scr_size.left + 1, _scr_size.bottom - _scr_size.top - 99, _scr_size.right - _scr_size.left + 1, map_display_draw) != 0) {
        debugPrint("tile_init failed in iso_init\n");
        return -1;
    }

    debugPrint(">tile_init\t\t");

    if (objectsInit(display_buf, _scr_size.right - _scr_size.left + 1, _scr_size.bottom - _scr_size.top - 99, _scr_size.right - _scr_size.left + 1) != 0) {
        debugPrint("obj_init failed in iso_init\n");
        return -1;
    }

    debugPrint(">obj_init\t\t");

    cycle_init();
    debugPrint(">cycle_init\t\t");

    tileScrollBlockingEnable();
    tileScrollLimitingEnable();

    if (intface_init() != 0) {
        debugPrint("intface_init failed in iso_init\n");
        return -1;
    }

    debugPrint(">intface_init\t\t");

    map_setup_paths();

    mapEntranceElevation = -1;
    mapEntranceTileNum = -1;
    mapEntranceRotation = -1;

    return 0;
}

// 0x481ED4
void iso_reset()
{
    if (map_global_vars != NULL) {
        internal_free(map_global_vars);
        map_global_vars = NULL;
        num_map_global_vars = 0;
    }

    if (map_local_vars != NULL) {
        internal_free(map_local_vars);
        map_local_vars = NULL;
        num_map_local_vars = 0;
    }

    art_reset();
    tileReset();
    objectsReset();
    cycle_reset();
    intface_reset();
    mapEntranceElevation = -1;
    mapEntranceTileNum = -1;
    mapEntranceRotation = -1;
}

// 0x481F48
void iso_exit()
{
    intface_exit();
    cycle_exit();
    objectsExit();
    tileExit();
    art_exit();

    if (map_global_vars != NULL) {
        internal_free(map_global_vars);
        map_global_vars = NULL;
        num_map_global_vars = 0;
    }

    if (map_local_vars != NULL) {
        internal_free(map_local_vars);
        map_local_vars = NULL;
        num_map_local_vars = 0;
    }
}

// 0x481FB4
void map_init()
{
    char* executable;
    config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, "executable", &executable);
    if (stricmp(executable, "mapper") == 0) {
        map_scroll_refresh = map_scroll_refresh_mapper;
    }

    if (messageListInit(&map_msg_file)) {
        char path[FILENAME_MAX];
        sprintf(path, "%smap.msg", msg_path);

        if (!messageListLoad(&map_msg_file, path)) {
            debugPrint("\nError loading map_msg_file!");
        }
    } else {
        debugPrint("\nError initing map_msg_file!");
    }

    map_new_map();
    tickersAdd(gmouse_bk_process);
    gmouse_disable(0);
    win_show(display_win);
}

// 0x482084
void map_exit()
{
    win_hide(display_win);
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
    tickersRemove(gmouse_bk_process);
    if (!messageListFree(&map_msg_file)) {
        debugPrint("\nError exiting map_msg_file!");
    }
}

// 0x4820C0
void map_enable_bk_processes()
{
    if (!map_bk_enabled) {
        textObjectsEnable();
        if (!game_ui_is_disabled()) {
            gmouse_enable();
        }
        tickersAdd(object_animate);
        tickersAdd(dude_fidget);
        _scr_enable_critters();
        map_bk_enabled = true;
    }
}

// 0x482104
bool map_disable_bk_processes()
{
    if (!map_bk_enabled) {
        return false;
    }

    _scr_disable_critters();
    tickersRemove(dude_fidget);
    tickersRemove(object_animate);
    gmouse_disable(0);
    textObjectsDisable();

    map_bk_enabled = false;

    return true;
}

// 0x482148
bool map_bk_processes_are_disabled()
{
    return map_bk_enabled == false;
}

// map_set_elevation
// 0x482158
int map_set_elevation(int elevation)
{
    if (!elevationIsValid(elevation)) {
        return -1;
    }

    bool gameMouseWasVisible = false;
    if (gmouse_get_cursor() != MOUSE_CURSOR_WAIT_PLANET) {
        gameMouseWasVisible = gmouse_3d_is_on();
        gmouse_3d_off();
        gmouse_set_cursor(MOUSE_CURSOR_NONE);
    }

    if (elevation != map_elevation) {
        wmMapMarkMapEntranceState(map_data.field_34, elevation, 1);
    }

    map_elevation = elevation;

    register_clear(gDude);
    dude_stand(gDude, gDude->rotation, gDude->fid);
    _partyMemberSyncPosition();

    if (map_script_id != -1) {
        scriptsExecMapUpdateProc();
    }

    if (gameMouseWasVisible) {
        gmouse_3d_on();
    }

    return 0;
}

// 0x482220
int map_set_global_var(int var, int value)
{
    if (var < 0 || var >= num_map_global_vars) {
        debugPrint("ERROR: attempt to reference map var out of range: %d", var);
        return -1;
    }

    map_global_vars[var] = value;

    return 0;
}

// 0x482250
int map_get_global_var(int var)
{
    if (var < 0 || var >= num_map_global_vars) {
        debugPrint("ERROR: attempt to reference map var out of range: %d", var);
        return 0;
    }

    return map_global_vars[var];
}

// 0x482280
int map_set_local_var(int var, int value)
{
    if (var < 0 || var >= num_map_local_vars) {
        debugPrint("ERROR: attempt to reference local var out of range: %d", var);
        return -1;
    }

    map_local_vars[var] = value;

    return 0;
}

// 0x4822B0
int map_get_local_var(int var)
{
    if (var < 0 || var >= num_map_local_vars) {
        debugPrint("ERROR: attempt to reference local var out of range: %d", var);
        return 0;
    }

    return map_local_vars[var];
}

// Make a room to store more local variables.
//
// 0x4822E0
int map_malloc_local_var(int a1)
{
    int oldMapLocalVarsLength = num_map_local_vars;
    num_map_local_vars += a1;

    int* vars = (int*)internal_realloc(map_local_vars, sizeof(*vars) * num_map_local_vars);
    if (vars == NULL) {
        debugPrint("\nError: Ran out of memory!");
    }

    map_local_vars = vars;
    memset((unsigned char*)vars + sizeof(*vars) * oldMapLocalVarsLength, 0, sizeof(*vars) * a1);

    return oldMapLocalVarsLength;
}

// 0x48234C
void map_set_entrance_hex(int tile, int elevation, int rotation)
{
    map_data.enteringTile = tile;
    map_data.enteringElevation = elevation;
    map_data.enteringRotation = rotation;
}

// 0x4824CC
char* map_get_name(int map, int elevation)
{
    if (map < 0 || map >= wmMapMaxCount()) {
        return NULL;
    }

    if (!elevationIsValid(elevation)) {
        return NULL;
    }

    MessageListItem messageListItem;
    return getmsg(&map_msg_file, &messageListItem, map * 3 + elevation + 200);
}

// TODO: Check, probably returns true if map1 and map2 represents the same city.
//
// 0x482528
bool is_map_idx_same(int map1, int map2)
{
    if (map1 < 0 || map1 >= wmMapMaxCount()) {
        return 0;
    }

    if (map2 < 0 || map2 >= wmMapMaxCount()) {
        return 0;
    }

    if (!wmMapIdxIsSaveable(map1)) {
        return 0;
    }

    if (!wmMapIdxIsSaveable(map2)) {
        return 0;
    }

    int city1;
    if (wmMatchAreaContainingMapIdx(map1, &city1) == -1) {
        return 0;
    }

    int city2;
    if (wmMatchAreaContainingMapIdx(map2, &city2) == -1) {
        return 0;
    }

    return city1 == city2;
}

// 0x4825CC
int get_map_idx_same(int map1, int map2)
{
    int city1 = -1;
    if (wmMatchAreaContainingMapIdx(map1, &city1) == -1) {
        return -1;
    }

    int city2 = -2;
    if (wmMatchAreaContainingMapIdx(map2, &city2) == -1) {
        return -1;
    }

    if (city1 != city2) {
        return -1;
    }

    return city1;
}

// 0x48261C
char* map_get_short_name(int map)
{
    int city;
    if (wmMatchAreaContainingMapIdx(map, &city) == -1) {
        return _aErrorF2;
    }

    MessageListItem messageListItem;
    char* name = getmsg(&map_msg_file, &messageListItem, 1500 + city);
    return name;
}

// 0x48268C
char* map_get_description_idx(int map)
{
    // 0x631E50
    static char scratchStr[40];

    // 0x51957C
    static char* errMapName = byte_50B058;

    int city;
    if (wmMatchAreaContainingMapIdx(map, &city) == 0) {
        wmGetAreaIdxName(city, scratchStr);
    } else {
        strcpy(scratchStr, errMapName);
    }

    return scratchStr;
}

// 0x4826B8
int map_get_index_number()
{
    return map_data.field_34;
}

// 0x4826C0
int map_scroll(int dx, int dy)
{
    if (getTicksSince(map_last_scroll_time) < 33) {
        return -2;
    }

    map_last_scroll_time = _get_time();

    int screenDx = dx * 32;
    int screenDy = dy * 24;

    if (screenDx == 0 && screenDy == 0) {
        return -1;
    }

    gmouse_3d_off();

    int centerScreenX;
    int centerScreenY;
    tileToScreenXY(gCenterTile, &centerScreenX, &centerScreenY, map_elevation);
    centerScreenX += screenDx + 16;
    centerScreenY += screenDy + 8;

    int newCenterTile = tileFromScreenXY(centerScreenX, centerScreenY, map_elevation);
    if (newCenterTile == -1) {
        return -1;
    }

    if (tileSetCenter(newCenterTile, 0) == -1) {
        return -1;
    }

    Rect r1;
    rectCopy(&r1, &map_display_rect);

    Rect r2;
    rectCopy(&r2, &r1);

    int width = _scr_size.right - _scr_size.left + 1;
    int pitch = width;
    int height = _scr_size.bottom - _scr_size.top - 99;

    if (screenDx != 0) {
        width -= 32;
    }

    if (screenDy != 0) {
        height -= 24;
    }

    if (screenDx < 0) {
        r2.right = r2.left - screenDx;
    } else {
        r2.left = r2.right - screenDx;
    }

    unsigned char* src;
    unsigned char* dest;
    int step;
    if (screenDy < 0) {
        r1.bottom = r1.top - screenDy;
        src = display_buf + pitch * (height - 1);
        dest = display_buf + pitch * (_scr_size.bottom - _scr_size.top - 100);
        if (screenDx < 0) {
            dest -= screenDx;
        } else {
            src += screenDx;
        }
        step = -pitch;
    } else {
        r1.top = r1.bottom - screenDy;
        dest = display_buf;
        src = display_buf + pitch * screenDy;

        if (screenDx < 0) {
            dest -= screenDx;
        } else {
            src += screenDx;
        }
        step = pitch;
    }

    for (int y = 0; y < height; y++) {
        memmove(dest, src, width);
        dest += step;
        src += step;
    }

    if (screenDx != 0) {
        map_scroll_refresh(&r2);
    }

    if (screenDy != 0) {
        map_scroll_refresh(&r1);
    }

    win_draw(display_win);

    return 0;
}

// 0x482900
char* map_file_path(char* name)
{
    // 0x631E78
    static char map_path[MAX_PATH];

    if (*name != '\\') {
        sprintf(map_path, "maps\\%s", name);
        return map_path;
    }
    return name;
}

// 0x482924
int mapSetEntranceInfo(int elevation, int tile_num, int orientation)
{
    mapEntranceElevation = elevation;
    mapEntranceTileNum = tile_num;
    mapEntranceRotation = orientation;
    return 0;
}

// 0x482938
void map_new_map()
{
    map_set_elevation(0);
    tileSetCenter(20100, TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS);
    memset(&map_state, 0, sizeof(map_state));
    map_data.enteringElevation = 0;
    map_data.enteringRotation = 0;
    map_data.localVariablesCount = 0;
    map_data.version = 20;
    map_data.name[0] = '\0';
    map_data.enteringTile = 20100;
    _obj_remove_all();
    anim_stop();

    if (map_global_vars != NULL) {
        internal_free(map_global_vars);
        map_global_vars = NULL;
        num_map_global_vars = 0;
    }

    if (map_local_vars != NULL) {
        internal_free(map_local_vars);
        map_local_vars = NULL;
        num_map_local_vars = 0;
    }

    square_reset();
    map_place_dude_and_mouse();
    tileWindowRefresh();
}

// 0x482A68
int map_load(char* fileName)
{
    int rc;

    strupr(fileName);

    rc = -1;

    char* extension = strstr(fileName, ".MAP");
    if (extension != NULL) {
        strcpy(extension, ".SAV");

        const char* filePath = map_file_path(fileName);

        File* stream = fileOpen(filePath, "rb");

        strcpy(extension, ".MAP");

        if (stream != NULL) {
            fileClose(stream);
            rc = map_load_in_game(fileName);
            wmMapMusicStart();
        }
    }

    if (rc == -1) {
        const char* filePath = map_file_path(fileName);
        File* stream = fileOpen(filePath, "rb");
        if (stream != NULL) {
            rc = map_load_file(stream);
            fileClose(stream);
        }

        if (rc == 0) {
            strcpy(map_data.name, fileName);
            gDude->data.critter.combat.whoHitMe = NULL;
        }
    }

    return rc;
}

// 0x482B34
int map_load_idx(int map)
{
    scriptSetFixedParam(map_script_id, map);

    char name[16];
    if (wmMapIdxToName(map, name) == -1) {
        return -1;
    }

    wmMapIdx = map;

    int rc = map_load(name);

    wmMapMusicStart();

    return rc;
}

// 0x482B74
int map_load_file(File* stream)
{
    map_save_in_game(true);
    gsound_background_play("wind2", 12, 13, 16);
    map_disable_bk_processes();
    _partyMemberPrepLoad();
    gmouse_disable_scrolling();

    int savedMouseCursorId = gmouse_get_cursor();
    gmouse_set_cursor(MOUSE_CURSOR_WAIT_PLANET);
    fileSetReadProgressHandler(gmouse_bk_process, 32768);
    tileDisable();

    int rc = 0;

    windowFill(display_win, 0, 0, _scr_size.right - _scr_size.left + 1, _scr_size.bottom - _scr_size.top - 99, colorTable[0]);
    win_draw(display_win);
    anim_stop();
    scriptsDisable();

    map_script_id = -1;

    const char* error = NULL;

    error = "Invalid file handle";
    if (stream == NULL) {
        goto err;
    }

    error = "Error reading header";
    if (map_read_MapData(&map_data, stream) != 0) {
        goto err;
    }

    error = "Invalid map version";
    if (map_data.version != 19 && map_data.version != 20) {
        goto err;
    }

    if (mapEntranceElevation == -1) {
        mapEntranceElevation = map_data.enteringElevation;
        mapEntranceTileNum = map_data.enteringTile;
        mapEntranceRotation = map_data.enteringRotation;
    }

    _obj_remove_all();

    if (map_data.globalVariablesCount < 0) {
        map_data.globalVariablesCount = 0;
    }

    if (map_data.localVariablesCount < 0) {
        map_data.localVariablesCount = 0;
    }

    error = "Error allocating global vars";
    // NOTE: Uninline.
    if (map_allocate_global_vars(map_data.globalVariablesCount) != 0) {
        goto err;
    }

    error = "Error loading global vars";
    // NOTE: Uninline.
    if (map_load_global_vars(stream) != 0) {
        goto err;
    }

    error = "Error allocating local vars";
    // NOTE: Uninline.
    if (map_allocate_local_vars(map_data.localVariablesCount) != 0) {
        goto err;
    }

    error = "Error loading local vars";
    if (fileReadInt32List(stream, map_local_vars, num_map_local_vars) != 0) {
        goto err;
    }

    if (square_load(stream, map_data.flags) != 0) {
        goto err;
    }

    error = "Error reading scripts";
    if (scriptLoadAll(stream) != 0) {
        goto err;
    }

    error = "Error reading objects";
    if (objectLoadAll(stream) != 0) {
        goto err;
    }

    if ((map_data.flags & 1) == 0) {
        map_fix_critter_combat_data();
    }

    error = "Error setting map elevation";
    if (map_set_elevation(mapEntranceElevation) != 0) {
        goto err;
    }

    error = "Error setting tile center";
    if (tileSetCenter(mapEntranceTileNum, TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS) != 0) {
        goto err;
    }

    light_set_ambient(LIGHT_LEVEL_MAX, false);
    objectSetLocation(gDude, gCenterTile, map_elevation, NULL);
    objectSetRotation(gDude, mapEntranceRotation, NULL);
    map_data.field_34 = wmMapMatchNameToIdx(map_data.name);

    if ((map_data.flags & 1) == 0) {
        char path[MAX_PATH];
        sprintf(path, "maps\\%s", map_data.name);

        char* extension = strstr(path, ".MAP");
        if (extension == NULL) {
            extension = strstr(path, ".map");
        }

        if (extension != NULL) {
            *extension = '\0';
        }

        strcat(path, ".GAM");
        game_load_info_vars(path, "MAP_GLOBAL_VARS:", &num_map_global_vars, &map_global_vars);
        map_data.globalVariablesCount = num_map_global_vars;
    }

    scriptsEnable();

    if (map_data.scriptIndex > 0) {
        error = "Error creating new map script";
        if (scriptAdd(&map_script_id, SCRIPT_TYPE_SYSTEM) == -1) {
            goto err;
        }

        Object* object;
        int fid = art_id(OBJ_TYPE_MISC, 12, 0, 0, 0);
        objectCreateWithFidPid(&object, fid, -1);
        object->flags |= (OBJECT_LIGHT_THRU | OBJECT_TEMPORARY | OBJECT_HIDDEN);
        objectSetLocation(object, 1, 0, NULL);
        object->sid = map_script_id;
        scriptSetFixedParam(map_script_id, (map_data.flags & 1) == 0);

        Script* script;
        scriptGetScript(map_script_id, &script);
        script->field_14 = map_data.scriptIndex - 1;
        script->flags |= SCRIPT_FLAG_0x08;
        object->id = scriptsNewObjectId();
        script->field_1C = object->id;
        script->owner = object;
        _scr_spatials_disable();
        scriptExecProc(map_script_id, SCRIPT_PROC_MAP_ENTER);
        _scr_spatials_enable();

        error = "Error Setting up random encounter";
        if (wmSetupRandomEncounter() == -1) {
            goto err;
        }
    }

    error = NULL;

err:

    if (error != NULL) {
        char message[100]; // TODO: Size is probably wrong.
        sprintf(message, "%s while loading map.", error);
        debugPrint(message);
        map_new_map();
        rc = -1;
    } else {
        _obj_preload_art_cache(map_data.flags);
    }

    _partyMemberRecoverLoad();
    intface_show();
    _proto_dude_update_gender();
    map_place_dude_and_mouse();
    fileSetReadProgressHandler(NULL, 0);
    map_enable_bk_processes();
    gmouse_disable_scrolling();
    gmouse_set_cursor(MOUSE_CURSOR_WAIT_PLANET);

    if (scriptsExecStartProc() == -1) {
        debugPrint("\n   Error: scr_load_all_scripts failed!");
    }

    scriptsExecMapEnterProc();
    scriptsExecMapUpdateProc();
    tileEnable();

    if (map_state.map > 0) {
        if (map_state.rotation >= 0) {
            objectSetRotation(gDude, map_state.rotation, NULL);
        }
    } else {
        tileWindowRefresh();
    }

    gameTimeScheduleUpdateEvent();

    if (gsound_sfx_q_start() == -1) {
        rc = -1;
    }

    wmMapMarkVisited(map_data.field_34);
    wmMapMarkMapEntranceState(map_data.field_34, map_elevation, 1);

    if (wmCheckGameAreaEvents() != 0) {
        rc = -1;
    }

    fileSetReadProgressHandler(NULL, 0);

    if (game_ui_is_disabled() == 0) {
        gmouse_enable_scrolling();
    }

    gmouse_set_cursor(savedMouseCursorId);

    mapEntranceElevation = -1;
    mapEntranceTileNum = -1;
    mapEntranceRotation = -1;

    gmPaletteFinish();

    map_data.version = 20;

    return rc;
}

// 0x483188
int map_load_in_game(char* fileName)
{
    debugPrint("\nMAP: Loading SAVED map.");

    char mapName[16]; // TODO: Size is probably wrong.
    strmfe(mapName, fileName, "SAV");

    int rc = map_load(mapName);

    if (gameTimeGetTime() >= map_data.lastVisitTime) {
        if (((gameTimeGetTime() - map_data.lastVisitTime) / GAME_TIME_TICKS_PER_HOUR) >= 24) {
            objectUnjamAll();
        }

        if (map_age_dead_critters() == -1) {
            debugPrint("\nError: Critter aging failed on map load!");
            return -1;
        }
    }

    if (!wmMapIsSaveable()) {
        debugPrint("\nDestroying RANDOM encounter map.");

        char v15[16];
        strcpy(v15, map_data.name);

        strmfe(map_data.name, v15, "SAV");

        MapDirEraseFile("MAPS\\", map_data.name);

        strcpy(map_data.name, v15);
    }

    return rc;
}

// 0x48328C
int map_age_dead_critters()
{
    if (!wmMapDeadBodiesAge()) {
        return 0;
    }

    int hoursSinceLastVisit = (gameTimeGetTime() - map_data.lastVisitTime) / GAME_TIME_TICKS_PER_HOUR;
    if (hoursSinceLastVisit == 0) {
        return 0;
    }

    Object* obj = objectFindFirst();
    while (obj != NULL) {
        if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER
            && obj != gDude
            && !objectIsPartyMember(obj)
            && !critter_is_dead(obj)) {
            obj->data.critter.combat.maneuver &= ~CRITTER_MANUEVER_FLEEING;
            if (critterGetKillType(obj) != KILL_TYPE_ROBOT && critter_flag_check(obj->pid, CRITTER_NO_HEAL) == 0) {
                critter_heal_hours(obj, hoursSinceLastVisit);
            }
        }
        obj = objectFindNext();
    }

    int agingType;
    if (hoursSinceLastVisit > 6 * 24) {
        agingType = 1;
    } else if (hoursSinceLastVisit > 14 * 24) {
        agingType = 2;
    } else {
        return 0;
    }

    int capacity = 100;
    int count = 0;
    Object** objects = (Object**)internal_malloc(sizeof(*objects) * capacity);

    obj = objectFindFirst();
    while (obj != NULL) {
        int type = PID_TYPE(obj->pid);
        if (type == OBJ_TYPE_CRITTER) {
            if (obj != gDude && critter_is_dead(obj)) {
                if (critterGetKillType(obj) != KILL_TYPE_ROBOT && critter_flag_check(obj->pid, CRITTER_NO_HEAL) == 0) {
                    objects[count++] = obj;

                    if (count >= capacity) {
                        capacity *= 2;
                        objects = (Object**)internal_realloc(objects, sizeof(*objects) * capacity);
                        if (objects == NULL) {
                            debugPrint("\nError: Out of Memory!");
                            return -1;
                        }
                    }
                }
            }
        } else if (agingType == 2 && type == OBJ_TYPE_MISC && obj->pid == 0x500000B) {
            objects[count++] = obj;
            if (count >= capacity) {
                capacity *= 2;
                objects = (Object**)internal_realloc(objects, sizeof(*objects) * capacity);
                if (objects == NULL) {
                    debugPrint("\nError: Out of Memory!");
                    return -1;
                }
            }
        }
        obj = objectFindNext();
    }

    int rc = 0;
    for (int index = 0; index < count; index++) {
        Object* obj = objects[index];
        if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
            if (critter_flag_check(obj->pid, CRITTER_NO_DROP) == 0) {
                item_drop_all(obj, obj->tile);
            }

            Object* blood;
            if (objectCreateWithPid(&blood, 0x5000004) == -1) {
                rc = -1;
                break;
            }

            objectSetLocation(blood, obj->tile, obj->elevation, NULL);

            Proto* proto;
            protoGetProto(obj->pid, &proto);

            int frame = randomBetween(0, 3);
            if ((proto->critter.flags & 0x800)) {
                frame += 6;
            } else {
                if (critterGetKillType(obj) != KILL_TYPE_RAT
                    && critterGetKillType(obj) != KILL_TYPE_MANTIS) {
                    frame += 3;
                }
            }

            objectSetFrame(blood, frame, NULL);
        }

        register_clear(obj);
        objectDestroy(obj, NULL);
    }

    internal_free(objects);

    return rc;
}

// 0x48358C
int map_target_load_area()
{
    int city = -1;
    if (wmMatchAreaContainingMapIdx(map_data.field_34, &city) == -1) {
        city = -1;
    }
    return city;
}

// 0x4835B4
int map_leave_map(MapTransition* transition)
{
    if (transition == NULL) {
        return -1;
    }

    memcpy(&map_state, transition, sizeof(map_state));

    if (map_state.map == 0) {
        map_state.map = -2;
    }

    if (isInCombat()) {
        game_user_wants_to_quit = 1;
    }

    return 0;
}

// 0x4835F8
int map_check_state()
{
    if (map_state.map == 0) {
        return 0;
    }

    gmouse_3d_off();

    gmouse_set_cursor(MOUSE_CURSOR_NONE);

    if (map_state.map == -1) {
        if (!isInCombat()) {
            anim_stop();
            wmTownMap();
            memset(&map_state, 0, sizeof(map_state));
        }
    } else if (map_state.map == -2) {
        if (!isInCombat()) {
            anim_stop();
            wmWorldMap();
            memset(&map_state, 0, sizeof(map_state));
        }
    } else {
        if (!isInCombat()) {
            if (map_state.map != map_data.field_34 || map_elevation == map_state.elevation) {
                map_load_idx(map_state.map);
            }

            if (map_state.tile != -1 && map_state.tile != 0
                && map_data.field_34 != MAP_MODOC_BEDNBREAKFAST && map_data.field_34 != MAP_THE_SQUAT_A
                && elevationIsValid(map_state.elevation)) {
                objectSetLocation(gDude, map_state.tile, map_state.elevation, NULL);
                map_set_elevation(map_state.elevation);
                objectSetRotation(gDude, map_state.rotation, NULL);
            }

            if (tileSetCenter(gDude->tile, TILE_SET_CENTER_REFRESH_WINDOW) == -1) {
                debugPrint("\nError: map: attempt to center out-of-bounds!");
            }

            memset(&map_state, 0, sizeof(map_state));

            int city;
            wmMatchAreaContainingMapIdx(map_data.field_34, &city);
            if (wmTeleportToArea(city) == -1) {
                debugPrint("\nError: couldn't make jump on worldmap for map jump!");
            }
        }
    }

    return 0;
}

// 0x483784
void map_fix_critter_combat_data()
{
    for (Object* object = objectFindFirst(); object != NULL; object = objectFindNext()) {
        if (object->pid == -1) {
            continue;
        }

        if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
            continue;
        }

        if (object->data.critter.combat.whoHitMeCid == -1) {
            object->data.critter.combat.whoHitMe = NULL;
        }
    }
}

// 0x483850
int map_save()
{
    char temp[80];
    temp[0] = '\0';

    char* masterPatchesPath;
    if (config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &masterPatchesPath)) {
        strcat(temp, masterPatchesPath);
        mkdir(temp);

        strcat(temp, "\\MAPS");
        mkdir(temp);
    }

    int rc = -1;
    if (map_data.name[0] != '\0') {
        char* mapFileName = map_file_path(map_data.name);
        File* stream = fileOpen(mapFileName, "wb");
        if (stream != NULL) {
            rc = map_save_file(stream);
            fileClose(stream);
        } else {
            sprintf(temp, "Unable to open %s to write!", map_data.name);
            debugPrint(temp);
        }

        if (rc == 0) {
            sprintf(temp, "%s saved.", map_data.name);
            debugPrint(temp);
        }
    } else {
        debugPrint("\nError: map_save: map header corrupt!");
    }

    return rc;
}

// 0x483980
int map_save_file(File* stream)
{
    if (stream == NULL) {
        return -1;
    }

    scriptsDisable();

    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        int tile;
        for (tile = 0; tile < SQUARE_GRID_SIZE; tile++) {
            int fid;

            fid = art_id(OBJ_TYPE_TILE, square[elevation]->field_0[tile] & 0xFFF, 0, 0, 0);
            if (fid != art_id(OBJ_TYPE_TILE, 1, 0, 0, 0)) {
                break;
            }

            fid = art_id(OBJ_TYPE_TILE, (square[elevation]->field_0[tile] >> 16) & 0xFFF, 0, 0, 0);
            if (fid != art_id(OBJ_TYPE_TILE, 1, 0, 0, 0)) {
                break;
            }
        }

        if (tile == SQUARE_GRID_SIZE) {
            Object* object = objectFindFirstAtElevation(elevation);
            if (object != NULL) {
                // TODO: Implementation is slightly different, check in debugger.
                while (object != NULL && (object->flags & OBJECT_TEMPORARY)) {
                    object = objectFindNextAtElevation();
                }

                if (object != NULL) {
                    map_data.flags &= ~map_data_elev_flags[elevation];
                } else {
                    map_data.flags |= map_data_elev_flags[elevation];
                }
            } else {
                map_data.flags |= map_data_elev_flags[elevation];
            }
        } else {
            map_data.flags &= ~map_data_elev_flags[elevation];
        }
    }

    map_data.localVariablesCount = num_map_local_vars;
    map_data.globalVariablesCount = num_map_global_vars;
    map_data.darkness = 1;

    map_write_MapData(&map_data, stream);

    if (map_data.globalVariablesCount != 0) {
        fileWriteInt32List(stream, map_global_vars, map_data.globalVariablesCount);
    }

    if (map_data.localVariablesCount != 0) {
        fileWriteInt32List(stream, map_local_vars, map_data.localVariablesCount);
    }

    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        if ((map_data.flags & map_data_elev_flags[elevation]) == 0) {
            _db_fwriteLongCount(stream, square[elevation]->field_0, SQUARE_GRID_SIZE);
        }
    }

    char err[80];

    if (scriptSaveAll(stream) == -1) {
        sprintf(err, "Error saving scripts in %s", map_data.name);
        _win_msg(err, 80, 80, colorTable[31744]);
    }

    if (objectSaveAll(stream) == -1) {
        sprintf(err, "Error saving objects in %s", map_data.name);
        _win_msg(err, 80, 80, colorTable[31744]);
    }

    scriptsEnable();

    return 0;
}

// 0x483C98
int map_save_in_game(bool a1)
{
    if (map_data.name[0] == '\0') {
        return 0;
    }

    anim_stop();
    _partyMemberSaveProtos();

    if (a1) {
        _queue_leaving_map();
        _partyMemberPrepLoad();
        _partyMemberPrepItemSaveAll();
        scriptsExecMapExitProc();

        if (map_script_id != -1) {
            Script* script;
            scriptGetScript(map_script_id, &script);
        }

        gameTimeScheduleUpdateEvent();
        _obj_reset_roof();
    }

    map_data.flags |= 0x01;
    map_data.lastVisitTime = gameTimeGetTime();

    char name[16];

    if (a1 && !wmMapIsSaveable()) {
        debugPrint("\nNot saving RANDOM encounter map.");

        strcpy(name, map_data.name);
        strmfe(map_data.name, name, "SAV");
        MapDirEraseFile("MAPS\\", map_data.name);
        strcpy(map_data.name, name);
    } else {
        debugPrint("\n Saving \".SAV\" map.");

        strcpy(name, map_data.name);
        strmfe(map_data.name, name, "SAV");
        if (map_save() == -1) {
            return -1;
        }

        strcpy(map_data.name, name);

        automap_pip_save();

        if (a1) {
            map_data.name[0] = '\0';
            _obj_remove_all();
            _proto_remove_all();
            square_reset();
            gameTimeScheduleUpdateEvent();
        }
    }

    return 0;
}

// 0x483E28
void map_setup_paths()
{
    char path[FILENAME_MAX];

    char* masterPatchesPath;
    if (config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &masterPatchesPath)) {
        strcpy(path, masterPatchesPath);
    } else {
        strcpy(path, "DATA");
    }

    mkdir(path);

    strcat(path, "\\MAPS");
    mkdir(path);
}

// 0x483ED0
static void map_display_draw(Rect* rect)
{
    win_draw_rect(display_win, rect);
}

// 0x483EE4
static void map_scroll_refresh_game(Rect* rect)
{
    Rect clampedDirtyRect;
    if (rectIntersection(rect, &map_display_rect, &clampedDirtyRect) == -1) {
        return;
    }

    tileRenderFloorsInRect(&clampedDirtyRect, map_elevation);
    _grid_render(&clampedDirtyRect, map_elevation);
    _obj_render_pre_roof(&clampedDirtyRect, map_elevation);
    tileRenderRoofsInRect(&clampedDirtyRect, map_elevation);
    _obj_render_post_roof(&clampedDirtyRect, map_elevation);
}

// 0x483F44
static void map_scroll_refresh_mapper(Rect* rect)
{
    Rect clampedDirtyRect;
    if (rectIntersection(rect, &map_display_rect, &clampedDirtyRect) == -1) {
        return;
    }

    bufferFill(display_buf + clampedDirtyRect.top * (_scr_size.right - _scr_size.left + 1) + clampedDirtyRect.left,
        clampedDirtyRect.right - clampedDirtyRect.left + 1,
        clampedDirtyRect.bottom - clampedDirtyRect.top + 1,
        _scr_size.right - _scr_size.left + 1,
        0);
    tileRenderFloorsInRect(&clampedDirtyRect, map_elevation);
    _grid_render(&clampedDirtyRect, map_elevation);
    _obj_render_pre_roof(&clampedDirtyRect, map_elevation);
    tileRenderRoofsInRect(&clampedDirtyRect, map_elevation);
    _obj_render_post_roof(&clampedDirtyRect, map_elevation);
}

// NOTE: Inlined.
//
// 0x483FE4
static int map_allocate_global_vars(int count)
{
    map_free_global_vars();

    if (count != 0) {
        map_global_vars = (int*)internal_malloc(sizeof(*map_global_vars) * count);
        if (map_global_vars == NULL) {
            return -1;
        }
    }

    num_map_global_vars = count;

    return 0;
}

// 0x484038
static void map_free_global_vars()
{
    if (map_global_vars != NULL) {
        internal_free(map_global_vars);
        map_global_vars = NULL;
        num_map_global_vars = 0;
    }
}

// NOTE: Inlined.
//
// 0x48405C
static int map_load_global_vars(File* stream)
{
    if (fileReadInt32List(stream, map_global_vars, num_map_global_vars) != 0) {
        return -1;
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x484080
static int map_allocate_local_vars(int count)
{
    map_free_local_vars();

    if (count != 0) {
        map_local_vars = (int*)internal_malloc(sizeof(*map_local_vars) * count);
        if (map_local_vars == NULL) {
            return -1;
        }
    }

    num_map_local_vars = count;

    return 0;
}

// 0x4840D4
static void map_free_local_vars()
{
    if (map_local_vars != NULL) {
        internal_free(map_local_vars);
        map_local_vars = NULL;
        num_map_local_vars = 0;
    }
}

// 0x48411C
static void map_place_dude_and_mouse()
{
    _obj_clear_seen();

    if (gDude != NULL) {
        if (FID_ANIM_TYPE(gDude->fid) != ANIM_STAND) {
            objectSetFrame(gDude, 0, 0);
            gDude->fid = art_id(OBJ_TYPE_CRITTER, gDude->fid & 0xFFF, ANIM_STAND, (gDude->fid & 0xF000) >> 12, gDude->rotation + 1);
        }

        if (gDude->tile == -1) {
            objectSetLocation(gDude, gCenterTile, map_elevation, NULL);
            objectSetRotation(gDude, map_data.enteringRotation, 0);
        }

        objectSetLight(gDude, 4, 0x10000, 0);
        gDude->flags |= OBJECT_TEMPORARY;

        dude_stand(gDude, gDude->rotation, gDude->fid);
        _partyMemberSyncPosition();
    }

    gmouse_3d_reset_fid();
    gmouse_3d_on();
}

// 0x484210
static void square_reset()
{
    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        int* p = square[elevation]->field_0;
        for (int y = 0; y < SQUARE_GRID_HEIGHT; y++) {
            for (int x = 0; x < SQUARE_GRID_WIDTH; x++) {
                // TODO: Strange math, initially right, but need to figure it out and
                // check subsequent calls.
                int fid = *p;
                fid &= ~0xFFFF;
                *p = (((art_id(OBJ_TYPE_TILE, 1, 0, 0, 0) & 0xFFF) | (((fid >> 16) & 0xF000) >> 12)) << 16) | (fid & 0xFFFF);

                fid = *p;
                int v3 = (fid & 0xF000) >> 12;
                int v4 = (art_id(OBJ_TYPE_TILE, 1, 0, 0, 0) & 0xFFF) | v3;

                fid &= ~0xFFFF;

                *p = v4 | ((fid >> 16) << 16);

                p++;
            }
        }
    }
}

// 0x48431C
static int square_load(File* stream, int flags)
{
    int v6;
    int v7;
    int v8;
    int v9;

    square_reset();

    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        if ((flags & map_data_elev_flags[elevation]) == 0) {
            int* arr = square[elevation]->field_0;
            if (_db_freadIntCount(stream, arr, SQUARE_GRID_SIZE) != 0) {
                return -1;
            }

            for (int tile = 0; tile < SQUARE_GRID_SIZE; tile++) {
                v6 = arr[tile];
                v6 &= ~(0xFFFF);
                v6 >>= 16;

                v7 = (v6 & 0xF000) >> 12;
                v7 &= ~(0x01);

                v8 = v6 & 0xFFF;
                v9 = arr[tile] & 0xFFFF;
                arr[tile] = ((v8 | (v7 << 12)) << 16) | v9;
            }
        }
    }

    return 0;
}

// 0x4843B8
static int map_write_MapData(MapHeader* ptr, File* stream)
{
    if (fileWriteInt32(stream, ptr->version) == -1) return -1;
    if (fileWriteFixedLengthString(stream, ptr->name, 16) == -1) return -1;
    if (fileWriteInt32(stream, ptr->enteringTile) == -1) return -1;
    if (fileWriteInt32(stream, ptr->enteringElevation) == -1) return -1;
    if (fileWriteInt32(stream, ptr->enteringRotation) == -1) return -1;
    if (fileWriteInt32(stream, ptr->localVariablesCount) == -1) return -1;
    if (fileWriteInt32(stream, ptr->scriptIndex) == -1) return -1;
    if (fileWriteInt32(stream, ptr->flags) == -1) return -1;
    if (fileWriteInt32(stream, ptr->darkness) == -1) return -1;
    if (fileWriteInt32(stream, ptr->globalVariablesCount) == -1) return -1;
    if (fileWriteInt32(stream, ptr->field_34) == -1) return -1;
    if (fileWriteInt32(stream, ptr->lastVisitTime) == -1) return -1;
    if (fileWriteInt32List(stream, ptr->field_3C, 44) == -1) return -1;

    return 0;
}

// 0x4844B4
static int map_read_MapData(MapHeader* ptr, File* stream)
{
    if (fileReadInt32(stream, &(ptr->version)) == -1) return -1;
    if (fileReadFixedLengthString(stream, ptr->name, 16) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->enteringTile)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->enteringElevation)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->enteringRotation)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->localVariablesCount)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->scriptIndex)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->flags)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->darkness)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->globalVariablesCount)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->field_34)) == -1) return -1;
    if (fileReadInt32(stream, &(ptr->lastVisitTime)) == -1) return -1;
    if (fileReadInt32List(stream, ptr->field_3C, 44) == -1) return -1;

    return 0;
}
