#pragma once
#define MAX_PATH 260
namespace II
{
	class config_manager // ��� ��� ���� �޴���
	{
	public:
		config_manager(); // ������
		~config_manager(); // �ı���
		 
	private:
		char m_strConfigFilePath[MAX_PATH]; // ���� ���� ���
		std::string get_substring(const std::string& a); //CONFIG_STRING element�� ���� '=' string ����.

	public:
		void GetConfigFilePath(unsigned short equipment_id_); // ���� ���� ��� ��ȯ
		bool CreateConfigFile(); // �������� ����� ���� ����.
		void load_from_file(std::vector<II::network::session_info>& i_vIFInfo, unsigned short equipment_id_); // �������� �ҷ�����
		void save_to_file(std::vector<II::network::session_info> i_vIFInfo); // �������� ����
	};
}
