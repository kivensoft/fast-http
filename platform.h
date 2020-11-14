
#pragma once
#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#ifdef _WIN32

#   include <windows.h>
#   define PATH_SPEC '\\'

// 控制台代码页改成utf8
#   define INIT_UTF8_TERM(x) UINT x = GetConsoleOutputCP(); SetConsoleOutputCP(65001)
#   define UNINIT_UTF8_TERM(x) SetConsoleOutputCP(x)

// 时间函数改成linux样式
#   define localtime_r(x, y) localtime_s(y, x)

#else

#   define PATH_SPEC '/'
#   define INIT_UTF8_TERM(x) (void)(0)
#   define UNINIT_UTF8_TERM(x) (void)(0)

#endif

#endif // __PLATFORM_H__