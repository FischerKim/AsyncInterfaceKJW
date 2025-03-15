#pragma once
#include <pch.h>

namespace II
{
	typedef void (*II_callback)(short interface_id_, unsigned char* buffer_, int size_); //�ݹ� �Լ� ����

	class AsyncKJWImpl //II ������
	{
	public:
#pragma region CONSTRUCTOR & DESTRUCTOR
		AsyncKJWImpl(unsigned short equipment_number_)  //II ������ ������
			: _equipment_number(equipment_number_) {}

		~AsyncKJWImpl() = default; //II ������ �ı���
#pragma endregion

		config_manager m_config_manager; // �������� ���� ��ü
		unsigned short _equipment_number; // (ini���� ��� ��Ģ�� ���̴�) Equipment Ÿ�� �ε���
		std::map<short, std::shared_ptr<session<II::network::modules::udp_handler>>> m_map_udp; // UDP ��Ÿ�� ��ü �����̳�
		std::map<short, std::shared_ptr<session<II::network::modules::tcp_server_handler>>> m_map_tcp_server; // TCP ���� ��Ÿ�� ��ü �����̳�
		std::map<short, std::shared_ptr<session<II::network::modules::tcp_client_handler>>> m_map_tcp_client; // TCP Ŭ���̾�Ʈ ��Ÿ�� ��ü �����̳�
		std::map<short, std::shared_ptr<session<II::network::modules::serial_handler>>> m_map_serial; // �ø��� ��Ÿ�� ��ü �����̳�
	};

#pragma region CONSTRUCTOR & DESTRUCTOR
	AsyncKJW::AsyncKJW(unsigned short equipment_number_) // II Ŭ����: II ��ü ����
	{
		pImpl = new AsyncKJWImpl(equipment_number_);
	}

	AsyncKJW::~AsyncKJW()  // II Ŭ����: II ��ü �ı�
	{
		delete pImpl;
		pImpl = nullptr;
	}
#pragma endregion

	char* AsyncKJW::get_version()	// II Ŭ����: ���� ������ ����
	{
		static char version[10] = II_VERSION;
		return version;
	}

	char* AsyncKJW::get_running_statuses(int* out_size)	// II Ŭ����: ���� ������ ����
	{
		std::string running_statuses = "";
		if (pImpl->m_map_udp.size() > 0)
		{
			for (std::pair<short, std::shared_ptr<session<II::network::modules::udp_handler>>> iter : pImpl->m_map_udp)
			{
				running_statuses += " Type: UDP";
				running_statuses += " ID: " + std::to_string(iter.first);
				running_statuses += " Status: ";
				running_statuses += (iter.second->is_running() ? "OK" : "NG");
				running_statuses += "\n";
			}
		}
		if (pImpl->m_map_tcp_server.size() > 0)
		{
			for (std::pair<short, std::shared_ptr<session<II::network::modules::tcp_server_handler>>> iter : pImpl->m_map_tcp_server)
			{
				running_statuses += " Type: TCP Server";
				running_statuses += " ID: " + std::to_string(iter.first);
				running_statuses += " Status: ";
				running_statuses += (iter.second->is_running() ? "OK" : "NG");
				running_statuses += "\n";
			}
		}
		if (pImpl->m_map_tcp_client.size() > 0)
		{
			for (std::pair<short, std::shared_ptr<session<II::network::modules::tcp_client_handler>>> iter : pImpl->m_map_tcp_client)
			{
				running_statuses += " Type: TCP Client";
				running_statuses += " ID: " + std::to_string(iter.first);
				running_statuses += " Status: ";
				running_statuses += (iter.second->is_running() ? "OK" : "NG");
				running_statuses += "\n";
			}
		}
		if (pImpl->m_map_serial.size() > 0)
		{
			for (std::pair<short, std::shared_ptr<session<II::network::modules::serial_handler>>> iter : pImpl->m_map_serial)
			{
				running_statuses += " Type: Serial";
				running_statuses += " ID: " + std::to_string(iter.first);
				running_statuses += " Status: ";
				running_statuses += (iter.second->is_running() ? "OK" : "NG");
				running_statuses += "\n";
			}
		}

		char* c_str = new char[running_statuses.size() + 1];
		strcpy(c_str, running_statuses.c_str());

		*out_size = running_statuses.size();
		return c_str;
	}

