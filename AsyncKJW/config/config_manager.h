#pragma once
#define MAX_PATH 260
using namespace II::network;

namespace II
{
	class config_manager // 통신 모듈 설정 메니져
	{
	public:
		config_manager(); // 생성자
		~config_manager() = default;// 파괴자

	private:
		bool _first = true;
		std::string _path; // 설정 파일 경로
		std::string get_substring(const std::string& a); //CONFIG_STRING element들 끝에 '=' string 제거.

	public:
		bool create_ini(); // 설정파일 부재시 파일 생성.
		void load_ini(std::vector<session_info>& session_infos_, const std::string& path_);// , const std::string& optional_path_ = "");
		void save_ini(const std::vector<session_info>& session_infos_); // 설정파일 저장
	private:
		void load_from_file(std::vector<session_info>& session_infos_, const std::string& path_, bool override_existing);
	};
}
