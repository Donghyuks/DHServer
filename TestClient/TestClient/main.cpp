/*
	2021/11/01 17:36 - CDH
	
	< 예정사항 > 
		1. 입력한 수 만큼의 테스트 쓰레드(클라이언트) 생성. MAX 쓰레드는 코어 수의 2배
		2. 정의된 테스트 루프를 실행할 수 있도록함.
			- TestCase 1 : 클라이언트 접속 후 패킷 보내고 종료. (Loop)
			- TestCase 2 : 절반의 클라이언트는 접속 후 종료 / 절반의 클라이언트는 접속 후 패킷만 보냄.
			- TestCase 3 : 패킷의 종류를 여러가지로 보냄. (최소 3개)
		3. 해당 테스트 케이스를 모두 정상적으로 수행하는지 확인하기 위하여 Logger를 만들어 관리 및 확인.
*/

int main()
{
	
}