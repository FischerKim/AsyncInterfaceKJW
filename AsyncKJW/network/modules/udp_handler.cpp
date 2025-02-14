#pragma once
#include <pch.h>

namespace II
{
	namespace network
	{
		namespace modules
		{
			//udp_handler* udp_handler::_this = nullptr;//���� ������

			udp_handler::udp_handler() // ������
			{
				//_this = this;

				memset(_outbound_packet, 0, sizeof(_outbound_packet));
				memset(_inbound_packet, 0, sizeof(_inbound_packet));
			}

			udp_handler::~udp_handler() // �ı���
			{
				_is_running = false;

#ifndef LINUX
				if (_udp_socket != NULL) {
					shutdown(_udp_socket, SD_BOTH);
					closesocket(_udp_socket);
					_udp_socket = NULL;
				}
				WSACleanup();
#else
				if (_udp_socket >= 0) {
					shutdown(_udp_socket, SHUT_RDWR);
					close(_udp_socket);
					_udp_socket = -1;
				}
#endif
				for (auto& t : workers)
				{
					if (t.joinable())
					{
						t.join();
					}
				}
			}

			void udp_handler::set_info(const session_info& info_) // ����
			{
				try
				{
					_udp_info._id = info_._id;
					strcpy(_udp_info._source_ip, info_._source_ip);
					_udp_info._source_port = info_._source_port;
					strcpy(_udp_info._destination_ip, info_._destination_ip);
					_udp_info._destination_port = info_._destination_port;
					_udp_info._type = UDP_UNICAST; //case for MULTICAST?
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Error: " << e.what() << std::endl;
				}
			}

			void	udp_handler::on_read(unsigned char* buffer_, int size_) // ������ ���Ž� �ҷ����� �Լ�.
			{
				try
				{
					std::unique_lock<std::mutex> lock(_read_mutex);
					//unsigned char* message_copy = new unsigned char[size_];
					//memset(message_copy, 0, size_);
					//memcpy(message_copy, buffer_, size_);

					_inbound_q.emplace_back(buffer_, size_);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Read Fail: " << e.what() << std::endl;
				}
			}

