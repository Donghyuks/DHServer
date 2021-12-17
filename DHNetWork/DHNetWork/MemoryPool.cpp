#include "MemoryPool.h"

void MemoryPool::CreateMemory(int _Sample_Key, int _Sample_Count)
{
	// tbb �ؽøʿ� �����ϱ� ���� ������.
	Hash_Map_Type::accessor m_accessor;

	// �޸� Ǯ�� Ű���� �Բ� ���.
	m_Memory_Pool.insert(m_accessor, std::pair<int, tbb::concurrent_queue<char*>*>(_Sample_Key, new tbb::concurrent_queue<char*>));

	// �޸𸮸� �������� ������ �ϴ� �Ҵ��ص�.
	char* Memory_Array = new char[_Sample_Key * _Sample_Count];

	for (int i = 0; i < _Sample_Count; i++)
	{
		// char�� ���� ���� Ű(4~512 ����Ʈ) ��ŭ �޸� �Ҵ�.
		char* m_Sample_Pointer = Memory_Array + i * _Sample_Key;
		// �ʱ�ȭ.
		memset(m_Sample_Pointer, 0, _Sample_Key);
		// ������� �����͸� �ش� �޸� Ǯ�� ���.
		m_accessor->second->push(m_Sample_Pointer);
	}

	// ����� �����ڴ� ��ȯ.
	m_accessor.release();
}

void* MemoryPool::FindMemory(Hash_Map_Type::accessor* _Accessor, int _Memory_Size, int _Sample_Count)
{
	char* m_Return_Pointer = nullptr;

	// �޸� Ǯ���� �ش� Ű ���� �ش��ϴ� ���� ã��.
	m_Memory_Pool.find(*_Accessor, _Memory_Size);

	// ���� ť�� �� ������ ���� ��� �Ҵ��صΰ� ����.
	if ((*_Accessor)->second->empty())
	{
		for (int i = 0; i < _Sample_Count / 10; i++)
		{
			// char�� ���� ���� Ű(4~512 ����Ʈ) ��ŭ �޸� �Ҵ�.
			char* m_Sample_Pointer = new char[_Memory_Size];
			// ������� �����͸� �ش� �޸� Ǯ�� ���.
			(*_Accessor)->second->push(m_Sample_Pointer);
		}
	}

	// ��밡���� �޸𸮰� �����ϸ� �ش� ť���� �ϳ��� ���´�.
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

	// �޸� ����.
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
	// ����� ť�� ���� ����.
	for (auto MemoryPool_List : m_Memory_Pool)
	{
		MemoryPool_List.second->clear();
	}

	m_Memory_Pool.clear();
}

void* MemoryPool::GetMemory(unsigned int _Memory_Size)
{
	// �ش� �ؽÿ� �����ϱ� ���� ������. ( ���� release�� ȣ������ �ʾƵ� �������� ���������� release�� ȣ��Ǹ� �ش� ���� ���� lock�� Ǯ��)
	Hash_Map_Type::accessor m_accessor;

	// �޸� ������� void* �� �ش� �ּڰ��� ��ȯ����.
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
	// tbb �ؽøʿ� �����ϱ� ���� ������.
	Hash_Map_Type::accessor m_accessor;

	// �޸� Ǯ���� �ش� Ű ���� �ش��ϴ� ���� ã��.
	m_Memory_Pool.find(m_accessor, _Used_Data_Size);

	// �����͸� �ʱ�ȭ ��Ű��..
	memset(_Used_Data_Pointer, 0, _Used_Data_Size);

	// ����� �޸� �ּڰ��� ��ȯ.
	m_accessor->second->push((char*)_Used_Data_Pointer);
}
