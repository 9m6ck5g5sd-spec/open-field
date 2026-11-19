// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step M3, the base pack: text on a pack's page (OS 1.7.0+0; docs/base.md). A unit of
// its own, new at M3.
//
// op1f_text(document, part, text, width) writes a text into a part of a document a pack made through op1f_picture,
// in the firmware's own letters, the way our effect presets' list has shown its names since H5
// (module/build_h_presets.cpp): the part, a placeholder group of the pack's picture, becomes a label object of the
// firmware's at its first use, and the label's text is set when it changes.
//
// The firmware's calls, each as it was read at its callers (and as our presets' list has used them since H5):
//   the lookup 0x8fc25cc0(screen, id, 1) and the label maker 0x8fc25abc(screen, node, align) read the screen at +4
//     and nowhere else and do not keep it, so a document no screen holds is handed in four words of which the second
//     is the document, as op1f_node does since M1. The label maker reads the node's bounds (its table's +0xc) and its
//     paint (its table's +0x14), stops the unit for a paint of 0 (the assert at 0x8fc241a0) and for a paint past the
//     document's table of paints ([document + 0x84] to [document + 0x88], twenty bytes an entry; 0x8fcc3210), builds
//     a label of 0xb4 bytes at the bounds' top-left and puts it in the node's place in the document. So the paint is
//     asked here first, through the same function of the node's table, and a part whose paint would stop the unit is
//     not handed on: op1f_text answers 0 and one line says so;
//   the text 0x8fc3aa3c(label, text, width) lays the glyphs out from the firmware's font, shortened with a mark while
//     wider than `width` pixels. It allocates, so it is called only when the text changed. The font's capitals stand
//     under the small letters' codes (H5): a text is handed on in small letters. The font's lookup (0x8fc29e02) takes
//     a character as a signed byte and stops the unit for one past its table, which a byte from 0x80 up is (the
//     review of M3): every character outside the plain ones is handed on as a question mark.
//
// From a frame function only (the cards unit says when it is calling one, op1f_cards_calling): a label is made and
// laid out on the display thread, where the document is drawn. A document is one a card made (op1f_cards_document).
// Sixty-four texts at most, each the first time a document's part is asked for. Lines: the first sixteen texts made,
// and the first sixteen refusals. Compiled for the PC as it is, with OP1F_HOST defined: the stand-ins come in through
// mock_files_m3.h, a header of tests this tree does not hold.

#include "build_m3_card.h"

typedef unsigned int u32;
typedef unsigned char u8;

typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
typedef void* (*FindNodeFn)(void* screen, const char* id, int optional);    // 0x8fc25cc0: 0 when optional and missing
typedef void* (*LabelFromNodeFn)(void* screen, void* node, int align);      // 0x8fc25abc: the node replaced by a label object in the document, the label returned
typedef void (*LabelTextFn)(void* label, const char* text, int width);      // 0x8fc3aa3c: the text laid out, shortened with a mark while wider than `width` pixels
typedef u32 (*PaintFn)(void* node);                                         // the node's table +0x14: its paint, an index into the document's paints from 1

#ifdef OP1F_HOST
#include "mock_files_m3.h"                             // the PC's stand-ins: the logger, a model of the documents, their parts and the labels
#else
#define STOCK_LOG ((LogFn)0x8c053a20u)
#define STOCK_FIND_NODE ((FindNodeFn)0x8fc25cc0u)
#define STOCK_LABEL_FROM_NODE ((LabelFromNodeFn)0x8fc25abcu)
#define STOCK_LABEL_TEXT ((LabelTextFn)0x8fc3aa3cu)
#define RD32(a) (*(volatile u32*)(a))
#endif

enum { LABELS = 64, PART_KEPT = 24, TEXT_KEPT = 40, SCREEN_WIDTH = 480, LINES_SAID = 16 };
enum { NODE_PAINT = 5, DOCUMENT_PAINTS = 0x84 / 4, DOCUMENT_PAINTS_END = 0x88 / 4, PAINT_BYTES = 20 };   // the node's table's word +0x14; the document's paints

static const char LOG_FILE[] = "../src/main/op1f_text.cpp";

extern "C" Op1fCards op1f_cards OP1F_NAMED("op1f_cards");
extern "C" u32 op1f_cards_calling() OP1F_NAMED("op1f_cards_calling");
extern "C" int op1f_cards_document(const void* document) OP1F_NAMED("op1f_cards_document");

