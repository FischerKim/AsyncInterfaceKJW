#pragma once
#include <pch.h>

namespace II
{
	namespace network
	{
		namespace modules
		{
			tcp_server_handler* tcp_server_handler::_this = nullptr;
			//socket -> bind -> listen -> accept -> send/recv
			tcp_server_handler::tcp_server_handler() : _pool(4) // 생성자
			{
				_this = this;
			}

			tcp_server_handler::~tcp_server_handler() // 파괴자
			{
				_is_running = false;
#ifndef LINUX
				if (_server_socket != NULL) 
				{
					shutdown(_server_socket, SD_BOTH);
					closesocket(_server_socket); 
					_server_socket = NULL;
				}
				WSACleanup();
#else
				if (_server_socket >= 0)
				{
					shutdown(_server_socket, SHUT_RDWR);
					close(_server_socket);
					_server_socket = -1;
				}
#endif
				/*memset(_outbound_packet, 0, sizeof(_outbound_packet));
				memset(_inbound_packet, 0, sizeof(_inbound_packet));*/
			}

			void	tcp_server_handler::set_info(const session_info& info_)  // 설정
			{
				try
				{
					_tcp_info._id = info_._id;
					strcpy(_tcp_info._source_ip, info_._source_ip);
					_tcp_info._source_port = info_._source_port;

					strcpy(_tcp_info._destination_ip, info_._destination_ip);
					_tcp_info._destination_port = info_._destination_port;
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Server] Error: " << e.what() << std::endl;
				}
			}

