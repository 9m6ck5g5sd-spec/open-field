// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Pictures step P2: the geometry of a stretch of a picture moved or scaled, as text (OS 1.7.0+0;
// analysis/picture_maker_study.md, section 9). `kit`'s picture maker calls this for an `m` instruction: the stretch
// of the unit's own stock picture goes through here into the picture that is made, with every coordinate mapped:
//     x' = scale/1000 x + dx/1000,   y' = scale/1000 y + dy/1000.
// "Shrink the tape" is one such stretch, the whole picture, with the scale 870.
//
// It is tools/picture_map.py, function for function and step for step: that file is the description, and the two are
// held against each other on the PC over every stock picture and some thousands of broken stretches
// (tools/tests/test_packs_p2.py), in what they write and in the words they refuse with. In short:
//   - whole numbers only: a number is a sign, a whole part up to 9,999 and up to six decimals; the scale is 1 to 4000
//     thousandths, an offset up to 99,999,999 thousandths either way; a result has three decimals (four inside a
//     transform), a half rounded away from zero. Every product fits 32 bits (scale x millionths under 2**32); the one
//     division is by subtraction, since the module links no runtime;
//   - bytes, no patterns: a tag is walked once; an attribute is taken where its name stands first or follows a byte
//     that is no letter, digit, underscore or hyphen, and is followed by `="`;
//   - mapped: x x1 x2 cx, y y1 y2 cy (points), r rx ry width height (lengths), points, d (written again: a command
//     letter before every group, the numbers joined by commas), transform="translate(a,b)"; a tag with
//     transform="matrix(...)" takes the map into its matrix and its children stay; the root's tag, <? <! and closing
//     tags stay;
//   - the stretch may begin and end inside a tag (text of ours, an id, standing in the middle of one).
// Nothing here calls stock or keeps anything between calls; what is written past the room given is counted and not
// written, so that the caller sees how much the stretch would have made.
typedef unsigned int u32;
typedef int i32;

// A function kept a function of its own. Put in line, the larger ones leave the compiler short of pointer registers
// in op1f_map_range, and it then counts with an index register (`I3 += M2`), which counts straight on only while its
// length register is zero. That holds wherever compiled code is called, but the picture loader's place is one where
// nothing of ours leans on it; out of line the question does not come up (tools/tests/test_packs_p2.py reads the
// listing for it).
#ifdef OP1F_HOST
#define OP1F_APART
#else
#define OP1F_APART __attribute__((noinline))
#endif

enum {
    MOST_WHOLE = 9999,
    MOST_SCALE = 4000,
    MOST_OFFSET = 99999999,
    MATRIX_BYTES = 18,              // transform="matrix(
    TRANSLATE_BYTES = 10,           // translate(
};

static const char TOO_LARGE[] = "a number in the picture is too large";
static const char DECIMALS[] = "a number in the picture has more than six decimals";
static const char EXPONENT[] = "a number in the picture has an exponent";
static const char PATH_BEGINS[] = "a path in the picture begins with a number";
static const char PATH_AFTER_CLOSE[] = "a path in the picture has a number after its close";
static const char PATH_ARC[] = "a path in the picture has an arc";
static const char PATH_TOO_FEW[] = "a path in the picture has a command with too few numbers";
static const char POINTS_ODD[] = "a list of points in the picture has an odd count";
static const char TRANSFORM[] = "a transform in the picture is of no form the unit maps";
static const char MATRIX_END[] = "a part with a matrix does not end inside the stretch";
static const char SCALE[] = "its instructions name a scale or an offset the unit does not take";

static const char MATRIX[] = "transform=\"matrix(";
static const char TRANSLATE[] = "translate(";
static const u32 TEN[10] = { 1000000000u, 100000000u, 10000000u, 1000000u, 100000u, 10000u, 1000u, 100u, 10u, 1u };

struct Run {
    u32 scale;
    i32 dx, dy;
    char* at;                       // where the next byte goes
    char* end;                      // the room's end
    u32 made;                       // the bytes made, written or not
    const char* why;                // 0, or the words the stretch is refused with
};

struct Number { u32 next; u32 whole; u32 millionths; int negative; };

static void put(Run* r, char c) {
    if (r->at < r->end) *r->at++ = c;
    ++r->made;
}

static void put_all(Run* r, const char* p, u32 count) {
    for (u32 i = 0; i < count; ++i) put(r, p[i]);
}

