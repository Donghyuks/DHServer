#pragma once

#define MEMORY_POOL_DEFALUT_COUNT 1000

/// TBB
#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_queue.h"

class MemoryPool
{
private:
	// �޸� Ǯ�� ���� ���� ������ ����.
	int m_4Byte_Block_Count		= 0;
	int m_8Byte_Block_Count		= 0;
	int m_16Byte_Block_Count	= 0;
	int m_32Byte_Block_Count	= 0;
	int m_64Byte_Block_Count	= 0;
	int m_128Byte_Block_Count	= 0;
	int m_256Byte_Block_Count	= 0;
	int m_512Byte_Block_Count	= 0;

	// �޸� Ǯ�� ���� map �ڷᱸ��.
	tbb::concurrent_hash_map<int,tbb::concurrent_queue<char*>*> m_Memory_Pool;
	// �ؽøʿ� �����ϱ� ���� �����ڸ� �����ϱ� ���� Ÿ�� define
	typedef tbb::concurrent_hash_map<int, tbb::concurrent_queue<char*>*> Hash_Map_Type;

	// �޸𸮸� �Ҵ��ϴ� �Լ�.
	void CreateMemory(int _Sample_Key, int _Sample_Count);

	// �޸𸮸� ��ȯ�ϱ� ���� �޸𸮸� ã�� �Լ�.
	void* FindMemory(Hash_Map_Type::accessor* _Accessor, int _Memory_Size, int _Sample_Count);


public:
	MemoryPool(int _Sample_Memory_Count = MEMORY_POOL_DEFALUT_COUNT);

	~MemoryPool();

	void* GetMemory(unsigned int _Memory_Size);

	void ResetMemory(void* _Used_Data_Pointer, int _Used_Data_Size);
};
