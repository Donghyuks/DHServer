#pragma once

// TBB
#include "tbb/concurrent_queue.h"
// STL
#include <vector>

template<class T>
class ObjectPool
{
private:
	int m_Sample_Obj_Count = 0;
	// ��ü ��� �� ������Ʈ.
	std::vector<T*> ALL_Object_List;

	tbb::concurrent_queue<T*> m_Available_Obj;

public:
	ObjectPool(int _Sample_Obj_Count = 2000);
	~ObjectPool();
	
	// ������ƮǮ���� ������Ʈ�� �����͸� ��ȯ�ϴ� �Լ�.
	T* GetObject();
	// �ش� ������Ʈ�� �ʱ�ȭ �ϰ� ������Ʈ�� ��ȯ�ϴ� �Լ�.
	void ResetObject(T* Used_Object);
	// ������Ʈ�� �ʱ�ȭ ���� �ʰ� ������Ʈ�� ��ȯ�ϴ� �Լ�. (�ʱ⿡ �������� �ʿ��� ���)
	void Non_ResetObject(T* Setting_Object);
};

template<class T>
void ObjectPool<T>::Non_ResetObject(T* Setting_Object)
{	
	// �ٽ� ��밡���� ť�� �ִ´�.
	m_Available_Obj.push(Setting_Object);
}

/// ���� �����ϴµ� �ٸ����� Obj�� ������̶��.. (ref count) �� ��� �ϴ� ��� ���
///	�ܺο��� �˻縦 �ϰ� �����ϴ� ������ ����! (���ο��� �����ϱ⿡�� ���׸����� ���� �� ����.)
template<class T>
void ObjectPool<T>::ResetObject(T* Used_Object)
{
	// �Ҹ��� ȣ��.
	Used_Object->~T();
	// Placement new �� �����. -> �޸𸮿� �Ҵ� �� ����, �ش� ������ �������� �ʰ� �����ڸ� ���� �ٽ� ���.
	T* Reset_Object = new (Used_Object) T;
	// �ٽ� ��� ������ ť�� �־��.
	m_Available_Obj.push(Reset_Object);
}

template<class T>
T* ObjectPool<T>::GetObject()
{
	T* Return_Obj = nullptr;

	// Case 01. ���� ��� ������ ������Ʈ�� ť�� �������� �ʴ´ٸ� ���ð����� 10���� 1��ŭ��
	//			���� ���� �Ҵ��� ���ش�. (�Ҵ� �� �� Case 02 ���� ��ȯ �Ҵ� ����)
	if (m_Available_Obj.empty())
	{
		for (int i = 0; i < (m_Sample_Obj_Count / 10); i++)
		{
			T* Add_Object = new T;
			m_Available_Obj.push(Add_Object);
			// ���� ����� Ȥ�� �����ϴ� ��� ������Ʈ����Ʈ�� �����ϱ����� ���� Vector�� �ּڰ��� �����صд�.
			ALL_Object_List.push_back(Add_Object);
		}
	}

	// Case 02. ��� ������ ������Ʈ�� ť�� �����Ѵٸ�, �ش� ������Ʈ�� ��ȯ���ش�.
	while (!m_Available_Obj.try_pop(Return_Obj)){}

	return Return_Obj;
}

// �Է¹��� �⺻ ���ð��� ��� ������Ʈ Ǯ�� �����Ѵ�.
template<class T>
ObjectPool<T>::ObjectPool(int _Sample_Obj_Count /*= 2000*/)
{
	m_Sample_Obj_Count = _Sample_Obj_Count;

	for (int i = 0; i < _Sample_Obj_Count; i++)
	{
		T* Add_Object = new T;
		m_Available_Obj.push(Add_Object);
		// ���� ����� Ȥ�� �����ϴ� ��� ������Ʈ����Ʈ�� �����ϱ����� ���� Vector�� �ּڰ��� �����صд�.
		ALL_Object_List.push_back(Add_Object);
	}
}

template<class T>
ObjectPool<T>::~ObjectPool()
{
	m_Available_Obj.clear();

	for (auto Generated_Object : ALL_Object_List)
	{
		delete Generated_Object;
		Generated_Object = nullptr;
	}
}

