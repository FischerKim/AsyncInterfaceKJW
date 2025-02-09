#pragma once
namespace II
{
	namespace network
	{
		namespace modules
		{
			class tcp_client_handler : 
				public	std::enable_shared_from_this< tcp_client_handler >
			{
			private:
				/*static const int _buffer_size = 8192;
				static const int _reconnect_time_ = 100;*/
				static	tcp_client_handler*			_this; //셀프 포인터
				//II::thread_pool						_pool; //스레드 풀
				//std::vector<std::thread>			_threads;
				//std::thread							_main_thread;

				std::vector<std::thread> workers;
			public:
				using receive_callback = std::function<void(short interface_id_, unsigned char* buffer, int size_)>; //콜백 함수 타입

				tcp_client_handler(); // 생성자
				tcp_client_handler(const tcp_client_handler&) = delete; // 해당 객체의 복제는 불가.
				tcp_client_handler& operator=(const tcp_client_handler&) = delete; // 해당 객체의 assign은 불가.
				~tcp_client_handler(); // 파괴자

				void set_info(const session_info& _info); //설정
				int setNonBlocking(int fd);

				bool start(); // 시작
				bool stop(); // 정지	

				void send_message(short interface_id_, unsigned char* buffer_, int size_); // 데이터 송신
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // 콜백 함수 등록
			private:
				void on_read(unsigned char* received_text_, int size_);  // 데이터 수신시 불려지는 함수.
				void attempt_to_connect();
				void read(); // 수신
				void write(); // 송신
				receive_callback _receive_callback; // 콜백 함수
				tcp_info _tcp_info; // TCP 통신 설정 값들
#ifndef LINUX
				using socket_type = SOCKET; //TCP 소켓
				using socket_address_type = sockaddr_in; //소켓 주소
				using socket_address_type2 = sockaddr; //소켓 주소
				int socket_error_type = SOCKET_ERROR;
#else
				using socket_type = int;
				using socket_address_type = struct sockaddr_in;
				using socket_address_type2 = struct sockaddr;
				int socket_error_type = SO_ERROR;
#endif
				//unsigned char _outbound_packet[_buffer_size] = {}; // 송신용 데이터
				//unsigned char _inbound_packet[_buffer_size] = {}; // 수신용 데이터
				std::deque<std::pair<unsigned char*, int>> _outbound_q; // 데이터 송신용 queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // 데이터 수신용 queue
				std::mutex _read_mutex;  // 수신용 queue에 대한 mutex 현재 lock은 모두 thread_pool에서 관리.
				std::mutex _write_mutex;  // 송신용 queue에 대한 mutex 

				std::mutex _contexts_mutex;
				bool _first_time = true;  // 처음 데이터를 읽는지 여부
				bool _print_to_console = true; // 콘솔에 프린트를 할 것인지 여부
				bool _is_running = false; // 현재 통신이 시작되었는지 여부
				bool _connected = false; // 현재 통신이 시작되었는지 여부

				HANDLE _iocp;
			protected:
				socket_type  _client_socket; // TCP 클라이언트 소켓 (non-pointer approach)
				socket_address_type	server_ep; //소켓 주소
			public:
				bool is_running();
			};
		}
	}
}