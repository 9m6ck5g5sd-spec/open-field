// Step M3, the base pack: the folders of the disk mode that the base answers the loader for. This is H6's unit,
// which every base before M3 carries, with two calls added and nothing else changed: the root's child maker asks
// the folders unit (module/build_m3_folders.cpp, op1f_folders_child) for a name that is not fx or lfo before it
// hands the name to stock, and the population, after fx, lfo and the old packs folder, has the folders unit make
// the folders the cards asked to be shown (op1f_folders_populate).
//
// What follows is H6's text, with what was added to it since.
//
// Build H image H6: the chain presets as files in the disk mode.
//
// What this unit uses of the firmware's disk mode (docs/methods.md says how such things are found):
//   * The disk mode shows a tree of folders and files that the firmware builds when the mode starts; it is no copy
//     of the disk's folders. 0x8fcaccf0(parent, name) makes a folder in the tree and 0x8fcacf48(folder, name) adds a
//     file to one. What a folder does is an object beside it, its behaviour, with six functions in this order: two
//     that take it away (and free nothing when the object is static), one with nothing to do, one that answers 0,
//     one that answers a folder inside it by its name (a behaviour, or 0 for none), and one that answers a file by
//     its name (a file object, or 0 to refuse the name). The root folder's function for a folder's name is
//     0x8fcab080, and the word that holds its address is 0x8490f810.
//   * A file object for a file that lies on the disk is made by 0x8fcb8d98(obj, path) in 0x40 bytes of the
//     firmware's allocator (0x8d4e5854), with the path as a C text; the firmware's own tape folders use it for a
//     tape's settings file. Reading, writing through a spare file and a rename, the size and the time are the
//     class's own. Its function for a delete from the computer, the eighth of its table, does nothing, so this unit
//     gives its file objects a table of their own with that one function replaced (K6b).
//
// The design: a root folder "fx" with sixty-four files "01.json" to "64.json" (H6b; H6 had 32), each the preset of
// that slot in the module's folder user/fx/ (the chain unit writes and reads them: build_h_chain.cpp), served by
// stock's filesystem-backed file class, so the host reads a preset as a small JSON text, writes one over a slot's
// file (the chain unit re-reads a changed file when the list opens) and sees the names it knows from the list.
// Two edits (group build-h6): the root behaviour's +0x10 slot (a `word` edit at 0x8490f810) -> op1f_mtp_root_child,
// and a CALL.a at 0x8fcaddea -> op1f_mtp_populate_stub (build_h_disk.s), which makes the "fx" folder after the
// stock folders. The chain unit makes the folder user/fx/ itself (with the trailing slash createFolderRecursive
// needs: H6 passed user/fx and only user was made, so every file write failed its rename). The tree's build logs nothing; the chain unit prints the
// counters from the frame hook. Since K6b a delete logs a line (stock's own folder maker logs on this thread).
// Disk-mode thread (the tree build, the lookups and the deletes), no allocation but the file objects stock frees.

typedef unsigned int u32;
typedef unsigned char u8;

typedef void* (*MakeFolderFn)(void* parent, const char* name);              // 0x8fcaccf0
typedef void* (*AddFileFn)(void* folder, const char* name);                  // 0x8fcacf48
typedef void* (*HeapAllocFn)(u32 bytes);                                     // 0x8d4e5854
typedef void (*FsFileCtorFn)(void* obj, const char* path);                   // 0x8fcb8d98: the filesystem-backed file object (0x40 bytes)
typedef void* (*RootChildFn)(void* behaviour, const char* name);             // 0x8fcab080: stock's root child maker

#define STOCK_MAKE_FOLDER ((MakeFolderFn)0x8fcaccf0u)
#define STOCK_ADD_FILE ((AddFileFn)0x8fcacf48u)
#define STOCK_HEAP_ALLOC ((HeapAllocFn)0x8d4e5854u)
#define STOCK_FS_FILE_CTOR ((FsFileCtorFn)0x8fcb8d98u)
#define STOCK_ROOT_CHILD ((RootChildFn)0x8fcab080u)

enum { SLOTS = 64, FS_FILE_BYTES = 0x40 };                                  // 64 slots (H6b)

