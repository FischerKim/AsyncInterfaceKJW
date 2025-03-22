#pragma once
#define MAX_PATH 260
using namespace II::network;

namespace II
{
	class config_manager // ��� ��� ���� �޴���
	{
	public:
		config_manager(); // ������
		~config_manager() = default;// �ı���

	private:
		bool _first = true;
		std::string _path; // ���� ���� ���
		std::string get_substring(const std::string& a); //CONFIG_STRING element�� ���� '=' string ����.

	public:
		bool create_ini(); // �������� ����� ���� ����.
		void load_ini(std::vector<session_info>& session_infos_, const std::string& path_);// , const std::string& optional_path_ = "");
		void save_ini(const std::vector<session_info>& session_infos_); // �������� ����
	private:
		void load_from_file(std::vector<session_info>& session_infos_, const std::string& path_, bool override_existing);
	};
}
