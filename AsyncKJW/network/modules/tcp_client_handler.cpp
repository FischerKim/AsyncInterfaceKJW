#pragma once
#include <pch.h>

namespace II
{
	//클라이언트는 iocp가 필요 없음.
	namespace network
	{
		namespace modules
		{
			tcp_client_handler* tcp_client_handler::_this = nullptr;
			//socket -> connect -> send/recv
			tcp_client_handler::tcp_client_handler() //: _pool(4)  // 생성자
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
				/*memset(_outbound_packet, 0, sizeof(_outbound_packet));
				memset(_inbound_packet, 0, sizeof(_inbound_packet));*/
			}

			void tcp_client_handler::set_info(const session_info& info_) //설정
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
					std::cerr << "[TCP Client] Error: " << e.what() << std::endl;
				}
			}

			int tcp_client_handler::setNonBlocking(int fd) {
#ifndef LINUX
#else
				int flags = fcntl(fd, F_GETFL, 0);
				return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
				return -1;
			}

			void	tcp_client_handler::on_read(unsigned char* buffer_, int size_) // 데이터 수신시 불려지는 함수.
			{
				try
				{
					std::unique_lock<std::mutex> lock(_read_mutex);

					/*unsigned char* message_copy = new unsigned char[size_];
					memset(message_copy, 0, size_);
					memcpy(message_copy, buffer_, size_);*/

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

				//_pool.push([this]() { this->attempt_to_connect();});
				//_pool.push([this]() { this->read(); });
				//_pool.push([this]() { this->write(); });
#ifndef LINUX
#else
				setNonBlocking(_client_socket);
#endif
#ifndef LINUX
				_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
				CreateIoCompletionPort((HANDLE)_client_socket, _iocp, (ULONG_PTR)_client_socket, 0); //4 넣어도 됨
#else
#endif
				std::thread connect_thread([this]() {
					while (!_connected)
					{
						this->attempt_to_connect();
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}
					this->read();
					});
				workers.emplace_back(std::move(connect_thread));


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
							}
							continue;
						}
						if (ioData == nullptr) {
							std::cerr << "[TCP Client] Null overlapped structure!" << std::endl;
							continue;  // Avoid dereferencing
						}
						if (ioData->operationType == OP_READ)
						{
							std::cout << "[TCP Client] read IOCP Called.." << std::endl;
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
							//this->read();
						}
						else if (ioData->operationType == OP_WRITE)
						{
							std::cout << "[TCP Client] write IOCP Called.." << std::endl;

							delete ioData;
							//	this->write();
						}
						else
						{
							std::cout << "undefined operation" << std::endl;
						}
						//std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
					});

				workers.emplace_back(std::move(iocp_thread));


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
						this->write();
					}
					});

				workers.emplace_back(std::move(write_thread));

				std::thread callback_thread([this]() {
					while (true)
					{
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
							if (_print_to_console) std::cout << "[TCP Client] Received: " << alphanumeric_text << " Size: " << size << std::endl;
#ifndef LINUX
							if (_receive_callback != NULL) _receive_callback(_tcp_info._id, std::move(buffer), size);
#else
							if (_receive_callback != nullptr) _receive_callback(_tcp_info._id, std::move(buffer), size);
#endif	
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
					});
				workers.emplace_back(std::move(callback_thread));



				return true;
			}

			bool	tcp_client_handler::stop() // 정지
			{
				bool result = false;
				//_pool.push([this, &result]() { //pool takes care of the lock
				try
				{
					_is_running = false;
					std::cout << "[TCP Client]: Stopping..." << std::endl;

					if (_client_socket == NULL) return true;

#ifndef LINUX
					if (_client_socket != NULL) shutdown(_client_socket, SD_BOTH);
					closesocket(_client_socket);
#else
					if (_client_socket >= 0) shutdown(_client_socket, SHUT_RDWR);
					close(_client_socket);
#endif
					std::cout << "[TCP Client]: Stopped " << std::endl;
				}
				catch (const std::exception& e)
				{
					result = false;
					std::cerr << "[TCP Client] Error: " << e.what() << std::endl;
				}
				result = true;
				//	});
				return result;
			}

			void	tcp_client_handler::send_message(short destination_id_, unsigned char* buffer_, int size_)  // 데이터 송신
			{
				//_pool.push([this, size_, buffer_]() { //pool takes care of the lock
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
					std::cerr << "[TCP Client] Error: " << e.what() << std::endl;
				}
				//	});
			}

			void	tcp_client_handler::attempt_to_connect()
			{
				std::unique_lock<std::mutex> lock(_contexts_mutex);
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
				if (_client_socket == -1)
				{
					std::cerr << "[TCP Client] Error when creating a socket" << std::endl;

					return;
				}
#else
				_client_socket = socket(AF_INET, SOCK_STREAM, 0);
				socket_address_type client_address;
				client_address.sin_family = AF_INET;
				client_address.sin_addr.s_addr = INADDR_ANY;
				client_address.sin_port = htons(_tcp_info._source_port);
				if (bind(_client_socket, (socket_address_type2*)&client_address, sizeof(client_address)) == -1)
				{
					std::cerr << "[TCP Client] Error when binding a socket to a specified address" << std::endl;
					close(_client_socket);

					return;
				}
				if (_client_socket == -1) {
					std::cerr << "[TCP Client] Error when creating a socket" << std::endl;
					close(_client_socket);

					return;
				}
#endif
				socket_address_type server_address;
				server_address.sin_family = AF_INET;
				server_address.sin_port = htons(_tcp_info._destination_port);
				inet_pton(AF_INET, _tcp_info._destination_ip, &server_address.sin_addr);
				int result = connect(_client_socket, (socket_address_type2*)&server_address, sizeof(server_address));

				if (result == -1)
				{
					std::cerr << "[TCP Client]: Error when connecting to the server" << std::endl;
					std::cout << "[TCP Client]: Trying again to connect to the server " << _tcp_info._destination_ip << std::endl;
#ifndef LINUX
					shutdown(_client_socket, SD_BOTH);  // SD_BOTH is generally more clear than '2'
					closesocket(_client_socket);        // Ensure socket handle is freed
					_client_socket = NULL;              // Prevent accidental reuse
					Sleep(1000);
#else
					close(_client_socket);
					_client_socket = -1;
					sleep(1000);
#endif

					return;
				}
				else
				{
					int recvTimeout = 5;  // 5 ms
					if (setsockopt(_client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR) {
						std::cerr << "[TCP Client]: setsockopt(SO_RCVTIMEO) failed: " << WSAGetLastError() << std::endl;
					}
					CreateIoCompletionPort((HANDLE)_client_socket, _iocp, (ULONG_PTR)&_client_socket, 0);

					socket_address_type actual_address;
#ifndef LINUX
					int addrlen = sizeof(actual_address);
#else
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
							std::cout << "[TCP Client]: Connected to the server " << std::endl;
							_connected = true;

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

			void	tcp_client_handler::read()  // 수신
			{
				if (_connected == false)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					return;
				}
				try
				{
					std::unique_lock<std::mutex> lock(_contexts_mutex);

					PER_IO_DATA* readData = new PER_IO_DATA{};
					readData->operationType = OP_READ;
					readData->buffer = new char[_buffer_size];
					readData->wsaBuf.buf = readData->buffer;
					readData->wsaBuf.len = _buffer_size;

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

					if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
						//std::cerr << "WSARecv failed: " << WSAGetLastError() << std::endl;
						delete[] readData->buffer;
						delete readData;
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Client] Read Error: " << e.what() << std::endl;
				}
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
					std::unique_lock<std::mutex> lock(_contexts_mutex);
					if (!_outbound_q.empty())
					{
						std::unique_lock<std::mutex> lock(_write_mutex);
						auto& front = _outbound_q.front();
						unsigned char* unsigned_buffer = front.first;
						int size = front.second;
						_outbound_q.pop_front();

						char* signed_buffer = new char[size];
						memset(signed_buffer, 0, size);
						memcpy(signed_buffer, unsigned_buffer, size);

						/*int ret = 0;
						{
							int ret = send(_client_socket, signed_buffer, size, 0);
						}*/

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


						if (ret == socket_error_type)
						{
#ifndef LINUX
							if (WSAGetLastError())
							{
								std::cout << "Disconnected" << std::endl;
								_is_running = false;
								closesocket(_client_socket);
							}
#else
							if (errno)
							{
								std::cout << "Disconnected" << std::endl;
								_is_running = false;
								close(_client_socket);
							}
#endif
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

						std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Client] Write Error: " << e.what() << std::endl;
				}
			}

			bool	tcp_client_handler::is_running()  // 서버에 연결된 상태인지 확인.
			{
				return _is_running;
			}
		}
	}
}