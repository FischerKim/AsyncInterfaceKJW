#pragma once
#include <pch.h>

namespace II
{
	namespace network
	{
		namespace modules
		{
			//tcp_client_handler* tcp_client_handler::_this = nullptr;

			//socket -> connect -> send/recv
			tcp_client_handler::tcp_client_handler()  // 생성자
			{
				//_this = this;
			}

			tcp_client_handler::~tcp_client_handler() // 파괴자
			{
				_is_running = false;

#ifndef LINUX
				if (_client_socket != NULL)
				{
					shutdown(_client_socket, SD_BOTH);
					closesocket(_client_socket);
					_client_socket = NULL;
				}
				WSACleanup();
#else
				if (_client_socket >= 0)
				{
					shutdown(_client_socket, SHUT_RDWR);
					close(_client_socket);
					_client_socket = -1;
				}
#endif
				for (auto& t : workers)
				{
					if (t.joinable()) {
						t.join();
					}
				}
			}

			void tcp_client_handler::set_info(const session_info& info_) //설정
			{
				try
				{
					_tcp_info._id = info_._id;
					strcpy(_tcp_info._source_ip, info_._source_ip);
					_tcp_info._source_port = info_._source_port;


					std::strncpy(_tcp_info._destination_ip, info_._destinations.begin()->first.c_str(), sizeof(_tcp_info._destination_ip) - 1);
					_tcp_info._destination_port = info_._destinations.begin()->second;
					//strcpy(_tcp_info._destination_ip, info_._destination_ip);
					//_tcp_info._destination_port = info_._destination_port;
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Client] Error: " << e.what() << std::endl;
				}
			}

			int tcp_client_handler::set_nonblocking(int fd)
			{
#ifndef LINUX
#else
				int flags = fcntl(fd, F_GETFL, 0);
				if (flags == -1)
				{
					std::cout << "[TCP Client] setting non block returned -1" << std::endl;
					return -1;
				}
				return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
				return -1; //no need for window
			}

			void	tcp_client_handler::on_read(unsigned char* buffer_, int size_) // 데이터 수신시 불려지는 함수.
			{
				try
				{
					std::unique_lock<std::mutex> lock(_read_mutex);
					_inbound_q.emplace_back(buffer_, size_);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Client] Read Fail: " << e.what() << std::endl;
				}
			}

