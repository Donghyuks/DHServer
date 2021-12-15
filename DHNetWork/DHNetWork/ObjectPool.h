#pragma once

/// TBB
#include "tbb/concurrent_queue.h"
#include <vector>

template<class T>
class ObjectPool
{
private:
	int m_Sample_Obj_Count = 0;
	// ��밡���� ������Ʈ�� ���� ����.
	tbb::concurrent_queue<T*> m_Available_Obj;
	// ���� ������� ������Ʈ�� ���� ���� (���� ������, ���� �����ϱ� ����)
	tbb::concurrent_queue<T*> m_Using_Obj;

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
	// �ٽ� ��� ������ ť�� �־��.
	m_Available_Obj.push(Setting_Object);
	// ������� ť���� ����.
	m_Using_Obj.try_pop(Setting_Object);
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
	// ������� ť���� ����.
	m_Using_Obj.try_pop(Reset_Object);
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
			m_Available_Obj.push(new T);
		}
	}

	// Case 02. ��� ������ ������Ʈ�� ť�� �����Ѵٸ�, �ش� ������Ʈ�� ��ȯ���ش�.
	while (!m_Available_Obj.try_pop(Return_Obj)){}
	// ������� ť�� ��ȯ�ϴ� ������Ʈ �߰��� ��.
	m_Using_Obj.push(Return_Obj);

	return Return_Obj;
}

// �Է¹��� �⺻ ���ð��� ��� ������Ʈ Ǯ�� �����Ѵ�.
template<class T>
ObjectPool<T>::ObjectPool(int _Sample_Obj_Count /*= 2000*/)
{
	m_Sample_Obj_Count = _Sample_Obj_Count;

	for (int i = 0; i < _Sample_Obj_Count; i++)
	{
		m_Available_Obj.push(new T);
	}
}

template<class T>
ObjectPool<T>::~ObjectPool()
{
	// �ι� �������� �����͸� �����ϱ� ����. ( ���߿� ���������� 1~2�� �߻� )
	std::vector<__int64> TwoTime_Delete_Pointer;
	bool Is_Deleted = false;
	T* My_Object = nullptr;

	while (!m_Using_Obj.empty())
	{
		m_Using_Obj.try_pop(My_Object);
		delete My_Object;
		TwoTime_Delete_Pointer.push_back((__int64)My_Object);
		My_Object = nullptr;
	}

	while (!m_Available_Obj.empty())
	{
		m_Available_Obj.try_pop(My_Object);

		for (auto Already_Delete_Ptr : TwoTime_Delete_Pointer)
		{
			if (Already_Delete_Ptr == (__int64)My_Object)
			{
				Is_Deleted = true;
				break;
			}
		}

		if (!Is_Deleted)
		{
			delete My_Object;
			My_Object = nullptr;
			Is_Deleted = false;
		}
	}

	TwoTime_Delete_Pointer.clear();
}

