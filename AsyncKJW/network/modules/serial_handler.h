#pragma once
namespace II
{
	namespace network
	{

		namespace modules
		{
			class serial_handler :
				public	std::enable_shared_from_this< serial_handler >
			{
			private:
				static	serial_handler* _this; //셀프 포인터
				std::vector<std::thread> workers;
			public:
				using receive_callback = std::function<void(short interface_id_, unsigned char* buffer, int size_)>; //콜백 함수 타입

			public:
				serial_handler(); // 생성자
				serial_handler(const serial_handler&) = delete; // 해당 객체의 복제는 불가.
				serial_handler& operator=(const serial_handler&) = delete; // 해당 객체의 assign은 불가.
				~serial_handler(); // 파괴자

				void set_info(const session_info& _info); //설정

				void on_read(unsigned char* received_text_, int size_); // 데이터 수신시 불려지는 함수.
				bool is_open(); // 시리얼 포트 열려있는지 확인.

				bool start(); // 시작
				bool stop(); // 정지		

				void send_message(short interface_id_, unsigned char* buffer_, int size_); // 데이터 송신
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // 콜백 함수 등록

				// 수신용 데이터
				std::deque<std::pair<unsigned char*, int>> _outbound_q; // 데이터 송신용 queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // 데이터 수신용 queue
			private:
				void read_serial(); // 수신
				void write_serial(); // 송신
#ifndef LINUX
				using serial_handle = HANDLE; // GC heap에 있는 객체를 가리키는 포인터
#else
				using serial_handle = int;
#endif
				serial_handle		_serial_handle; // 시리얼 객체
				serial_info _serial_info;  // 시리얼 정보
				receive_callback _receive_callback; // 콜백 함수

				std::mutex _read_mutex;  // 수신용 queue에 대한 mutex
				std::mutex _write_mutex;  // 송신용 queue에 대한 mutex 
				bool _first_time = true;  // 처음 데이터를 읽는지 여부
				//bool print_to_console = true;  // 콘솔에 프린트를 할 것인지 여부
				bool _is_running = false;  // 현재 통신이 시작되었는지 여부
			public:
				bool is_running();
			};
		}
	}
}