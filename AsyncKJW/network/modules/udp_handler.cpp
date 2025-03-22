#pragma once
#include <pch.h>

namespace II
{
	namespace network
	{
		namespace modules
		{
			//udp_handler* udp_handler::_this = nullptr;//셀프 포인터

			udp_handler::udp_handler() // 생성자
			{
				//_this = this;
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
				for (auto& t : workers)
				{
					if (t.joinable())
					{
						t.join();
					}
				}
			}

			void udp_handler::set_info(const session_info& info_) // 설정
			{
				try
				{
					_udp_info._id = info_._id;
					strcpy(_udp_info._source_ip, info_._udp_source_ip);
					_udp_info._source_port = info_._udp_source_port;

					/*strcpy(_udp_info._destination_ip, info_._destination_ip);
					_udp_info._destination_port = info_._destination_port;*/
					_udp_info._destinations = info_._udp_destinations;

					_udp_info._unicast_or_multicast = info_._type;
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Error: " << e.what() << std::endl;
				}
			}

			int udp_handler::set_nonblocking(int fd)
			{
#ifndef LINUX
#else
				int flags = fcntl(fd, F_GETFL, 0);
				if (flags == -1)
				{
					std::cout << "[UDP] setting non block returned -1" << std::endl;
					return -1;
				}
				return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
				return -1; //no need for window
			}

			void	udp_handler::on_read(unsigned char* buffer_, int size_) // 데이터 수신시 불려지는 함수.
			{
				try
				{
					std::unique_lock<std::mutex> lock(_read_mutex);
					_inbound_q.emplace_back(buffer_, size_);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Read Fail: " << e.what() << std::endl;
				}
			}

			void udp_handler::print_ip_as_hex(const char* ip)
			{
				std::printf("      IP Address in Hex: ");
				for (int i = 0; i < strlen(ip); ++i)
				{
					std::printf("%02x ", (unsigned char)ip[i]);
				}
				std::printf("\n");
			}

			std::string udp_handler::clean_ip(const std::string& ip)
			{
				std::string cleaned_ip = ip;
				cleaned_ip.erase(std::find_if(cleaned_ip.rbegin(), cleaned_ip.rend(), [](unsigned char ch)
					{
						return !std::isspace(ch) && ch != '\r' && ch != '\n';
					}).base(), cleaned_ip.end());
				return cleaned_ip;
			}