			bool	tcp_client_handler::start() // 시작
			{
				_is_running = true;
				std::printf("     L [IFC]  TCP CLIENT : %s\n", _tcp_info._name);
				std::printf("     L [IFC]    + CLIENT IP : %s\n", _tcp_info._source_ip);
				std::printf("     L [IFC]    + CLIENT PORT : %d\n", _tcp_info._source_port);
				std::printf("     L CONNECTING TO...\n");
				std::printf("     L [IFC]    + SERVER IP : %s\n", _tcp_info._destination_ip);
				std::printf("     L [IFC]    + SERVER PORT : %d\n", _tcp_info._destination_port);
#ifndef LINUX
				_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
				//	CreateIoCompletionPort((HANDLE)_client_socket, _iocp, (ULONG_PTR)_client_socket, 0); //4 넣어도 됨
#else
#endif
				std::thread connect_thread([this]() {
					while (true)
					{
						if (_connected) break;
						this->attempt_to_connect(); // connected 되면 read 부름
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}
					});
				workers.emplace_back(std::move(connect_thread));

#ifndef LINUX
				std::thread iocp_thread([this]() {
					bool first = true;
					while (true)
					{
						DWORD bytes_transferred;
						ULONG_PTR completionKey;
						PER_IO_DATA* ioData = nullptr;
						BOOL result = GetQueuedCompletionStatus( // 여기가 블로킹 콜임.
							_iocp,
							&bytes_transferred, //실제 전송된 바이트
							&completionKey,
							(LPOVERLAPPED*)&ioData, //overlapped IO 객체
							INFINITE //대기 시간
						);
						if (!result)
						{
							DWORD error = GetLastError();
							std::cerr << "GetQueuedCompletionStatus failed with error: " << error << std::endl;

							if (error == ERROR_NETNAME_DELETED) {
								std::cout << "Client disconnected." << std::endl;

								// Clean up the client socket and resources
								closesocket((SOCKET)completionKey);
								delete ioData;
								ioData = nullptr;
							}
						}
						if (ioData == nullptr) {
							std::cerr << "[TCP Client] Null overlapped structure!" << std::endl;
							break;  // Avoid dereferencing
						}
						if (ioData->operationType == OP_READ)
						{
							//	std::cout << "[TCP Client] read IOCP Called.." << std::endl;
							if (bytes_transferred > 0)
							{
								unsigned char* received_data_unsigned = new unsigned char[bytes_transferred];
								memset(received_data_unsigned, 0, sizeof(received_data_unsigned));
								std::memcpy(received_data_unsigned, ioData->buffer, bytes_transferred);
								this->on_read(std::move(received_data_unsigned), bytes_transferred);
								delete[] ioData->buffer;
								ioData->buffer = nullptr;
							}
							//							else if (bytes_transferred == socket_error_type)
							//							{
							//#ifndef LINUX
							//								if (WSAGetLastError())
							//								{
							//									std::cout << "Disconnected" << std::endl;
							//									_is_running = false;
							//									closesocket(_client_socket);
							//								}
							//#else
							//								if (errno)
							//								{
							//									std::cout << "Disconnected" << std::endl;
							//									_is_running = false;
							//									close(_client_socket);
							//								}
							//#endif
							//							}
							delete ioData;
							this->read();
						}
						else if (ioData->operationType == OP_WRITE)
						{
							//	std::cout << "[TCP Client] write IOCP Called.." << std::endl;

							delete ioData;
							//this->read();
						}
						else
						{
							std::cout << "undefined operation" << std::endl;
						}
						//std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
					});

				workers.emplace_back(std::move(iocp_thread));
#else
				std::thread epoll_thread([this]() {
					while (true)
					{
						if (!_connected)
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							continue;
						}
						int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); //-1이면 waiting time inifinite
						if (nfds == -1)
						{
							std::cout << "[TCP Client] epoll_wait error" << std::endl;
							perror("epoll_wait");
							close(_client_socket);
							close(epoll_fd);
							return;
						}
						/*
						EPOLLIN : 수신할 데이터가 있다. 
						EPOLLOUT : 송신 가능하다. 
						EPOLLPRI : 중요한 데이터(OOB) 발생. 
						EPOLLRDHUD : 연결 종료 또는 Half-close 발생 
						EPOLLERR : 에러 발생 
						EPOLLET : 엣지 트리거 방식으로 설정(기본은 레벨 트리거 방식) 
						EPOLLONESHOT : 한번만 이벤트 받음 
						*/
						for (int i = 0; i < nfds; i++)
						{
							if (events[i].events & EPOLLIN)
							{
								this->read();
							}
							/*if (events[i].events & EPOLLOUT)
							{
								this->write();
							}*/
						}
					}
					});

				workers.emplace_back(std::move(epoll_thread));

#endif