	void AsyncKJW::load() // II Ŭ����: ���� �ҷ�����
	{
		try
		{
			std::vector<II::network::session_info> info_;
			info_.clear();

			// Load main config and optional UDP runtime file
			pImpl->m_config_manager.load_ini(info_, "Config/II.ini", "Config/udp_runtime.ini");
			// Load only main config
			/*pImpl->m_config_manager.load_ini(info_, "Config/II.ini");
			pImpl->m_config_manager.load_ini(info_, pImpl->_equipment_number);*/
			close();
			//std::unique_lock<std::mutex> lock(m_lock);
			printf("  [II] ------------ Load Sessions ---------------\n");

			if (info_.size() > 0)
			{
				for (II::network::session_info info : info_)
				{
					if (info._id == -1) continue;

					printf("  [II] Session ID =  %d\n", info._id);
					switch (info._type) 
					{
					case E_INTERFACE_TYPE::TYPE_UDP_UNICAST:
					{
						std::shared_ptr<session<II::network::modules::udp_handler>> S_ =
							std::make_shared<session<II::network::modules::udp_handler>>(info);
						pImpl->m_map_udp.insert({ info._id, std::move(S_) });
					}
					break;
					case E_INTERFACE_TYPE::TYPE_UDP_MULTICAST:
					{
						std::shared_ptr<session<II::network::modules::udp_handler>> S_ =
							std::make_shared<session<II::network::modules::udp_handler>>(info);
						pImpl->m_map_udp.insert({ info._id, std::move(S_) });
					}
					break;
					case E_INTERFACE_TYPE::TYPE_TCP_SERVER:
					{
						std::shared_ptr<session<II::network::modules::tcp_server_handler>> S_ =
							std::make_shared<session<II::network::modules::tcp_server_handler>>(info);
						pImpl->m_map_tcp_server.insert({ info._id, std::move(S_) });
					}
					break;
					case E_INTERFACE_TYPE::TYPE_TCP_CLIENT:
					{
						std::shared_ptr<session<II::network::modules::tcp_client_handler>> S_ =
							std::make_shared<session<II::network::modules::tcp_client_handler>>(info);
						pImpl->m_map_tcp_client.insert({ info._id, std::move(S_) });
					}
					break;
					case TYPE_SERIAL:
					{
						std::shared_ptr<session<II::network::modules::serial_handler>> S_ =
							std::make_shared<session<II::network::modules::serial_handler>>(info);
						pImpl->m_map_serial.insert({ info._id, std::move(S_) });
					}
					break;
					}
				}
			}

			printf("[II] UDP Session Count:  %zu\n", pImpl->m_map_udp.size());
			printf("[II] TCP Server Session Count:  %zu\n", pImpl->m_map_tcp_server.size());
			printf("[II] TCP Client Session Count:  %zu\n", pImpl->m_map_tcp_client.size());
			printf("[II] Serial Session Count:  %zu\n", pImpl->m_map_serial.size());
		}
		catch (std::exception ex)
		{
			std::cerr << ex.what() << std::endl;
		}
	}

	bool AsyncKJW::start(short source_id_) // II Ŭ����: ����
	{
		try
		{
				if (pImpl->m_map_udp.size() > 0)
				{
					for (std::pair<short, std::shared_ptr<session<II::network::modules::udp_handler>>> iter : pImpl->m_map_udp)
					{
						iter.second->start();
					}
				}
				if (pImpl->m_map_tcp_server.size() > 0)
				{
					for (std::pair<short, std::shared_ptr<session<II::network::modules::tcp_server_handler>>> iter : pImpl->m_map_tcp_server)
					{
						iter.second->start();
					}
				}
				if (pImpl->m_map_tcp_client.size() > 0)
				{
					for (std::pair<short, std::shared_ptr<session<II::network::modules::tcp_client_handler>>> iter : pImpl->m_map_tcp_client)
					{
						iter.second->start();
					}
				}
				if (pImpl->m_map_serial.size() > 0)
				{
					for (std::pair<short, std::shared_ptr<session<II::network::modules::serial_handler>>> iter : pImpl->m_map_serial)
					{
						iter.second->start();
					}
				}
		}
		catch (std::exception ex)
		{
			return false;
			std::cerr << ex.what() << std::endl;
		}
		return true;
	}