			void	tcp_server_handler::on_read(unsigned char* buffer_, int size_) // 데이터 수신시 불려지는 함수.
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
					std::cerr << "[TCP Server] Read Fail: " << e.what() << std::endl;
				}
			}

			bool	tcp_server_handler::start() // 시작
			{
				if (_is_running) return false;
				std::printf("     L [IFC]  TCP SERVER : %s\n", _tcp_info._name);
				std::printf("     L [IFC]    + SERVER IP : %s\n", _tcp_info._source_ip);
				std::printf("     L [IFC]    + SERVER PORT : %d\n", _tcp_info._source_port);

				int i = 0;
				for (; i < strlen((char*)_tcp_info._source_ip); i++)
				{
					if (_tcp_info._source_ip[i] == ' ') break;
				}
				_tcp_info._source_ip[i] = '\0';
#ifndef LINUX
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
				{
					std::cerr << "[TCP Server] Error when initializing Winsock" << std::endl;
					return false;
				}

				_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (_server_socket == -1) 
				{
					std::cerr << "[TCP Server] Error when creating a socket" << std::endl;
					return false;
				}
#else
				_server_socket = socket(AF_INET, SOCK_STREAM, 0);
				if (_server_socket == -1) {
					std::cerr << "[TCP Server] Error when creating a socket" << std::endl;
					return false;
				}
#endif

				socket_address_type server_address;
				server_address.sin_family = AF_INET;
				server_address.sin_port = htons(_tcp_info._source_port);
				inet_pton(AF_INET, _tcp_info._source_ip, &server_address.sin_addr);
				if (bind(_server_socket, (socket_address_type2*)&server_address, sizeof(server_address)) == -1) 
				{
					std::cerr << "[TCP Server] Error when binding a socket" << std::endl;
				}

				if (listen(_server_socket, 1) == -1) 
				{
					std::cerr << "Error when listening for incoming connections" << std::endl;
					return false;
				}

				std::cout << "TCP server started. Listening for incoming connections on " << _tcp_info._source_ip << ":" 
					<< std::to_string(_tcp_info._source_port) << std::endl;

				_is_running = true;
				_pool.push([this]() { //detaching from UI thread.
					bool read_block = true;
					bool write_block = true;
					bool accept_block = true;

					while (!_connected)
					{
						if (accept_block)
						{
							accept_block = false;
							_pool.push([this, &accept_block]() {
								//이게 블락킹콜로 점유하고 있으면 recv send가 안되는듯.
								accept_block = this->accept_connection();
								//search 버튼 이벤트 같은거 만들어서 _connected를 false로 잠깐 바꿔서 클라이언트가 더 들어오게끔 처리가 가능할듯.
								}); //async IO
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}


					while (_connected)
					{
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
							//_pool.push([this, &write_block]() {
									write_block = this->write(); 
							//}); //async IO 로 하면 더 빠르나 패킷이 합쳐서 들어감
						}

						std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
				});

				return true;
			}

			bool	tcp_server_handler::stop() // 정지	
			{
				bool result = false;
				_pool.push([this , &result]() { //pool takes care of the lock
					try
					{
						_is_running = false;
						std::cout << "[TCP Server]: Stopping..." << std::endl;

						if (_server_socket == NULL) return true;
	#ifndef LINUX
						for (auto& client : _client_socket)
						{
							closesocket(client.second._socket);
						}
						if (_server_socket != NULL) closesocket(_server_socket);
	#else
						for (auto& client : _client_socket)
						{
							close(client.second._socket);
						}
						if (_server_socket >= 0) close(_server_socket);
	#endif
						_client_socket.clear();

						//_main_thread.join();
						std::cout << "[TCP Server]: Stopped " << std::endl;
					}
					catch (const std::exception& e)
					{
						std::cerr << "[TCP Server] Error: " << e.what() << std::endl;
						result = false;
					}
					result = true;
				});
				return result;
			}

			void	tcp_server_handler::send_message(short destination_id_, unsigned char* buffer_, int size_) // 데이터 송신
			{
				_pool.push([this, size_, buffer_]() { //pool takes care of the lock
				try
				{
					std::unique_lock<std::mutex>lock(_write_mutex);

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
					std::cerr << "[TCP Server] Error: " << e.what() << std::endl;
				}
				});
			}

			bool	tcp_server_handler::accept_connection() // 클라이언트 연결 승인
			{
				try
				{
				//	std::unique_lock<std::mutex> lock(_contexts_mutex);
#ifndef LINUX
					socket_type client_socket = accept(_server_socket, NULL, NULL); //blocking call
#else
					socket_type client_socket = accept(_server_socket, nullptr, nullptr);
#endif
						if (client_socket == -1)
						{
							std::cerr << "Error when accepting an incoming connection" << std::endl;
						}
						else //if accepted
						{
							client_context oclient;
							oclient._id = _num_users_entered;
							oclient._socket = client_socket;

							_client_socket.insert(std::make_pair(_num_users_entered, std::move(oclient)));

							socket_address_type client_addr;
#ifndef LINUX
							int client_addr_len = sizeof(client_addr);
#else
							socklen_t client_addr_len = sizeof(client_addr);
#endif
							getpeername(client_socket, (socket_address_type2*)&client_addr, &client_addr_len);
							int port = ntohs(client_addr.sin_port);
							std::string address = inet_ntoa(client_addr.sin_addr);

							_num_users_entered++;
							_connected = true;

							std::cout << " [IFC]  TCP Server : Accepted a Client IP: " << address
								<< " Port: " << std::to_string(port) << std::endl;
						}
					//std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				catch (std::error_code& error_)
				{
					std::cerr << "[TCP Server] Error when binding acceptor: " << error_.message() << std::endl;
				}
				return true;
			}

			bool	tcp_server_handler::read() // 수신 //비동기일때 Context는 한개다.
			{
					try
					{
						//std::unique_lock<std::mutex> lock(_contexts_mutex);
						//if (_newly_added_client_socket.size() > 0)
						//	_client_socket.insert(_newly_added_client_socket.begin(), _newly_added_client_socket.end());
							
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
							if (_print_to_console) std::cout << "[TCP Server] Received: " << alphanumeric_text << " Size: " << size << std::endl;
#ifndef LINUX
							if (_receive_callback != NULL) _receive_callback(_tcp_info._id, std::move(buffer), size);
#else
							if (_receive_callback != nullptr) _receive_callback(_tcp_info._id, std::move(buffer), size);
#endif	
						}
						std::map<int, client_context>::iterator it = _client_socket.begin();
						while (it != _client_socket.end())
						{
							//TEST start------------------------
							socket_address_type client_addr;
							int client_addr_len = sizeof(client_addr);
							getpeername(it->second._socket, (socket_address_type2*)&client_addr, &client_addr_len);
							int port = ntohs(client_addr.sin_port);
							std::string address = inet_ntoa(client_addr.sin_addr);
							std::cout << "TEST LOG [TCP Server] Total clients: " << _client_socket.size() << " Current IP: " << address << std::endl;
							//TEST end------------------------

							char* received_data = new char[_buffer_size];
							std::memset(received_data, 0, _buffer_size);
							int bytes_transferred = 0;
							{
								bytes_transferred = recv(it->second._socket, received_data, _buffer_size, 0); //blocking call
							}
							if (bytes_transferred > 0)
							{
								unsigned char* received_data_unsigned = new unsigned char[bytes_transferred];
								memset(received_data_unsigned, 0, sizeof(received_data_unsigned));
								std::memcpy(received_data_unsigned, received_data, bytes_transferred);
								this->on_read(std::move(received_data_unsigned), bytes_transferred);
								delete[] received_data;
								received_data = nullptr;
							}
							else if (bytes_transferred == socket_error_type)
							{
#ifndef LINUX
								if (WSAGetLastError())// == WSAENOTCONN)
								{
									std::cout << "Disconnected" << std::endl;
									_is_running = false;
									closesocket(it->second._socket);
									_num_users_exited++;
								}
#else
								if (errno)
								{
									std::cout << "Disconnected" << std::endl;
									_is_running = false;
									close(it->second._socket);
									_num_users_exited++;
								}
#endif
								auto it_to_erase = it; 
								++it;
								_client_socket.erase(it_to_erase);
								continue; //
							}
							++it;
						}
							
					}
					catch (const std::exception& e)
					{
						std::cerr << "[TCP Server] Read Error: " << e.what() << std::endl;
						return true;
					}
				//	std::this_thread::sleep_for(std::chrono::milliseconds(5));
			//write();재귀방식은 stack overflow 에러 야기.
				return true;
			}

			bool	tcp_server_handler::write() // 송신
			{
					try
					{
						//std::unique_lock<std::mutex> lock(_contexts_mutex);
						/*if (_newly_added_client_socket.size() > 0)
							_client_socket.insert(_newly_added_client_socket.begin(), _newly_added_client_socket.end());*/
						
						if (!_outbound_q.empty())
						{
							std::unique_lock<std::mutex>lock(_write_mutex);
							auto& front = _outbound_q.front();
							unsigned char* unsigned_buffer = front.first;
							int size = front.second;
							_outbound_q.pop_front();

							char* signed_buffer = new char[size];
							memset(signed_buffer, 0, size);
							memcpy(signed_buffer, unsigned_buffer, size);

							std::map<int, client_context>::iterator it = _client_socket.begin();
							while (it != _client_socket.end())
							{
								int ret = 0;
								{
									//send function copies the data from the buffer. doesn't take ownership of the buffer itself.
									//std::move casts its argument to an rvalue reference
									if (signed_buffer != nullptr && size > 0) ret = send(it->second._socket, signed_buffer, size, 0); //blocking call
								}
								if (ret == socket_error_type)
								{
#ifndef LINUX
									if (WSAGetLastError())// == WSAENOTCONN)
									{
										std::cout << "Disconnected" << std::endl;
										_is_running = false;
										//closesocket(*it);
										closesocket(it->second._socket);
										_num_users_exited++;
									}
#else
									if (errno)
									{
										std::cout << "Disconnected" << std::endl;
										_is_running = false;
										//close(*it);
										close(it->second._socket);
										_num_users_exited++;
									}
#endif       
									auto it_to_erase = it;  
									++it;                
									_client_socket.erase(it_to_erase); 
									continue;
								}
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
								++it;
							}
							std::this_thread::sleep_for(std::chrono::milliseconds(5));
						}
						
					}
					catch (const std::exception& e)
					{
						std::cerr << "[TCP Server] Write Error: " << e.what() << std::endl;
					}
				//	std::this_thread::sleep_for(std::chrono::milliseconds(5));
				return true;
			}

			int		tcp_server_handler::num_users_connected()  // 서버에 연결된 클라이언트 수.
			{
				//하트비트를 기능을 넣어서 determine해야함.
				return _num_users_entered - _num_users_exited;
			}

			bool	tcp_server_handler::is_running()  // 서버에 연결된 상태인지 확인.
			{
				return _is_running;
			}
		}
	}
}