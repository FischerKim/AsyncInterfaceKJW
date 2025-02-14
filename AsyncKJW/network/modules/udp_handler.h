#pragma once
namespace II
{
	namespace network
	{
		namespace modules
		{
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

				bool start(); // ����
				bool stop(); // ����	

				void send_message(short destination_id_, unsigned char* buffer_, int size_); // ������ �۽�
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // �ݹ� �Լ� ���
			private:
				unsigned char		_outbound_packet[BUFFER_SIZE] = {}; //initialize it with zeros
				unsigned char		_inbound_packet[BUFFER_SIZE] = {};

				void on_read(unsigned char* received_text_, int size_); // ������ ���Ž� �ҷ����� �Լ�.
				bool read(); // ����
				bool write(); // �۽�
				receive_callback _receive_callback;
				udp_info	_udp_info;
#ifndef LINUX
				using socket_type = SOCKET; //UDP ����
				using socket_address_type = sockaddr_in; //���� �ּ�
				using socket_address_type2 = sockaddr; //���� �ּ�
#else
				using socket_type = int;
				using socket_address_type = struct sockaddr_in;
				using socket_address_type2 = struct sockaddr;
#endif
				socket_type _udp_socket;  // UDP ���� (non-pointer approach)
				socket_address_type _source_address, _destination_address; // �ּ�
				std::deque<std::pair<unsigned char*, int>> _outbound_q; // ������ �۽ſ� queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // ������ ���ſ� queue
				std::mutex _read_mutex; // ���ſ� queue�� ���� mutex
				std::mutex _write_mutex; // �۽ſ� queue�� ���� mutex 
				std::mutex _contexts_mutex;
				bool _ready = true; // �غ����
				bool _print_to_console = true; // �ֿܼ� ����Ʈ�� �� ������ ����
				bool _is_running = false; // ���� ����� ���۵Ǿ����� ����
			public:
				bool is_running();
			};
		}
	}
}