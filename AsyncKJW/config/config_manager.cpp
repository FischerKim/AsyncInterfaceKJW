#include <pch.h>
using namespace II::network;

namespace II
{
	using namespace std;

	config_manager::config_manager() // 생성자
	{
	}

	std::string config_manager::get_substring(const std::string& a) // CONFIG_STRING element들 끝에 '=' string 제거.
	{
		return a.substr(0, a.size() - 1);
	}


	bool config_manager::create_ini()
	{
		try
		{

			// Open the file
			std::ofstream file(_path);
			if (!file.is_open())
			{
				std::cerr << "Failed to open file for writing: " << _path << std::endl;
				return false;
			}

			std::cout << "Creating INI file: " << _path << std::endl;

			file << "; Type : UDP Unicast=0, UDP Multicast=1, Tcp Server=2, Tcp Client=3, Serial=4\n";
			file << "[General]\n";
			file << "TotalConnections=6  ; Optional, to keep track of total defined connections\n\n";

			file << "[Connection_0]\n";
			file << "ID=0\n";
			file << "Type=2\n";
			file << "Description=TCP Server\n";
			file << "ListeningAddress=127.0.0.1\n";
			file << "ListeningPort=16000\n\n";

			file << "[Connection_1]\n";
			file << "ID=1\n";
			file << "Type=3\n";
			file << "Description=TCP Client\n";
			file << "ServerAddress=127.0.0.1\n";
			file << "ServerPort=16000\n";
			file << "ClientAddress=127.0.0.1\n";
			file << "ClientPort=16001\n\n";

			file << "[Connection_2]\n";
			file << "ID=2\n";
			file << "Type=0\n";
			file << "Description=UDP Client\n";
			file << "SourceAddress=127.0.0.1\n";
			file << "SourcePort=15000\n\n";

			file << "[UDP_Destination_0]\n";
			file << "ParentID=2  ; Links it to Connection_2\n";
			file << "DestinationID=1\n";
			file << "DestinationAddress=127.0.0.1\n";
			file << "DestinationPort=15001\n\n";

			file << "[UDP_Destination_1]\n";
			file << "ParentID=2\n";
			file << "DestinationID=2\n";
			file << "DestinationAddress=127.0.0.1\n";
			file << "DestinationPort=15002\n\n";

			file << "[Connection_3]\n";
			file << "ID=3\n";
			file << "Type=4\n";
			file << "Description=Serial Port\n";
			file << "Port=COM3\n";
			file << "BaudRate=38400\n";
			file << "DataWidth=8\n";
			file << "StopBits=1\n";
			file << "ParityMode=None\n\n";

			file << "[Connection_4]\n";
			file << "ID=4\n";
			file << "Type=4\n";
			file << "Description=Serial Port\n";
			file << "Port=COM4\n";
			file << "BaudRate=38400\n";
			file << "DataWidth=8\n";
			file << "StopBits=1\n";
			file << "ParityMode=None\n";

			file.close();
			std::cout << "INI file created successfully: " << _path << std::endl;
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception occurred while creating INI file: " << e.what() << std::endl;
			return false;
		}
	}

	void config_manager::load_ini(std::vector<session_info>& session_infos_, const std::string& path_)
	{
		std::cout << "Loading II.ini file: " << path_ << std::endl;

		// Load the main config file
		load_from_file(session_infos_, path_, false);

		//std::cout << "Loading udp_runtime.ini file: " << optional_path_ << std::endl;
		//// Load the optional UDP runtime file, if provided
		//if (!optional_path_.empty())
		//{
		//	load_from_file(session_infos_, optional_path_, true);
		//}
	}

