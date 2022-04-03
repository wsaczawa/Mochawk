#include "pch.h"

#include "dataFTP.h"

void loginFTP::getloginFTP(std::wstring s) {
	ip = getcell(s, 1);
	user = getcell(s, 2);
	pass = getcell(s, 3);
	dir = getcell(s, 4);
}

dataFTP::dataFTP(std::string csv) {
	std::wifstream f;
	std::wstring s;
	f.open(csv);
	std::getline(f, s);
	while (std::getline(f, s)) {
		loginFTP l;
		l.getloginFTP(s);
		login.push_back(l);
	}

}

ftp::ftp(loginFTP l) {
	bye();
	login = l;
	find = FALSE;
	file = FALSE;

	internet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	session = InternetConnect(internet, login.ip.c_str(), INTERNET_DEFAULT_FTP_PORT, login.user.c_str(), login.pass.c_str(), INTERNET_SERVICE_FTP, 0, 0);
}

void ftp::cd(std::wstring dir) {
	if (!dir.empty()) {
		FtpSetCurrentDirectory(session, dir.c_str());
	}
}

void ftp::get() {
	if (FtpGetFile(session, info.cFileName, info.cFileName, TRUE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
		std::filesystem::copy(info.cFileName, L"./New/" + (std::wstring)info.cFileName, std::filesystem::copy_options::overwrite_existing);
		std::filesystem::remove(info.cFileName);
	}
	FtpDeleteFile(session, info.cFileName);
}

void ftp::getAll() {
	find = FtpFindFirstFile(session, TEXT("*.CNF"), &info, 0, 0);
	get();
	while (InternetFindNextFile(find, &info)) {
		get();
	}
}

bool ftp::put(std::wstring from, std::wstring to) {
	return FtpPutFile(session, from.c_str(), to.c_str(), FTP_TRANSFER_TYPE_ASCII, 0);
}

void ftp::bye() {
	InternetCloseHandle(internet);
}

void ftp::download() {
	cd(login.dir);
	getAll();
	bye();
}