// K6b: a delete that works. The disk mode's delete calls the file object's function at +0x1c and then takes the
// node away. For the firmware's filesystem-backed class that function does nothing (0x8fcb8a74), so a delete of a
// preset or a pack hid the file until the next visit and left it on the disk. Here every file object of ours gets
// a copy of the class's twelve functions with the delete ours: the file is written anew as an empty slot's file, {}
// from the firmware's json writer, which is what the preset units and the slot maker write for an empty slot. The
// slot stays listed, empty: these folders are lists of slots.
typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
typedef void (*JsonCtorFn)(void* json, int kind);
typedef void (*JsonDtorFn)(void* json);
typedef int (*JsonWriteFileFn)(void* path_string, void* json);
typedef void (*StringCtorFn)(void* out, const char* text);

#define STOCK_LOG ((LogFn)0x8c0539f0u)
#define STOCK_JSON_CTOR ((JsonCtorFn)0x8c085b78u)
#define STOCK_JSON_DTOR ((JsonDtorFn)0x8c0844b0u)
#define STOCK_JSON_WRITE_FILE ((JsonWriteFileFn)0x8c053138u)
#define STOCK_STRING_CTOR ((StringCtorFn)0x8c03e40cu)
#define STOCK_FS_EMPTY_DELETE ((u32)0x8fcb8a74u)

enum { FS_FILE_SLOTS = 12, FS_DELETE_SLOT = 7, FS_PATH_OWN = 8, FS_PATH_ROOM = 0x1c, JSON_OBJECT_KIND = 7, DELETE_LINES = 16 };
static const char LOG_FILE[] = "../src/main/op1f_disk.cpp";

extern "C" u32 op1f_disk_deletes asm("op1f_disk_deletes");                   // files of ours deleted from the computer (emptied)
u32 op1f_disk_deletes;
static u32 file_vtable[FS_FILE_SLOTS];                                      // stock's twelve slots with the delete ours; made at the first file object

// The delete slot: (object). The object's path is the stock string at +4: its own buffer at +8 while the room word
// at +0x1c is under sixteen, else a pointer there (as stock's own size, time and set-time virtuals read it).
extern "C" void op1f_mtp_file_delete(void* obj) asm("op1f_mtp_file_delete");
extern "C" void op1f_mtp_file_delete(void* obj) {
    const char* own = (const char*)obj + FS_PATH_OWN;
    const char* path = *(const u32*)((const char*)obj + FS_PATH_ROOM) >= 16 ? *(const char* const*)own : own;
    u32 json[4];
    for (int i = 0; i < 4; ++i) json[i] = 0;
    STOCK_JSON_CTOR(json, JSON_OBJECT_KIND);
    u32 text[12];
    for (int i = 0; i < 12; ++i) text[i] = 0;
    STOCK_STRING_CTOR(text, path);
    int written = STOCK_JSON_WRITE_FILE(text, json);
    STOCK_JSON_DTOR(json);
    ++op1f_disk_deletes;
    if (!written || op1f_disk_deletes <= DELETE_LINES) {
        STOCK_LOG(written ? 3 : 1, LOG_FILE, 1, "disk: %s deleted from the computer: %s", path, written ? "the slot is empty now" : "its file could NOT be written anew");
    }
}

// A file object fresh from stock's constructor gets the delete above.
static void give_delete(void* obj) {
    u32* stock = *(u32**)obj;
    if (!file_vtable[0]) {
        if (stock[FS_DELETE_SLOT] != STOCK_FS_EMPTY_DELETE) return;         // not the class as it was read: left as stock made it
        for (int i = 0; i < FS_FILE_SLOTS; ++i) file_vtable[i] = stock[i];
        file_vtable[FS_DELETE_SLOT] = (u32)&op1f_mtp_file_delete;
    }
    *(u32**)obj = file_vtable;
}
static const char FOLDER_NAME[] = "fx";
static const char PATH_PREFIX[] = "user/fx/";                               // + "NN.json": 15 characters, inside the string object's own buffer