				std::thread callback_thread([this]() {
					while (true)
					{
						if (!_inbound_q.empty())
						{
							std::unique_lock<std::mutex> lock(_read_mutex);

							auto& front = _inbound_q.front();
							/*unsigned char* buffer = front.first;
							int size = front.second;
							_inbound_q.pop_front();*/

							/*std::string alphanumeric_text;
							for (int i = 0; i < size; ++i)
							{
								if (std::isalnum(buffer[i])) alphanumeric_text += buffer[i];
							}
							if (_print_to_console) std::cout << "[TCP Client] Received: " << alphanumeric_text << " Size: " << size << std::endl;*/

							std::cout << "[TCP Client] Received Data Size: " << front.second << std::endl;
#ifndef LINUX
							if (_receive_callback != NULL) _receive_callback(_tcp_info._id, front.first, front.second);
#else
							if (_receive_callback != nullptr) _receive_callback(_tcp_info._id, front.first, front.second);
#endif	
							delete[] front.first;
							front.first = nullptr;
							_inbound_q.pop_front();
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					});
				workers.emplace_back(std::move(callback_thread));


				std::thread write_thread([this]() {
					while (true)
					{
						this->write();
					}
					});

				workers.emplace_back(std::move(write_thread));

				return true;
			}

			bool	tcp_client_handler::stop() // 정지
			{
				bool result = false;

				try
				{
					_is_running = false;
					std::cout << "[TCP Client]: Stopping..." << std::endl;

					if (_client_socket == NULL) return true;

#ifndef LINUX

					for (auto& t : workers)
					{
						PostQueuedCompletionStatus(_iocp, 0, 0, NULL);
					}

					for (auto& t : workers)
					{
						if (t.joinable()) {
							t.join();
						}
					}
					if (_client_socket != NULL) shutdown(_client_socket, SD_BOTH);
					closesocket(_client_socket);
#else
					for (auto& t : workers)
					{
						if (t.joinable()) {
							t.join();
						}
					}
					if (_client_socket >= 0) shutdown(_client_socket, SHUT_RDWR);
					close(_client_socket);
#endif
					_client_socket = NULL;              // Prevent accidental reuse
					std::cout << "[TCP Client]: Stopped " << std::endl;
				}
				catch (const std::exception& e)
				{
					result = false;
					std::cerr << "[TCP Client] Error: " << e.what() << std::endl;
				}
				result = true;
				for (auto& t : workers)
				{
					if (t.joinable()) {
						t.join();
					}
				}
				return result;
			}

			void	tcp_client_handler::send_message(short destination_id_, unsigned char* buffer_, int size_)  // 데이터 송신
			{
				try
				{
					//std::cout << "client send called" << std::endl;
					std::unique_lock<std::mutex> lock(_write_mutex);

					//1024 바이트 이상일 경우 나눠서 송신 
					/*int num_chunks = size_ / 1024;
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
					}*/

					unsigned char* last_chunk = new unsigned char[size_];
					memset(last_chunk, 0, size_);
					memcpy(last_chunk, buffer_, size_);
					_outbound_q.emplace_back(std::move(last_chunk), size_);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Client] Error: " << e.what() << std::endl;
				}
			}

