#pragma once

/// SETTING COUNT.
#define TOTAL_OVERLAPPED_COUNT			2000	// 총 오버랩드 개수.
#define CLIENT_THREAD_COUNT				1		// 클라이언트의 쓰레드 개수 (클라이언트의 쓰레드는 웬만하면.. 1개여도 충분!)
#define CLIENT_IOCP_THREAD_COUNT		2		// 클라이언트의 IOCP 쓰레드 개수
#define CLIENT_OVERLAPPED_COUNT			100		// 클라이언트의 IOCP 쓰레드 개수

/// BUFSIZE
#define OVERLAPPED_BUFIZE		2048
#define MAX_PACKET_SIZE			20480
#define PACKET_BUFSIZE			1024
#define ERROR_MSG_BUFIZE		128

/// LOGIC DEFINE
#define LOGIC_SUCCESS			1
#define LOGIC_FAIL				0

/// STATIC SIZE DEFINE
#define IP_SIZE					16

/// 공용 API 정의들.
#include "DHNetWorkAPIDefine.h"

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
