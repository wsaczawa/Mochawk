#pragma once

#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdint>
#include <exception>
#include <ctime>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <iterator>

#include <windows.h>
#include <WinInet.h>

#pragma comment(lib, "Wininet")

namespace fs = std::filesystem;
using pathvec = std::vector<fs::directory_entry>;
using namespace std::chrono_literals;

inline std::wstring getcell(std::wstring line, size_t col) {
	size_t begin = 0;
	size_t end = 0;

	for (size_t i = 0; i < col - 1; i++) {
		begin = line.find(',', begin) + 1;
	}

	if (line.find(',', begin + 1) == std::string::npos) {
		end = line.length();
	}
	else {
		end = line.find(',', begin + 1);
	}

	return line.substr(begin, end - begin);
}

class loginFTP {
public:
	std::wstring ip;
	std::wstring user;
	std::wstring pass;
	std::wstring dir;

	void getloginFTP(std::wstring s);
};

class dataFTP {
public:
	std::vector<loginFTP> login;

	dataFTP(std::string csv);
};

class ftp {
public:
	HINTERNET internet;
	HINTERNET session;
	HINTERNET find;
	HINTERNET file;
	WIN32_FIND_DATA info;
	loginFTP login;

	ftp(loginFTP l);
	void cd(std::wstring d);
	void get();
	void getAll();
	bool put(std::wstring from, std::wstring to);
	void bye();
	void download();
};

class folders {
public:
	const fs::path Archive = "./Archive";
	const fs::path Flush = "./Flush";
	const fs::path Buffer = "./Buffer";
	const fs::path New = "./New";
	const fs::path Summed = "./Summed";
	const fs::path backup = "./backup.txt";

	std::vector<std::pair<std::wstring, std::wstring>> signals;

	folders();
	void manage(std::string file);
	void upgrade(fs::path from, fs::path to);
	void send(ftp exp);
	void flush(fs::path d);
	void clear(fs::path d, int last);
	std::size_t count(fs::path d);
};

class spectrum {
public:
	std::string name;
	double duration;
	double start;
	double time;
	double preset;
	float vol;

	spectrum();
	bool operator<(const spectrum& a) const;
	void getData(std::string path);
};

class CNFset {
public:
	double downLimit;
	double spectraTime;
	int spectraLimit;
	std::string asf;

	unsigned plus;
	unsigned minus;

	std::vector<spectrum> spectra;

	CNFset();

	void getConst(fs::path csv);
	void getSpectra(fs::path folder);
	void load(fs::path backup);
	void save(fs::path backup);
	void append(spectrum s);
	bool check(spectrum s);
};

class nuclide {
public:
	std::wstring name;
	double mean;
	double error;
	double mda;

	nuclide();
};

class analysis {
public:
	std::vector<nuclide> nuclides;
	spectrum s;
	std::wstring tsExp;
	std::wstring tsDisp;
	std::wstring detector;

	analysis(CNFset set);

	void strip(std::string sumFile, std::string addFile, int f);
	void getNuclides(std::string summFile, std::string asf);
	void getTimestamp();
	void process();
	void writeFTP(std::vector<std::pair<std::wstring, std::wstring>> signals);
	void writeHTML();
};

