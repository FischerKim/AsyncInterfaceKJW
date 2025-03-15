#pragma once
#ifndef LINUX
#define EXPORT_CLASS __declspec(dllexport)
#else
#define EXPORT_CLASS __attribute__((visibility("default")))
#endif

namespace II
{
	class AsyncKJWImpl;	// Forward declaration

	class EXPORT_CLASS AsyncKJW // II Ŭ����
	{
	public:
		AsyncKJW(unsigned short equipment_number_); // II Ŭ����: II ��ü ����
		~AsyncKJW(); // II Ŭ����: II ��ü �ı�

		char* get_version(); // II Ŭ����: ���� ������ ����
		char* get_running_statuses(int* out_size); // II Ŭ����: ��� II ��� ��� ���� �ҷ�����
		void load(); // II Ŭ����: ���� �ҷ�����
		bool start(short id_); // II Ŭ����: ����
		bool stop(short id_); // II Ŭ����: ����
		bool restart(short id_); // II Ŭ����: �����
		void close(); // II Ŭ����: ����
		int write(short source_id_, short destination_id_, unsigned char* buffer_, int size_); // II Ŭ����: ������ ����

		//C# ���� ��� std::function�� ���������� ������� ���� �׷��� void*�� ó��.
		void register_callback(int id_, void* callback_); // II Ŭ����: ������ ���� �� �ҷ����� �ݹ��Լ�

		bool is_open_serial(short id_); // II Ŭ����: �ø��� ��Ʈ �����ִ��� Ȯ��
		bool is_connected_tcp_client(short id_); // II Ŭ����: TCP Ŭ���̾�Ʈ�� ������ ����Ǿ��ִ��� Ȯ��
		int	 num_users_connected_tcp_server(short id_); // II Ŭ����: TCP ������ �� ���� Ŭ���̾�Ʈ�� ����Ǿ��ִ� �� �� Ȯ��.
	private:
		AsyncKJWImpl* pImpl; // P Invoke style
	};

	extern "C" 
	{
		// C-style exported functions for C# or non-C++ users
		EXPORT_CLASS AsyncKJW* API_Create(unsigned short equipment_number_); // II API �Լ�: II ��ü ����
		EXPORT_CLASS void API_Destroy(AsyncKJW* instance_); // II API �Լ�: II ��ü �ı�
		EXPORT_CLASS char* API_get_version(AsyncKJW* instance_); // II API �Լ�: ���� ������ ����
		EXPORT_CLASS char* API_get_running_statuses(AsyncKJW* instance_, int* out_size); // ��� II ��� ��� ���� �ҷ�����
		EXPORT_CLASS void API_load(AsyncKJW* instance_); // II API �Լ�: ���� �ҷ�����
		EXPORT_CLASS bool API_start(AsyncKJW* instance_, short id_); // II API �Լ�: ����
		EXPORT_CLASS bool API_stop(AsyncKJW* instance_, short id_); // II API �Լ�: ����
		EXPORT_CLASS bool API_restart(AsyncKJW* instance_, short id_); // II API �Լ�: �����
		EXPORT_CLASS void API_close(AsyncKJW* instance_); // II API �Լ�: ����
		EXPORT_CLASS int API_write(AsyncKJW* instance_, short source_id_, short destination_id_, unsigned char* buffer_, int size_); // II API �Լ�: ������ ����
		
		EXPORT_CLASS void API_register_callback(AsyncKJW* instance_, int id_, void* callback_);

		EXPORT_CLASS bool API_is_open_serial(AsyncKJW* instance_, short id_); // II API �Լ�: �ø��� ��Ʈ �����ִ��� Ȯ��
		EXPORT_CLASS bool API_is_connected_tcp_client(AsyncKJW* instance_, short id_); // II API �Լ�: TCP Ŭ���̾�Ʈ�� ������ ����Ǿ��ִ��� Ȯ��
		EXPORT_CLASS int API_num_users_connected_tcp_server(AsyncKJW* instance_, short id_); // II API �Լ�: TCP ������ �� ���� Ŭ���̾�Ʈ�� ����Ǿ��ִ� �� �� Ȯ��
	}
}