static int is_digit(char c) { return c >= '0' && c <= '9'; }
static int is_word(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || is_digit(c) || c == '_'; }
static int is_name(char c) { return is_word(c) || c == ':' || c == '.' || c == '-'; }
static int is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

static int is_command(char c) {
    switch (c) {
    case 'M': case 'm': case 'L': case 'l': case 'H': case 'h': case 'V': case 'v': case 'C': case 'c':
    case 'S': case 's': case 'Q': case 'q': case 'T': case 't': case 'A': case 'a': case 'Z': case 'z':
        return 1;
    }
    return 0;
}

static int same(const char* a, const char* b, u32 count) {
    for (u32 i = 0; i < count; ++i) if (a[i] != b[i]) return 0;
    return 1;
}

// n / d and what is left, by subtraction: no division in the module.
static u32 divide(u32 n, u32 d, u32* rest) {
    u32 q = 0, r = 0;
    for (int bit = 31; bit >= 0; --bit) {
        r = (r << 1) | ((n >> bit) & 1);
        if (r >= d) {
            r -= d;
            q |= 1u << bit;
        }
    }
    *rest = r;
    return q;
}

// A number that begins at `at`: 1 and the number, 0 when none begins there, -1 when it is refused (r->why says why).
static int read_number(Run* r, const char* d, u32 at, u32 end, Number* n) {
    u32 q = at;
    int negative = q < end && d[q] == '-';
    if (negative) ++q;
    u32 whole = 0, digits = 0;
    while (q < end && is_digit(d[q])) {
        whole = whole * 10 + (u32)(d[q] - '0');
        if (whole > MOST_WHOLE) {
            r->why = TOO_LARGE;
            return -1;
        }
        ++digits;
        ++q;
    }
    u32 millionths = 0, decimals = 0;
    if (q < end && d[q] == '.') {
        u32 p = q + 1;
        while (p < end && is_digit(d[p])) {
            if (decimals == 6) {
                r->why = DECIMALS;
                return -1;
            }
            millionths = millionths * 10 + (u32)(d[p] - '0');
            ++decimals;
            ++p;
        }
        if (digits == 0 && decimals == 0) return 0;         // a point by itself is no number
        q = p;
    } else if (digits == 0) {
        return 0;
    }
    if (q < end && (d[q] == 'e' || d[q] == 'E')) {          // e or E, and what makes it an exponent
        u32 p = q + 1;
        if (p < end && (d[p] == '+' || d[p] == '-')) ++p;
        if (p < end && is_digit(d[p])) {
            r->why = EXPONENT;
            return -1;
        }
    }
    while (decimals < 6) {
        millionths *= 10;
        ++decimals;
    }
    n->next = q;
    n->whole = whole;
    n->millionths = millionths;
    n->negative = negative;
    return 1;
}

// scale/1000 x number + offset/1000, a half rounded away from zero: a whole number of thousandths, or of ten
// thousandths when `four`.
static i32 mapped(const Number* n, u32 scale, i32 offset, int four) {
    u32 unit = four ? 100000u : 1000000u;                   // what one step of the result is, in 10**-9
    u32 rest;
    u32 quotient = divide(scale * n->millionths, unit, &rest);
    i32 size = (i32)(scale * n->whole * (four ? 10u : 1u) + quotient);
    i32 base = four ? offset * 10 : offset;
    if (!n->negative) {
        base += size;
    } else if (rest) {
        base -= size + 1;
        rest = unit - rest;
    } else {
        base -= size;
    }
    u32 half = unit >> 1;
    if (rest > half || (rest == half && base >= 0)) ++base;
    return base;
}

// A whole number of thousandths (ten thousandths when `four`) as text: no zero at the end, no point without
// decimals, no minus before 0.
static void put_number(Run* r, i32 value, int four) {
    u32 m = value < 0 ? (u32)(-value) : (u32)value;
    if (value < 0) put(r, '-');
    char d[10];
    for (int k = 0; k < 10; ++k) {
        char c = '0';
        while (m >= TEN[k]) {
            m -= TEN[k];
            ++c;
        }
        d[k] = c;
    }
    int point = four ? 6 : 7;                               // the digits before the point
    int first = 0;
    while (first < point - 1 && d[first] == '0') ++first;
    for (int k = first; k < point; ++k) put(r, d[k]);
    int last = 10;
    while (last > point && d[last - 1] == '0') --last;
    if (last > point) {
        put(r, '.');
        for (int k = point; k < last; ++k) put(r, d[k]);
    }
}

