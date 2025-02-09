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
			tcp_server_handler::tcp_server_handler() //: _pool(4) // 생성자
			{
				_this = this;

				SYSTEM_INFO sysInfo;
				GetSystemInfo(&sysInfo);
				numThreads = sysInfo.dwNumberOfProcessors * 2;
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


			int tcp_server_handler::setNonBlocking(int fd) {
#ifndef LINUX
#else
				int flags = fcntl(fd, F_GETFL, 0);
				return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
				return -1;
			}

			void	tcp_server_handler::on_read(unsigned char* buffer_, int size_) // 데이터 수신시 불려지는 함수.
			{
				try
				{
					std::unique_lock<std::mutex> lock(_read_mutex);

					//unsigned char* message_copy = new unsigned char[size_]; 
					//memset(message_copy, 0, size_);
					//memcpy(message_copy, buffer_, size_);
					//std::cout << "asdf" << buffer_ << std::endl;
					_inbound_q.emplace_back(buffer_, size_);
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

				if (listen(_server_socket, SOMAXCONN) == -1)
				{
					std::cerr << "Error when listening for incoming connections" << std::endl;
					return false;
				}

				std::cout << "TCP server started. Listening for incoming connections on " << _tcp_info._source_ip << ":"
					<< std::to_string(_tcp_info._source_port) << std::endl;

#ifndef LINUX
#else
				setNonBlocking(_server_socket);
#endif
#ifndef LINUX
				_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
				CreateIoCompletionPort((HANDLE)_server_socket, _iocp, (ULONG_PTR)_server_socket, 0); //4 넣어도 됨
#else
#endif

				_is_running = true;
				//_first_time = true;
				std::thread accept_thread([this]() {
					while (true)
					{
						this->accept_connection();

						//if (_connected && _first_time)
						//{
						//	_first_time = false;

						//	//this->read();
						//	//this->write();
						//}
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}
					});

				workers.emplace_back(std::move(accept_thread));

				std::thread iocp_thread([this]() {
					//HANDLE iocp = (HANDLE)lpParam;
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

						//클라가 접속을 끊었을때
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
							std::cerr << "[TCP Server] Null overlapped structure!" << std::endl;
							continue;  // Avoid dereferencing
						}
						// 클라이언트 등록
						if (ioData->operationType == OP_ACCEPT)
						{
							//							std::cout << "[1.." << std::endl;
							//								SOCKET client_socket = static_cast<SOCKET>(completionKey);
							//#ifndef LINUX    
							//#else
							//								setNonBlocking(client_socket);
							//#endif
							//								client_context oclient;
							//								oclient._id = _num_users_entered;
							//								oclient._socket = client_socket;
							//
							//								if (completionKey == NULL && ioData == NULL) {
							//									std::cerr << "Critical IOCP error: " << WSAGetLastError() << std::endl;
							//									break;
							//								}
							//								_client_socket.insert(std::make_pair(_num_users_entered, std::move(oclient)));
							//
							//								socket_address_type client_addr;
							//#ifndef LINUX
							//								int client_addr_len = sizeof(client_addr);
							//#else
							//								socklen_t client_addr_len = sizeof(client_addr);
							//#endif
							//								getpeername(client_socket, (socket_address_type2*)&client_addr, &client_addr_len);
							//								int port = ntohs(client_addr.sin_port);
							//								std::string address = inet_ntoa(client_addr.sin_addr);
							//
							//								_num_users_entered++;
							//								_connected = true;
							//
							//								std::cout << " [IFC]  TCP Server : Accepted a Client IP: " << address
							//									<< " Port: " << std::to_string(port) << std::endl;
							//							/*	int socketState;
							//								int socketStateLen = sizeof(socketState);*/
							//						/*		if (getsockopt(client_socket, SOL_SOCKET, SO_CONNECT_TIME, (char*)&socketState, &socketStateLen) == SOCKET_ERROR) {
							//									std::cerr << "getsockopt failed: " << WSAGetLastError() << std::endl;
							//									closesocket(client_socket);
							//									return;
							//								}*/
							//								// Update the accept context
							//								if (setsockopt(client_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
							//									(char*)&_server_socket, sizeof(_server_socket)) == SOCKET_ERROR) {
							//									std::cerr << "setsockopt failed: " << WSAGetLastError() << std::endl;
							//									closesocket(client_socket);
							//									return;
							//								}
							//								// Associate the socket with the IOCP
							//								if (CreateIoCompletionPort((HANDLE)client_socket, _iocp, (ULONG_PTR)client_socket, 0) == NULL)
							//								{
							//									std::cerr << "Error when accepting an incoming connection" << std::endl;
							//								}
							//
							//								delete ioData;
							//

							//

							//							this->accept_connection();
						}
						//Overlapped I/O Recv 
						else if (ioData->operationType == OP_READ)
						{

							std::unique_lock<std::mutex> client_lock(_contexts_mutex);
							/*	std::cout << "[read IOCP Called.." << std::endl;*/  //OK

							SOCKET client_socket = static_cast<SOCKET>(completionKey);

							/*std::cout << "[READ] Received " << bytes_transferred << " bytes from client "
								<< std::string(ioData->buffer, bytes_transferred) << std::endl;*/

							if (bytes_transferred > 0)
							{
								unsigned char* received_data_unsigned = new unsigned char[bytes_transferred];
								memset(received_data_unsigned, 0, sizeof(received_data_unsigned));
								std::memcpy(received_data_unsigned, ioData->buffer, bytes_transferred);
								this->on_read(std::move(received_data_unsigned), bytes_transferred);
								delete[] ioData->buffer;
								ioData->buffer = nullptr;
							}

							delete ioData;
							//this->read();
						}
						//Overlapped I/O Send
						else if (ioData->operationType == OP_WRITE)
						{
							//std::cout << "[TCP Server] write IOCP Called.." << std::endl; //OK

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
							if (_print_to_console) std::cout << "[TCP Server] Received: " << alphanumeric_text << " Size: " << size << std::endl;
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
				return true;
			}

			bool	tcp_server_handler::stop() // 정지	
			{
				bool result = false;
				//_pool.push([this, &result]() { //pool takes care of the lock
				try
				{
					std::unique_lock<std::mutex> lock(_contexts_mutex);
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
				//	});
				return result;
			}

			void	tcp_server_handler::send_message(short destination_id_, unsigned char* buffer_, int size_) // 데이터 송신
			{
				//_pool.push([this, size_, buffer_]() { //pool takes care of the lock
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
				//	});
			}

			bool	tcp_server_handler::accept_connection()
			{
				try
				{
#ifndef LINUX
					socket_type client_socket = accept(_server_socket, NULL, NULL); //blocking call
#else
					socket_type client_socket = accept(_server_socket, nullptr, nullptr);
#endif
					if (client_socket == -1)
					{
						std::cerr << "[TCP Server]: Error when accepting an incoming connection" << std::endl;
					}
					else //if accepted
					{
						//std::unique_lock<std::mutex> lock(_contexts_mutex);
						client_context oclient;
						oclient._id = _num_users_entered;
						oclient._socket = client_socket;
						int recvTimeout = 5;  // 5 ms
						if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR) {
							std::cerr << "[TCP Server]: setsockopt(SO_RCVTIMEO) failed: " << WSAGetLastError() << std::endl;
						}
						CreateIoCompletionPort((HANDLE)oclient._socket, _iocp, (ULONG_PTR)&oclient._socket, 0); // does not require locking
						_newly_added_client_socket.insert(std::make_pair(_num_users_entered, std::move(oclient)));

						socket_address_type client_addr;
#ifndef LINUX
						int client_addr_len = sizeof(client_addr);
#else
						socklen_t client_addr_len = sizeof(client_addr);
#endif
						getpeername(client_socket, (socket_address_type2*)&client_addr, &client_addr_len);
				
						int port = ntohs(client_addr.sin_port);
						std::string address = inet_ntoa(client_addr.sin_addr);

						std::cout << " [IFC]  TCP Server : Accepted a Client IP: " << address
							<< " Port: " << std::to_string(port) << std::endl;

						_num_users_entered++;
						_connected = true;
					}
					//std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				catch (std::error_code& error_)
				{
					std::cerr << "[TCP Server] Error when binding acceptor: " << error_.message() << std::endl;
				}
				return true;
			}

#pragma region AcceptEx
			//void	tcp_server_handler::accept_connection() // 클라이언트 연결 승인
			//{
			//	try
			//	{
			//		LPFN_ACCEPTEX lpfnAcceptEx = NULL;
			//		GUID guidAcceptEx = WSAID_ACCEPTEX;
			//		DWORD bytesReturned = 0;

			//		int result = WSAIoctl(
			//			_server_socket,                    // The listening socket
			//			SIO_GET_EXTENSION_FUNCTION_POINTER, // Request for extension function pointer
			//			&guidAcceptEx,                     // GUID identifying AcceptEx
			//			sizeof(guidAcceptEx),
			//			&lpfnAcceptEx,                     // Pointer to the function pointer
			//			sizeof(lpfnAcceptEx),
			//			&bytesReturned,
			//			NULL,
			//			NULL
			//		);

			//		if (result == SOCKET_ERROR) {
			//			std::cerr << "WSAIoctl failed to load AcceptEx: " << WSAGetLastError() << std::endl;
			//			closesocket(_server_socket);
			//			WSACleanup();
			//			return;
			//		}

			//		//	std::unique_lock<std::mutex> lock(_contexts_mutex);
			//		SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			//		if (client_socket == INVALID_SOCKET) {
			//			std::cerr << "WSASocket failed: " << WSAGetLastError() << std::endl;	
			//			closesocket(_server_socket);
			//			WSACleanup();
			//			return;
			//		}

			//		PER_IO_DATA* acceptData = new PER_IO_DATA{};
			//		acceptData->operationType = OP_ACCEPT;
			//		acceptData->buffer = new char[(sizeof(SOCKADDR_IN) + 16) * 2];
			//		memset(&acceptData->overlapped, 0, sizeof(acceptData->overlapped));

			//		BOOL accepted = lpfnAcceptEx(_server_socket, client_socket, acceptData->buffer,
			//			1024 - ((sizeof(sockaddr_in) + 16) * 2),
			//			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			//			&bytesReturned, &acceptData->overlapped);

			//		if (accepted == FALSE && WSAGetLastError() != ERROR_IO_PENDING) {
			//			std::cerr << "AcceptEx failed: " << WSAGetLastError() << std::endl;
			//			closesocket(_server_socket);
			//			closesocket(client_socket);
			//			delete[] acceptData->buffer;
			//			delete acceptData;
			//			return;
			//		}


			//	}
			//	catch (std::error_code& error_)
			//	{
			//		std::cerr << "[TCP Server] Error when binding acceptor: " << error_.message() << std::endl;
			//	}
			//}
#pragma endregion
			void	tcp_server_handler::read() // 수신 //비동기일때 Context는 한개다.
			{
				try
				{
					std::unique_lock<std::mutex> lock(_contexts_mutex);
					std::map<int, client_context>::iterator it = _client_socket.begin();
					while (it != _client_socket.end())
					{

						PER_IO_DATA* readData = new PER_IO_DATA{};
						readData->operationType = OP_READ;
						readData->buffer = new char[_buffer_size];
						readData->wsaBuf.buf = readData->buffer;
						readData->wsaBuf.len = _buffer_size;

						//bytes_transferred = recv(it->second._socket, received_data, _buffer_size, 0); //blocking call
						DWORD flags = 0;
						int ret = WSARecv(
							it->second._socket,
							&readData->wsaBuf,
							1,
							NULL,                   // No immediate byte count
							&flags,
							&readData->overlapped,
							NULL                    // No completion routine (IOCP handles it)
						);
						//5 milisec timeout

						if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
							//std::cerr << "WSARecv failed: " << WSAGetLastError() << std::endl;
							delete[] readData->buffer;
							delete readData;
						}
						++it;
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[TCP Server] Read Error: " << e.what() << std::endl;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}

			void	tcp_server_handler::write() // 송신
			{
				try
				{
					std::unique_lock<std::mutex> client_lock(_contexts_mutex);
					if (!_newly_added_client_socket.empty()) {
						_client_socket.insert(
							std::make_move_iterator(_newly_added_client_socket.begin()),
							std::make_move_iterator(_newly_added_client_socket.end())
						);
						_newly_added_client_socket.clear();


						std::map<int, client_context>::iterator it = _client_socket.begin();
						std::cout << "[TCP Server] Total clients: " << _client_socket.size();
						while (it != _client_socket.end())
						{
							socket_address_type client_addr;
							int client_addr_len = sizeof(client_addr);
							getpeername(it->second._socket, (socket_address_type2*)&client_addr, &client_addr_len);

							int port = ntohs(client_addr.sin_port);
							std::string address = inet_ntoa(client_addr.sin_addr);

							std::cout << " Client ID: " << it->first << " Client IP: " << address
								<< " Port: " << std::to_string(port) << std::endl;
							it++;
						}

					}


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

							PER_IO_DATA* writeData = new PER_IO_DATA{};

							writeData->operationType = OP_WRITE;
							writeData->buffer = signed_buffer;
							writeData->wsaBuf.buf = writeData->buffer;
							writeData->wsaBuf.len = static_cast<ULONG>(size);
							writeData->bytesSent = 0;
							writeData->totalSize = static_cast<DWORD>(size);
							DWORD bytesSent = 0;
							DWORD flags = 0;

							//--------------------
							socket_address_type client_addr;
							int client_addr_len = sizeof(client_addr);
							getpeername(it->second._socket, (socket_address_type2*)&client_addr, &client_addr_len);

							int port = ntohs(client_addr.sin_port);
							std::string address = inet_ntoa(client_addr.sin_addr);

							std::cout << " Writing to-> Client ID: " << it->first << " Client IP: " << address
								<< " Port: " << std::to_string(port) << std::endl;

						/*	u_long nonBlocking = 0;
							int isvalid = ioctlsocket(it->second._socket, FIONBIO, &nonBlocking);
							if (isvalid == SOCKET_ERROR) {
								std::cerr << "Socket is not valid: " << WSAGetLastError() << std::endl;
								return;
							}
							else
							{
								std::cout << " Valid " << std::endl;
							}*/
							//-----------------

							int ret = WSASend(
								it->second._socket,
								&writeData->wsaBuf,
								1,
								&bytesSent,          // This will be 0 for overlapped I/O
								flags,
								&writeData->overlapped,
								NULL                 // No completion routine, as IOCP handles completion
							);
							//if (signed_buffer != nullptr && size > 0) ret = WSASend(it->second._socket, signed_buffer, size, 0); //blocking call


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
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
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