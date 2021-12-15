#pragma once

/// TBB
#include "tbb/concurrent_queue.h"
#include <vector>

template<class T>
class ObjectPool
{
private:
	int m_Sample_Obj_Count = 0;
	// 사용가능한 오브젝트에 대한 관리.
	tbb::concurrent_queue<T*> m_Available_Obj;
	// 현재 사용중인 오브젝트에 대한 관리 (추후 삭제시, 같이 삭제하기 위함)
	tbb::concurrent_queue<T*> m_Using_Obj;

public:
	ObjectPool(int _Sample_Obj_Count = 2000);
	~ObjectPool();
	
	// 오브젝트풀에서 오브젝트의 포인터를 반환하는 함수.
	T* GetObject();
	// 해당 오브젝트를 초기화 하고 오브젝트에 반환하는 함수.
	void ResetObject(T* Used_Object);
	// 오브젝트를 초기화 하지 않고 오브젝트에 반환하는 함수. (초기에 설정값이 필요한 경우)
	void Non_ResetObject(T* Setting_Object);
};

template<class T>
void ObjectPool<T>::Non_ResetObject(T* Setting_Object)
{
	// 다시 사용 가능한 큐에 넣어둠.
	m_Available_Obj.push(Setting_Object);
	// 사용중인 큐에서 제거.
	m_Using_Obj.try_pop(Setting_Object);
}

/// 만약 삭제하는데 다른데서 Obj가 사용중이라면.. (ref count) 를 써야 하는 경우 라면
///	외부에서 검사를 하고 삭제하는 식으로 하자! (내부에서 관리하기에는 제네릭하지 않은 것 같다.)
template<class T>
void ObjectPool<T>::ResetObject(T* Used_Object)
{
	// 소멸자 호출.
	Used_Object->~T();
	// Placement new 를 사용함. -> 메모리에 할당 해 놓고, 해당 영역을 삭제하지 않고 생성자를 통해 다시 사용.
	T* Reset_Object = new (Used_Object) T;
	// 다시 사용 가능한 큐에 넣어둠.
	m_Available_Obj.push(Reset_Object);
	// 사용중인 큐에서 제거.
	m_Using_Obj.try_pop(Reset_Object);
}

template<class T>
T* ObjectPool<T>::GetObject()
{
	T* Return_Obj = nullptr;

	// Case 01. 만약 사용 가능한 오브젝트가 큐에 존재하지 않는다면 샘플갯수의 10분의 1만큼을
	//			새로 만들어서 할당을 해준다. (할당 한 뒤 Case 02 에서 반환 할당 해줌)
	if (m_Available_Obj.empty())
	{
		for (int i = 0; i < (m_Sample_Obj_Count / 10); i++)
		{
			m_Available_Obj.push(new T);
		}
	}

	// Case 02. 사용 가능한 오브젝트가 큐에 존재한다면, 해당 오브젝트를 반환해준다.
	while (!m_Available_Obj.try_pop(Return_Obj)){}
	// 사용중인 큐에 반환하는 오브젝트 추가해 둠.
	m_Using_Obj.push(Return_Obj);

	return Return_Obj;
}

// 입력받은 기본 샘플갯수 대로 오브젝트 풀을 생성한다.
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
	// 두번 지워지는 포인터를 방지하기 위함. ( 도중에 강종했을때 1~2개 발생 )
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