static void put_x(Run* r, const Number* n, int four) { put_number(r, mapped(n, r->scale, r->dx, four), four); }
static void put_y(Run* r, const Number* n, int four) { put_number(r, mapped(n, r->scale, r->dy, four), four); }
static void put_length(Run* r, const Number* n, int four) { put_number(r, mapped(n, r->scale, 0, four), four); }

// A list of points: every pair as "x,y", the pairs a space apart.
static void map_points(Run* r, const char* v, u32 end) {
    u32 at = 0, count = 0;
    Number x;
    while (at < end) {
        Number n;
        int got = read_number(r, v, at, end, &n);
        if (got < 0) return;
        if (got == 0) {
            ++at;
            continue;
        }
        at = n.next;
        if ((count & 1) == 0) {
            x = n;
        } else {
            if (count > 1) put(r, ' ');
            put_x(r, &x, 0);
            put(r, ',');
            put_y(r, &n, 0);
        }
        ++count;
    }
    if (count & 1) r->why = POINTS_ODD;
}

// Path data written again: every command letter with its numbers joined by commas. An absolute command's numbers
// are points, a relative command's are lengths. After M the further pairs are L's, after m they are l's.
static void map_path(Run* r, const char* v, u32 end) {
    u32 at = 0;
    char command = 0;
    while (at < end) {
        char byte = v[at];
        if (is_command(byte)) {
            command = byte;
            ++at;
            if (command == 'Z' || command == 'z') {
                put(r, command);
                continue;
            }
        } else {
            Number n;
            int got = read_number(r, v, at, end, &n);
            if (got < 0) return;
            if (got == 0) {
                ++at;                                       // a comma, a space: whatever stands between
                continue;
            }
            if (command == 0) {
                r->why = PATH_BEGINS;
                return;
            }
            if (command == 'Z' || command == 'z') {
                r->why = PATH_AFTER_CLOSE;
                return;
            }
            if (command == 'M') command = 'L';              // the pairs that follow a move are lines
            else if (command == 'm') command = 'l';
        }
        char kind = (char)(command & 0xDF);                 // the capital
        if (kind == 'A') {
            r->why = PATH_ARC;
            return;
        }
        int relative = command != kind;
        int count = (kind == 'H' || kind == 'V') ? 1 : kind == 'C' ? 6 : (kind == 'S' || kind == 'Q') ? 4 : 2;
        put(r, command);
        for (int k = 0; k < count; ++k) {
            Number n;
            for (;;) {                                      // the next number, whatever stands before it but a command
                if (at >= end || is_command(v[at])) {
                    r->why = PATH_TOO_FEW;
                    return;
                }
                int got = read_number(r, v, at, end, &n);
                if (got < 0) return;
                if (got) break;
                ++at;
            }
            at = n.next;
            if (k) put(r, ',');
            if (kind == 'H') {
                if (relative) put_length(r, &n, 0); else put_x(r, &n, 0);
            } else if (kind == 'V') {
                if (relative) put_length(r, &n, 0); else put_y(r, &n, 0);
            } else if (relative) {
                put_length(r, &n, 0);
            } else if ((k & 1) == 0) {
                put_x(r, &n, 0);
            } else {
                put_y(r, &n, 0);
            }
        }
    }
}

// `translate(a,b)` with its two lengths scaled, to four decimals: 1 written, 0 a value of another form, -1 refused.
static int map_translate(Run* r, const char* v, u32 end) {
    if (end < TRANSLATE_BYTES || !same(v, TRANSLATE, TRANSLATE_BYTES)) return 0;
    Number a, b;
    int got = read_number(r, v, TRANSLATE_BYTES, end, &a);
    if (got <= 0) return got;
    u32 at = a.next, between = at;
    while (at < end && (v[at] == ',' || is_space(v[at]))) ++at;
    if (at == between) return 0;
    got = read_number(r, v, at, end, &b);
    if (got <= 0) return got;
    if (b.next + 1 != end || v[b.next] != ')') return 0;
    put_all(r, TRANSLATE, TRANSLATE_BYTES);
    put_length(r, &a, 1);
    put(r, ',');
    put_length(r, &b, 1);
    put(r, ')');
    return 1;
}

