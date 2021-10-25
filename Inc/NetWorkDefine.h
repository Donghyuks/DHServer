#pragma once

#define TOTAL_OVERLAPPED_COUNT 2000 // 총 오버랩드 개수.
#define FIRST_ACCEPT_BUFSIZE 1024
#define STRUCT_BUFSIZE 512
#define MSG_BUFSIZE 128
#define IP_SIZE 16
#define LOGIC_SUCCESS 1
#define LOGIC_FAIL 0

/// 클라이언트의 Work 쓰레드 개수.
#define CLIENT_THREAD_COUNT 3

/// 공용 API 정의들.
#include "C2NetworkAPIDefine.h"

/// 공용 헤더
// 기타
#include <tchar.h>

// 쓰레드/함수관련
#include <functional>
#include <thread>

// 메모리관리/디버깅
#include <memory>
#include <assert.h>

// 자료구조
#include <vector>
#include <list>
#include <map>
#include <concurrent_queue.h>
