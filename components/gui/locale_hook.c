/**
 * 汉化翻译钩子实现 - Cardputer ADV / Flipper Zero ESP32 移植版
 *
 * 加载 SD 卡 /ext/locale/zh.txt 中的 "英文=中文" 映射，
 * 在 canvas 绘制入口做运行时替换。
 */

#include "locale_hook.h"
#include <furi.h>
#include <storage/storage.h>
#include <string.h>
#include <stdlib.h>

#define TAG "LocaleHook"

#define LOCALE_PATH "/ext/locale/zh.txt"
#define MAX_ENTRIES 300
#define MAX_EN_LEN 128
#define MAX_ZH_LEN 128

typedef struct {
    char en[MAX_EN_LEN];
    char zh[MAX_ZH_LEN];
} LocaleEntry;

static LocaleEntry locale_table[MAX_ENTRIES];
static size_t locale_count = 0;
static bool locale_initialized = false;

/**
 * 去掉字符串首尾空白
 */
static char* trim(char* s) {
    while(*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    char* end = s + strlen(s) - 1;
    while(end > s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return s;
}

/**
 * 从 SD 卡加载翻译表
 * 格式: 每行 "英文=中文"，# 开头为注释
 */
bool locale_hook_reload(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) {
        FURI_LOG_E(TAG, "Storage not available");
        return false;
    }

    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, LOCALE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_W(TAG, "No translation file at %s, using English", LOCALE_PATH);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    locale_count = 0;

    // 一次性读入内存再按行解析（翻译表通常只有几 KB）
    #define LOCALE_BUF_SIZE 4096
    char* buf = malloc(LOCALE_BUF_SIZE);
    if(!buf) {
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    size_t total = storage_file_read(file, buf, LOCALE_BUF_SIZE - 1);
    buf[total] = '\0';

    char* cursor = buf;
    while(*cursor && locale_count < MAX_ENTRIES) {
        char* line = cursor;
        char* nl = strchr(cursor, '\n');
        if(nl) {
            *nl = '\0';
            cursor = nl + 1;
        } else {
            cursor += strlen(cursor); // 到末尾
        }

        char* p = trim(line);
        if(*p == '\0' || *p == '#') continue;

        char* eq = strchr(p, '=');
        if(!eq) continue;

        *eq = '\0';
        char* en = trim(p);
        char* zh = trim(eq + 1);
        if(*en == '\0' || *zh == '\0') continue;

        if(strlen(en) >= MAX_EN_LEN || strlen(zh) >= MAX_ZH_LEN) {
            FURI_LOG_W(TAG, "Entry too long, skipped: %s", en);
            continue;
        }

        strncpy(locale_table[locale_count].en, en, MAX_EN_LEN - 1);
        locale_table[locale_count].en[MAX_EN_LEN - 1] = '\0';
        strncpy(locale_table[locale_count].zh, zh, MAX_ZH_LEN - 1);
        locale_table[locale_count].zh[MAX_ZH_LEN - 1] = '\0';
        locale_count++;
    }

    free(buf);
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I(TAG, "Loaded %zu translations", locale_count);
    return locale_count > 0;
}

void locale_hook_init(void) {
    if(locale_initialized) return;
    locale_initialized = true;
    locale_hook_reload();
}

const char* locale_hook_translate(const char* str) {
    if(!str) return str;
    if(!locale_initialized) locale_hook_init();
    if(locale_count == 0) return str;

    for(size_t i = 0; i < locale_count; i++) {
        if(strcmp(str, locale_table[i].en) == 0) {
            return locale_table[i].zh;
        }
    }
    return str;
}

bool locale_hook_is_translated(const char* str) {
    if(!str || locale_count == 0) return false;
    for(size_t i = 0; i < locale_count; i++) {
        if(strcmp(str, locale_table[i].en) == 0) return true;
    }
    return false;
}

bool locale_hook_cn_font_active(void) {
    return locale_count > 0;
}
