#pragma once

#ifdef DHKEYIO_EXPORTS
	#define DHKEY_DLL __declspec(dllexport)
#else
	#define DHKEY_DLL __declspec(dllimport)
#endif

struct DHKEY_DLL Key_Flag
{
	bool Key_Current_State = false;
	bool Key_Prev_State = false;
};

class DHKEY_DLL DHKeyIO
{
private:
	bool Keeping = false;	// 키를 누르고 있을때를 체크.
public:
	Key_Flag m_Key_Flag[256];

	/// 키를 눌렀다 뗏을 때.
	bool Key_Up(unsigned char Key_Input);
	/// 키가 처음 눌러졌을 때.
	bool Key_Down(unsigned char Key_Input);
	/// 키를 누르고 있을 때.
	bool Key_Press(unsigned char Key_Input);
};