			void	tcp_client_handler::attempt_to_connect()
			{
				try
				{
					std::shared_lock<_sharedmutex> lock(_contexts_mutex);
					int i = 0;
					for (; i < strlen((char*)_tcp_info._destination_ip); i++)
					{
						if (_tcp_info._destination_ip[i] == ' ')
							break;
					}
					_tcp_info._destination_ip[i] = '\0';
#ifndef LINUX
					WSADATA wsaData;
					if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
					{
						std::cerr << "[TCP Client] Error when initializing Winsock" << std::endl;
						return;
					}

					_client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					socket_address_type client_address;
					client_address.sin_family = AF_INET;
					client_address.sin_addr.s_addr = INADDR_ANY;
					client_address.sin_port = htons(_tcp_info._source_port);
					if (bind(_client_socket, (socket_address_type2*)&client_address, sizeof(client_address)) == socket_error_type)
					{
						std::cerr << "[TCP Client] Error when binding a socket to a specified address" << std::endl;
						closesocket(_client_socket);
						return;
					}
					//if (_client_socket == -1)
					//{
					//	std::cerr << "[TCP Client] Error when creating a socket" << std::endl;

					//	return;
					//}
#else
					_client_socket = socket(AF_INET, SOCK_STREAM, 0);
					if (_client_socket == -1)
					{
						std::cerr << "[TCP Server] Error when creating a socket" << std::endl;
						perror("socket");
						return;
					}
					// Set the socket to be non-blocking 이걸 여기서 셋팅하면 connect가 안됨
				/*	if (set_nonblocking(_client_socket) == -1)
					{
						perror("set_nonblocking");
						close(_client_socket);
						return;
					}*/

					socket_address_type client_address;
					client_address.sin_family = AF_INET;
					client_address.sin_addr.s_addr = INADDR_ANY;
					client_address.sin_port = htons(_tcp_info._source_port);

					if (bind(_client_socket, (socket_address_type2*)&client_address, sizeof(client_address)) == -1)
					{
						std::cerr << "[TCP Client] Error when binding a socket to a specified address port:" << _tcp_info._source_port << std::endl;
						close(_client_socket);
						_client_socket = -1;
						sleep(1);
						return;
					}
					/*if (_client_socket == -1)
					{
						std::cerr << "[TCP Client] Error when creating a socket" << std::endl;
						close(_client_socket);
						return;
					}*/
#endif

					socket_address_type server_address;
					server_address.sin_family = AF_INET;
					server_address.sin_port = htons(_tcp_info._destination_port);
					inet_pton(AF_INET, _tcp_info._destination_ip, &server_address.sin_addr);
					int result = connect(_client_socket, (socket_address_type2*)&server_address, sizeof(server_address));

					if (result == -1)
					{
						std::cerr << "[TCP Client]: Error when connecting to the server" << std::endl;
						std::cout << "[TCP Client]: Trying again to connect to the server " << _tcp_info._destination_ip << ":" << _tcp_info._destination_port << std::endl;
#ifndef LINUX
						shutdown(_client_socket, SD_BOTH);  // SD_BOTH is generally more clear than '2'
						closesocket(_client_socket);        // Ensure socket handle is freed
						_client_socket = NULL;              // Prevent accidental reuse
						Sleep(1000);
#else
						close(_client_socket);
						_client_socket = -1;
						sleep(1);
#endif

						return;
					}
					else
					{
#ifndef LINUX
						int recvTimeout = 0;
						if (setsockopt(_client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR) {
							std::cerr << "[TCP Client]: setsockopt(SO_RCVTIMEO) failed: " << WSAGetLastError() << std::endl;
						}
						CreateIoCompletionPort((HANDLE)_client_socket, _iocp, (ULONG_PTR)&_client_socket, 0);

						socket_address_type actual_address;
						int addrlen = sizeof(actual_address);
#else
						socket_address_type actual_address;
						socklen_t addrlen = sizeof(actual_address);
#endif
						if (getpeername(_client_socket, (socket_address_type2*)&actual_address, &addrlen) == 0)
						{
							if (ntohs(actual_address.sin_port) != _tcp_info._destination_port)
							{
								std::cerr << "Error: Connected to a different port!" << std::endl;
#ifndef LINUX
								closesocket(_client_socket);
#else
								close(_client_socket);
#endif
								result = socket_error_type;

								return;
							}
							else
							{
#ifndef LINUX
#else
								epoll_fd = epoll_create1(0);
								if (epoll_fd == -1)
								{
									perror("epoll_create1");
									close(_client_socket);
									return;
								}

								// Add the listening socket to the epoll instance
								ev.events = EPOLLIN | EPOLLET;
								ev.data.fd = _client_socket; //이 fd를 통해 앞으로 epoll에 등록된 fd들을 조작함.
								if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _client_socket, &ev) == -1) //epoll에 fd 등록
								{
									perror("epoll_ctl: _client_socket");
									close(_client_socket);
									close(epoll_fd);
									return;
								}

#endif
								std::cout << "[TCP Client]: Connected to the server " << std::endl;
								_connected = true;
								//Start receiving when connected--------
								read();

								return;
							}
						}
						else
						{
							std::cerr << "Error getting peer name." << std::endl;
#ifndef LINUX
							closesocket(_client_socket);
#else
							close(_client_socket);
#endif
							result = socket_error_type;

							return;
						}
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Exception caught: " << e.what() << std::endl;
				}
				catch (...) {
					std::cerr << "Unknown exception caught" << std::endl;
				}

			}

