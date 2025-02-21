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
				static	serial_handler* _this; //���� ������
				std::vector<std::thread> workers;
			public:
				using receive_callback = std::function<void(short interface_id_, unsigned char* buffer, int size_)>; //�ݹ� �Լ� Ÿ��

			public:
				serial_handler(); // ������
				serial_handler(const serial_handler&) = delete; // �ش� ��ü�� ������ �Ұ�.
				serial_handler& operator=(const serial_handler&) = delete; // �ش� ��ü�� assign�� �Ұ�.
				~serial_handler(); // �ı���

				void set_info(const session_info& _info); //����

				void on_read(unsigned char* received_text_, int size_); // ������ ���Ž� �ҷ����� �Լ�.
				bool is_open(); // �ø��� ��Ʈ �����ִ��� Ȯ��.

				bool start(); // ����
				bool stop(); // ����		

				void send_message(short interface_id_, unsigned char* buffer_, int size_); // ������ �۽�
				void register_callback(receive_callback callback_) { _receive_callback = callback_; } // �ݹ� �Լ� ���

				// ���ſ� ������
				std::deque<std::pair<unsigned char*, int>> _outbound_q; // ������ �۽ſ� queue 
				std::deque<std::pair<unsigned char*, int>> _inbound_q; // ������ ���ſ� queue
			private:
				void read_serial(); // ����
				void write_serial(); // �۽�
#ifndef LINUX
				using serial_handle = HANDLE; // GC heap�� �ִ� ��ü�� ����Ű�� ������
#else
				using serial_handle = int;
#endif
				serial_handle		_serial_handle; // �ø��� ��ü
				serial_info _serial_info;  // �ø��� ����
				receive_callback _receive_callback; // �ݹ� �Լ�

				std::mutex _read_mutex;  // ���ſ� queue�� ���� mutex
				std::mutex _write_mutex;  // �۽ſ� queue�� ���� mutex 
				bool _first_time = true;  // ó�� �����͸� �д��� ����
				//bool print_to_console = true;  // �ֿܼ� ����Ʈ�� �� ������ ����
				bool _is_running = false;  // ���� ����� ���۵Ǿ����� ����
			public:
				bool is_running();
			};
		}
	}
}