			bool udp_handler::start() // ����
			{
				if (_is_running) return false;;
				std::string type = "";
				if (_udp_info._type == UDP_UNICAST) type = "UNICAST";
				else type = "MULTICAST";
				std::printf("     L [IFC]  UDP CONNECT : %s\n", _udp_info._name);
				std::printf("     L [IFC]    + SRC IP : %s\n", _udp_info._source_ip);
				std::printf("     L [IFC]    + SRC PORT : %d\n", _udp_info._source_port);
				std::printf("     L [IFC]    + DST IP : %s\n", _udp_info._destination_ip);
				std::printf("     L [IFC]    + DST PORT : %d\n", _udp_info._destination_port);
				std::printf("     L [IFC]    + TYPE : %s\n", &type[0]);

				int i = 0;
				for (; i < strlen((char*)_udp_info._source_ip); i++)
				{
					if (_udp_info._source_ip[i] == ' ') break;
				}
				_udp_info._source_ip[i] = '\0';
				i = 0;
				for (; i < strlen((char*)_udp_info._destination_ip); i++)
				{
					if (_udp_info._destination_ip[i] == ' ') break;
				}
				_udp_info._destination_ip[i] = '\0';

				//1. Initialize winsock
#ifndef LINUX
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				{
					std::cerr << "Error when initializing Winsock" << std::endl;
					return false;
				}
#endif

				//2. Create UDP socket
#ifndef LINUX
				//_udp_socket = new SOCKET; //heap���� �ϸ� �ȵ�
				_udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
				_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
#endif
				if (_udp_socket == -1) {
					std::cerr << "Error when creating a socket" << std::endl;
					return false;
				}
				int optval = 1;
				if (setsockopt(_udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) == -1) {
					std::cerr << "Failed to set SO_REUSEADDR option" << std::endl;
					return false;
				}
#ifndef LINUX
				u_long mode = 1;  // 1 to enable non-blocking mode
				if (ioctlsocket(_udp_socket, FIONBIO, &mode) != 0) {
					std::cerr << "Failed to set non-blocking mode" << std::endl;
					closesocket(_udp_socket);
					WSACleanup();
					return false;
				}
#else

				int flags = fcntl(_udp_socket, F_GETFL, 0);
				if (flags == -1) {
					std::cerr << "Failed to get socket flags" << std::endl;
					return false;
				}

				if (fcntl(_udp_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
					std::cerr << "Failed to set non-blocking mode" << std::endl;
					return false;
				}
#endif

				// Setup IP from config
				memset(&_source_address, 0, sizeof(_source_address));
				memset(&_destination_address, 0, sizeof(_destination_address));

				_source_address.sin_family = AF_INET; // IPv4 
				//_server_address.sin_addr.s_addr = inet_pton(AF_INET, _udp_info._source_ip, &_server_address.sin_addr);
				_source_address.sin_port = htons(_udp_info._source_port);

				if (inet_pton(AF_INET, _udp_info._source_ip, &_source_address.sin_addr) != 1) {
					std::cerr << "Invalid IP address format: " << _udp_info._source_ip << std::endl;
#ifndef LINUX
					closesocket(_udp_socket);
					WSACleanup();
#else
					close(_udp_socket);  // Close the socket in Linux
#endif
					return false;
				}

				_destination_address.sin_family = AF_INET;
				//_remote_address.sin_addr.s_addr = inet_pton(AF_INET, _udp_info._destination_ip, &_remote_address.sin_addr);
				_destination_address.sin_port = htons(_udp_info._destination_port);
				if (inet_pton(AF_INET, _udp_info._destination_ip, &_destination_address.sin_addr) != 1) {
					std::cerr << "Invalid destination IP address format" << std::endl;
#ifndef LINUX
					closesocket(_udp_socket);
					WSACleanup();
#else
					close(_udp_socket);  // Close the socket in Linux
#endif
					return false;
				}

				// Binding UDP socket with IP.
				if (bind(_udp_socket, (socket_address_type2*)&_source_address, sizeof(_source_address)) == -1)
				{
#ifndef LINUX
					std::cerr << "Error when binding a socket: " << WSAGetLastError() << std::endl;
					closesocket(_udp_socket);
					WSACleanup();
#else
					std::cerr << "Error when binding a socket: " << errno << std::endl;
					close(_udp_socket);  // Close the socket in Linux
#endif
					return false;
				}
				else
				{
					_is_running = true;

					std::cout << "UDP address bound. " << _udp_info._source_ip << ":" << std::to_string(_udp_info._source_port) << std::endl;
					std::cout << "Destination IP:port is: " << _udp_info._destination_ip << ":" << std::to_string(_udp_info._destination_port) << std::endl;

					if (_udp_socket == -1)
					{
						std::cout << "Failed to initialize UDP." << std::endl;
						return false;
					}
#ifndef LINUX
					int recvTimeout = 5;  // 5 ms
					if (setsockopt(_udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR) {
						std::cerr << "[UDP] setsockopt(SO_RCVTIMEO) failed: " << WSAGetLastError() << std::endl;
					}
#else
					struct timeval recvTimeout;
					recvTimeout.tv_sec = 0;
					recvTimeout.tv_usec = 5000;  // 5 ms
					if (setsockopt(_udp_socket, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout)) < 0) {
						std::cerr << "[UDP]: setsockopt(SO_RCVTIMEO) failed: " << strerror(errno) << std::endl;
					}
#endif
					std::thread read_thread([this]() {
						while (true)
						{
							read();
						}
						});
					workers.emplace_back(std::move(read_thread));

					std::thread write_thread([this]() {
						while (true)
						{
							write();
						}
						});
					workers.emplace_back(std::move(write_thread));
				}
				return true;
			}

			bool udp_handler::stop() // ����	
			{
				bool result = false;

				try
				{
					_is_running = false;
					std::cout << "[UDP] Stopping..." << std::endl;
					if (_udp_socket == NULL) return true;
#ifndef LINUX
					if (_udp_socket != NULL) closesocket(_udp_socket);
#else
					if (_udp_socket >= 0) close(_udp_socket);  // Close the socket in Linux
#endif
					std::cout << "[UDP] Stopped " << _udp_info._source_ip << ":" << std::to_string(_udp_info._source_port) << std::endl;
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Error: " << e.what() << std::endl;
					result = false;
				}

				for (auto& t : workers)
				{
					if (t.joinable())
					{
						t.join();
					}
				}

				return true;
			}

			void udp_handler::send_message(short destination_id_, unsigned char* buffer_, int size_) // ������ �۽�
			{
				try
				{
					std::unique_lock<std::mutex> lock(_write_mutex);

					//1024 ����Ʈ �̻��� ��� ������ �۽� 
					int num_chunks = size_ / 1024;
					int remaining_bytes = size_ % 1024;
					for (int i = 0; i < num_chunks; ++i)
					{
						unsigned char* chunk = new unsigned char[1024];
						memset(chunk, 0, 1024);
						memcpy(chunk, buffer_ + (i * 1024), 1024);
						_outbound_q.emplace_back(std::move(chunk), 1024);
					}

					if (remaining_bytes > 0)
					{
						unsigned char* last_chunk = new unsigned char[remaining_bytes];
						memset(last_chunk, 0, remaining_bytes);
						memcpy(last_chunk, buffer_ + (num_chunks * 1024), remaining_bytes);
						_outbound_q.emplace_back(std::move(last_chunk), remaining_bytes);
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Error: " << e.what() << std::endl;
				}
			}

			bool udp_handler::read() // ����
			{
				try
				{
					std::unique_lock<std::mutex> socket_lock(_contexts_mutex);

					while (!_inbound_q.empty())
					{
						std::unique_lock<std::mutex> lock(_read_mutex);

						auto& front = _inbound_q.front();
						unsigned char* buffer = front.first;
						int size = front.second;
						_inbound_q.pop_front();

						std::string alphanumeric_text;
						for (int i = 0; i < size; ++i)
						{
							if (std::isalnum(buffer[i])) alphanumeric_text += buffer[i];
						}
						std::cout << "[UDP] Received: " << alphanumeric_text << " Size: " << size << std::endl;
#ifndef LINUX	
						if (_receive_callback != NULL) _receive_callback(_udp_info._id, std::move(buffer), size);
#else
						if (_receive_callback != nullptr) _receive_callback(_udp_info._id, std::move(buffer), size);
#endif
					}

					socket_address_type senderAddr;
					socklen_t len = sizeof(senderAddr);
					memset(_inbound_packet, 0, sizeof(_inbound_packet));
					std::vector<char> receiveBuffer(sizeof(_inbound_packet));

					int bytes_transferred = recv(_udp_socket, receiveBuffer.data(), receiveBuffer.size(), 0);
					if (bytes_transferred > 0)
					{
						std::memcpy(_inbound_packet, receiveBuffer.data(), bytes_transferred);
						unsigned char* received_data_unsigned = new unsigned char[bytes_transferred];
						memset(received_data_unsigned, 0, sizeof(received_data_unsigned));
						std::memcpy(received_data_unsigned, _inbound_packet, bytes_transferred);
						this->on_read(std::move(received_data_unsigned), bytes_transferred);
					}

				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Read Error: " << e.what() << std::endl;
				}
				return true;
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}

			bool udp_handler::write() // �۽�
			{
				try
				{
					if (!_outbound_q.empty())
					{
						std::unique_lock<std::mutex> socket_lock(_contexts_mutex);
						std::unique_lock<std::mutex> lock(_write_mutex);
						auto& front = _outbound_q.front();
						unsigned char* unsigned_buffer = front.first;
						int size = front.second;
						_outbound_q.pop_front(); // does not free the memory pointed to, but it only removes the entry in the queue.

						char* signed_buffer = new char[size];
						memset(signed_buffer, 0, size);
						memcpy(signed_buffer, unsigned_buffer, size);
						int len = sizeof(_destination_address);

						int bytes_sent = sendto(_udp_socket, signed_buffer, size, 0, (socket_address_type2*)&_destination_address, len);

						delete[] signed_buffer;
						signed_buffer = nullptr;

#ifndef LINUX
						if (_CrtIsValidHeapPointer(unsigned_buffer))
						{
							delete[] unsigned_buffer;
							unsigned_buffer = nullptr;
						}
#else
						if (unsigned_buffer)
						{
							delete[] unsigned_buffer;
							unsigned_buffer = nullptr; // Avoid dangling pointer
						}
#endif
						if (bytes_sent == -1)
						{
#ifndef LINUX
							int errCode = WSAGetLastError();
							wchar_t* s = nullptr;

							FormatMessageW(
								FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL
							);

							std::wcerr << L"Error sending data: " << s << std::endl;
							LocalFree(s);
#else
							int errCode = errno;  // In Linux, errors are stored in errno
							const char* error_message = strerror(errCode);  // Get the error message based on errno
							std::cerr << "Error sending data: " << error_message << std::endl;
#endif
						}
						else
						{
							std::cout << "Sent " << bytes_sent << " bytes to "
								<< _udp_info._destination_ip << ":"
								<< _udp_info._destination_port << std::endl;
						}

						std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Write Error: " << e.what() << std::endl;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				return true;
			}

			bool udp_handler::is_running()
			{
				return _is_running;
			}
		}
	}
}