
namespace II
{
	//설명 : 생성자
	//1. 세션 생성자를 생성한다.
	template <typename T>
	session<T>::session() //: _pool(2)
	{
		_T = std::make_shared<T>();
	}

	//설명 : 생성자
	//1. 세션 객체를 설정파일에 맞게 생성한다.
	template <typename T>
	session<T>::session(II::network::session_info _info) //: _pool(3)
	{
		_T = std::make_shared<T>();
		set_info(_info);
	}

	//설명 : 파괴자
	template <typename T>
	session<T>::~session()
	{
		remove();
	}

	//설명 : 통신 모듈 ID 값 가져오기
	template <typename T>
	short session<T>::get_session_id()
	{
		return m_session_info._id;
	}

	//설명 : 세션 설정
	template <typename T>
	void session<T>::set_info(const II::network::session_info& _info)
	{
		try
		{
			m_session_info._id = _info._id;
			_T->set_info(_info);
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}

	//설명 : 세션 시작 
	template <typename T>
	void session<T>::start()
	{
		//_pool.push([this]() {
		_T->start();
		//	});
	}

	//설명 : 세션 정지
	template <typename T>
	void session<T>::stop()
	{
		//_pool.push([this]() {
		_T->stop();
		//	});
	}

	//설명 : 세션 제거
	template <typename T>
	void session<T>::remove()
	{
		std::printf("    L [IFC]  REMOVED MODULE\n");
	}

	//설명 : 데이터 송신
	template <typename T>
	void session<T>::send(short interface_id_, unsigned char* buffer_, int size_)
	{
		//_pool.push([this, interface_id_, buffer_, size_]() {
		_T->send_message(interface_id_, buffer_, size_);
		//	});
	}

	//설명 : 콜백 함수 등록
	template <typename T>
	void session<T>::register_callback(std::function<void(short interface_id_, unsigned char* buffer, int size_)> callback_)
	{
		_T->register_callback(callback_);
	}

	//설명 : 시리얼 포트 열려있는지 확인
	template <typename T>
	bool session<T>::is_open_serial()
	{
		if (std::is_same<T, II::network::modules::serial_handler>::value)
		{
			return _T->is_open();
		}
	}

	//설명 : TCP 클라이언트가 서버에 연결되어있는지 확인
	template <typename T>
	bool session<T>::is_connected_tcp_client()
	{
		if (std::is_same<T, II::network::modules::tcp_client_handler>::value)
		{
			return _T->is_running();
		}
	}

	//설명 : TCP 서버에 몇 개의 클라이언트가 연결되어있는 지 값 확인
	template <typename T>
	int session<T>::num_users_connected_tcp_server()
	{
		if (std::is_same<T, II::network::modules::tcp_server_handler>::value)
		{
			return _T->num_users_connected();
		}
	}

	template <typename T>
	bool session<T>::is_running()
	{
		return _T->is_running();
	}
}