	bool AsyncKJW::stop(short id_) // II Ŭ����: ����
	{
		bool result = true;
		try
		{
			auto iter_udp = pImpl->m_map_udp.find(id_);
			if (iter_udp != pImpl->m_map_udp.end())
			{
				if (iter_udp->second) iter_udp->second->stop();
			}
			auto iter_tcp_server = pImpl->m_map_tcp_server.find(id_);
			if (iter_tcp_server != pImpl->m_map_tcp_server.end())
			{
				if (iter_tcp_server->second) iter_tcp_server->second->stop();
			}
			auto iter_tcp_client = pImpl->m_map_tcp_client.find(id_);
			if (iter_tcp_client != pImpl->m_map_tcp_client.end())
			{
				if (iter_tcp_client->second) iter_tcp_client->second->stop();
			}
			auto iter_serial = pImpl->m_map_serial.find(id_);
			if (iter_serial != pImpl->m_map_serial.end())
			{
				if (iter_serial->second) iter_serial->second->stop();
			}
		}
		catch (std::exception ex)
		{
			bool bStop = false;
			std::cout << "Error when Stopping" << std::endl;
			std::cerr << ex.what() << std::endl;
		}

		return result;
	}

	bool AsyncKJW::restart(short id_) // II Ŭ����: �����
	{
		stop(id_);
		start(id_);

		return false;
	}

	void AsyncKJW::close() // II Ŭ����: ����
	{
		try
		{
			printf("  [II] Reset all connections\n");
			if (pImpl->m_map_udp.size() > 0)
			{
				for (std::pair<short, std::shared_ptr<session<II::network::modules::udp_handler>>> iter : pImpl->m_map_udp)
				{
					if (iter.second) iter.second->stop();
				}
				pImpl->m_map_udp.clear();
			}
			if (pImpl->m_map_tcp_server.size() > 0)
			{
				for (std::pair<short, std::shared_ptr<session<II::network::modules::tcp_server_handler>>> iter : pImpl->m_map_tcp_server)
				{
					if (iter.second) iter.second->stop();
				}
				pImpl->m_map_tcp_server.clear();
			}
			if (pImpl->m_map_tcp_client.size() > 0)
			{
				for (std::pair<short, std::shared_ptr<session<II::network::modules::tcp_client_handler>>> iter : pImpl->m_map_tcp_client)
				{
					if (iter.second) iter.second->stop();
				}
				pImpl->m_map_tcp_client.clear();
			}
			if (pImpl->m_map_serial.size() > 0)
			{
				for (std::pair<short, std::shared_ptr<session<II::network::modules::serial_handler>>> iter : pImpl->m_map_serial)
				{
					if (iter.second) iter.second->stop();
				}
				pImpl->m_map_serial.clear();
			}
		}
		catch (std::exception ex)
		{
			//std::cerr << ex.what() << std::endl;
		}
	}

	int AsyncKJW::write(short source_id_, short destination_id_, unsigned char* buffer_, int size_) // II Ŭ����: ������ ����
	{
		try
		{
			int nSendStatus = -1;
			//map<short, boost::shared_ptr<session<II::network::modules::udp_handler>>>::iterator iter = m_map_udp.find(interface_id_);

			auto iter_udp = pImpl->m_map_udp.find(source_id_);
			if (iter_udp != pImpl->m_map_udp.end())
			{
				iter_udp->second->send(destination_id_, buffer_, size_);
			}
			auto iter_tcp_server = pImpl->m_map_tcp_server.find(source_id_);
			if (iter_tcp_server != pImpl->m_map_tcp_server.end())
			{
				iter_tcp_server->second->send(destination_id_, buffer_, size_);
			}
			auto iter_tcp_client = pImpl->m_map_tcp_client.find(source_id_);
			if (iter_tcp_client != pImpl->m_map_tcp_client.end())
			{
				iter_tcp_client->second->send(destination_id_, buffer_, size_);
			}
			auto iter_serial = pImpl->m_map_serial.find(source_id_);
			if (iter_serial != pImpl->m_map_serial.end())
			{
				iter_serial->second->send(destination_id_, buffer_, size_);
			}
			printf("  [II] WriteData : ID : %d, DATA : %s, DataSIZE : %d\n",
				source_id_, buffer_, size_);
		}
		catch (std::exception ex)
		{
			std::cerr << ex.what() << std::endl;
		}
		return size_;
	}

