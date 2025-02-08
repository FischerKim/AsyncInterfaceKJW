#pragma once
#define MAX_PATH 260
namespace II
{
	class config_manager // 통신 모듈 설정 메니져
	{
	public:
		config_manager(); // 생성자
		~config_manager(); // 파괴자
		 
	private:
		char m_strConfigFilePath[MAX_PATH]; // 설정 파일 경로
		std::string get_substring(const std::string& a); //CONFIG_STRING element들 끝에 '=' string 제거.

	public:
		void GetConfigFilePath(unsigned short equipment_id_); // 설정 파일 경로 반환
		bool CreateConfigFile(); // 설정파일 부재시 파일 생성.
		void load_from_file(std::vector<II::network::session_info>& i_vIFInfo, unsigned short equipment_id_); // 설정파일 불러오기
		void save_to_file(std::vector<II::network::session_info> i_vIFInfo); // 설정파일 저장
	};
}