// What an attribute's name asks for: 1 an x, 2 a y, 3 a length, 4 points, 5 path data, 6 a transform, 0 nothing.
static int kind_of(const char* name, u32 count) {
    if (count == 1) return name[0] == 'x' ? 1 : name[0] == 'y' ? 2 : name[0] == 'r' ? 3 : name[0] == 'd' ? 5 : 0;
    if (count == 2) {
        if (name[0] == 'x' && (name[1] == '1' || name[1] == '2')) return 1;
        if (name[0] == 'y' && (name[1] == '1' || name[1] == '2')) return 2;
        if (name[0] == 'c' && name[1] == 'x') return 1;
        if (name[0] == 'c' && name[1] == 'y') return 2;
        if (name[0] == 'r' && (name[1] == 'x' || name[1] == 'y')) return 3;
        return 0;
    }
    if (count == 5 && same(name, "width", 5)) return 3;
    if (count == 6 && same(name, "height", 6)) return 3;
    if (count == 6 && same(name, "points", 6)) return 4;
    if (count == 9 && same(name, "transform", 9)) return 6;
    return 0;
}

// A tag, or a piece of one, with its geometry mapped.
static void map_tag(Run* r, const char* tag, u32 end) {
    u32 at = 0, done = 0;
    while (at < end) {
        if (is_name(tag[at]) && (at == 0 || (!is_word(tag[at - 1]) && tag[at - 1] != '-'))) {
            u32 stop = at;
            while (stop < end && is_name(tag[stop])) ++stop;
            if (stop + 1 < end && tag[stop] == '=' && tag[stop + 1] == '"') {
                u32 first = stop + 2, close = first;
                while (close < end && tag[close] != '"') ++close;
                if (close == end) break;                    // a value that does not end ends the walk
                int kind = kind_of(tag + at, stop - at);
                if (kind >= 1 && kind <= 3) {
                    Number n;
                    int got = read_number(r, tag, first, close, &n);
                    if (got < 0) return;
                    if (got == 1 && n.next == close) {      // one number and no more: mapped; anything else (0px) stays
                        put_all(r, tag + done, first - done);
                        if (kind == 1) put_x(r, &n, 0); else if (kind == 2) put_y(r, &n, 0); else put_length(r, &n, 0);
                        done = close;
                    }
                } else if (kind >= 4) {
                    put_all(r, tag + done, first - done);
                    if (kind == 4) {
                        map_points(r, tag + first, close - first);
                    } else if (kind == 5) {
                        map_path(r, tag + first, close - first);
                    } else if (map_translate(r, tag + first, close - first) == 0) {
                        r->why = TRANSFORM;
                    }
                    if (r->why) return;
                    done = close;
                }
                at = close + 1;
                continue;
            }
            at = stop;
            continue;
        }
        ++at;
    }
    put_all(r, tag + done, end - done);
}

static u32 find_byte(const char* d, u32 at, u32 end, char c) {
    while (at < end && d[at] != c) ++at;
    return at;
}

// Where `transform="matrix(` stands in a tag, or `end` when it does not.
static u32 find_matrix(const char* tag, u32 end) {
    for (u32 at = 0; at + MATRIX_BYTES <= end; ++at) if (tag[at] == 't' && same(tag + at, MATRIX, MATRIX_BYTES)) return at;
    return end;
}

// A tag with transform="matrix(a b c d e f)": the four of the linear part scaled, the last two mapped as a point, all
// to four decimals and joined by spaces. The rest of the tag stays.
static OP1F_APART void map_matrix_tag(Run* r, const char* tag, u32 end) {
    u32 first = find_matrix(tag, end) + MATRIX_BYTES;
    Number six[6];
    u32 q = first;
    for (int k = 0; k < 6; ++k) {
        u32 between = q;
        while (q < end && (is_space(tag[q]) || (k && tag[q] == ','))) ++q;      // commas only between the numbers
        if (k && q == between) {
            r->why = TRANSFORM;
            return;
        }
        int got = read_number(r, tag, q, end, &six[k]);
        if (got < 0) return;
        if (got == 0) {
            r->why = TRANSFORM;
            return;
        }
        q = six[k].next;
    }
    while (q < end && is_space(tag[q])) ++q;
    if (q + 1 >= end || tag[q] != ')' || tag[q + 1] != '"') {
        r->why = TRANSFORM;
        return;
    }
    put_all(r, tag, first);
    for (int k = 0; k < 4; ++k) {
        put_length(r, &six[k], 1);
        put(r, ' ');
    }
    put_x(r, &six[4], 1);
    put(r, ' ');
    put_y(r, &six[5], 1);
    put_all(r, tag + q, end - q);
}

