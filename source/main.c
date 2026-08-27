// Ultrahand Mover - SDL2 GUI
// Moves Ultrahand / Mission Control files between
// /atmosphere/contents and a folder on the SD root.
//
// Needs: switch-sdl2 switch-sdl2_ttf switch-sdl2_gfx switch-freetype
//        switch-libpng switch-bzip2 switch-mesa

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <switch.h>

#define SCR_W 1280
#define SCR_H 720

#define CONTENTS  "/atmosphere/contents"
#define LOGFILE   "/ultrahand-mover.log"
#define CFGFILE   "/ultrahand-mover.cfg"
#define APP_VER   "1.0.0"
#define APP_OWNER "miyqwx"

#define MAX_ITEMS 64
#define NAMELEN   64

// ---------------- languages ----------------

enum {
    S_ENABLED, S_DISABLED, S_TO_ROOT, S_TO_ATM, S_DISABLE, S_ENABLE,
    S_CURRENT, S_NOTFOUND, S_NOTHING, S_MOVED, S_RESTART, S_CONFIRM,
    S_HINTS, S_SETTINGS, S_LANGUAGE, S_ABOUT, S_ABOUT_TEXT, S_BACK,
    S_RESULT, S_RESULT_ERR, S_LANG_HINT, S_COUNT
};

#define LANG_COUNT 6

static const char *LANG_NAMES[LANG_COUNT] = {
    "English", "Türkçe", "Español", "Deutsch", "Français", "日本語"
};

