#pragma once
namespace II
{
	template <typename T >
	class session // ��� ��� �������̽�
		: public	std::enable_shared_from_this<session<T>>
	{
		static_assert(std::is_class<T>::value, "Type must be a class");

	public:
		session();
		session(II::network::session_info _info);
		~session();

	private:
		std::shared_ptr <T>					_T;											// ��� ��� Ÿ��
		II::network::session_info			m_session_info;								// ��� ��� ���� ����
		//II::thread_pool						_pool;										// ������ Ǯ
		std::thread							_thread;
	public:
		short			get_session_id();												// ��� ��� ID �� ��������
		void			set_info(const II::network::session_info& _info);				// ���� ����
		void			start();														// ���� ����
		void			stop();															// ���� ����
		void			remove();														// ���� ����
		void			send(short interface_id_, unsigned char* buffer_, int size_);	// ������ �۽�

		void			register_callback(std::function<void(short interface_id_, unsigned char* buffer, int size_)> callback_);

		bool			is_open_serial(); // �ø��� ��Ʈ �����ִ��� Ȯ��
		bool			is_connected_tcp_client(); // TCP Ŭ���̾�Ʈ�� ������ ����Ǿ��ִ��� Ȯ��
		int				num_users_connected_tcp_server(); // TCP ������ �� ���� Ŭ���̾�Ʈ�� ����Ǿ��ִ� �� �� Ȯ��
		bool			is_running(); // TCP ������ �� ���� Ŭ���̾�Ʈ�� ����Ǿ��ִ� �� �� Ȯ��
	};
}