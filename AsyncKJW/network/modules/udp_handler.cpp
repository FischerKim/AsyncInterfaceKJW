#pragma once
#include <pch.h>

namespace II
{
	namespace network
	{
		namespace modules
		{
			udp_handler* udp_handler::_this = nullptr;//셀프 포인터

			udp_handler::udp_handler() : _pool(4) // 생성자
			{
				_this = this;

				memset(_outbound_packet, 0, sizeof(_outbound_packet));
				memset(_inbound_packet, 0, sizeof(_inbound_packet));
			}

			udp_handler::~udp_handler() // 파괴자
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
			}

			void udp_handler::set_info(const session_info& info_) // 설정
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

			void	udp_handler::on_read(unsigned char* buffer_, int size_) // 데이터 수신시 불려지는 함수.
			{
				try
				{
					std::unique_lock<std::mutex> lock(_read_mutex);
					unsigned char* message_copy = new unsigned char[size_]; //이걸 안하면 ownership 이 on_read를 부른 부분에 있음. 따라서 이렇게 하고 나중에 메모리 해제하기
					memset(message_copy, 0, size_);
					memcpy(message_copy, buffer_, size_);

					_inbound_q.emplace_back(std::move(message_copy), size_);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Read Fail: " << e.what() << std::endl;
				}
			}

			bool udp_handler::start() // 시작
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
				//_udp_socket = new SOCKET; //heap에다 하면 안됨
				_udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
				_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
#endif
				if (_udp_socket == -1) {
					std::cerr << "Error when creating a socket" << std::endl;
					return false;
				}
				// 2-1. Enable address REUSE option
				int optval = 1;
				if (setsockopt(_udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) == -1) {
					std::cerr << "Failed to set SO_REUSEADDR option" << std::endl;
					return false;
				}
				// 2-2. Set the socket to non-blocking mode
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

				// 3. Setup IP from config
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

				// 4. binding socket with IP.
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
					_pool.push([this]() { //detaching from UI thread.
						bool read_block = true;
						bool write_block = true;
						////first time
					/*	_pool.push([this, &read_block]() { read_block = this->read(); });
						_pool.push([this, &write_block]() { write_block = this->write(); });*/

						while (true)
						{
							int x = 0;
							if (read_block)
							{
								read_block = false;
								_pool.push([this, &read_block]() {
									read_block = this->read();
									}); //async IO
								std::this_thread::sleep_for(std::chrono::milliseconds(5));
							}

							if (write_block)
							{
								write_block = false;
								_pool.push([this, &write_block]() {
									write_block = this->write();
									}); //async IO
								std::this_thread::sleep_for(std::chrono::milliseconds(5));
							}
							std::this_thread::sleep_for(std::chrono::milliseconds(5));
						}
						});
				}
			
				return true;
			}

			bool udp_handler::stop() // 정지	
			{
				bool result = false;
				_pool.push([this, &result]() { //pool takes care of the lock
					try
					{
						_is_running = false;
						std::cout << "[UDP] Stopping..." << std::endl;
						if (_udp_socket == NULL) return true;
#ifndef LINUX
						if(_udp_socket != NULL) closesocket(_udp_socket);
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
					result = true;
					});
				return result;
			}

			void udp_handler::send_message(short destination_id_, unsigned char* buffer_, int size_) // 데이터 송신
			{
				_pool.push([this, size_, buffer_]() { //pool takes care of the lock
				try
				{
					std::unique_lock<std::mutex> lock(_write_mutex);

					//1024 바이트 이상일 경우 나눠서 송신 
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
					});
			}

			bool udp_handler::read() // 수신
			{
				/*	while (_is_running)
					{*/
						try
						{
							//_pool.clear_queue();

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
					//	std::this_thread::sleep_for(std::chrono::milliseconds(5));
					//}
			}

			bool udp_handler::write() // 송신
			{
				/*while (_is_running)
				{*/
					try
					{
						if (!_outbound_q.empty())
						{
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
				//	std::this_thread::sleep_for(std::chrono::milliseconds(5));
				//}
					return true;
			}

			bool udp_handler::is_running()
			{
				return _is_running;
			}
		}
	}
}