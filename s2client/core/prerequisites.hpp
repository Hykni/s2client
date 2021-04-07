#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _ITERATOR_DEBUG_LEVEL 0

#define M_PI 3.14159265358979323846f
#define M_PI_2 1.57079632679f

#include <cstdint>
#include <cctype>
#include <cwctype>
#ifndef __clang__
#include <limits>
#endif
#include <cmath>
#include <type_traits>

#include <WinSock2.h>
#include <Ws2tcpip.h>
#ifndef __clang__
#include <intrin.h>
#endif
#include <direct.h>

#pragma comment(lib, "ws2_32.lib")

#ifndef __clang__
#include <functional>
#include <optional>
#include <string>
#endif
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <array>
#include <queue>
using std::string;
using std::wstring;
using std::vector;
using std::deque;
using std::array;
using std::map;
using std::unordered_map;
using std::set;
using std::priority_queue;
template<typename T>
using min_heap = priority_queue<T, vector<T>, std::greater<T>>;

#include <chrono>
using namespace std::chrono;

#include <memory>

#include <string_view>
using std::string_view;

#ifdef _DEBUG
#include <assert.h>
#else
#define assert(...) ((void*)0)
#endif

#undef max
#undef min
#include <algorithm>
using std::min;
using std::max;
