#pragma once
namespace II
{
	namespace network
	{
		namespace modules
		{
			enum IO_OPERATION { OP_ACCEPT, OP_READ, OP_WRITE };

			struct PER_IO_DATA {
				OVERLAPPED overlapped;		// For asynchronous operation tracking
				WSABUF wsaBuf;				// Data buffer descriptor
				char* buffer;				// Dynamically allocated buffer
				DWORD bytesSent;			// Number of bytes sent (for write tracking)
				DWORD totalSize;			// Total data size to be sent/received
				IO_OPERATION operationType;  // Identifies the I/O operation type
			};

			class tcp_server_handler:
				public	std::enable_shared_from_this< tcp_server_handler >
			{
			private:
				static	tcp_server_handler*			_this; // ���� ������
			//	II::thread_pool						_pool; // ������ Ǯ int numThreads = sysInfo.dwNumberOfProcessors * 2;

				std::vector<std::thread> workers;
			public:
				using receive_callback = std::function<void(short interface_id_, unsigned char* buffer, int size_)>; // �ݹ� �Լ� Ÿ��

				tcp_server_handler(); // ������
				tcp_server_handler(const tcp_server_handler&) = delete; // �ش� ��ü�� ������ �Ұ�.
				tcp_server_handler& operator=(const tcp_server_handler&) = delete; // �ش� ��ü�� assign�� �Ұ�.
				~tcp_server_handler(); // �ı���

				void set_info(const session_info& _info); // ����
				int setNonBlocking(int fd);
				bool start(); // ����
				bool stop(); // ����	
				bool accept_connection(); // Ŭ���̾�Ʈ ���� ����

				void send_message(short interface_id_, unsigned char* buffer_, int size_); // ������ �۽�
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // �ݹ� �Լ� ���
				int num_users_connected(); // ������ ����� Ŭ���̾�Ʈ ��.
			private:
				receive_callback _receive_callback; // �ݹ� �Լ�
				int _num_users_entered = 0; // Ŭ���̾�Ʈ ��
				int _num_users_exited = 0; // Ŭ���̾�Ʈ ��
				tcp_info _tcp_info; // TCP ��� ���� ����
#ifndef LINUX
				using socket_type = SOCKET; // TCP ����
				using socket_address_type = sockaddr_in; // ���� �ּ�
				using socket_address_type2 = sockaddr; // ���� �ּ�
				int socket_error_type = SOCKET_ERROR;
#else
				using socket_type = int;
				using socket_address_type = struct sockaddr_in;
				using socket_address_type2 = struct sockaddr;
				int socket_error_type = SO_ERROR;
#endif
				socket_type _server_socket; // TCP ���� ���� (non-pointer approach)
				//std::deque<socket_type> _client_socket; // TCP Ŭ���̾�Ʈ ����
				struct client_context
				{
					short _id;
					socket_type _socket;
					/*std::thread _read_thread;
					std::thread _write_thread;*/
				};
				std::map<int, client_context> _client_socket;
				std::map<int, client_context> _newly_added_client_socket;

				void on_read(unsigned char* received_text_, int size_); // ������ ���Ž� �ҷ����� �Լ�.
				void read(); // ����
				void write(); // �۽�
				//unsigned char _outbound_packet[_buffer_size] = {}; // �۽ſ� ������
				//unsigned char _inbound_packet[_buffer_size] = {}; // ���ſ� ������
				std::deque<std::pair<unsigned char*, int>> _outbound_q; // ������ �۽ſ� queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // ������ ���ſ� queue

				std::mutex _read_mutex; // ���ſ� queue�� ���� mutex
				std::mutex _write_mutex; // �۽ſ� queue�� ���� mutex 
				std::mutex _contexts_mutex;
				bool _print_to_console = true; // �ֿܼ� ����Ʈ�� �� ������ ����
				//bool _first_time = true; // ó�� �����͸� �д��� ����
				bool _is_running = false; // ���� ����� ���۵Ǿ����� ����
				bool _connected = false; // ���� ����� ���۵Ǿ����� ����

				HANDLE _iocp;
				//PER_IO_DATA* _ioData;
				//char			mRecvBuf[MAX_SOCKBUF]
				int numThreads;
			public:
				bool is_running();
			};
		}
	}
}