// Where the element ends whose opening tag is d[start .. tag_end): the tag itself when it closes itself, else
// behind the closing tag that matches it, tags of the same name counted. 0 when it does not end before `end`.
static OP1F_APART u32 element_end(const char* d, u32 start, u32 tag_end, u32 end) {
    if (d[tag_end - 2] == '/') return tag_end;
    u32 stop = start + 1;
    while (stop < tag_end && is_word(d[stop])) ++stop;
    const char* name = d + start + 1;
    u32 count = stop - (start + 1);
    u32 depth = 1, at = tag_end;
    for (;;) {
        at = find_byte(d, at, end, '<');
        if (at >= end) return 0;
        if (at + count + 3 <= end && d[at + 1] == '/' && same(d + at + 2, name, count) && d[at + 2 + count] == '>') {
            --depth;
            at += count + 3;
            if (depth == 0) return at;
            continue;
        }
        if (at + 1 + count <= end && same(d + at + 1, name, count) && (at + 1 + count >= end || !is_word(d[at + 1 + count]))) {
            u32 close = find_byte(d, at, end, '>');
            if (close >= end) return 0;
            if (d[close - 1] != '/') ++depth;
            at = close + 1;
            continue;
        }
        ++at;
    }
}

// The root's tag, <? <! and a closing tag: not mapped.
static int left_alone(const char* tag, u32 count) {
    if (count >= 4 && tag[0] == '<' && tag[1] == 's' && tag[2] == 'v' && tag[3] == 'g') return 1;
    return count >= 2 && (tag[1] == '?' || tag[1] == '!' || tag[1] == '/');
}

// A stretch of a picture with its geometry mapped, written to `out` as far as `room` goes: the bytes it makes (more
// than `room` when it did not fit), and in *why 0 or the words it is refused with.
extern "C" u32 op1f_map_range(const char* d, u32 bytes, u32 scale, i32 dx, i32 dy, char* out, u32 room, const char** why) asm("op1f_map_range");
extern "C" u32 op1f_map_range(const char* d, u32 bytes, u32 scale, i32 dx, i32 dy, char* out, u32 room, const char** why) {
    Run r = { scale, dx, dy, out, out + room, 0, 0 };
    if (scale < 1 || scale > MOST_SCALE || dx < -MOST_OFFSET || dx > MOST_OFFSET || dy < -MOST_OFFSET || dy > MOST_OFFSET) {
        *why = SCALE;
        return 0;
    }
    u32 at = 0, end = bytes;
    u32 first_close = find_byte(d, 0, end, '>'), first_open = find_byte(d, 0, end, '<');
    if (first_close < end && first_close < first_open) {    // the stretch begins inside a tag: the rest of that tag
        map_tag(&r, d, first_close + 1);
        at = first_close + 1;
    }
    while (r.why == 0) {
        u32 start = find_byte(d, at, end, '<');
        if (start + 1 < end && d[start + 1] == '>') {       // "<>" is no tag: a tag has a byte between its marks
            put_all(&r, d + at, start + 1 - at);
            at = start + 1;
            continue;
        }
        put_all(&r, d + at, start - at);
        if (start == end) break;
        const char* tag = d + start;
        u32 close = find_byte(d, start + 1, end, '>');
        if (close == end) {                                 // the stretch ends inside this tag
            u32 count = end - start;
            if (left_alone(tag, count)) put_all(&r, tag, count);
            else if (find_matrix(tag, count) < count) r.why = MATRIX_END;
            else map_tag(&r, tag, count);
            break;
        }
        u32 count = close + 1 - start;
        if (left_alone(tag, count)) {
            put_all(&r, tag, count);
            at = close + 1;
        } else if (find_matrix(tag, count) < count) {
            u32 stop = element_end(d, start, close + 1, end);
            if (stop == 0) {
                r.why = MATRIX_END;
                break;
            }
            map_matrix_tag(&r, tag, count);
            if (r.why) break;
            put_all(&r, d + close + 1, stop - (close + 1));
            at = stop;
        } else {
            map_tag(&r, tag, count);
            at = close + 1;
        }
    }
    *why = r.why;
    return r.made;
}