	void config_manager::load_from_file(std::vector<session_info>& session_infos_, const std::string& path_, bool override_existing)
	{
		try
		{

			std::ifstream file(path_);
			if (!file.is_open())
			{
				std::cerr << "Error: Unable to open config file: " << path_ << std::endl;
				//create_ini();
				return;
			}

			std::string line, section;
			session_info temp_session;
			bool parsing_destination = false;
			int parent_id = -1;
			int destination_id = -1;

			while (std::getline(file, line))
			{
				line.erase(0, line.find_first_not_of(" \t"));
				if (line.empty() || line[0] == ';' || line[0] == '#')
				{
					//std::cout << "Ignoring line: " << line << std::endl;
					continue;
				}

				if (line[0] == '[')
				{
					std::cout << "Section: " << line << std::endl;
					if (!section.empty() && !parsing_destination && temp_session._id >= 0)
					{
						/*		std::cout << "id " << temp_session._id << std::endl;
								std::cout << "type " << temp_session._type << std::endl;
								std::cout << "description " << temp_session._description << std::endl;
								std::cout << "source ip " << temp_session._source_ip << std::endl;
								std::cout << "source port " << temp_session._source_port << std::endl;
								std::cout << "serial port " << temp_session._serial_port << std::endl;*/
						session_infos_.push_back(temp_session);
						temp_session = session_info(); // Reset
					}

					section = line.substr(1, line.size() - 2);
					parsing_destination = section.find("UDP_Destination_") == 0;

					if (parsing_destination)
					{
						parent_id = -1;
						destination_id = -1;
					}
					continue;
				}

				std::istringstream iss(line);

				std::string key, value;
				if (!std::getline(iss, key, '=') || !std::getline(iss, value))
					continue;

				key.erase(key.find_last_not_of(" \t") + 1);
				value.erase(0, value.find_first_not_of(" \t"));

				std::cout << "key value: " << key << " = " << value << std::endl;

				if (parsing_destination)
				{
					if (key.find("ParentID") != std::string::npos)
					{
						parent_id = std::stoi(value);
						std::cout << "ParentID set to: " << parent_id << std::endl;
					}
					else if (key.find("DestinationID") != std::string::npos)
					{
						destination_id = std::stoi(value);
						std::cout << "DestinationID set to: " << destination_id << std::endl;
					}
					else if (key.find("DestinationAddress") != std::string::npos)
					{
						std::string ip = value;
						std::cout << "Destination Address: " << ip << ",";
						// read next line for port
						if (std::getline(file, line))
						{
							std::istringstream next_line_stream(line);
							if (std::getline(next_line_stream, key, '=') && key.find("DestinationPort") != std::string::npos && std::getline(next_line_stream, value))
							{
								value.erase(0, value.find_first_not_of(" \t")); // Trim leading spaces
								unsigned int port_no = std::stoul(value);
								std::cout << "Destination Port: " << port_no << std::endl;
								for (auto& session : session_infos_) // Use auto& to modify sessions
								{
									if (parent_id == session._id)
									{
										if (override_existing)
										{
											session._udp_destinations[destination_id] = { ip, port_no }; // Override destination
										}
										else
										{
											session._udp_destinations.insert({ destination_id, { ip, port_no } }); // Add destination
										}
										std::cout << "Added destination: " << ip << ":" << port_no << " to session ID: " << parent_id << std::endl;
									}
								}
							}
						}
					}
				}
				else
				{
					if (key.find("ID") != string::npos)
					{
						temp_session._id = std::stoi(value);
					}
					else if (key.find("Type") != string::npos)
					{
						temp_session._type = std::stoi(value);
					}
					else if (key.find("Description") != string::npos)
					{
						strncpy(temp_session._description, value.c_str(), sizeof(temp_session._description) - 1);
					}
					//tcp server
					else if (key.find("ListeningAddress") != string::npos)
					{
						strncpy(temp_session._server_ip, value.c_str(), sizeof(temp_session._server_ip) - 1);
					}
					else if (key.find("ListeningPort") != string::npos)
					{
						temp_session._server_port = std::stoul(value);
					}
					//tcp client
					else if (key.find("ServerAddress") != string::npos)
					{
						strncpy(temp_session._server_ip, value.c_str(), sizeof(temp_session._server_ip) - 1);
					}
					else if (key.find("ServerPort") != string::npos)
					{
						temp_session._server_port = std::stoul(value);
					}
					else if (key.find("ClientAddress") != string::npos)
					{
						strncpy(temp_session._client_ip, value.c_str(), sizeof(temp_session._client_ip) - 1);
					}
					else if (key.find("ClientPort") != string::npos)
					{
						temp_session._client_port = std::stoul(value);
					}
					//udp
					else if (key.find("SourceAddress") != string::npos)
					{
						strncpy(temp_session._udp_source_ip, value.c_str(), sizeof(temp_session._udp_source_ip) - 1);
					}
					else if (key.find("SourcePort") != string::npos)
					{
						temp_session._udp_source_port = std::stoul(value);
					}
					//serial
					else if (key.find("Port") != string::npos)
					{
						strncpy(temp_session._serial_port, value.c_str(), sizeof(temp_session._serial_port) - 1);
					}
					else if (key.find("BaudRate") != string::npos)
					{
						temp_session._baud_rate = std::stoul(value);
					}
					else if (key.find("DataWidth") != string::npos)
					{
						temp_session._data_width = std::stoul(value);
					}
					else if (key.find("StopBits") != string::npos)
					{
						strncpy(temp_session._stop_bits, value.c_str(), sizeof(temp_session._stop_bits) - 1);
					}
					else if (key.find("ParityMode") != string::npos)
					{
						strncpy(temp_session._parity_mode, value.c_str(), sizeof(temp_session._parity_mode) - 1);
					}
				}
			}

			if (temp_session._id >= 0) // Store last session
			{
				session_infos_.push_back(temp_session);
			}

			file.close();

		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception occurred when loading ini file: " << e.what() << std::endl;
		}
	}