	void AsyncKJW::register_callback(int id_, void* callback_) // II Ŭ����: UDP ������ ���� �� �ҷ����� �ݹ��Լ�
	{
		try
		{
			if (!callback_) {
				std::cerr << "Error: callback_ is null." << std::endl;
				return;
			}
			// Cast the void* callback to the correct function pointer type
			II_callback callback_fn = reinterpret_cast<II_callback>(callback_);

			// Wrap the function pointer in a std::function to use it
			std::function<void(short, unsigned char*, int)> callback = [callback_fn](short interface_id_, unsigned char* buffer, int size_)
			{
				// Call the original function pointer
				callback_fn(interface_id_, buffer, size_);
			};
			bool BFound = false;
			auto tcp_server_pair = pImpl->m_map_tcp_server.find(id_);
			if (tcp_server_pair != pImpl->m_map_tcp_server.end())
			{
				BFound = true;
				auto& handler = tcp_server_pair->second;
				handler->register_callback(callback);
				std::cout << "Registering Callback. ID: " << id_ << std::endl;
			}

			auto tcp_client_pair = pImpl->m_map_tcp_client.find(id_);
			if (tcp_client_pair != pImpl->m_map_tcp_client.end())
			{
				BFound = true;
				auto& handler = tcp_client_pair->second;
				handler->register_callback(callback);
				std::cout << "Registering Callback. ID: " << id_ << std::endl;
			}

			auto udp_pair = pImpl->m_map_udp.find(id_);
			if (udp_pair != pImpl->m_map_udp.end())
			{
				BFound = true;
				auto& handler = udp_pair->second;
				handler->register_callback(callback);
				std::cout << "Registering Callback. ID: " << id_ << std::endl;
			}

			auto serial_pair = pImpl->m_map_serial.find(id_);
			if (serial_pair != pImpl->m_map_serial.end())
			{
				BFound = true;
				auto& handler = serial_pair->second;
				handler->register_callback(callback);
				std::cout << "Registering Callback. ID: " << id_ << std::endl;
			}
			if (BFound == false) std::cout << "Cannot register callback for ID: " << id_ << std::endl;
		}
		catch (std::exception ex)
		{
			std::cerr << ex.what() << std::endl;
		}
	}

	bool AsyncKJW::is_open_serial(short id_) // II Ŭ����: �ø��� ��Ʈ �����ִ��� Ȯ��
	{
		bool result = true;
		try
		{
			auto iter_serial = pImpl->m_map_serial.find(id_);
			if (iter_serial != pImpl->m_map_serial.end())
			{
				if (iter_serial->second) result = iter_serial->second->is_open_serial();
			}
			else
			{
				return false;
			}
		}
		catch (std::exception ex)
		{
			bool bStop = false;
			std::cout << "Error when Stopping" << std::endl;
			std::cerr << ex.what() << std::endl;
		}

		return result;
	}

	bool AsyncKJW::is_connected_tcp_client(short id_) // II Ŭ����: TCP Ŭ���̾�Ʈ�� ������ ����Ǿ��ִ��� Ȯ��
	{
		bool result = true;
		try
		{
			auto iter_tcp_client = pImpl->m_map_tcp_client.find(id_);
			if (iter_tcp_client != pImpl->m_map_tcp_client.end())
			{
				result = iter_tcp_client->second->is_connected_tcp_client();
			}
			else
			{
				return false;
			}
		}
		catch (std::exception ex)
		{
			bool bStop = false;
			std::cout << "Error when Stopping" << std::endl;
			std::cerr << ex.what() << std::endl;
		}

		return result;
	}

