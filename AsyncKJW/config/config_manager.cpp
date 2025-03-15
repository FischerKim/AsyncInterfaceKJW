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
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception occurred while creating INI file: " << e.what() << std::endl;
			return false;
		}
	}

	void config_manager::load_ini(std::vector<session_info>& session_infos_, const std::string& path_, const std::string& optional_path_)
	{
		std::cout << "Loading INI file: " << path_ << std::endl;
		session_infos_.clear();

		// Load the main config file
		load_from_file(session_infos_, path_, false);

		// Load the optional UDP runtime file, if provided
		if (!optional_path_.empty())
		{
			load_from_file(session_infos_, optional_path_, true);
		}
	}

	void config_manager::load_from_file(std::vector<session_info>& session_infos_, const std::string& path_, bool override_existing)
	{
		std::ifstream file(path_);
		if (!file.is_open())
		{
			std::cerr << "Error: Unable to open config file: " << path_ << std::endl;
			return;
		}

		std::string line, section;
		session_info temp_session;
		bool parsing_destination = false;
		int parent_id = -1;

		while (std::getline(file, line))
		{
			line.erase(0, line.find_first_not_of(" \t")); // Trim leading spaces
			if (line.empty() || line[0] == ';' || line[0] == '#') // Ignore comments and empty lines
				continue;

			if (line[0] == '[' && line.back() == ']') // New section
			{
				section = line.substr(1, line.size() - 2);
				parsing_destination = section.find("UDP_Destination_") == 0;
				if (!parsing_destination && temp_session._id >= 0)
				{
					// Store previous session before creating a new one
					session_infos_.push_back(temp_session);
					temp_session = session_info(); // Reset
				}
				continue;
			}

			std::istringstream iss(line);
			std::string key, value;
			if (!std::getline(iss, key, '=') || !std::getline(iss, value))
				continue;

			key.erase(key.find_last_not_of(" \t") + 1); // Trim trailing spaces
			value.erase(0, value.find_first_not_of(" \t")); // Trim leading spaces

			if (parsing_destination)
			{
				if (key == "ParentID")
				{
					parent_id = std::stoi(value);
				}
				else if (key == "DestinationAddress")
				{
					std::string ip = value;
					if (std::getline(iss, value))
					{
						unsigned int port = std::stoul(value);

						auto it = std::find_if(session_infos_.begin(), session_infos_.end(),
							[parent_id](const session_info& s) { return s._id == parent_id; });

						if (it != session_infos_.end())
						{
							if (override_existing)
							{
								it->_destinations[ip] = port; // Override destination
							}
							else
							{
								it->_destinations.insert({ ip, port }); // Add destination
							}
						}
					}
				}
			}
			else
			{
				if (key == "ID") temp_session._id = std::stoi(value);
				else if (key == "Type") temp_session._type = std::stoi(value);
				else if (key == "Description") strncpy(temp_session._description, value.c_str(), sizeof(temp_session._description) - 1);
				else if (key == "ListeningAddress" || key == "ServerAddress" || key == "SourceAddress")
					strncpy(temp_session._source_ip, value.c_str(), sizeof(temp_session._source_ip) - 1);
				else if (key == "ListeningPort" || key == "ServerPort" || key == "SourcePort")
					temp_session._source_port = std::stoul(value);
				else if (key == "Port") strncpy(temp_session._serial_port, value.c_str(), sizeof(temp_session._serial_port) - 1);
				else if (key == "BaudRate") temp_session._baud_rate = std::stoul(value);
				else if (key == "DataWidth") temp_session._data_width = std::stoul(value);
				else if (key == "StopBits") strncpy(temp_session._stop_bits, value.c_str(), sizeof(temp_session._stop_bits) - 1);
				else if (key == "ParityMode") strncpy(temp_session._parity_mode, value.c_str(), sizeof(temp_session._parity_mode) - 1);
			}
		}

		if (temp_session._id >= 0) // Store last session
		{
			session_infos_.push_back(temp_session);
		}

		file.close();
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
				file << "ListeningAddress=" << session._source_ip << "\n";
				file << "ListeningPort=" << session._source_port << "\n";
			}
			else if (session._type == 3) // TCP Client
			{
				file << "ServerAddress=" << session._source_ip << "\n";
				file << "ServerPort=" << session._source_port << "\n";
			}
			else if (session._type == 0) // UDP Client
			{
				file << "SourceAddress=" << session._source_ip << "\n";
				file << "SourcePort=" << session._source_port << "\n";
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
			if (session._type == 0 && !session._destinations.empty())
			{
				for (const auto& dest : session._destinations)
				{
					file << "[UDP_Destination_" << udp_dest_index++ << "]\n";
					file << "ParentID=" << session._id << "\n";
					file << "DestinationAddress=" << dest.first << "\n";
					file << "DestinationPort=" << dest.second << "\n\n";
				}
			}
		}

		file.close();
		std::cout << "Configuration saved successfully to: " << _path << std::endl;
	}
}