struct Text {
    void* document;
    char part[PART_KEPT];                              // the part's name, whole: a name of more than 23 characters takes no text
    void* label;
    char text[TEXT_KEPT];                              // the text as last laid out, in small letters
    u32 laid;                                          // the text was laid out at least once
};

static Text texts[LABELS];
static u32 text_count, lines_made, lines_refused;

static int same(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}

// A line, the first sixteen texts made and the first sixteen refusals, each counted apart.
static void said(int level, int line, const char* format, const char* pack, const char* part, const char* why) {
    u32* lines = level == 3 ? &lines_made : &lines_refused;
    if (*lines >= (u32)LINES_SAID) return;
    ++*lines;
    STOCK_LOG(level, LOG_FILE, line, format, pack, part, why);
}

// The text of a document's part made, once: the part looked up, its paint held, the label made. 0 when it cannot be.
static Text* made(void* document, const char* part, const char* pack) {
    u32 n = 0;
    while (part[n]) {
        if (++n == (u32)PART_KEPT) {
            said(2, 1, "text: '%s' has no part %s that can take a text: %s", pack, "?", "its name is longer than 23 characters");
            return 0;
        }
    }
    for (u32 i = 0; i < text_count; ++i) {
        if (texts[i].document == document && same(part, texts[i].part)) return &texts[i];
    }
    if (text_count == (u32)LABELS) {
        said(2, 2, "text: '%s' asked for the part %s: %s", pack, part, "the base holds sixty-four texts, and they are taken");
        return 0;
    }
    u32 held[4] = { 0, (u32)document, 0, 0 };                                     // no screen holds it: the lookup and the label maker read the word at +4 and nothing else
    void* node = STOCK_FIND_NODE(held, part, 1);
    if (node == 0) {
        said(2, 1, "text: '%s' has no part %s that can take a text: %s", pack, part, "its picture has no part of that name");
        return 0;
    }
    u32 paint = ((PaintFn)((*(u32**)node)[NODE_PAINT]))(node);                    // as the label maker asks it, before it stops the unit for a 0
    const u32* doc = (const u32*)document;
    u32 paints = doc[DOCUMENT_PAINTS_END] - doc[DOCUMENT_PAINTS];                 // the bytes of the document's paints
    if (paint == 0 || (paint - 1u) * (u32)PAINT_BYTES >= paints) {
        said(2, 1, "text: '%s' has no part %s that can take a text: %s", pack, part, "it is no group with a stroke");
        return 0;
    }
    void* label = STOCK_LABEL_FROM_NODE(held, node, 0);                            // left aligned, at the part's top-left
    if (label == 0) return 0;
    Text* t = &texts[text_count++];
    t->document = document;
    for (u32 i = 0; i <= n; ++i) t->part[i] = part[i];
    t->label = label;
    t->text[0] = 0;
    t->laid = 0;
    said(3, 3, "text: '%s' made a text of the part %s of its picture%s", pack, part, "");
    return t;
}

extern "C" int op1f_text(void* document, const char* part, const char* text, int width) {
    if (op1f_cards_calling() == 0 || document == 0 || part == 0 || text == 0) return 0;   // from a frame function only
    int slot = op1f_cards_document(document);
    if (slot < 0) return 0;                                                       // a document a card made, and no other
    Text* t = made(document, part, op1f_cards.slots[slot].card->name);
    if (t == 0) return 0;
    char small[TEXT_KEPT];
    u32 n = 0;
    for (; n < (u32)TEXT_KEPT - 1 && text[n]; ++n) {
        u8 c = (u8)text[n];
        if (c < 0x20 || c > 0x7e) c = '?';                                        // the font's lookup (0x8fc29e02) reads a character as signed: a byte from 0x80 up lands past its table, and the unit stops
        small[n] = c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : (char)c;          // the font's capitals stand under the small letters
    }
    small[n] = 0;
    if (t->laid && same(small, t->text)) return 1;                                // the same text: nothing laid out again
    STOCK_LABEL_TEXT(t->label, small, width > 0 ? width : (int)SCREEN_WIDTH);
    for (u32 i = 0; i <= n; ++i) t->text[i] = small[i];
    t->laid = 1;
    return 1;
}