// the file names, "01.json" .. "64.json" (the node copies the name; these stay for the lookups)
static const char FILE_NAMES[SLOTS][8] = {
    "01.json", "02.json", "03.json", "04.json", "05.json", "06.json", "07.json", "08.json", "09.json", "10.json", "11.json", "12.json", "13.json", "14.json", "15.json", "16.json",
    "17.json", "18.json", "19.json", "20.json", "21.json", "22.json", "23.json", "24.json", "25.json", "26.json", "27.json", "28.json", "29.json", "30.json", "31.json", "32.json",
    "33.json", "34.json", "35.json", "36.json", "37.json", "38.json", "39.json", "40.json", "41.json", "42.json", "43.json", "44.json", "45.json", "46.json", "47.json", "48.json",
    "49.json", "50.json", "51.json", "52.json", "53.json", "54.json", "55.json", "56.json", "57.json", "58.json", "59.json", "60.json", "61.json", "62.json", "63.json", "64.json", };

extern "C" u32 op1f_disk_folders asm("op1f_disk_folders");                   // the fx folder made (per disk mode start)
extern "C" u32 op1f_disk_files asm("op1f_disk_files");                       // file names accepted at the population
extern "C" u32 op1f_disk_lookups asm("op1f_disk_lookups");                   // lookups answered with a file object (the population's and the host's)
extern "C" u32 op1f_disk_refused asm("op1f_disk_refused");                   // names refused (not NN.json, or out of 1..32)
extern "C" u32 op1f_disk_root_children asm("op1f_disk_root_children");       // stock's root child requests passed through
u32 op1f_disk_folders, op1f_disk_files, op1f_disk_lookups, op1f_disk_refused, op1f_disk_root_children;

static int same(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}

// "NN.json" with NN in 01..64, else -1
static int slot_of_name(const char* name) {
    if (!name) return -1;
    if (name[0] < '0' || name[0] > '9' || name[1] < '0' || name[1] > '9') return -1;
    if (!same(name + 2, ".json")) return -1;
    int n = (name[0] - '0') * 10 + (name[1] - '0');
    return (n >= 1 && n <= SLOTS) ? n - 1 : -1;
}

// ---------------------------------------------------------------- the fx folder's behaviour

extern "C" void op1f_mtp_fx_noop(void* self) asm("op1f_mtp_fx_noop");
extern "C" void op1f_mtp_fx_noop(void* self) { (void)self; }                // the destructors (the object is static) and the +8 slot

extern "C" void* op1f_mtp_fx_zero(void* self, const char* name) asm("op1f_mtp_fx_zero");
extern "C" void* op1f_mtp_fx_zero(void* self, const char* name) { (void)self; (void)name; return 0; }   // +0xc, and +0x10: no child folders

extern "C" void* op1f_mtp_fx_lookup(void* self, const char* name) asm("op1f_mtp_fx_lookup");
extern "C" void* op1f_mtp_fx_lookup(void* self, const char* name) {       // +0x14: the slot's file as stock's filesystem-backed file object
    (void)self;
    int slot = slot_of_name(name);
    if (slot < 0) { ++op1f_disk_refused; return 0; }
    void* obj = STOCK_HEAP_ALLOC(FS_FILE_BYTES);
    if (!obj) return 0;
    char path[16];
    char* p = path;
    for (const char* s = PATH_PREFIX; *s; ++s) *p++ = *s;
    for (const char* s = FILE_NAMES[slot]; *s; ++s) *p++ = *s;
    *p = 0;
    STOCK_FS_FILE_CTOR(obj, path);
    give_delete(obj);
    ++op1f_disk_lookups;
    return obj;
}

typedef void* (*BehaviourFn)();
static const BehaviourFn fx_behaviour_vtable[6] = {
    (BehaviourFn)op1f_mtp_fx_noop,                                          // the destructor
    (BehaviourFn)op1f_mtp_fx_noop,                                          // the deleting destructor: nothing to free, the object is the module's
    (BehaviourFn)op1f_mtp_fx_noop,                                          // +8
    (BehaviourFn)op1f_mtp_fx_zero,                                          // +0xc
    (BehaviourFn)op1f_mtp_fx_zero,                                          // +0x10: no child folders
    (BehaviourFn)op1f_mtp_fx_lookup,                                        // +0x14: the files
};
static const void* fx_behaviour[2] = { fx_behaviour_vtable, 0 };

