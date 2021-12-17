#pragma once

#define MEMORY_POOL_DEFALUT_COUNT 1000

/// TBB
#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_queue.h"

class MemoryPool
{
private:
	// 메모리 풀에 대해 각각 생성할 갯수.
	int m_4Byte_Block_Count		= 0;
	int m_8Byte_Block_Count		= 0;
	int m_16Byte_Block_Count	= 0;
	int m_32Byte_Block_Count	= 0;
	int m_64Byte_Block_Count	= 0;
	int m_128Byte_Block_Count	= 0;
	int m_256Byte_Block_Count	= 0;
	int m_512Byte_Block_Count	= 0;

	// 메모리 풀에 대한 map 자료구조.
	tbb::concurrent_hash_map<int,tbb::concurrent_queue<char*>*> m_Memory_Pool;
	// 해시맵에 접근하기 위한 접근자를 생성하기 위한 타입 define
	typedef tbb::concurrent_hash_map<int, tbb::concurrent_queue<char*>*> Hash_Map_Type;

	// 메모리를 할당하는 함수.
	void CreateMemory(int _Sample_Key, int _Sample_Count);

	// 메모리를 반환하기 위해 메모리를 찾는 함수.
	void* FindMemory(Hash_Map_Type::accessor* _Accessor, int _Memory_Size, int _Sample_Count);


public:
	MemoryPool(int _Sample_Memory_Count = MEMORY_POOL_DEFALUT_COUNT);

	~MemoryPool();

	void* GetMemory(unsigned int _Memory_Size);

	void ResetMemory(void* _Used_Data_Pointer, int _Used_Data_Size);
};