	int AsyncKJW::num_users_connected_tcp_server(short id_) // II Ŭ����: TCP ������ �� ���� Ŭ���̾�Ʈ�� ����Ǿ��ִ� �� �� Ȯ��.
	{
		try
		{
			auto iter_tcp_server = pImpl->m_map_tcp_server.find(id_);
			if (iter_tcp_server != pImpl->m_map_tcp_server.end())
			{
				return iter_tcp_server->second->num_users_connected_tcp_server();
			}
			else
			{
				return 0;
			}
		}
		catch (std::exception ex)
		{
			bool bStop = false;
			std::cout << "Error when Stopping" << std::endl;
			std::cerr << ex.what() << std::endl;
		}

		return 0;
	}

#pragma region API FUNCTION
	extern "C" EXPORT_CLASS AsyncKJW* API_Create(unsigned short equipment_number_) // II API �Լ�: II ��ü ����
	{
		return new AsyncKJW(equipment_number_);
	}

	extern "C" EXPORT_CLASS void API_Destroy(AsyncKJW* instance_) // II API �Լ�: II ��ü �ı�
	{
		if (instance_) 
		{
			instance_->close();
			delete instance_;
			instance_ = nullptr;
		}
	}

	extern "C" EXPORT_CLASS char* API_get_version(AsyncKJW* instance_) // II API �Լ�: ���� ������ ����
	{
		return instance_->get_version();
	}

	extern "C" EXPORT_CLASS char* API_get_running_statuses(AsyncKJW * instance_, int* out_size)
	{
		return instance_->get_running_statuses(out_size);
	}

	extern "C" EXPORT_CLASS void API_load(AsyncKJW * instance_) // II API �Լ�: ���� �ҷ�����
	{
		instance_->load();
	}

	extern "C" EXPORT_CLASS bool API_start(AsyncKJW* instance_, short id_) // II API �Լ�: ����
	{
		return instance_->start(id_);
	}

	extern "C" EXPORT_CLASS bool API_stop(AsyncKJW* instance_, short id_) // II API �Լ�: ����
	{
		return instance_->stop(id_);
	}

	extern "C" EXPORT_CLASS bool API_restart(AsyncKJW* instance_, short id_) // II API �Լ�: �����
	{
		return instance_->restart(id_);
	}

	extern "C" EXPORT_CLASS void API_close(AsyncKJW* instance_) // II API �Լ�: ����
	{
		return instance_->close();
	}

	extern "C" EXPORT_CLASS int API_write(AsyncKJW * instance_, short source_id_, short destination_id_, unsigned char* buffer_, int size_) // II API �Լ�: ������ ����
	{
		int result = false;
		try
		{
			if (buffer_ == nullptr || size_ <= 0)
			{
				std::cerr << "Invalid buffer or size" << std::endl;
				return -1;
			}

			result = instance_->write(source_id_, destination_id_, buffer_, size_);
		}
		catch (std::exception ex)
		{
			std::cerr << ex.what() << std::endl;
		}
		return result;
	}

	extern "C" EXPORT_CLASS void API_register_callback(AsyncKJW * instance_, int id_, void* callback_) // II API �Լ�: UDP ������ ���� �� �ҷ����� �ݹ��Լ�
	{
		return instance_->register_callback(id_, callback_);
	}

	extern "C" EXPORT_CLASS bool API_is_open_serial(AsyncKJW * instance_, short id_) // II API �Լ�: �ø��� ��Ʈ �����ִ��� Ȯ��
	{
		return instance_->is_open_serial(id_);
	}

	extern "C" EXPORT_CLASS bool API_is_connected_tcp_client(AsyncKJW * instance_, short id_) // II API �Լ�: TCP Ŭ���̾�Ʈ�� ������ ����Ǿ��ִ��� Ȯ��
	{
		return instance_->is_connected_tcp_client(id_);
	}

	extern "C" EXPORT_CLASS int API_num_users_connected_tcp_server(AsyncKJW * instance_, short id_) // II API �Լ�: TCP ������ �� ���� Ŭ���̾�Ʈ�� ����Ǿ��ִ� �� �� Ȯ��.
	{
		return instance_->num_users_connected_tcp_server(id_);
	}
#pragma endregion

		/*void AsyncKJW::save(std::vector<II::network::session_info> info_)
		{
			if (info_.size() > 0)
			{
				m_config_manager.save_to_file(info_);
				insert(info_);
			}
			else
			{
				std::cout << "  [II] (100) Size Error! - total : " << info_.size() << std::endl;
			}
		}*/
}