	void config_manager::save_ini(const std::vector<II::network::session_info>& session_infos_)
	{
		std::ofstream file(_path);
		if (!file.is_open())
		{
			std::cerr << "Error: Unable to open config file for writing: " << _path << std::endl;
			return;
		}

		file << "; Type : UDP Unicast=0, UDP Multicast=1, Tcp Server=2, Tcp Client=3, Serial=4\n";
		file << "[General]\n";
		file << "TotalConnections=" << session_infos_.size() << "\n\n";

		int udp_dest_index = 0;
		for (const auto& session : session_infos_)
		{
			file << "[Connection_" << session._id << "]\n";
			file << "ID=" << session._id << "\n";
			file << "Type=" << session._type << "\n";
			file << "Description=" << session._description << "\n";

			if (session._type == 2) // TCP Server
			{
				file << "ListeningAddress=" << session._server_ip << "\n";
				file << "ListeningPort=" << session._server_port << "\n";
			}
			else if (session._type == 3) // TCP Client
			{
				file << "ServerAddress=" << session._server_ip << "\n";
				file << "ServerPort=" << session._server_port << "\n";
				file << "ClientAddress=" << session._client_ip << "\n";
				file << "ClientPort=" << session._client_port << "\n";
			}
			else if (session._type == 0) // UDP Client
			{
				file << "SourceAddress=" << session._udp_source_ip << "\n";
				file << "SourcePort=" << session._udp_source_port << "\n";
			}
			else if (session._type == 4) // Serial Port
			{
				file << "Port=" << session._serial_port << "\n";
				file << "BaudRate=" << session._baud_rate << "\n";
				file << "DataWidth=" << session._data_width << "\n";
				file << "StopBits=" << session._stop_bits << "\n";
				file << "ParityMode=" << session._parity_mode << "\n";
			}

			file << "\n";

			// Save UDP Destinations
			if (session._type == 0 && !session._udp_destinations.empty())
			{
				for (const auto& dest : session._udp_destinations)
				{
					file << "[UDP_Destination_" << udp_dest_index++ << "]\n";
					file << "ParentID=" << session._id << "\n";
					/*	file << "DestinationAddress=" << dest.first << "\n";
						file << "DestinationPort=" << dest.second << "\n\n";*/
				}
			}
		}

		file.close();
		std::cout << "Configuration saved successfully to: " << _path << std::endl;
	}
}