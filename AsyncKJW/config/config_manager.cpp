#include <pch.h>
using namespace II::network;

namespace II
{
	using namespace std;

	config_manager::config_manager() // 생성자
	{
	}

	config_manager::~config_manager() // 파괴자
	{
	}

	std::string config_manager::get_substring(const std::string& a) // CONFIG_STRING element들 끝에 '=' string 제거.
	{
		return a.substr(0, a.size() - 1);
	}

	void config_manager::GetConfigFilePath(unsigned short i_ushEquipNo) // 설정 파일 경로 반환
	{
		try
		{
			m_strConfigFilePath[0] = '\0';
			strcpy(m_strConfigFilePath, ("Config/KDDX_SIM_" + EQUIPMENT_NAME[i_ushEquipNo] + ".ini").c_str());
			cout << "LOAD CONFIG FILE : " << m_strConfigFilePath << endl;
		}
		catch (const exception& e)
		{
			cerr << "Error: " << e.what() << endl;
		}
	}

	bool config_manager::CreateConfigFile() // 설정파일 부재시 파일 생성.
	{
		try
		{
			ofstream file(m_strConfigFilePath);
			if (!file.is_open())
			{
				cout << "Failed to open file for writing: " << m_strConfigFilePath << endl;
			}
			else
			{
				cout << "Create ini " << m_strConfigFilePath << endl;

				file << "// [INTERFACE_ID : The interface ID is designated as a UNIQUE NUMBER according to the order in which it was created." << endl;
				file << "// [IF_TYPE : UDP(0), MULTICAST(1), TCP_SERVER(2), TCP_CLIENT(3), SERIAL(4)]" << endl;
				file << "----------------------------------------" << endl;

				file << CONFIG_STRING[CSI_START_SECTION] << endl;
				file << CONFIG_STRING[CSI_IF_ID] << "0" << endl;
				file << CONFIG_STRING[CSI_IF_TYPE] << "0" << endl;
				file << CONFIG_STRING[CSI_IF_DESCRIPTION] << endl;
				file << CONFIG_STRING[CSI_ETHERNET_SECTION] << endl;
				file << CONFIG_STRING[CSI_SRC_IP] << "0.0.0.0" << endl;
				file << CONFIG_STRING[CSI_SRC_PORT] << "0" << endl;
				file << CONFIG_STRING[CSI_DST_IP] << "0.0.0.0" << endl;
				file << CONFIG_STRING[CSI_DST_PORT] << "0" << endl;
				file << CONFIG_STRING[CSI_SERIAL_SECTION] << endl;
				file << CONFIG_STRING[CSI_SERIAL_PORT] << "0" << endl;
				file << CONFIG_STRING[CSI_SERIAL_BAUD_RATE] << "0" << endl;
				file << CONFIG_STRING[CSI_SERIAL_DATA_WIDTH] << "0" << endl;
				file << CONFIG_STRING[CSI_SERIAL_STOP_BITS] << "0" << endl;
				file << CONFIG_STRING[CSI_SERIAL_PARITY_MODE] << "0" << endl;
				file << CONFIG_STRING[CSI_END_SECTION] << endl;
				file << CONFIG_STRING[CSI_END_CONFIG] << endl;
				file.close();
				return true;
			}
		}
		catch (const exception& e)
		{
			cerr << "Error: " << e.what() << endl;
		}

		return false;
	}

	void config_manager::load_from_file(vector<session_info>& i_vIFInfo, unsigned short equipment_number_) // 설정파일 불러오기
	{
		try
		{
			GetConfigFilePath(equipment_number_);
			ifstream findFile(m_strConfigFilePath);
			if (!findFile)
			{
				cout << "IIMODULE[] - Config/KDDX_SIM_<Equipment ID>.ini 파일은 AsyncKJW.dll이 위치되어있는 폴더 내에 존재하여야 합니다." << endl;
				cout << "attempting to create a file..." << endl;
				CreateConfigFile();
				cout << "successfully created the file!" << endl;
			}
			else
			{
				cout << "reading config file Config/KDDX_SIM_<Equipment ID>.ini ..." << endl;
				string line;
				session_info stIFInfo;
				//memset(&stIFInfo, 0x00, sizeof(session_info));
				i_vIFInfo.clear();

				//설정파일에서 session_info 구조체에 값들을 넣음 
				while (getline(findFile, line))
				{
					istringstream iss(line);
					string key, value;
					if (getline(iss, key, '=') && getline(iss, value)) //= 이후 (excluding =) 나머지 string을 extract
					{
						if (get_substring(CONFIG_STRING[CSI_IF_ID]) == key)
						{
							stIFInfo._id = static_cast<short>(stoul(value));  //unsigned integer를 string에서 찾음. 만약 음수면 outofrange 에러 throw함.
							continue;
						}
						else if (get_substring(CONFIG_STRING[CSI_IF_TYPE]) == key)
						{
							stIFInfo._type = static_cast<short>(stoul(value)); //Ethernet인지 Serial인지
							continue;
						}
						else if (get_substring(CONFIG_STRING[CSI_IF_DESCRIPTION]) == key)
						{
							//character 버퍼에 해당 string을 적음.
							snprintf(stIFInfo._description, sizeof(stIFInfo._description), "%s", value.c_str());
							continue;
						}

						if (stIFInfo._type >= E_INTERFACE_TYPE::MAX)
						{
							continue;
						}
						else
						{
							if (stIFInfo._type != _SERIAL)
							{
								if (get_substring(CONFIG_STRING[CSI_SRC_IP]) == key)
								{
									snprintf(stIFInfo._source_ip, sizeof(stIFInfo._source_ip), "%s", value.c_str());
								}
								else if (get_substring(CONFIG_STRING[CSI_SRC_PORT]) == key)
								{
									stIFInfo._source_port = stoul(value);
								}
								else if (get_substring(CONFIG_STRING[CSI_DST_IP]) == key)
								{
									snprintf(stIFInfo._destination_ip, sizeof(stIFInfo._destination_ip), "%s", value.c_str());
								}
								else if (get_substring(CONFIG_STRING[CSI_DST_PORT]) == key)
								{
									stIFInfo._destination_port = stoul(value);
								}
							}
							else
							{
								if (get_substring(CONFIG_STRING[CSI_SERIAL_PORT]) == key)
								{
									snprintf(stIFInfo._serial_port, sizeof(stIFInfo._serial_port), "%s", value.c_str());
								}
								else if (get_substring(CONFIG_STRING[CSI_SERIAL_BAUD_RATE]) == key)
								{
									stIFInfo._baud_rate = stoul(value);
								}
								else if (get_substring(CONFIG_STRING[CSI_SERIAL_DATA_WIDTH]) == key)
								{
									stIFInfo._data_width = stoul(value);
								}
								else if (get_substring(CONFIG_STRING[CSI_SERIAL_STOP_BITS]) == key)
								{
									snprintf(stIFInfo._stop_bits, sizeof(stIFInfo._stop_bits), "%s", value.c_str());
								}
								else if (get_substring(CONFIG_STRING[CSI_SERIAL_PARITY_MODE]) == key)
								{
									snprintf(stIFInfo._parity_mode, sizeof(stIFInfo._parity_mode), "%s", value.c_str());
								}
							}
						}
					}
					if (line.find(CONFIG_STRING[CSI_END_SECTION]) != std::string::npos)
					{
						i_vIFInfo.push_back(stIFInfo);
					}
					if (line.find(CONFIG_STRING[CSI_END_CONFIG]) != std::string::npos)
					{
						break;
					}
				}
				findFile.close();
			}
		}
		catch (const exception& e)
		{
			cerr << "Error when loading data: " << e.what() << endl;
		}
	}

