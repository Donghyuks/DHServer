#pragma once

/// SETTING COUNT.
#define TOTAL_OVERLAPPED_COUNT	2000 // �� �������� ����.
#define CLIENT_THREAD_COUNT		3

/// BUFSIZE 
#define OVERLAPPED_BUFIZE		2048
#define PACKET_BUFIZE			1024
#define ERROR_MSG_BUFIZE		128

/// LOGIC DEFINE
#define LOGIC_SUCCESS			1
#define LOGIC_FAIL				0

/// STATIC SIZE DEFINE
#define IP_SIZE					16

/// ���� API ���ǵ�.
#include "C2NetworkAPIDefine.h"

/// ���� ���
// ��Ÿ
#include <tchar.h>

// ������/�Լ�����
#include <functional>
#include <thread>

// �޸𸮰���/�����
#include <memory>
#include <assert.h>

// �ڷᱸ��
#include <vector>
#include <list>
#include <map>
#include <concurrent_queue.h>