static const char *STR[LANG_COUNT][S_COUNT] = {
// --- English ---
{
    "%s is ENABLED", "%s is DISABLED", "MOVE TO ROOT", "MOVE TO ATMOSPHERE",
    "Disable", "Enable", "CURRENT", "%s not found",
    "Nothing to move on this console.", "%s has been moved.",
    "Restart the console to apply the changes?", "A: Confirm        B: Cancel",
    "A: apply    < >: change choice    +: exit",
    "Settings", "Language", "About",
    "Moves Ultrahand and Mission Control to another folder with a single button press, "
    "so that Atmosphère does not crash while you play Tears of the Kingdom.",
    "B: back", "%d moved, %d skipped, %d error(s).", "%d moved, %d error - %s",
    "Up / Down: change language"
},
// --- Türkçe ---
{
    "%s ETKİN", "%s DEVRE DIŞI", "KÖKE TAŞI", "ATMOSPHERE TAŞI",
    "Devre dışı bırak", "Etkinleştir", "ŞU AN", "%s bulunamadı",
    "Bu cihazda taşınacak bir şey yok.", "%s taşındı.",
    "Değişiklikleri uygulamak için yeniden başlatılsın mı?", "A: Onayla        B: İptal",
    "A: uygula    < >: seçim    +: uygulamadan çık",
    "Ayarlar", "Dil", "Hakkında",
    "Tears of the Kingdom oynarken Atmosphère'in çökmesini önlemek için Ultrahand ve "
    "Mission Control'ü tek tuşla başka bir klasöre taşır.",
    "B: geri", "%d taşındı, %d atlandı, %d hata.", "%d taşındı, %d hata - %s",
    "Yukarı / Aşağı: dil değiştir"
},
// --- Español ---
{
    "%s está ACTIVADO", "%s está DESACTIVADO", "MOVER A LA RAÍZ", "MOVER A ATMOSPHERE",
    "Desactivar", "Activar", "ACTUAL", "%s no encontrado",
    "No hay nada que mover en esta consola.", "%s se ha movido.",
    "¿Reiniciar la consola para aplicar los cambios?", "A: Confirmar        B: Cancelar",
    "A: aplicar    < >: elegir    +: salir",
    "Ajustes", "Idioma", "Acerca de",
    "Mueve Ultrahand y Mission Control a otra carpeta con un solo botón, para que "
    "Atmosphère no se bloquee mientras juegas a Tears of the Kingdom.",
    "B: volver", "%d movidos, %d omitidos, %d error(es).", "%d movidos, %d error - %s",
    "Arriba / Abajo: cambiar idioma"
},
// --- Deutsch ---
{
    "%s ist AKTIV", "%s ist DEAKTIVIERT", "INS ROOT VERSCHIEBEN", "NACH ATMOSPHERE",
    "Deaktivieren", "Aktivieren", "AKTUELL", "%s nicht gefunden",
    "Auf dieser Konsole gibt es nichts zu verschieben.", "%s wurde verschoben.",
    "Konsole neu starten, um die Änderungen zu übernehmen?", "A: Bestätigen        B: Abbrechen",
    "A: anwenden    < >: Auswahl    +: beenden",
    "Einstellungen", "Sprache", "Über",
    "Verschiebt Ultrahand und Mission Control mit einem einzigen Tastendruck in einen "
    "anderen Ordner, damit Atmosphère beim Spielen von Tears of the Kingdom nicht abstürzt.",
    "B: zurück", "%d verschoben, %d übersprungen, %d Fehler.", "%d verschoben, %d Fehler - %s",
    "Hoch / Runter: Sprache wechseln"
},
// --- Français ---
{
    "%s est ACTIVÉ", "%s est DÉSACTIVÉ", "DÉPLACER VERS LA RACINE", "VERS ATMOSPHERE",
    "Désactiver", "Activer", "ACTUEL", "%s introuvable",
    "Rien à déplacer sur cette console.", "%s a été déplacé.",
    "Redémarrer la console pour appliquer les changements ?", "A : Confirmer        B : Annuler",
    "A : appliquer    < > : choix    + : quitter",
    "Réglages", "Langue", "À propos",
    "Déplace Ultrahand et Mission Control vers un autre dossier d'une seule touche, "
    "afin qu'Atmosphère ne plante pas pendant une partie de Tears of the Kingdom.",
    "B : retour", "%d déplacés, %d ignorés, %d erreur(s).", "%d déplacés, %d erreur - %s",
    "Haut / Bas : changer de langue"
},
// --- 日本語 ---
{
    "%s は有効です", "%s は無効です", "ルートへ移動", "アトモスフィアへ移動",
    "無効にする", "有効にする", "現在", "%s が見つかりません",
    "この本体には移動するものがありません。", "%s を移動しました。",
    "変更を適用するために再起動しますか？", "A：決定　　　　B：キャンセル",
    "A：実行　　＜＞：選択　　＋：終了",
    "設定", "言語", "情報",
    "「ティアーズ オブ ザ キングダム」のプレイ中にアトモスフィアが落ちるのを防ぐため、"
    "ウルトラハンドとミッションコントロールをボタンひとつで別のフォルダーへ移動します。",
    "B：戻る", "%d 件移動、%d 件スキップ、%d 件エラー。", "%d 件移動、%d 件エラー：%s",
    "上／下：言語を変更"
},
};

static int lang = 0;
#define T(k) (STR[lang][k])

static void load_cfg(void) {
    FILE *f = fopen(CFGFILE, "r");
    if (!f) return;
    int v = 0;
    if (fscanf(f, "lang=%d", &v) == 1 && v >= 0 && v < LANG_COUNT) lang = v;
    fclose(f);
}

static void save_cfg(void) {
    FILE *f = fopen(CFGFILE, "w");
    if (!f) return;
    fprintf(f, "lang=%d\n", lang);
    fclose(f);
}

// ---------------- app entries ----------------

typedef struct {
    const char *name;
    const char *store;
    const char *listfile;
    const char *defaults[8];
    int         default_count;
    char        items[MAX_ITEMS][NAMELEN];
    int         item_count;
    int         state;                 // 1 = enabled, 0 = disabled, -1 = not found
    int         n_contents;
    int         n_store;
} App;

