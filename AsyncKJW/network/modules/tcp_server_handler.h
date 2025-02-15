﻿#pragma once
namespace II
{
	namespace network
	{
		namespace modules
		{
#ifndef LINUX
			enum IO_OPERATION { OP_ACCEPT, OP_READ, OP_WRITE };

			struct PER_IO_DATA {
				OVERLAPPED overlapped;		// For asynchronous operation tracking
				WSABUF wsaBuf;				// Data buffer descriptor
				char* buffer;				// Dynamically allocated buffer
				DWORD bytesSent;			// Number of bytes sent (for write tracking)
				DWORD totalSize;			// Total data size to be sent/received
				IO_OPERATION operationType;  // Identifies the I/O operation type
			};

#else
			typedef union epoll_data {
				void* ptr;
				int fd;
				uint32_t u32;
				uint64_t u64;
			} epoll_data_t;

			//struct epoll_event
			//{
			//	uint32_t events; /* Epoll events */
			//	epoll_data_t data; /* User data variable */
			//};

#endif
			class tcp_server_handler :
				public	std::enable_shared_from_this< tcp_server_handler >
			{
			private:
				static	tcp_server_handler* _this; // 셀프 포인터
				std::vector<std::thread> workers;
			public:
				using receive_callback = std::function<void(short interface_id_, unsigned char* buffer, int size_)>; // 콜백 함수 타입

				tcp_server_handler(); // 생성자
				tcp_server_handler(const tcp_server_handler&) = delete; // 해당 객체의 복제는 불가.
				tcp_server_handler& operator=(const tcp_server_handler&) = delete; // 해당 객체의 assign은 불가.
				~tcp_server_handler(); // 파괴자

				void set_info(const session_info& _info); // 설정
				int set_nonblocking(int fd);
				bool start(); // 시작
				bool stop(); // 정지	
				bool accept_connection(); // 클라이언트 연결 승인

				void send_message(short interface_id_, unsigned char* buffer_, int size_); // 데이터 송신
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // 콜백 함수 등록
				int num_users_connected(); // 서버에 연결된 클라이언트 수.
			private:
				receive_callback _receive_callback; // 콜백 함수
				int _num_users_entered = 0; // 클라이언트 수
				int _num_users_exited = 0; // 클라이언트 수
				tcp_info _tcp_info; // TCP 통신 설정 값들
#ifndef LINUX
				using socket_type = SOCKET; // TCP 소켓
				using socket_address_type = sockaddr_in; // 소켓 주소
				using socket_address_type2 = sockaddr; // 소켓 주소
				int socket_error_type = SOCKET_ERROR;
#else
				using socket_type = int;
				using socket_address_type = struct sockaddr_in;
				using socket_address_type2 = struct sockaddr;
				int socket_error_type = SO_ERROR;
#endif
				socket_type _server_socket; // TCP 서버 소켓
				struct client_context
				{
					short _id;
					socket_type _socket;
				};
				std::map<int, client_context> _client_socket;
				std::map<int, client_context> _newly_added_client_socket;
				std::set<int> _exited_client;

				void on_read(unsigned char* received_text_, int size_); // 데이터 수신시 불려지는 함수.
				void read(); // 수신
				void write(); // 송신
				std::deque<std::pair<unsigned char*, int>> _outbound_q; // 데이터 송신용 queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // 데이터 수신용 queue

				std::mutex _read_mutex; // 수신용 queue에 대한 mutex
				std::mutex _write_mutex; // 송신용 queue에 대한 mutex 
				std::shared_mutex _contexts_mutex;
				bool _print_to_console = true; // 콘솔에 프린트를 할 것인지 여부
				bool _is_running = false; // 현재 통신이 시작되었는지 여부
				bool _connected = false; // 현재 통신이 시작되었는지 여부
#ifndef LINUX
				HANDLE _iocp;
#else
				int epoll_fd;
				struct epoll_event ev, events[MAX_EVENTS];
				std::condition_variable cv;
#endif
				//PER_IO_DATA* _ioData; //need to be dynamically allocated.
				//int numThreads;
			public:
				bool is_running();
			};
		}
	}
}