			void	tcp_client_handler::read()  // 수신
			{
				/*if (_connected == false)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					return;
				}*/
				try
				{
					//std::cout << "client: read called" << std::endl;
					std::shared_lock<_sharedmutex> lock(_contexts_mutex);
#ifndef LINUX
					PER_IO_DATA* readData = new PER_IO_DATA{};
					readData->operationType = OP_READ;
					readData->buffer = new char[BUFFER_SIZE];
					readData->wsaBuf.buf = readData->buffer;
					readData->wsaBuf.len = BUFFER_SIZE;

					DWORD flags = 0;
					int ret = WSARecv(
						_client_socket,
						&readData->wsaBuf,
						1,
						NULL,                   // No immediate byte count
						&flags,
						&readData->overlapped,
						NULL                    // No completion routine (IOCP handles it)
					);

					if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
					{
						//std::cerr << "WSARecv failed: " << WSAGetLastError() << std::endl;
						delete[] readData->buffer;
						delete readData;
					}
#else
					char buffer[BUFFER_SIZE];
					ssize_t bytes_transferred = recv(_client_socket, buffer, sizeof(buffer), 0);

					if (bytes_transferred > 0)
					{
						unsigned char* received_data_unsigned = new unsigned char[bytes_transferred];
						memset(received_data_unsigned, 0, sizeof(received_data_unsigned));
						std::memcpy(received_data_unsigned, buffer, bytes_transferred);
						this->on_read(std::move(received_data_unsigned), bytes_transferred);
						//delete[] buffer;
					}
#endif
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Client] Read Error: " << e.what() << std::endl;
				}
				//std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}

			void	tcp_client_handler::write() // 송신
			{
				if (_connected == false)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					return;
				}
				try
				{
					std::shared_lock<_sharedmutex> lock(_contexts_mutex);

					unsigned char* unsigned_buffer = nullptr;
					int size = 0;
					if (!_outbound_q.empty())
					{
						std::unique_lock<std::mutex> lock(_write_mutex);
						auto& front = _outbound_q.front();
						unsigned_buffer = front.first;
						size = front.second;
						_outbound_q.pop_front();

						/*int ret = 0;
						{
							int ret = send(_client_socket, signed_buffer, size, 0);
						}*/
					}
					if (size > 0)
					{
						char* signed_buffer = new char[size];
						memset(signed_buffer, 0, size);
						memcpy(signed_buffer, unsigned_buffer, size);

						//std::cout << "client: write called" << std::endl;
#ifndef LINUX
						PER_IO_DATA* writeData = new PER_IO_DATA{};

						writeData->operationType = OP_WRITE;
						writeData->buffer = signed_buffer;
						writeData->wsaBuf.buf = writeData->buffer;
						writeData->wsaBuf.len = static_cast<ULONG>(size);
						writeData->bytesSent = 0;
						writeData->totalSize = static_cast<DWORD>(size);
						DWORD bytesSent = 0;
						DWORD flags = 0;
						int ret = WSASend(
							_client_socket,
							&writeData->wsaBuf,
							1,
							&bytesSent,          // This will be 0 for overlapped I/O
							flags,
							&writeData->overlapped,
							NULL                 // No completion routine, as IOCP handles completion
						);
#else
						ssize_t written = send(_client_socket, signed_buffer, size, 0);
						if (written == -1)
						{
							perror("send");
							close(_client_socket);
							return;
						}

						//// Re-arm the EPOLLOUT event for this client
						////struct epoll_event ev;
						//ev.events = EPOLLOUT | EPOLLET;
						//ev.data.fd = _client_socket;
						//if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, _client_socket, &ev) == -1)
						//{
						//	perror("epoll_ctl: mod _client_socket");
						//	close(_client_socket);
						//}
#endif

#ifndef LINUX
						if (ret == socket_error_type)
						{
							if (WSAGetLastError())
							{
								std::cout << "[TCP Client] Disconnected" << std::endl;
								_is_running = false;
								closesocket(_client_socket);
							}
						}
#else
						if (errno)
						{
							std::cout << "[TCP Client] Disconnected" << std::endl;
							_is_running = false;
							close(_client_socket);
						}
#endif
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
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Client] Write Error: " << e.what() << std::endl;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			bool	tcp_client_handler::is_running()  // 서버에 연결된 상태인지 확인.
			{
				return _is_running;
			}
		}
	}
}