// ---------------------------------------------------------------- the lfo folder's behaviour (I8)
//
// Build I image I8: the LFO presets' sixty-four files as a second root
// folder, "lfo", the same way as "fx": the same names, the same stock file class, on the files the LFO presets unit
// keeps (build_i_lfopresets.cpp). On the eMMC their folder is user/lf/: one letter shorter than the name the disk
// mode shows, so that user/lf/NN.json keeps to the fifteen characters a stock string holds in its own buffer, as
// user/fx/NN.json does (user/lfo/NN.json has sixteen).
static const char LFO_FOLDER_NAME[] = "lfo";
static const char LFO_PATH_PREFIX[] = "user/lf/";                           // + "NN.json": 15 characters

extern "C" u32 op1f_disk_lfo_folders asm("op1f_disk_lfo_folders");           // the lfo folder made (per disk mode start)
extern "C" u32 op1f_disk_lfo_files asm("op1f_disk_lfo_files");               // file names accepted at its population
extern "C" u32 op1f_disk_lfo_lookups asm("op1f_disk_lfo_lookups");           // its lookups answered with a file object
extern "C" u32 op1f_disk_lfo_refused asm("op1f_disk_lfo_refused");           // its names refused
u32 op1f_disk_lfo_folders, op1f_disk_lfo_files, op1f_disk_lfo_lookups, op1f_disk_lfo_refused;

extern "C" void* op1f_mtp_lfo_lookup(void* self, const char* name) asm("op1f_mtp_lfo_lookup");
extern "C" void* op1f_mtp_lfo_lookup(void* self, const char* name) {      // +0x14: the slot's file, as op1f_mtp_fx_lookup
    (void)self;
    int slot = slot_of_name(name);
    if (slot < 0) { ++op1f_disk_lfo_refused; return 0; }
    void* obj = STOCK_HEAP_ALLOC(FS_FILE_BYTES);
    if (!obj) return 0;
    char path[16];
    char* p = path;
    for (const char* s = LFO_PATH_PREFIX; *s; ++s) *p++ = *s;
    for (const char* s = FILE_NAMES[slot]; *s; ++s) *p++ = *s;
    *p = 0;
    STOCK_FS_FILE_CTOR(obj, path);
    give_delete(obj);
    ++op1f_disk_lfo_lookups;
    return obj;
}

static const BehaviourFn lfo_behaviour_vtable[6] = {
    (BehaviourFn)op1f_mtp_fx_noop,                                          // the destructor
    (BehaviourFn)op1f_mtp_fx_noop,                                          // the deleting destructor
    (BehaviourFn)op1f_mtp_fx_noop,                                          // +8
    (BehaviourFn)op1f_mtp_fx_zero,                                          // +0xc
    (BehaviourFn)op1f_mtp_fx_zero,                                          // +0x10: no child folders
    (BehaviourFn)op1f_mtp_lfo_lookup,                                       // +0x14: the files
};
static const void* lfo_behaviour[2] = { lfo_behaviour_vtable, 0 };

// ---------------------------------------------------------------- the packs folder's behaviour (J3)
//
// Build J image J3: a third root folder, "packs", with the one slot
// "01.pack", so that the owner can copy a pack file onto the disk. On the eMMC it is user/pk/01.pack (fifteen
// characters). The folder is there only in an image that links the pack unit (build_j_pack.cpp), which makes the
// eMMC folder and the slot's placeholder when the tree is built; without it the name is stock's to refuse.
// Since J4 there are two slots, 01.pack and the spare 02.pack, and the unit that makes them is build_j_slots.cpp.
static const char PACK_FOLDER_NAME[] = "packs";
static const char PACK_FILE_NAME[] = "01.pack";
static const char PACK_FILE_PATH[] = "user/pk/01.pack";
static const char PACK_SPARE_NAME[] = "02.pack";                            // J4: the spare slot, read when the first does not pass
static const char PACK_SPARE_PATH[] = "user/pk/02.pack";

extern "C" void __attribute__((weak)) op1f_pack_make_slots() asm("op1f_pack_make_slots");

extern "C" void* op1f_mtp_pack_lookup(void* self, const char* name) asm("op1f_mtp_pack_lookup");
extern "C" void* op1f_mtp_pack_lookup(void* self, const char* name) {     // +0x14: the slot's file, as op1f_mtp_fx_lookup
    (void)self;
    if (!name) return 0;
    const char* path = same(name, PACK_FILE_NAME) ? PACK_FILE_PATH : same(name, PACK_SPARE_NAME) ? PACK_SPARE_PATH : 0;   // J4: two slots
    if (!path) return 0;
    void* obj = STOCK_HEAP_ALLOC(FS_FILE_BYTES);
    if (!obj) return 0;
    STOCK_FS_FILE_CTOR(obj, path);
    give_delete(obj);
    return obj;
}

