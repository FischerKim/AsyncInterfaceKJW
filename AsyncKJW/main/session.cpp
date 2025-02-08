#include <pch.h>

namespace II
{
	template <typename T>
	session<T>::session() : T(){
	}

	template <typename T>
	session<T>::session(session_info _info)
		: T()
	{
		session<T>::set_info(_info);
	}

	template <typename T>
	session<T>::~session()
	{
		session<T>::stop();
		session<T>::remove();
	}

	template <typename T>
	void session<T>::init()
	{
		try
		{
#ifndef LINUX
			std::memset(&m_session_info, 0x00, sizeof(session_info));
#else
			std::memset(&m_session_info, 0, sizeof(session_info));
#endif
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}

	template <typename T>
	h_short16_t session<T>::get_session_id()
	{
		return m_session_info._id;
	}

	template <typename T>
	void session<T>::set_info(const session_info& _info)
	{
		T::set_info(_info);
	}

	template <typename T>
	bool session<T>::start()
	{
		return T::start();
	}

	template <typename T>
	bool session<T>::stop()
	{
		return T::stop();
	}

	template <typename T>
	void session<T>::remove()
	{
		std::printf("     L [IFC]  REMOVED MODULE\n");
	}

	template <typename T>
	h_int32_t session<T>::write(h_char_t* i_pBuffer, h_int32_t i_nLength)
	{
		T::send(i_pBuffer, i_nLength);
		return 0;
	}
}