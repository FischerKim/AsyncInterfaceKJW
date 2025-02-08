#pragma once
namespace II
{
	template <typename T >
	class session // 통신 모듈 인터페이스
		: public	std::enable_shared_from_this<session<T>>
	{
		static_assert(std::is_class<T>::value, "Type must be a class");

	public:
		session();
		session(II::network::session_info _info);
		~session();

	private:
		std::shared_ptr <T>					_T;											// 통신 모듈 타입
		II::network::session_info			m_session_info;								// 통신 모듈 설정 정보
		//II::thread_pool						_pool;										// 스레드 풀
		std::thread							_thread;
	public:
		short			get_session_id();												// 통신 모듈 ID 값 가져오기
		void			set_info(const II::network::session_info& _info);				// 세션 설정
		void			start();														// 세션 시작
		void			stop();															// 세션 정지
		void			remove();														// 세션 제거
		void			send(short interface_id_, unsigned char* buffer_, int size_);	// 데이터 송신

		void			register_callback(std::function<void(short interface_id_, unsigned char* buffer, int size_)> callback_);

		bool			is_open_serial(); // 시리얼 포트 열려있는지 확인
		bool			is_connected_tcp_client(); // TCP 클라이언트가 서버에 연결되어있는지 확인
		int				num_users_connected_tcp_server(); // TCP 서버에 몇 개의 클라이언트가 연결되어있는 지 값 확인
		bool			is_running(); // TCP 서버에 몇 개의 클라이언트가 연결되어있는 지 값 확인
	};
}