			bool udp_handler::start() // 시작
			{
				if (_is_running) return false;;
				std::string type = "";
				if (_udp_info._unicast_or_multicast == UDP_UNICAST) type = "UNICAST";
				else type = "MULTICAST";
				std::printf("     L [IFC]  UDP CONNECT : %s\n", _udp_info._name);
				std::printf("     L [IFC]    + SRC IP : %s\n", _udp_info._source_ip);
				std::printf("     L [IFC]    + SRC PORT : %d\n", _udp_info._source_port);
				for (auto& dest : _udp_info._destinations)
				{
					std::printf("     L [IFC]    + DST ID : %d\n", dest.first);
					std::printf("     L [IFC]    + DST IP : %s\n", dest.second.first.c_str());
					std::printf("     L [IFC]    + DST PORT : %d\n", dest.second.second);
				}
				std::printf("     L [IFC]    + TYPE : %s\n", &type[0]);
#ifndef LINUX
				_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
#else
#endif
				_udp_info._source_ip[15] = '\0'; // Ensure null-termination

				int i = 0;
				for (; i < strlen((char*)_udp_info._source_ip); i++)
				{
					if (_udp_info._source_ip[i] == ' ' || _udp_info._source_ip[i] == '\r' || _udp_info._source_ip[i] == '\n') {
						_udp_info._source_ip[i] = '\0';
						break;
					}
				}

				print_ip_as_hex(_udp_info._source_ip);  // Print the IP address as hex to check for hidden characters

				/*for (; i < strlen((char*)_udp_info._destination_ip); i++)
				{
					if (_udp_info._destination_ip[i] == ' ') break;
				}
				_udp_info._destination_ip[i] = '\0';*/

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
#ifndef LINUX
				BOOL bNewBehavior = FALSE;
				DWORD dwBytesReturned = 0;

				int result = WSAIoctl(
					_udp_socket,
					SIO_UDP_CONNRESET,
					&bNewBehavior,
					sizeof(bNewBehavior),
					NULL,
					0,
					&dwBytesReturned,
					NULL,
					NULL
				);

				if (result == SOCKET_ERROR) {
					std::cerr << "WSAIoctl failed with error: " << WSAGetLastError() << std::endl;
				}
#else
				int enable = 1;
				if (setsockopt(_udp_socket, SOL_IP, IP_RECVERR, &enable, sizeof(enable)) < 0) {
					std::cerr << "setsockopt failed with error: " << strerror(errno) << std::endl;
					close(_udp_socket);
					return false;
				}
#endif
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

				_source_address.sin_family = AF_INET; // IPv4 
				//_server_address.sin_addr.s_addr = inet_pton(AF_INET, _udp_info._source_ip, &_server_address.sin_addr);
				_source_address.sin_port = htons(_udp_info._source_port);

				if (inet_pton(AF_INET, _udp_info._source_ip, &_source_address.sin_addr) != 1)
				{
					std::cerr << "Invalid IP address format: " << _udp_info._source_ip << std::endl;
#ifndef LINUX
					closesocket(_udp_socket);
					WSACleanup();
#else
					close(_udp_socket);  // Close the socket in Linux
#endif
					return false;
				}

				for (auto& destination_info : _udp_info._destinations)
				{
					std::string cleaned_dst_ip = clean_ip(destination_info.second.first);
					/*	std::printf("     L [DEBUG] Initial DST IP: %s\n", destination_info.second.first.c_str());
						std::printf("     L [DEBUG] Cleaned DST IP: %s\n", cleaned_dst_ip.c_str());*/
					print_ip_as_hex(cleaned_dst_ip.c_str());

					socket_address_type destination_address;
					memset(&destination_address, 0, sizeof(destination_address));
					destination_address.sin_family = AF_INET;
					//destination_address.sin_addr.s_addr = inet_pton(AF_INET, cleaned_dst_ip.c_str(), &destination_address.sin_addr);
					destination_address.sin_port = htons(destination_info.second.second);
					if (inet_pton(AF_INET, cleaned_dst_ip.c_str(), &destination_address.sin_addr) != 1) {
						std::cerr << "Invalid destination IP address format" << std::endl;
#ifndef LINUX
						closesocket(_udp_socket);
						WSACleanup();
#else
						close(_udp_socket);  // Close the socket in Linux
#endif
						continue;
					}
					_destination_address_list.insert({ destination_info.first, destination_address });
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
					if (_udp_info._unicast_or_multicast == UDP_MULTICAST)
					{
						// 
						_mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
						_mreq.imr_interface.s_addr = htonl(INADDR_ANY);

						// Join the multicast group
						if (setsockopt(_udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&_mreq, sizeof(_mreq)) < 0)
						{
#ifndef LINUX
							std::cerr << "Error when setting the multicast group to join" << WSAGetLastError() << std::endl;
							closesocket(_udp_socket);
							WSACleanup();
#else
							std::cerr << "Error when setting the multicast group to join" << errno << std::endl;
							close(_udp_socket);  // Close the socket in Linux
#endif
							return false;
						}
						int time_live = 64;
						if (setsockopt(_udp_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&time_live, sizeof(time_live)) < 0)
						{
							std::cerr << "Failed to set MULTICAST TTL" << std::endl;
							return false;
						}
					}

					_is_running = true;

					std::cout << "UDP address bound. " << _udp_info._source_ip << ":" << std::to_string(_udp_info._source_port) << std::endl;
					for (auto& destination_info : _udp_info._destinations)
					{
						std::cout << "Destination IP:port is: " << destination_info.second.first.c_str() << ":" << std::to_string(destination_info.second.second) << std::endl;
					}


					if (_udp_socket == -1)
					{
						std::cout << "Failed to initialize UDP." << std::endl;
						return false;
					}
#ifndef LINUX
					int recvTimeout = 0; //ms
					if (setsockopt(_udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR) {
						std::cerr << "[UDP] setsockopt(SO_RCVTIMEO) failed: " << WSAGetLastError() << std::endl;
					}
#else
					struct timeval recvTimeout;
					recvTimeout.tv_sec = 0;
					recvTimeout.tv_usec = 0; //microsec
					if (setsockopt(_udp_socket, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout)) < 0) {
						std::cerr << "[UDP]: setsockopt(SO_RCVTIMEO) failed: " << strerror(errno) << std::endl;
					}
#endif

#ifndef LINUX
					CreateIoCompletionPort((HANDLE)_udp_socket, _iocp, (ULONG_PTR)&_udp_socket, 0);
#else

					epoll_fd = epoll_create1(0);
					if (epoll_fd == -1)
					{
						perror("epoll_create1");
						close(_udp_socket);
						return false;
					}

					// Add the listening socket to the epoll instance
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = _udp_socket; //이 fd를 통해 앞으로 epoll에 등록된 fd들을 조작함.
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _udp_socket, &ev) == -1) //epoll에 fd 등록
					{
						perror("epoll_ctl: _udp_socket");
						close(_udp_socket);
						close(epoll_fd);
						return false;
					}
#endif

#ifndef LINUX
					std::thread iocp_thread([this]() {
						try
						{
							//bool first = true;

					/*		const int MAX_COMPLETION_PORT_ENTRIES = 10;
							std::vector<OVERLAPPED_ENTRY> completionEntries(MAX_COMPLETION_PORT_ENTRIES);
							DWORD numEntriesRemoved = 0;*/

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

								/*BOOL result = GetQueuedCompletionStatusEx(
									_iocp,
									completionEntries.data(),
									static_cast<ULONG>(completionEntries.size()),
									&numEntriesRemoved,
									INFINITE,
									FALSE
								);*/
								/*for (DWORD i = 0; i < numEntriesRemoved; ++i)
								{*/
								//DWORD bytes_transferred = 0;
								if (!result)
								{
									int errCode = GetLastError();
									if (errCode == ERROR_NETNAME_DELETED || errCode == WSAECONNRESET || errCode == ERROR_INVALID_LEVEL) {
										std::cout << "Ignoring error: " << errCode << std::endl;
										delete ioData;
										ioData = nullptr;
									}
									else
									{
										wchar_t* s = nullptr;
										FormatMessageW(
											FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
											NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL
										);
										std::wcerr << L"GetQueuedCompletionStatus failed with error " << errCode << L": " << s << std::endl;
										if (ioData->operationType == OP_WRITE)
										{
											std::cout << "it is from write OP " << errCode << std::endl;
										}
										if (ioData->operationType == OP_READ)
										{
											std::cout << "it is from read OP " << errCode << std::endl;
											/*
											ERROR_PORT_UNREACHABLE
											1234 (0x4D2)
											No service is operating at the destination network endpoint on the remote system.
											*/
										}
										LocalFree(s);
										delete ioData;
										ioData = nullptr;
									}
								}

								/*bytes_transferred = completionEntries[i].dwNumberOfBytesTransferred;
								ULONG_PTR completionKey = completionEntries[i].lpCompletionKey;
								PER_IO_DATA* ioData = reinterpret_cast<PER_IO_DATA*>(completionEntries[i].lpOverlapped);*/

								if (ioData == nullptr)
								{
									std::cerr << "[UDP] Null overlapped structure!" << std::endl;
									break;  // Avoid dereferencing
								}
								if (ioData->operationType == OP_READ)
								{
									if (bytes_transferred > 0)
									{
										unsigned char* received_data_unsigned = new unsigned char[bytes_transferred];
										memset(received_data_unsigned, 0, bytes_transferred);
										std::memcpy(received_data_unsigned, ioData->buffer, bytes_transferred);
										this->on_read(std::move(received_data_unsigned), bytes_transferred);
										delete[] ioData->buffer;
										ioData->buffer = nullptr;
									}
									delete ioData;
									ioData = nullptr;
									this->read();
								}
								else if (ioData->operationType == OP_WRITE)
								{
									delete ioData;
									ioData = nullptr;
								}
								else
								{
									std::cout << "undefined operation" << std::endl;
								}
								//}
							}
						}
						catch (const std::exception& e)
						{
							std::cerr << "[UDP] IOCP Error: " << e.what() << std::endl;
						}
						});

