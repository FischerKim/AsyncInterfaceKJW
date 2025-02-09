#pragma once
#ifndef SESSION_IPP
#define SESSION_IPP
#ifndef LINUX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#pragma comment(lib, "Ws2_32.lib")
#else // Assuming Linux/Unix system
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h>   // For inet_pton
#include <sys/epoll.h>
#include <fcntl.h>
#endif 
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>
#include <codecvt>
#include <shared_mutex>
#include <mutex>
#include <math.h>
#include <cstring>
#include <functional>
#include <deque>
#include <condition_variable>
#include <queue>
#include <system_error>
#include <chrono>
#include <future>
#include <cctype>  // for tolower
#include <atomic>

#ifndef LINUX
#include <conio.h>
#include <string>
#else
#include <cerrno>
#include <string.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#endif
#include <common/thread_pool.h>
#include <common/type.h>
//#include <common/usertypedef.h>
#include <network/module_info.h>
#include <network/modules/udp_handler.h>
#include <network/modules/tcp_client_handler.h>
#include <network/modules/tcp_server_handler.h>
#include <network/modules/serial_handler.h>

#include <main/session.hpp>
#include <main/session.ipp>
#include <config/config_manager.h>
#include <main/AsyncKJW.h>
//#include <main/CAPI.h>
#endif  // SESSION_IPP
