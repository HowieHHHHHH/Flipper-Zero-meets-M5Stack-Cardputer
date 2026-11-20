#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 汉化翻译钩子 - 运行时把英文界面文字替换为中文
 *
 * 设计原则：
 *  - 只在绘制入口拦截，不改业务逻辑、不改字符串池
 *  - 翻译表从 SD 卡 /ext/locale/zh.txt 加载，可热更新（重启生效）
 *  - 未命中的字符串原样返回，降级安全
 *  - 线性查找，100~300 条规模足够快
 */

/** 初始化翻译系统（在 GUI 服务启动后调用一次，或首次绘制时懒加载） */
void locale_hook_init(void);

/** 查询翻译。命中返回中文指针，未命中返回原指针 */
const char* locale_hook_translate(const char* str);

/** 判断字符串是否已被翻译（含中文字符） */
bool locale_hook_is_translated(const char* str);

/** 重新从 SD 卡加载翻译表（用于运行时刷新） */
bool locale_hook_reload(void);

/** 字体是否已启用中文（用于 canvas 切换字体） */
bool locale_hook_cn_font_active(void);

#ifdef __cplusplus
}
#endif
