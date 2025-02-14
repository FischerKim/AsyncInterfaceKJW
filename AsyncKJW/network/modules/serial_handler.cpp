#pragma once
#include <pch.h>
namespace II
{
	namespace network
	{
		namespace modules
		{
			//serial_handler* serial_handler::_this = nullptr;  //셀프 포인터

			serial_handler::serial_handler() //: _pool(2) // 생성자
			{
				//_this = this;
				_serial_handle = NULL;
			}

			serial_handler::~serial_handler() // 파괴자
			{
				try
				{
					_is_running = false;

					for (auto& t : workers)
					{
						if (t.joinable()) {
							t.join();
						}
					}
#ifndef LINUX
					if (_serial_handle && _serial_handle != INVALID_HANDLE_VALUE)
					{
						if (!CloseHandle(_serial_handle))
						{
						}
						_serial_handle = NULL;
						WSACleanup();
					}
#else
					if (_serial_handle != -1)
					{
						close(_serial_handle);
						_serial_handle = -1;
					}
#endif
				}
				catch (const std::exception& e)
				{
					std::cerr << "[Serial] Error: " << e.what() << std::endl;
				}
			}

			void serial_handler::set_info(const session_info& info_) //설정
			{
				try
				{
					_serial_info._id = info_._id;
					strcpy(_serial_info._name, info_._serial_port);

					_serial_info._baud_rate = info_._baud_rate;
					_serial_info._data_width = info_._data_width;

					std::string temp = std::string(info_._stop_bits);
					for (int i = 0; temp[i] != '\0'; i++)
					{
						temp[i] = tolower(temp[i]);
					}
					strcpy(_serial_info._stop_bits, temp.c_str());

					temp = std::string(info_._parity_mode);
					for (int i = 0; temp[i] != '\0'; i++)
					{
						temp[i] = tolower(temp[i]);
					}
					strcpy(_serial_info._parity_mode, temp.c_str());
				}
				catch (const std::exception& e)
				{
					std::cerr << "[Serial] Error: " << e.what() << std::endl;
				}
			}


			bool serial_handler::is_open()  // 시리얼 포트 열려있는지 확인.
			{
#ifndef LINUX
				if (_serial_handle == NULL || _serial_handle == INVALID_HANDLE_VALUE) return false;
#else
				if (_serial_handle == -1) return false;
#endif
				return true;
			}

