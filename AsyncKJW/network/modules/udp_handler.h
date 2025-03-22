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
				//static	udp_handler* _this; //���� ������
				std::vector<std::thread> workers;
			public:
				using receive_callback = std::function<void(short interface_id_, unsigned char* buffer, int size_)>; //�ݹ� �Լ� Ÿ��

				udp_handler(); // ������
				udp_handler(const udp_handler&) = delete; // �ش� ��ü�� ������ �Ұ�.
				udp_handler& operator=(const udp_handler&) = delete; // �ش� ��ü�� assign�� �Ұ�.
				~udp_handler(); // �ı���

				void set_info(const session_info& _info);//����
				int set_nonblocking(int fd);

				bool start(); // ����
				bool stop(); // ����	

				void send_message(short destination_id_, unsigned char* buffer_, int size_); // ������ �۽�
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // �ݹ� �Լ� ���
			private:
				void on_read(unsigned char* received_text_, int size_); // ������ ���Ž� �ҷ����� �Լ�.
				void print_ip_as_hex(const char* ip);
				std::string clean_ip(const std::string& ip);
				void read(); // ����
				void write(); // �۽�
				receive_callback _receive_callback;
				udp_info	_udp_info;
#ifndef LINUX
				using socket_type = SOCKET; //UDP ����
				using socket_address_type = sockaddr_in; //���� �ּ�
				using socket_address_type2 = sockaddr; //���� �ּ�
				int socket_error_type = SOCKET_ERROR;
				using _sharedmutex = std::shared_mutex;
#else
				using socket_type = int;
				using socket_address_type = struct sockaddr_in;
				using socket_address_type2 = struct sockaddr;
				int socket_error_type = SO_ERROR;
				using _sharedmutex = std::shared_timed_mutex;
#endif
				std::deque<std::pair<int, std::pair<unsigned char*, int>>> _outbound_q; // ������ �۽ſ� queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // ������ ���ſ� queue
				std::mutex _read_mutex; // ���ſ� queue�� ���� mutex
				std::mutex _write_mutex; // �۽ſ� queue�� ���� mutex 
				_sharedmutex _contexts_mutex;
				bool _ready = true; // �غ����
				//bool _print_to_console = true; // �ֿܼ� ����Ʈ�� �� ������ ����
				bool _is_running = false; // ���� ����� ���۵Ǿ����� ����

#ifndef LINUX
				HANDLE _iocp;
#else
				int epoll_fd;
				struct epoll_event ev, events[MAX_EVENTS];
				std::condition_variable cv;
#endif
			protected:
				socket_type _udp_socket;  // UDP ���� (non-pointer approach)
				socket_address_type _source_address; // �ּ�
				std::map <int, socket_address_type> _destination_address_list;
				struct ip_mreq _mreq;
				int _bytes_transferred = -1;
			public:
				bool is_running();
			};
		}
	}
}