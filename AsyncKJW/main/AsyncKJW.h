#pragma once
#ifndef LINUX
#define EXPORT_CLASS __declspec(dllexport)
#else
#define EXPORT_CLASS __attribute__((visibility("default")))
#endif

namespace II
{
	class AsyncKJWImpl;	// Forward declaration

	class EXPORT_CLASS AsyncKJW // II 클래스
	{
	public:
		AsyncKJW(unsigned short equipment_number_); // II 클래스: II 객체 생성
		~AsyncKJW(); // II 클래스: II 객체 파괴

		char* get_version(); // II 클래스: 버젼 가지고 오기
		char* get_running_statuses(int* out_size); // II 클래스: 모든 II 통신 모듈 상태 불러오기
		void load(); // II 클래스: 설정 불러오기
		bool start(short id_); // II 클래스: 시작
		bool stop(short id_); // II 클래스: 정지
		bool restart(short id_); // II 클래스: 재시작
		void close(); // II 클래스: 종료
		int write(short source_id_, short destination_id_, unsigned char* buffer_, int size_); // II 클래스: 데이터 전송

		//C# 같은 경우 std::function를 직접적으로 사용하지 못함 그래서 void*로 처리.
		void register_callback(int id_, void* callback_); // II 클래스: 데이터 수신 시 불려지는 콜백함수

		bool is_open_serial(short id_); // II 클래스: 시리얼 포트 열려있는지 확인
		bool is_connected_tcp_client(short id_); // II 클래스: TCP 클라이언트가 서버에 연결되어있는지 확인
		int	 num_users_connected_tcp_server(short id_); // II 클래스: TCP 서버에 몇 개의 클라이언트가 연결되어있는 지 값 확인.
	private:
		AsyncKJWImpl* pImpl; // P Invoke style
	};

	extern "C" 
	{
		// C-style exported functions for C# or non-C++ users
		EXPORT_CLASS AsyncKJW* CreateAsyncKJW(unsigned short equipment_number_); // II API 함수: II 객체 생성
		EXPORT_CLASS void DestroyAsyncKJW(AsyncKJW* instance_); // II API 함수: II 객체 파괴
		EXPORT_CLASS char* API_get_version(AsyncKJW* instance_); // II API 함수: 버젼 가지고 오기
		EXPORT_CLASS char* API_get_running_statuses(AsyncKJW* instance_, int* out_size); // 모든 II 통신 모듈 상태 불러오기
		EXPORT_CLASS void API_load(AsyncKJW* instance_); // II API 함수: 설정 불러오기
		EXPORT_CLASS bool API_start(AsyncKJW* instance_, short id_); // II API 함수: 시작
		EXPORT_CLASS bool API_stop(AsyncKJW* instance_, short id_); // II API 함수: 정지
		EXPORT_CLASS bool API_restart(AsyncKJW* instance_, short id_); // II API 함수: 재시작
		EXPORT_CLASS void API_close(AsyncKJW* instance_); // II API 함수: 종료
		EXPORT_CLASS int API_write(AsyncKJW* instance_, short source_id_, short destination_id_, unsigned char* buffer_, int size_); // II API 함수: 데이터 전송
		
		EXPORT_CLASS void API_register_callback(AsyncKJW* instance_, int id_, void* callback_);

		EXPORT_CLASS bool API_is_open_serial(AsyncKJW* instance_, short id_); // II API 함수: 시리얼 포트 열려있는지 확인
		EXPORT_CLASS bool API_is_connected_tcp_client(AsyncKJW* instance_, short id_); // II API 함수: TCP 클라이언트가 서버에 연결되어있는지 확인
		EXPORT_CLASS int API_num_users_connected_tcp_server(AsyncKJW* instance_, short id_); // II API 함수: TCP 서버에 몇 개의 클라이언트가 연결되어있는 지 값 확인
	}
}