			bool serial_handler::start() // 시작
			{
				if (_is_running) return false;

				//가상으로 셋업할때 com0com 프로그램 사용하기
				std::printf("     L [IFC]  SERIAL : \n");
				std::printf("     L [IFC]    + PORT NO	: %s\n", _serial_info._name); //윈도우면 COM3, COM4 ... 리눅스면 /dev/ttyS0 등
				std::printf("     L [IFC]    + BAUD RATE	: %d\n", _serial_info._baud_rate);
				std::printf("     L [IFC]    + DATA WID	: %d\n", _serial_info._data_width);
				std::printf("     L [IFC]    + STOP BITS : %s\n", _serial_info._stop_bits);
				std::printf("     L [IFC]    + PARITY	: %s\n", _serial_info._parity_mode);
#ifndef LINUX
				int i = 0;
				for (; i < strlen((char*)_serial_info._name); i++)
				{
					if (_serial_info._name[i] == ' ') break;
				}
				_serial_info._name[i] = '\0';
				std::string serial_name = "\\\\.\\" + std::string(_serial_info._name, i + 1);
				std::wstring wide_serial_name = std::wstring(serial_name.begin(), serial_name.end());

				_serial_handle = CreateFile(wide_serial_name.c_str(),  // wide string
					GENERIC_READ | GENERIC_WRITE,  // desired access rights
					FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allows other processes to read/write
					NULL,  // security attributes
					OPEN_EXISTING,  // creation disposition
					0,  // file attributes
					NULL  // template file
				);
				if (_serial_handle == INVALID_HANDLE_VALUE)
				{
					std::cout << "Handle " << serial_name << " Failed to Initialized" << std::endl;
					return false;
				}

				SetCommMask(_serial_handle, EV_BREAK | EV_ERR | EV_RXCHAR | EV_TXEMPTY);
				SetupComm(_serial_handle, 4096, 4096); //InQueue, OutQueue 크기 설정
				PurgeComm(_serial_handle, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);
#else
				if (_serial_handle != -1)
				{
					std::cerr << "Serial port already initialized" << std::endl;
					return false;
				}
				// Construct the full serial port name (e.g., /dev/ttyS0)
				std::string serial_name = "/dev/" + std::string(_serial_info._name);

				// Open the serial port
				_serial_handle = open(serial_name.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
				if (_serial_handle == -1)
				{
					std::cerr << "Error opening serial port " << serial_name << std::endl;
					return false;
				}

				struct termios options;

				// Get the current options for the port
				tcgetattr(_serial_handle, &options);

				// Set the baud rates to 9600
				cfsetispeed(&options, B9600);
				cfsetospeed(&options, B9600);

				// Enable the receiver and set local mode
				options.c_cflag |= (CLOCAL | CREAD);

				// Set the new options for the port
				tcsetattr(_serial_handle, TCSANOW, &options);
#endif

#ifndef LINUX
				DCB serialParams = { 0 };
				serialParams.DCBlength = sizeof(serialParams);

				GetCommState(_serial_handle, &serialParams);
				serialParams.BaudRate = _serial_info._baud_rate;
				serialParams.ByteSize = _serial_info._data_width;

				if (std::string(_serial_info._stop_bits) == "one") serialParams.StopBits = 0;
				else if (std::string(_serial_info._stop_bits) == "onepointfive") serialParams.StopBits = 1;
				else if (std::string(_serial_info._stop_bits) == "two") serialParams.StopBits = 2;

				if (std::string(_serial_info._parity_mode) == "none") serialParams.Parity = 0;
				else if (std::string(_serial_info._parity_mode) == "odd") serialParams.Parity = 1;
				else if (std::string(_serial_info._parity_mode) == "even") serialParams.Parity = 2;

				SetCommState(_serial_handle, &serialParams);

				std::cout << "1" << std::endl;
				COMMTIMEOUTS timeout = { 0 };
				timeout.ReadIntervalTimeout = 50;
				timeout.ReadTotalTimeoutConstant = 50;
				timeout.ReadTotalTimeoutMultiplier = 50;
				timeout.WriteTotalTimeoutConstant = 50;
				timeout.WriteTotalTimeoutMultiplier = 10;

				SetCommTimeouts(_serial_handle, &timeout);
#else
				struct termios serialParams;
				memset(&serialParams, 0, sizeof(serialParams));

				// Get the current serial port configuration
				if (tcgetattr(_serial_handle, &serialParams) != 0)
				{
					std::cerr << "Error getting serial port attributes: " << strerror(errno) << std::endl;
					close(_serial_handle);
					_serial_handle = -1;
					return false;
				}

				// Set the baud rate
				cfsetispeed(&serialParams, _serial_info._baud_rate);
				cfsetospeed(&serialParams, _serial_info._baud_rate);

				// Set data width (8 data bits)
				serialParams.c_cflag &= ~CSIZE;
				serialParams.c_cflag |= _serial_info._data_width == 8 ? CS8 : CS7;

				// Set stop bits
				if (std::string(_serial_info._stop_bits) == "one")
				{
					serialParams.c_cflag &= ~CSTOPB;
				}
				else if (std::string(_serial_info._stop_bits) == "two")
				{
					serialParams.c_cflag |= CSTOPB;
				}

				// Set parity mode
				if (std::string(_serial_info._parity_mode) == "none")
				{
					serialParams.c_cflag &= ~PARENB;  // Disable parity
				}
				else if (std::string(_serial_info._parity_mode) == "odd")
				{
					serialParams.c_cflag |= PARENB;   // Enable parity
					serialParams.c_cflag |= PARODD;   // Odd parity
				}
				else if (std::string(_serial_info._parity_mode) == "even")
				{
					serialParams.c_cflag |= PARENB;   // Enable parity
					serialParams.c_cflag &= ~PARODD;  // Even parity
				}

				// Set the port to raw mode (disable canonical mode, disable echo)
				serialParams.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
				serialParams.c_iflag &= ~(IXON | IXOFF | IXANY);
				serialParams.c_oflag &= ~OPOST;

				// Set timeouts
				serialParams.c_cc[VTIME] = 5;  // Timeout for reading: 0.5 seconds
				serialParams.c_cc[VMIN] = 0;   // Minimum number of characters to read

				// Apply the serial port settings
				if (tcsetattr(_serial_handle, TCSANOW, &serialParams) != 0)
				{
					std::cerr << "Error setting serial port attributes: " << strerror(errno) << std::endl;
					close(_serial_handle);
					_serial_handle = -1;
					return false;
				}
#endif
				_is_running = true;
				std::cout << "Serial Comm Started" << std::endl;



				std::thread read_thread([this]() {
					while (true)
					{
						read_serial();
					}
					});

				workers.emplace_back(std::move(read_thread));

				std::thread write_thread([this]() {
					while (true)
					{
						write_serial();
					}
					});

				workers.emplace_back(std::move(write_thread));

				return true;
			}

			bool serial_handler::stop() // 정지
			{
				bool result = false;

				for (auto& t : workers)
				{
					if (t.joinable()) {
						t.join();
					}
				}

				try
				{
					_is_running = false;

					std::cout << "[Serial] Stopping..." << std::endl;
					std::cout << "[Serial] Stopped" << std::endl;
					if (_serial_handle == NULL) return true;
				}
				catch (const std::exception& e)
				{
					std::cerr << "[Serial] Error: " << e.what() << std::endl;
					result = false;
				}
				result = true;
				return result;
			}

			void serial_handler::send_message(short interface_id_, unsigned char* buffer_, int size_) // 데이터 송신
			{
				try
				{
					std::unique_lock<std::mutex> lock(_write_mutex);

					unsigned char* data_to_send = new unsigned char[size_];
					memset(data_to_send, 0, size_);
					memcpy(data_to_send, buffer_, size_);
					_outbound_q.emplace_back(std::move(data_to_send), size_);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[Serial] Error: " << e.what() << std::endl;
				}
			}

			void serial_handler::read_serial() // 수신  1byte 씩 받기때문에 받은 메세지에 대한 II 로그는 안찍음.
			{ // 함수명을 read()라고 하면 name confliction남. 
				while (_is_running)
				{
					try
					{
						if (!_is_running) return;
						unsigned char _inbound_packet[1] = {};
#ifndef LINUX
						DWORD bytes_transferred;
						memset(_inbound_packet, 0, sizeof(_inbound_packet));
						bool is_read = ReadFile(_serial_handle, _inbound_packet, sizeof(_inbound_packet), &bytes_transferred, NULL);
#else
						memset(_inbound_packet, 0, sizeof(_inbound_packet));
						ssize_t bytes_transferred = read(_serial_handle, _inbound_packet, sizeof(_inbound_packet));
#endif

#ifndef LINUX
						if (is_read && bytes_transferred > 0)
						{
							if (_receive_callback != NULL) _receive_callback(_serial_info._id, std::move(_inbound_packet), sizeof(_inbound_packet));
						}
#else
						if (bytes_transferred > 0)
						{
							if (_receive_callback != nullptr) _receive_callback(_serial_info._id, std::move(_inbound_packet), sizeof(_inbound_packet));

						}
#endif	
					}
					catch (const std::exception& e)
					{
						std::cerr << "[UDP] Error: " << e.what() << std::endl;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
				}

			void serial_handler::write_serial() // 송신
			{
				while (_is_running)
				{
					try
					{
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
#ifndef LINUX
							DWORD bytes_transferred;
							BOOL write_result = WriteFile(_serial_handle, std::move(signed_buffer), size, &bytes_transferred, NULL);

							if (write_result && bytes_transferred == size)
							{
								if (!FlushFileBuffers(_serial_handle))
								{
									std::cerr << "FlushFileBuffers failed: " << GetLastError() << std::endl;
								}
							}
							else
							{
								std::cerr << "WriteFile failed: " << GetLastError() << std::endl;
							}

							//if (_CrtIsValidHeapPointer(unsigned_buffer)) 
							//{
							//	delete[] unsigned_buffer;
							//}
							std::cout << "Bytes written: " << bytes_transferred << std::endl;
#else
							ssize_t bytes_transferred = write(_serial_handle, signed_buffer, size);

							if (bytes_transferred == size)
							{
								if (fsync(_serial_handle) == -1)
								{ // Use fsync() for flushing
									std::cerr << "fsync() failed: " << strerror(errno) << std::endl;
								}
							}
							else {
								if (bytes_transferred == -1)
								{
									std::cerr << "write() failed: " << strerror(errno) << std::endl;
								}
								else {
									std::cerr << "write() wrote " << bytes_transferred << " bytes, expected " << size << std::endl;
								}
							}
							delete[] signed_buffer;
							signed_buffer = nullptr;

							if (unsigned_buffer)
							{
								delete[] unsigned_buffer;
								unsigned_buffer = nullptr; // Avoid dangling pointer
							}
							if (bytes_transferred >= 0)
							{
								std::cout << "Bytes written: " << bytes_transferred << std::endl;
							}
#endif
						}
						}
					catch (const std::exception& e)
					{
						std::cerr << "[Serial] Error: " << e.what() << std::endl;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					}
				}

			bool serial_handler::is_running()
			{
				return _is_running;
			}
			}
			}
		}