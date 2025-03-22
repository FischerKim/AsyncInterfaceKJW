#pragma once
namespace II
{
	namespace network
	{
		namespace modules
		{
#define MULTICAST_GROUP "239.255.255.250"

			class udp_handler :
				public	std::enable_shared_from_this< udp_handler >
			{
			private:
				//static	udp_handler* _this; //셀프 포인터
				std::vector<std::thread> workers;
			public:
				using receive_callback = std::function<void(short interface_id_, unsigned char* buffer, int size_)>; //콜백 함수 타입

				udp_handler(); // 생성자
				udp_handler(const udp_handler&) = delete; // 해당 객체의 복제는 불가.
				udp_handler& operator=(const udp_handler&) = delete; // 해당 객체의 assign은 불가.
				~udp_handler(); // 파괴자

				void set_info(const session_info& _info);//설정
				int set_nonblocking(int fd);

				bool start(); // 시작
				bool stop(); // 정지	

				void send_message(short destination_id_, unsigned char* buffer_, int size_); // 데이터 송신
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // 콜백 함수 등록
			private:
				void on_read(unsigned char* received_text_, int size_); // 데이터 수신시 불려지는 함수.
				void print_ip_as_hex(const char* ip);
				std::string clean_ip(const std::string& ip);
				void read(); // 수신
				void write(); // 송신
				receive_callback _receive_callback;
				udp_info	_udp_info;
#ifndef LINUX
				using socket_type = SOCKET; //UDP 소켓
				using socket_address_type = sockaddr_in; //소켓 주소
				using socket_address_type2 = sockaddr; //소켓 주소
				int socket_error_type = SOCKET_ERROR;
				using _sharedmutex = std::shared_mutex;
#else
				using socket_type = int;
				using socket_address_type = struct sockaddr_in;
				using socket_address_type2 = struct sockaddr;
				int socket_error_type = SO_ERROR;
				using _sharedmutex = std::shared_timed_mutex;
#endif
				std::deque<std::pair<int, std::pair<unsigned char*, int>>> _outbound_q; // 데이터 송신용 queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // 데이터 수신용 queue
				std::mutex _read_mutex; // 수신용 queue에 대한 mutex
				std::mutex _write_mutex; // 송신용 queue에 대한 mutex 
				_sharedmutex _contexts_mutex;
				bool _ready = true; // 준비상태
				//bool _print_to_console = true; // 콘솔에 프린트를 할 것인지 여부
				bool _is_running = false; // 현재 통신이 시작되었는지 여부

#ifndef LINUX
				HANDLE _iocp;
#else
				int epoll_fd;
				struct epoll_event ev, events[MAX_EVENTS];
				std::condition_variable cv;
#endif
			protected:
				socket_type _udp_socket;  // UDP 소켓 (non-pointer approach)
				socket_address_type _source_address; // 주소
				std::map <int, socket_address_type> _destination_address_list;
				struct ip_mreq _mreq;
				int _bytes_transferred = -1;
			public:
				bool is_running();
			};
		}
	}
}