					workers.emplace_back(std::move(iocp_thread));
#else
					std::thread epoll_thread([this]() {
						while (true)
						{
							int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); //-1이면 waiting time inifinite
							if (nfds == -1)
							{
								std::cout << "[UDP] epoll_wait error" << std::endl;
								perror("epoll_wait");
								close(_udp_socket);
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

								std::cout << "[UDP] Received Data Size: " << front.second << std::endl;
#ifndef LINUX	
								if (_receive_callback != NULL) _receive_callback(_udp_info._id, front.first, front.second);
#else
								if (_receive_callback != nullptr) _receive_callback(_udp_info._id, front.first, front.second);
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

					this->read();
				}
				return true;
			}

			bool udp_handler::stop() // 정지	
			{
				bool result = false;

				try
				{
					_is_running = false;
					std::cout << "[UDP] Stopping..." << std::endl;
					if (_udp_socket == NULL) return true;
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
					if (_udp_socket != NULL) shutdown(_udp_socket, SD_BOTH);
					if (_udp_socket != NULL) closesocket(_udp_socket);
#else
					for (auto& t : workers)
					{
						if (t.joinable()) {
							t.join();
						}
					}
					if (_udp_socket >= 0) close(_udp_socket);  // Close the socket in Linux
#endif
					std::cout << "[UDP] Stopped " << _udp_info._source_ip << ":" << std::to_string(_udp_info._source_port) << std::endl;
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Error: " << e.what() << std::endl;
					result = false;
				}

				return true;
			}

			void udp_handler::send_message(short destination_id_, unsigned char* buffer_, int size_) // 데이터 송신
			{
				try
				{
					std::unique_lock<std::mutex> lock(_write_mutex);

					////1024 바이트 이상일 경우 나눠서 송신 
					//int num_chunks = size_ / 1024;
					//int remaining_bytes = size_ % 1024;
					//for (int i = 0; i < num_chunks; ++i)
					//{
					//	unsigned char* chunk = new unsigned char[1024];
					//	memset(chunk, 0, 1024);
					//	memcpy(chunk, buffer_ + (i * 1024), 1024);
					//	_outbound_q.emplace_back(std::move(chunk), 1024);
					//}

					//if (remaining_bytes > 0)
					//{
					//	unsigned char* last_chunk = new unsigned char[remaining_bytes];
					//	memset(last_chunk, 0, remaining_bytes);
					//	memcpy(last_chunk, buffer_ + (num_chunks * 1024), remaining_bytes);
					//	_outbound_q.emplace_back(std::move(last_chunk), remaining_bytes);
					//}

					unsigned char* last_chunk = new unsigned char[size_];
					memset(last_chunk, 0, size_);
					memcpy(last_chunk, buffer_, size_);
					_outbound_q.emplace_back(destination_id_, std::make_pair(std::move(last_chunk), size_));
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Error: " << e.what() << std::endl;
				}
			}

			void udp_handler::read() // 수신
			{
				try
				{
					std::shared_lock<_sharedmutex> lock(_contexts_mutex);

#ifndef LINUX
					PER_IO_DATA* readData = new PER_IO_DATA{};
					readData->operationType = OP_READ;
					readData->buffer = new char[BUFFER_SIZE];
					readData->wsaBuf.buf = readData->buffer;
					readData->wsaBuf.len = BUFFER_SIZE;
					//int addr_len = sizeof(_destination_address);
					/*for (auto& destination_address : _destination_address_list)
					{*/
					//	socket_address_type dest_addr_copy = destination_address.second;
					////	std::cout << "asdf " << destination_address .first << std::endl;
					//	int addrLen = sizeof(destination_address.second);
					DWORD flags = 0;
					//	int ret = WSARecvFrom(
					//		_udp_socket,
					//		&readData->wsaBuf,
					//		1,
					//		NULL,                   // No immediate byte count
					//		&flags,
					//		(socket_address_type2*)&dest_addr_copy,
					//		&addrLen,
					//		&readData->overlapped,
					//		NULL                    // No completion routine (IOCP handles it)
					//	);
					int ret = WSARecv( //If you do not need the source address information, you can use WSARecv with a UDP socket.
						_udp_socket,
						&readData->wsaBuf,
						1,
						NULL,                   // No immediate byte count
						&flags,
						&readData->overlapped,
						NULL                    // No completion routine (IOCP handles it)
					);

					if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
					{
						//	std::cerr << "WSARecv failed: " << WSAGetLastError() << std::endl;
						delete[] readData->buffer;
						delete readData;
					}

					//}
#else
					char buffer[BUFFER_SIZE];
					ssize_t bytes_transferred = recv(_udp_socket, buffer, sizeof(buffer), 0);

					if (bytes_transferred > 0)
					{
						unsigned char* received_data_unsigned = new unsigned char[bytes_transferred];
						memset(received_data_unsigned, 0, sizeof(received_data_unsigned));
						std::memcpy(received_data_unsigned, buffer, bytes_transferred);
						this->on_read(std::move(received_data_unsigned), bytes_transferred);
						//	delete[] buffer;
					}
#endif
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Read Error: " << e.what() << std::endl;
				}
			}

			void udp_handler::write() // 송신
			{
				try
				{
					std::shared_lock<_sharedmutex> lock(_contexts_mutex);
					unsigned char* unsigned_buffer = nullptr;
					int size = 0;
					if (!_outbound_q.empty())
					{
						std::unique_lock<std::mutex> lock(_write_mutex);
						auto& front = _outbound_q.front();
						unsigned char* unsigned_buffer = front.second.first;
						int size = front.second.second;
						_outbound_q.pop_front(); // does not free the memory pointed to, but it only removes the entry in the queue.
						for (auto& destination_address : _destination_address_list)
						{
							//-1 == send to all
							if (front.first != -1 && (front.first != destination_address.first))
							{
								/*	std::cout << "request ID: " << front.first << std::endl;
									std::cout << "available ID: " << destination_address.first << std::endl;*/
								continue;
							}
							if (unsigned_buffer == nullptr)
							{
								std::cout << "Data is empty... " << front.first << std::endl;
								continue;
							}
							if (size > 0)
							{
								char* signed_buffer = new char[size];
								memset(signed_buffer, 0, size);
								memcpy(signed_buffer, unsigned_buffer, size);
								socket_address_type dest_addr_copy = destination_address.second;
#ifndef LINUX
								PER_IO_DATA* writeData = new PER_IO_DATA{};

								writeData->operationType = OP_WRITE;
								writeData->buffer = signed_buffer;
								writeData->wsaBuf.buf = writeData->buffer;
								writeData->wsaBuf.len = static_cast<ULONG>(size);
								writeData->bytesSent = 0;
								writeData->totalSize = static_cast<DWORD>(size);
								int addrLen = sizeof(destination_address.second);
								fd_set writeSet;
								FD_ZERO(&writeSet);
								FD_SET(_udp_socket, &writeSet);

								timeval timeout;
								timeout.tv_sec = 0;
								timeout.tv_usec = 10000;

								int selectResult = select(0, NULL, &writeSet, NULL, &timeout);
								if (selectResult > 0 && FD_ISSET(_udp_socket, &writeSet))
								{
									DWORD bytesSent = 0;
									DWORD flags = 0;
									int ret = WSASendTo(
										_udp_socket,
										&writeData->wsaBuf,
										1,
										&bytesSent,          // This will be 0 for overlapped I/O
										flags,
										(socket_address_type2*)&dest_addr_copy,
										addrLen,
										&writeData->overlapped,
										NULL                 // No completion routine, as IOCP handles completion
									);

									if (ret == SOCKET_ERROR)
									{
										int errCode = WSAGetLastError();
										if (errCode != WSA_IO_PENDING)
										{
											std::wcerr << L"WSASendTo failed with error " << errCode << std::endl;
										}
									}

									//std::cout << "Sent " << bytesSent << " bytes to "
									//	<< _udp_info._destination_ip << ":"
									//	<< _udp_info._destination_port << std::endl;

								}
#else
								ssize_t bytesSent = sendto(
									_udp_socket,
									signed_buffer,
									size,
									0,
									(struct sockaddr*)&dest_addr_copy,
									sizeof(destination_address)
								);

								if (bytesSent < 0) {
									std::cerr << "sendto failed with error: " << strerror(errno) << std::endl;
								}
								/*else {
									std::cout << "Sent " << bytesSent << " bytes to "
										<< _udp_info._destination_ip << ":"
										<< _udp_info._destination_port << std::endl;
								}*/

								/*ev.events = EPOLLOUT | EPOLLET;
								ev.data.fd = _udp_socket;
								if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, _udp_socket, &ev) == -1)
								{
									perror("epoll_ctl: mod _udp_socket");
									close(_udp_socket);
								}*/
#endif

								delete[] signed_buffer;
								signed_buffer = nullptr;

							}
						}
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
						//std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[UDP] Write Error: " << e.what() << std::endl;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			bool udp_handler::is_running()
			{
				return _is_running;
			}
		}
	}
}