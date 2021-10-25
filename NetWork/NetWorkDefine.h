#pragma once

#define TOTAL_OVERLAPPED_COUNT 2000 // �� �������� ����.
#define FIRST_ACCEPT_BUFSIZE 1024
#define STRUCT_BUFSIZE 512
#define MSG_BUFSIZE 128
#define IP_SIZE 16
#define LOGIC_SUCCESS 1
#define LOGIC_FAIL 0

/// Ŭ���̾�Ʈ�� Work ������ ����.
#define CLIENT_THREAD_COUNT 3

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