	void config_manager::save_to_file(vector<session_info> i_vIFInfo) // 설정파일 저장
	{
		try
		{
			printf("--------------- [II] SAVE CONFIG INFO!!!!!!!!! ---------------\n");
			fstream targetFile(m_strConfigFilePath, ios_base::out | ios_base::trunc | ios_base::binary); //exclusive access

			if (!targetFile.is_open())
			{
				cout << "attempting to create file..." << endl;
				CreateConfigFile();
				cout << "successfully created the file! please retry saving again." << endl;
			}
			else
			{
				if (!targetFile)
				{
					cerr << "Failed to open the config file for writing." << endl;
					return;
				}

				targetFile << "// [INTERFACE_ID : The interface ID is designated as a UNIQUE NUMBER according to the order in which it was created." << endl;
				targetFile << "// [IF_TYPE : UDP(0), MULTICAST(1), TCP_SERVER(2), TCP_CLIENT(3), SERIAL(4)]" << endl;
				targetFile << "----------------------------------------" << endl;
				for (const session_info& stIFInfo : i_vIFInfo)
				{
					char* p1 = (char*)strchr(stIFInfo._description, '\0');
					while (p1 > stIFInfo._description && *(p1 - 1) == '\0')
					{
						*--p1 = '\0';
					}
					char* p2 = (char*)strchr(stIFInfo._source_ip, '\0');
					while (p2 > stIFInfo._source_ip && *(p2 - 1) == '\0')
					{
						*--p2 = '\0';
					}
					char* p3 = (char*)strchr(stIFInfo._destination_ip, '\0');
					while (p3 > stIFInfo._destination_ip && *(p3 - 1) == '\0')
					{
						*--p3 = '\0';
					}

					targetFile << CONFIG_STRING[CSI_START_SECTION] << endl;
					targetFile << CONFIG_STRING[CSI_IF_ID] << stIFInfo._id << endl;
					targetFile << CONFIG_STRING[CSI_IF_TYPE] << stIFInfo._type << endl;
					targetFile << CONFIG_STRING[CSI_IF_DESCRIPTION] << stIFInfo._description << endl;
					if (stIFInfo._type != _SERIAL)
					{
						targetFile << CONFIG_STRING[CSI_ETHERNET_SECTION] << endl;
						targetFile << CONFIG_STRING[CSI_SRC_IP] << stIFInfo._source_ip << endl;
						targetFile << CONFIG_STRING[CSI_SRC_PORT] << stIFInfo._source_port << endl;
						targetFile << CONFIG_STRING[CSI_DST_IP] << stIFInfo._destination_ip << endl;
						targetFile << CONFIG_STRING[CSI_DST_PORT] << stIFInfo._destination_port << endl;
					}
					else
					{
						targetFile << CONFIG_STRING[CSI_SERIAL_SECTION] << endl;
						targetFile << CONFIG_STRING[CSI_SERIAL_PORT] << stIFInfo._serial_port << endl;
						targetFile << CONFIG_STRING[CSI_SERIAL_BAUD_RATE] << stIFInfo._baud_rate << endl;
						targetFile << CONFIG_STRING[CSI_SERIAL_DATA_WIDTH] << stIFInfo._data_width << endl;
						targetFile << CONFIG_STRING[CSI_SERIAL_STOP_BITS] << stIFInfo._stop_bits << endl;
						targetFile << CONFIG_STRING[CSI_SERIAL_PARITY_MODE] << stIFInfo._parity_mode << endl;
					}
					targetFile << CONFIG_STRING[CSI_END_SECTION] << endl;
				}
				targetFile << CONFIG_STRING[CSI_END_CONFIG] << endl;
				targetFile.close();
			}
		}
		catch (const exception& e)
		{
			cerr << "Error: " << e.what() << endl;
		}
	}
}