static App apps[2] = {
    {
        .name = "Ultrahand",
        .store = "/ultrahand",
        .listfile = "/ultrahand/list.txt",
        .defaults = { "420000000007E51A", "420000000007E51B" },
        .default_count = 2,
    },
    {
        .name = "Mission Control",
        .store = "/mission-control",
        .listfile = "/mission-control/list.txt",
        .defaults = { "010000000000BD00" },
        .default_count = 1,
    },
};

// ---------------- colors ----------------
static const SDL_Color COL_TEXT   = { 235, 237, 240, 255 };
static const SDL_Color COL_DIM    = { 120, 124, 132, 255 };
static const SDL_Color COL_GREEN  = {  48, 209,  88, 255 };
static const SDL_Color COL_RED    = { 255,  95,  85, 255 };
static const SDL_Color COL_YELLOW = { 255, 190,  60, 255 };
static const SDL_Color COL_CYAN   = {   0, 210, 255, 255 };

#define BG_R 42
#define BG_G 45
#define BG_B 52

// ---------------- logging ----------------

static void log_line(const char *fmt, ...) {
    FILE *f = fopen(LOGFILE, "a");
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

static void log_start(void) {
    log_line("");
    log_line("===== Ultrahand Mover started (unix time %ld) =====", (long)time(NULL));
}

// ---------------- file helpers ----------------

static int is_dir(const char *p) {
    struct stat st;
    return (stat(p, &st) == 0 && S_ISDIR(st.st_mode));
}
static int fexists(const char *p) {
    struct stat st;
    return (stat(p, &st) == 0);
}

static int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }

    char *buf = (char *)malloc(0x10000);
    if (!buf) { fclose(in); fclose(out); return -1; }

    size_t n; int rc = 0;
    while ((n = fread(buf, 1, 0x10000, in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { rc = -1; break; }
    }
    free(buf);
    fclose(in);
    fclose(out);
    return rc;
}

static int copy_tree(const char *src, const char *dst) {
    if (!is_dir(src)) return copy_file(src, dst);
    mkdir(dst, 0777);

    DIR *d = opendir(src);
    if (!d) return -1;
    struct dirent *e;
    int rc = 0;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char s[FS_MAX_PATH], t[FS_MAX_PATH];
        snprintf(s, sizeof(s), "%s/%s", src, e->d_name);
        snprintf(t, sizeof(t), "%s/%s", dst, e->d_name);
        if (copy_tree(s, t) != 0) { rc = -1; break; }
    }
    closedir(d);
    return rc;
}

static int remove_tree(const char *path) {
    if (!is_dir(path)) return unlink(path);
    DIR *d = opendir(path);
    if (!d) return -1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char p[FS_MAX_PATH];
        snprintf(p, sizeof(p), "%s/%s", path, e->d_name);
        remove_tree(p);
    }
    closedir(d);
    return rmdir(path);
}

static int dir_is_empty(const char *path) {
    if (!is_dir(path)) return 0;
    DIR *d = opendir(path);
    if (!d) return 0;
    struct dirent *e;
    int empty = 1;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        empty = 0;
        break;
    }
    closedir(d);
    return empty;
}

// an empty leftover folder means nothing is really installed there
static int real_entry(const char *path) {
    if (!fexists(path)) return 0;
    if (dir_is_empty(path)) { rmdir(path); return 0; }
    return 1;
}

static int move_item(const char *from, const char *to) {
    if (rename(from, to) == 0) {
        log_line("  rename ok: %s -> %s", from, to);
        return 0;
    }
    log_line("  rename failed (errno %d: %s), trying copy", errno, strerror(errno));

    if (copy_tree(from, to) != 0) {
        log_line("  copy failed (errno %d: %s)", errno, strerror(errno));
        return -1;
    }
    if (remove_tree(from) != 0) {
        log_line("  copy ok but source could not be deleted (errno %d: %s)", errno, strerror(errno));
        return -1;
    }
    log_line("  copy + delete ok");
    return 0;
}