static const BehaviourFn pack_behaviour_vtable[6] = {
    (BehaviourFn)op1f_mtp_fx_noop,                                          // the destructor
    (BehaviourFn)op1f_mtp_fx_noop,                                          // the deleting destructor
    (BehaviourFn)op1f_mtp_fx_noop,                                          // +8
    (BehaviourFn)op1f_mtp_fx_zero,                                          // +0xc
    (BehaviourFn)op1f_mtp_fx_zero,                                          // +0x10: no child folders
    (BehaviourFn)op1f_mtp_pack_lookup,                                      // +0x14: the file
};
static const void* pack_behaviour[2] = { pack_behaviour_vtable, 0 };

// ---------------------------------------------------------------- the hooks

// M3: the folders the cards asked to be shown (module/build_m3_folders.cpp): a folder's behaviour by its name, or 0;
// and the folders made in the tree with their files.
extern "C" void* op1f_folders_child(const char* name) asm("op1f_folders_child");
extern "C" void op1f_folders_populate(void* root) asm("op1f_folders_populate");

// The root behaviour's +0x10 (the `word` edit): "fx" is the module's folder, everything else stock's.
extern "C" void* op1f_mtp_root_child(void* behaviour, const char* name) asm("op1f_mtp_root_child");
extern "C" void* op1f_mtp_root_child(void* behaviour, const char* name) {
    if (name && same(name, FOLDER_NAME)) return (void*)fx_behaviour;
    if (name && same(name, LFO_FOLDER_NAME)) return (void*)lfo_behaviour;   // I8
    if (name && op1f_pack_make_slots && same(name, PACK_FOLDER_NAME)) return (void*)pack_behaviour;   // J3
    void* card_folder = op1f_folders_child(name);                          // M3: a folder a card asked to be shown
    if (card_folder) return card_folder;
    ++op1f_disk_root_children;
    return STOCK_ROOT_CHILD(behaviour, name);
}

// I8: the lfo folder and its sixty-four files, after the fx folder.
static void populate_lfo(void* root) {
    void* folder = STOCK_MAKE_FOLDER(root, LFO_FOLDER_NAME);               // through op1f_mtp_root_child
    if (!folder) return;
    ++op1f_disk_lfo_folders;
    for (int i = 0; i < SLOTS; ++i)
        if (STOCK_ADD_FILE(folder, FILE_NAMES[i])) ++op1f_disk_lfo_files;  // each through op1f_mtp_lfo_lookup
}

// Called by op1f_mtp_populate_stub (build_h_disk.s) with the root folder after stock made the mixdown folder.
extern "C" void op1f_mtp_populate_fx(void* root) asm("op1f_mtp_populate_fx");
extern "C" void op1f_mtp_populate_fx(void* root) {
    void* folder = STOCK_MAKE_FOLDER(root, FOLDER_NAME);                   // through op1f_mtp_root_child
    if (folder) {
        ++op1f_disk_folders;
        for (int i = 0; i < SLOTS; ++i)
            if (STOCK_ADD_FILE(folder, FILE_NAMES[i])) ++op1f_disk_files; // each through op1f_mtp_fx_lookup
    }
    populate_lfo(root);                                                    // I8: the same stub, no new edit
    if (op1f_pack_make_slots) {                                            // J3: the packs folder, where the pack unit is linked
        op1f_pack_make_slots();                                            // user/pk/ and the slot's placeholder on the eMMC
        void* packs = STOCK_MAKE_FOLDER(root, PACK_FOLDER_NAME);           // through op1f_mtp_root_child
        if (packs) {
            STOCK_ADD_FILE(packs, PACK_FILE_NAME);                         // through op1f_mtp_pack_lookup
            STOCK_ADD_FILE(packs, PACK_SPARE_NAME);                        // J4: the spare
        }
    }
    op1f_folders_populate(root);                                           // M3: the cards' folders, after ours
}
