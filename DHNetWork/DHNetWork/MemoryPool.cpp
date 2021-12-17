#include "MemoryPool.h"

void MemoryPool::CreateMemory(int _Sample_Key, int _Sample_Count)
{
	// tbb 해시맵에 접근하기 위한 접근자.
	Hash_Map_Type::accessor m_accessor;

	// 메모리 풀에 키값과 함께 등록.
	m_Memory_Pool.insert(m_accessor, std::pair<int, tbb::concurrent_queue<char*>*>(_Sample_Key, new tbb::concurrent_queue<char*>));

	// 메모리를 연속적인 공간에 일단 할당해둠.
	char* Memory_Array = new char[_Sample_Key * _Sample_Count];

	for (int i = 0; i < _Sample_Count; i++)
	{
		// char형 으로 샘플 키(4~512 바이트) 만큼 메모리 할당.
		char* m_Sample_Pointer = Memory_Array + i * _Sample_Key;
		// 초기화.
		memset(m_Sample_Pointer, 0, _Sample_Key);
		// 만들어진 포인터를 해당 메모리 풀에 등록.
		m_accessor->second->push(m_Sample_Pointer);
	}

	// 사용한 접근자는 반환.
	m_accessor.release();
}

void* MemoryPool::FindMemory(Hash_Map_Type::accessor* _Accessor, int _Memory_Size, int _Sample_Count)
{
	char* m_Return_Pointer = nullptr;

	// 메모리 풀에서 해당 키 값에 해당하는 값을 찾고.
	m_Memory_Pool.find(*_Accessor, _Memory_Size);

	// 만약 큐가 다 썻으면 새로 몇개를 할당해두고 쓴다.
	if ((*_Accessor)->second->empty())
	{
		for (int i = 0; i < _Sample_Count / 10; i++)
		{
			// char형 으로 샘플 키(4~512 바이트) 만큼 메모리 할당.
			char* m_Sample_Pointer = new char[_Memory_Size];
			// 만들어진 포인터를 해당 메모리 풀에 등록.
			(*_Accessor)->second->push(m_Sample_Pointer);
		}
	}

	// 사용가능한 메모리가 존재하면 해당 큐에서 하나를 뺴온다.
	while (!((*_Accessor)->second->try_pop(m_Return_Pointer))) {};
	return (void*)m_Return_Pointer;
}

MemoryPool::MemoryPool(int _Sample_Memory_Count /*= MEMORY_POOL_DEFALUT_COUNT*/)
{
	m_4Byte_Block_Count = _Sample_Memory_Count;
	m_8Byte_Block_Count = _Sample_Memory_Count;
	m_16Byte_Block_Count = _Sample_Memory_Count;
	m_32Byte_Block_Count = _Sample_Memory_Count;
	m_64Byte_Block_Count = _Sample_Memory_Count;
	m_128Byte_Block_Count = _Sample_Memory_Count / 2;
	m_256Byte_Block_Count = _Sample_Memory_Count / 2;
	m_512Byte_Block_Count = _Sample_Memory_Count / 4;

	// 메모리 생성.
	CreateMemory(4, m_4Byte_Block_Count);
	CreateMemory(8, m_8Byte_Block_Count);
	CreateMemory(16, m_16Byte_Block_Count);
	CreateMemory(32, m_32Byte_Block_Count);
	CreateMemory(64, m_64Byte_Block_Count);
	CreateMemory(128, m_128Byte_Block_Count);
	CreateMemory(256, m_256Byte_Block_Count);
	CreateMemory(512, m_512Byte_Block_Count);
}

MemoryPool::~MemoryPool()
{
	// 사용한 큐와 맵을 해제.
	for (auto MemoryPool_List : m_Memory_Pool)
	{
		MemoryPool_List.second->clear();
	}

	m_Memory_Pool.clear();
}

void* MemoryPool::GetMemory(unsigned int _Memory_Size)
{
	// 해당 해시에 접근하기 위한 접근자. ( 따로 release를 호출하지 않아도 스코프를 빠져나가면 release가 호출되며 해당 값에 대한 lock이 풀림)
	Hash_Map_Type::accessor m_accessor;

	// 메모리 사이즈별로 void* 로 해당 주솟값을 반환해줌.
	switch (_Memory_Size)
	{
	case 4:
		return FindMemory(&m_accessor, _Memory_Size, m_4Byte_Block_Count);
	case 8:
		return FindMemory(&m_accessor, _Memory_Size, m_8Byte_Block_Count);
	case 16:
		return FindMemory(&m_accessor, _Memory_Size, m_16Byte_Block_Count);
	case 32:
		return FindMemory(&m_accessor, _Memory_Size, m_32Byte_Block_Count);
	case 64:
		return FindMemory(&m_accessor, _Memory_Size, m_64Byte_Block_Count);
	case 128:
		return FindMemory(&m_accessor, _Memory_Size, m_128Byte_Block_Count);
	case 256:
		return FindMemory(&m_accessor, _Memory_Size, m_256Byte_Block_Count);
	case 512:
		return FindMemory(&m_accessor, _Memory_Size, m_512Byte_Block_Count);
	default:
		return nullptr;
	}
}

void MemoryPool::ResetMemory(void* _Used_Data_Pointer, int _Used_Data_Size)
{
	// tbb 해시맵에 접근하기 위한 접근자.
	Hash_Map_Type::accessor m_accessor;

	// 메모리 풀에서 해당 키 값에 해당하는 값을 찾고.
	m_Memory_Pool.find(m_accessor, _Used_Data_Size);

	// 데이터를 초기화 시키고..
	memset(_Used_Data_Pointer, 0, _Used_Data_Size);

	// 사용한 메모리 주솟값을 반환.
	m_accessor->second->push((char*)_Used_Data_Pointer);
}