// ---------------- list handling ----------------

static void use_defaults(App *a) {
    a->item_count = 0;
    for (int i = 0; i < a->default_count && a->item_count < MAX_ITEMS; i++)
        snprintf(a->items[a->item_count++], NAMELEN, "%s", a->defaults[i]);
}

static void write_default_list(App *a) {
    mkdir(a->store, 0777);
    FILE *f = fopen(a->listfile, "w");
    if (!f) return;
    fprintf(f, "# %s\n", a->name);
    fprintf(f, "# one folder name per line\n");
    for (int i = 0; i < a->default_count; i++)
        fprintf(f, "%s\n", a->defaults[i]);
    fclose(f);
}

// only plain folder names are accepted - no dots, slashes or spaces
static int valid_name(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++)
        if (*p == '/' || *p == '\\' || *p == '.' || *p == ' ' || *p == '\t') return 0;
    return 1;
}

static void load_list(App *a) {
    a->item_count = 0;
    FILE *f = fopen(a->listfile, "r");
    if (!f) { write_default_list(a); use_defaults(a); return; }

    char line[512];
    while (fgets(line, sizeof(line), f) && a->item_count < MAX_ITEMS) {
        if (!strchr(line, '\n') && !feof(f)) {
            int ch;
            while ((ch = fgetc(f)) != EOF && ch != '\n') { }
        }

        size_t n = strlen(line);
        while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r' ||
                         line[n-1] == ' '  || line[n-1] == '\t')) line[--n] = 0;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 0 || *p == '#') continue;

        if (!valid_name(p)) {
            log_line("list %s: ignoring bad entry \"%s\"", a->name, p);
            continue;
        }
        snprintf(a->items[a->item_count++], NAMELEN, "%s", p);
    }
    fclose(f);

    if (a->item_count == 0) use_defaults(a);
}

static int get_state(App *a) {
    a->n_contents = 0;
    a->n_store    = 0;

    for (int i = 0; i < a->item_count; i++) {
        char c[FS_MAX_PATH], t[FS_MAX_PATH];
        snprintf(c, sizeof(c), "%s/%s", CONTENTS, a->items[i]);
        snprintf(t, sizeof(t), "%s/%s", a->store,  a->items[i]);
        if (real_entry(c)) a->n_contents++;
        if (real_entry(t)) a->n_store++;
    }

    log_line("state %s: %d in atmosphere, %d in root (of %d listed)",
             a->name, a->n_contents, a->n_store, a->item_count);

    if (a->n_contents > 0) return 1;
    if (a->n_store    > 0) return 0;
    return -1;
}

// dir = 1 : contents -> store   |   dir = 0 : store -> contents
static int do_move(App *a, int dir, char *msg, size_t msglen) {
    mkdir(a->store, 0777);
    int done = 0, fail = 0, skipped = 0;
    char reason[96] = {0};

    log_line("move %s: %s", a->name, dir ? "atmosphere -> root" : "root -> atmosphere");

    for (int i = 0; i < a->item_count; i++) {
        char in_contents[FS_MAX_PATH], in_store[FS_MAX_PATH];
        snprintf(in_contents, sizeof(in_contents), "%s/%s", CONTENTS, a->items[i]);
        snprintf(in_store,    sizeof(in_store),    "%s/%s", a->store,  a->items[i]);

        const char *from = dir ? in_contents : in_store;
        const char *to   = dir ? in_store    : in_contents;

        if (!real_entry(from)) {
            skipped++;
            log_line("  skip %s: source missing (%s)", a->items[i], from);
            continue;
        }
        if (real_entry(to)) {
            fail++;
            snprintf(reason, sizeof(reason), "%s", a->items[i]);
            log_line("  fail %s: target already exists (%s)", a->items[i], to);
            continue;
        }

        if (move_item(from, to) == 0) done++;
        else {
            fail++;
            snprintf(reason, sizeof(reason), "%s", a->items[i]);
        }
    }

    if (fail && reason[0]) snprintf(msg, msglen, T(S_RESULT_ERR), done, fail, reason);
    else                   snprintf(msg, msglen, T(S_RESULT), done, skipped, fail);

    log_line("result: %d moved, %d skipped, %d error(s)", done, skipped, fail);
    return done;
}

// ---------------- drawing ----------------

static SDL_Renderer *ren = NULL;
static TTF_Font *f_title = NULL, *f_card = NULL, *f_body = NULL, *f_small = NULL;
static SDL_Texture *tex_photo = NULL;

static void text_at(TTF_Font *f, const char *s, int x, int y, SDL_Color c, int centered) {
    if (!f || !s || !*s) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, s, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = { centered ? x - surf->w / 2 : x, y, surf->w, surf->h };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static void text_right(TTF_Font *f, const char *s, int x, int y, SDL_Color c) {
    if (!f || !s || !*s) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, s, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = { x - surf->w, y, surf->w, surf->h };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

// wraps by utf-8 character, prefers to break at spaces
static int utf8_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static void text_wrapped(TTF_Font *f, const char *s, int x, int y, int w, int line_h, SDL_Color c) {
    char line[512];
    int len = 0, last_space = -1, ty = y;

    for (const char *p = s; *p; ) {
        int cl = utf8_len((unsigned char)*p);
        if (len + cl >= (int)sizeof(line) - 1) break;

        memcpy(line + len, p, cl);
        len += cl;
        line[len] = 0;
        if (*p == ' ') last_space = len;

        int tw = 0, th = 0;
        TTF_SizeUTF8(f, line, &tw, &th);
        if (tw > w) {
            char rest[512];
            int rest_len;
            if (last_space > 0) {
                rest_len = len - last_space;
                memcpy(rest, line + last_space, rest_len);
                line[last_space - 1] = 0;          // drop the space itself
            } else {
                rest_len = cl;
                memcpy(rest, p, cl);
                line[len - cl] = 0;
            }
            rest[rest_len] = 0;
            text_at(f, line, x, ty, c, 0);
            ty += line_h;
            memcpy(line, rest, rest_len + 1);
            len = rest_len;
            last_space = -1;
        }
        p += cl;
    }
    if (len) text_at(f, line, x, ty, c, 0);
}

#define CARD_W 440
#define CARD_H 300
#define CARD_Y 200
#define CARD_L_X 100
#define CARD_R_X 740

// mode: 0 = normal, 1 = selected, 2 = grayed out (current state)
static void draw_card(int x, int is_disable_card, int mode) {
    int x2 = x + CARD_W, y2 = CARD_Y + CARD_H;

    if (mode == 1)
        roundedBoxRGBA(ren, x - 5, CARD_Y - 5, x2 + 5, y2 + 5, 26, 0, 210, 255, 255);

    if (mode == 2) roundedBoxRGBA(ren, x, CARD_Y, x2, y2, 22, 48, 50, 56, 255);
    else           roundedBoxRGBA(ren, x, CARD_Y, x2, y2, 22, 60, 64, 74, 255);

    int ix = x + CARD_W / 2, iy = CARD_Y + 40;
    Uint8 ir, ig, ib;
    if (mode == 2)            { ir = 78;  ig = 80;  ib = 86;  }
    else if (is_disable_card) { ir = 255; ig = 159; ib = 40;  }
    else                      { ir = 48;  ig = 209; ib = 88;  }
    roundedBoxRGBA(ren, ix - 55, iy, ix + 55, iy + 110, 22, ir, ig, ib, 255);

    Uint8 ar = 30, ag = 32, ab = 38;
    if (is_disable_card) {
        boxRGBA(ren, ix - 12, iy + 25, ix + 12, iy + 62, ar, ag, ab, 255);
        filledTrigonRGBA(ren, ix - 32, iy + 58, ix + 32, iy + 58, ix, iy + 88, ar, ag, ab, 255);
    } else {
        filledTrigonRGBA(ren, ix - 32, iy + 52, ix + 32, iy + 52, ix, iy + 22, ar, ag, ab, 255);
        boxRGBA(ren, ix - 12, iy + 48, ix + 12, iy + 85, ar, ag, ab, 255);
    }

    SDL_Color tc = (mode == 2) ? COL_DIM : COL_TEXT;
    text_at(f_card, is_disable_card ? T(S_TO_ROOT) : T(S_TO_ATM), ix, CARD_Y + 175, tc, 1);
    text_at(f_body, is_disable_card ? T(S_DISABLE) : T(S_ENABLE), ix, CARD_Y + 218, COL_DIM, 1);

    if (mode == 2) {
        roundedBoxRGBA(ren, ix - 70, CARD_Y + 258, ix + 70, CARD_Y + 288, 15, 28, 70, 40, 255);
        roundedRectangleRGBA(ren, ix - 70, CARD_Y + 258, ix + 70, CARD_Y + 288, 15, 48, 209, 88, 255);
        text_at(f_small, T(S_CURRENT), ix, CARD_Y + 262, COL_GREEN, 1);
    }
}

static void draw_blocked(const char *name) {
    int cx = SCR_W / 2, cy = 330;
    filledCircleRGBA(ren, cx, cy, 78, 255, 95, 85, 255);
    filledCircleRGBA(ren, cx, cy, 60, BG_R, BG_G, BG_B, 255);
    thickLineRGBA(ren, cx - 52, cy + 52, cx + 52, cy - 52, 18, 255, 95, 85, 255);

    char buf[160];
    snprintf(buf, sizeof(buf), T(S_NOTFOUND), name);
    text_at(f_card, buf, cx, cy + 110, COL_TEXT, 1);
    text_at(f_small, T(S_NOTHING), cx, cy + 160, COL_DIM, 1);
}

static void draw_dialog(const char *name) {
    boxRGBA(ren, 0, 0, SCR_W, SCR_H, 0, 0, 0, 175);
    int x1 = 250, y1 = 230, x2 = 1030, y2 = 500;
    roundedBoxRGBA(ren, x1, y1, x2, y2, 24, 60, 64, 74, 255);
    roundedRectangleRGBA(ren, x1, y1, x2, y2, 24, 0, 210, 255, 255);

    char buf[160];
    snprintf(buf, sizeof(buf), T(S_MOVED), name);
    text_at(f_card, buf, SCR_W / 2, y1 + 45, COL_TEXT, 1);
    text_at(f_body, T(S_RESTART), SCR_W / 2, y1 + 105, COL_TEXT, 1);
    text_at(f_body, T(S_CONFIRM), SCR_W / 2, y1 + 185, COL_YELLOW, 1);
}

static void draw_mover_page(App *a, int sel, const char *msg) {
    char status[160];
    if (a->state == 1) {
        snprintf(status, sizeof(status), T(S_ENABLED), a->name);
        text_at(f_body, status, SCR_W / 2, 125, COL_GREEN, 1);
    } else if (a->state == 0) {
        snprintf(status, sizeof(status), T(S_DISABLED), a->name);
        text_at(f_body, status, SCR_W / 2, 125, COL_RED, 1);
    }

    if (a->state == -1) {
        draw_blocked(a->name);
    } else {
        int left_mode  = (a->state == 0) ? 2 : (sel == 0 ? 1 : 0);
        int right_mode = (a->state == 1) ? 2 : (sel == 1 ? 1 : 0);
        draw_card(CARD_L_X, 1, left_mode);
        draw_card(CARD_R_X, 0, right_mode);

        if (msg && msg[0])
            text_at(f_body, msg, SCR_W / 2, 545, COL_TEXT, 1);
    }

    text_at(f_small, T(S_HINTS), SCR_W / 2, 645, COL_DIM, 1);
}

static void draw_settings_page(int lang_sel) {
    text_at(f_card, T(S_SETTINGS), SCR_W / 2, 120, COL_TEXT, 1);

    // language panel
    int lx = 90, ly = 190, lw = 480, lh = 400;
    roundedBoxRGBA(ren, lx, ly, lx + lw, ly + lh, 22, 60, 64, 74, 255);
    text_at(f_body, T(S_LANGUAGE), lx + 30, ly + 22, COL_CYAN, 0);

    for (int i = 0; i < LANG_COUNT; i++) {
        int ry = ly + 70 + i * 48;
        if (i == lang_sel)
            roundedBoxRGBA(ren, lx + 18, ry - 6, lx + lw - 18, ry + 38, 12, 0, 210, 255, 255);
        text_at(f_body, LANG_NAMES[i], lx + 40, ry,
                (i == lang_sel) ? (SDL_Color){ 20, 24, 30, 255 } : COL_TEXT, 0);
    }
    text_at(f_small, T(S_LANG_HINT), lx + 30, ly + lh - 38, COL_DIM, 0);

    // about panel
    int ax = 610, ay = 190, aw = 580, ah = 400;
    roundedBoxRGBA(ren, ax, ay, ax + aw, ay + ah, 22, 60, 64, 74, 255);
    text_at(f_body, T(S_ABOUT), ax + 30, ay + 22, COL_CYAN, 0);

    SDL_Rect photo = { ax + 30, ay + 75, 130, 130 };
    if (tex_photo) {
        SDL_RenderCopy(ren, tex_photo, NULL, &photo);
    } else {
        roundedBoxRGBA(ren, photo.x, photo.y, photo.x + photo.w, photo.y + photo.h,
                       18, 78, 80, 86, 255);
    }
    text_at(f_card, "Ultrahand Mover", ax + 185, ay + 85, COL_TEXT, 0);
    text_at(f_body, APP_OWNER, ax + 185, ay + 130, COL_GREEN, 0);
    text_at(f_small, "v" APP_VER, ax + 185, ay + 170, COL_DIM, 0);

    text_wrapped(f_small, T(S_ABOUT_TEXT), ax + 30, ay + 235, aw - 60, 30, COL_DIM);

    text_at(f_small, T(S_BACK), SCR_W / 2, 645, COL_DIM, 1);
}

static void render(int page, int sel, int lang_sel, const char *msg, int dialog) {
    SDL_SetRenderDrawColor(ren, BG_R, BG_G, BG_B, 255);
    SDL_RenderClear(ren);

    text_at(f_title, "Ultrahand Mover", SCR_W / 2, 40, COL_TEXT, 1);

    if (page != 2) {
        char sethint[96];
        snprintf(sethint, sizeof(sethint), "-  %s", T(S_SETTINGS));
        text_at(f_body, sethint, 45, 50, COL_CYAN, 0);
    }

    if (page == 0) {
        text_right(f_body, "Mission Control  R", SCR_W - 45, 50, COL_CYAN);
        draw_mover_page(&apps[0], sel, msg);
    } else if (page == 1) {
        text_at(f_body, "L  Ultrahand", 45, 90, COL_CYAN, 0);
        draw_mover_page(&apps[1], sel, msg);
    } else {
        draw_settings_page(lang_sel);
    }

    if (dialog) draw_dialog(apps[page].name);

    SDL_RenderPresent(ren);
}

static void reboot_console(void) {
    if (R_SUCCEEDED(spsmInitialize())) {
        spsmShutdown(true);
        spsmExit();
    }
}

// ---------------- main ----------------

int main(int argc, char **argv) {
    romfsInit();
    load_cfg();

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *win = SDL_CreateWindow("ultrahand-mover",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCR_W, SCR_H, 0);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    SDL_Surface *bmp = SDL_LoadBMP("romfs:/profile.bmp");
    if (bmp) {
        tex_photo = SDL_CreateTextureFromSurface(ren, bmp);
        SDL_FreeSurface(bmp);
    } else {
        log_line("profile.bmp not loaded: %s", SDL_GetError());
    }

    plInitialize(PlServiceType_User);
    PlFontData fd;
    plGetSharedFontByType(&fd, PlSharedFontType_Standard);
    f_title = TTF_OpenFontRW(SDL_RWFromMem(fd.address, fd.size), 1, 44);
    f_card  = TTF_OpenFontRW(SDL_RWFromMem(fd.address, fd.size), 1, 30);
    f_body  = TTF_OpenFontRW(SDL_RWFromMem(fd.address, fd.size), 1, 26);
    f_small = TTF_OpenFontRW(SDL_RWFromMem(fd.address, fd.size), 1, 22);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    log_start();

    char msg[160] = {0};
    for (int i = 0; i < 2; i++) {
        load_list(&apps[i]);
        apps[i].state = get_state(&apps[i]);
    }

    int page = 0, prev_page = 0;
    int sel  = (apps[0].state == 1) ? 0 : 1;
    int lang_sel = lang;
    int dialog = 0, running = 1;

    while (running && appletMainLoop()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
            if (ev.type == SDL_QUIT) running = 0;

        padUpdate(&pad);
        u64 k = padGetButtonsDown(&pad);

        if (dialog) {
            if (k & HidNpadButton_A)      { reboot_console(); dialog = 0; }
            else if (k & HidNpadButton_B) { dialog = 0; }
            render(page, sel, lang_sel, msg, dialog);
            continue;
        }

        if (page == 2) {                       // settings
            if (k & (HidNpadButton_B | HidNpadButton_Minus)) {
                page = prev_page;
                msg[0] = 0;
            }
            if (k & HidNpadButton_AnyUp)   lang_sel = (lang_sel + LANG_COUNT - 1) % LANG_COUNT;
            if (k & HidNpadButton_AnyDown) lang_sel = (lang_sel + 1) % LANG_COUNT;
            if (lang_sel != lang) {
                lang = lang_sel;
                save_cfg();
            }
        } else {                               // mover pages
            App *a = &apps[page];

            if (k & HidNpadButton_Plus) running = 0;
            if (k & HidNpadButton_B)    running = 0;

            if (k & HidNpadButton_Minus) {
                prev_page = page;
                page = 2;
                lang_sel = lang;
            } else if ((k & HidNpadButton_R) && page == 0) {
                page = 1; msg[0] = 0;
                sel = (apps[1].state == 1) ? 0 : 1;
            } else if ((k & HidNpadButton_L) && page == 1) {
                page = 0; msg[0] = 0;
                sel = (apps[0].state == 1) ? 0 : 1;
            }

            if (page != 2) {
                a = &apps[page];

                if (k & (HidNpadButton_AnyLeft | HidNpadButton_AnyRight)) {
                    if (a->state == 1)      sel = 0;
                    else if (a->state == 0) sel = 1;
                }

                if (k & HidNpadButton_X) {
                    load_list(a);
                    a->state = get_state(a);
                    sel = (a->state == 1) ? 0 : 1;
                }

                if (k & HidNpadButton_A) {
                    if (a->state != -1) {
                        int moved = do_move(a, a->state == 1 ? 1 : 0, msg, sizeof(msg));
                        a->state = get_state(a);
                        sel = (a->state == 1) ? 0 : 1;
                        if (moved > 0) dialog = 1;
                    }
                }
            }
        }

        render(page, sel, lang_sel, msg, dialog);
    }

    if (tex_photo) SDL_DestroyTexture(tex_photo);
    if (f_title) TTF_CloseFont(f_title);
    if (f_card)  TTF_CloseFont(f_card);
    if (f_body)  TTF_CloseFont(f_body);
    if (f_small) TTF_CloseFont(f_small);
    plExit();
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    romfsExit();
    return 0;
}
