#include "pch.h"

#include <windows.h>
#include <WinInet.h>
#include <strsafe.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <filesystem>
#include <string>
#include <string.h>
#include <sstream>
#include <time.h>
#include <thread>
#include <chrono>
#include <cstdint>
#include <exception>
#include <ctime>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <queue>
#include "crackers.h"

#include <tchar.h>
#include <stdio.h>

#include "citypes.h"

#include "spasst.h"
#include "sad.h"
#include "ci_files.h"
//#include "campdef.h"
//#include "cam_n.h"
#include "Sad_nest.h"
#include "utility.h"
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Wininet")

#import  "Analyze.tlb" raw_interfaces_only, raw_native_types, named_guids
#import  "DataAccess.tlb" raw_interfaces_only, raw_native_types, named_guids

#define mCAM_F_SQUANT 0X20010005L
#define mCAM_X_EREAL 0X20000016L
#define mCAM_X_ELIVE 0X20000017L
#define mCAM_T_NCLNAME 0X20070002L
#define mCAM_G_NCLMDA 0X20070006L
#define mCAM_G_NCLWTMEAN 0X2007001EL
#define mCAM_G_NCLWTMERR 0X20070021L
#define mCAM_X_ASTIME 0X20000015L

namespace fs = std::filesystem;
using pathvec = std::vector<fs::directory_entry>;
using namespace std::chrono_literals;
using namespace CanberraSequenceAnalyzerLib;
using namespace CanberraDataAccessLib;

LPCWSTR server = L"000.000.0.00";
LPCWSTR user = L"username";
LPCWSTR password = L"pass";

HINTERNET hInternet;
HINTERNET hFtpSession;
HINTERNET hFind;
HINTERNET hFile;
DWORD dwFindFlags;
DWORD dwError;
DWORD x = 0;
DWORD ChunkSize = 307200;
WIN32_FIND_DATA FileInfo;
char buffFTP[307200];

HMEM         hDSC;
INT          iRC;
SHORT        sRC;
DOUBLE		 dCAMTime, adTimes[2], adTimesSum[2];
REAL		 rQuant, rQuantSum;
static		 ListOfCAM_T astParams[2] = {
	{ mCAM_X_EREAL, 0, 0 },
	{ mCAM_X_ELIVE, 0, 0 }
};
struct tm	 pstCTime;
FLAG		 fReturn;
ULONG        ulRC;
ULONG		 aulChanData1[1024], aulVectorData1[1024], aulChanData2[1024], aulVectorData2[1024], aulChanData3[1024], aulVectorData3[1024], aulChanData4[1024], aulVectorData4[1024];

auto NominalWaitingTime = 1min;
double ThresholdTimeSec = 3600.0;
int NumberOfNuclides = 31;
size_t SpectraLimit = 10;

fs::path Flush = "C:\\Mochawk\\Flush";
fs::path NewDir = "C:\\Mochawk\\New";
fs::path ArchiveDir = "C:\\Mochawk\\Archive";
fs::path backup = "C:\\Mochawk\\Backup.txt";
const char *dSumSpectrum = "C:\\Users\\EMO\\Desktop\\00000.CNF";
const char* SqnAn = "C:\\GENIE2K\\CTLFILES\\sequency.ASF";

struct spectr {
	fs::path path;
	ULONG data0[1024];
	ULONG data1025[1024];
	ULONG data2049[1024];
	ULONG data3073[1024];
	REAL quant;
	double time;
	double times[2];

	bool operator<(const spectr& a) const
	{
		return time < a.time;
	}
};
struct nucl {
	std::string name;
	double mean;
	double error;
	double mda;
};
struct ftpname {
	LPCWSTR winname;
	std::wstring stringname;
};

struct older_file {
	bool operator()(const fs::directory_entry& p, const fs::directory_entry& p2)
	{
		return p.last_write_time() > p2.last_write_time();
	}
};

template <typename T>
void KillVector(T & t) {
	T tmp;
	t.swap(tmp);
}

void ImportFtp(LPCWSTR server, LPCWSTR user, LPCWSTR password, int CopyThreshold, int DeleteThreshold) {
	std::vector<ftpname> FtpNames;
	int limit = 0;
	InternetCloseHandle(hInternet);
	hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	hFtpSession = InternetConnect(hInternet, server, INTERNET_DEFAULT_FTP_PORT, user, password, INTERNET_SERVICE_FTP, 0, 0);
	if (hFtpSession == NULL) {
		dwError = GetLastError();
		std::cout << "Error connection in import: " << dwError << "\n";
		DWORD dwInetError;
		DWORD dwExtLength = 1000;
		TCHAR *szExtErrMsg = NULL;
		TCHAR errmsg[1000];
		szExtErrMsg = errmsg;
		int returned = InternetGetLastResponseInfo(&dwInetError, szExtErrMsg, &dwExtLength);
		printf("dwInetError: %d  Returned: %d\n", dwInetError, returned);
		_tprintf(_T("Buffer: %s\n"), szExtErrMsg);
	}
	hFind = FtpFindFirstFile(hFtpSession, TEXT("*.CNF"), &FileInfo, 0, 0);
	if (hFind == NULL)
	{
		InternetCloseHandle(hFtpSession);
		InternetCloseHandle(hInternet);
		std::cout << "Connection closed due to lack of new spectra \n\n";
	}
	else {
		do
	{
		ftpname newCNF;
		newCNF.stringname = FileInfo.cFileName;
		newCNF.winname = newCNF.stringname.c_str();
		FtpNames.push_back(newCNF);
		limit++;

		bool result = FALSE;
		int protector = 0;
		do {
			result = FtpGetFile(hFtpSession,
				FileInfo.cFileName, FileInfo.cFileName, TRUE,
				FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0);
			if (result == FALSE) {
				dwError = GetLastError();
				std::cout << "Error getting file: " << dwError << "\n";
				DWORD dwInetError;
				DWORD dwExtLength = 1000;
				TCHAR *szExtErrMsg = NULL;
				TCHAR errmsg[1000];
				szExtErrMsg = errmsg;
				int returned = InternetGetLastResponseInfo(&dwInetError, szExtErrMsg, &dwExtLength);
				printf("dwInetError: %d  Returned: %d\n", dwInetError, returned);
				_tprintf(_T("Buffer: %s\n"), szExtErrMsg);

				std::this_thread::sleep_for(std::chrono::seconds(10));
			}
			protector++;
			if (protector > CopyThreshold) break;

		} while (result == FALSE);

		result = FALSE;
		protector = 0;
		do {
			result = FtpDeleteFile(hFtpSession,
				FileInfo.cFileName);
			if (result == FALSE) {
				dwError = GetLastError();
				std::cout << "Error deleting file: " << dwError << "\n";
				DWORD dwInetError;
				DWORD dwExtLength = 1000;
				TCHAR *szExtErrMsg = NULL;
				TCHAR errmsg[1000];
				szExtErrMsg = errmsg;
				int returned = InternetGetLastResponseInfo(&dwInetError, szExtErrMsg, &dwExtLength);
				printf("dwInetError: %d  Returned: %d\n", dwInetError, returned);
				_tprintf(_T("Buffer: %s\n"), szExtErrMsg);

				std::this_thread::sleep_for(std::chrono::seconds(10));
			}
			protector++;
			if (protector > DeleteThreshold) break;

		} while (result == FALSE);
		if (limit > 20) break;

	} while (InternetFindNextFile(hFind, &FileInfo));
		
	for (unsigned i = 0; i < FtpNames.size(); i++) {
			std::wcout << FtpNames[i].stringname << " downloaded \n";
			std::wstring immigrant = L"C:\\Mochawk\\" + FtpNames[i].stringname;
			fs::path importfrom = immigrant;
			std::wstring emmigrant = L"C:\\Mochawk\\New\\" + FtpNames[i].stringname;
			fs::path importto = emmigrant;
			fs::copy(importfrom, importto, std::filesystem::copy_options::overwrite_existing);
			fs::remove(importfrom);
		}
	}

	InternetCloseHandle(hInternet);
}

std::size_t CountSpectra(fs::path path)
{
	using fs::directory_iterator;
	return std::distance(directory_iterator(path), directory_iterator{});
}

std::vector<spectr> SpectraVector(fs::path dir) {
	std::vector<spectr> b_vector;
	for (auto& p : fs::directory_iterator(dir)) {
		spectr spectrum;
		fs::path Name_path = p.path();
		spectrum.path = Name_path.filename();
		std::string Name_string = Name_path.string();
		const char* Name_char = Name_string.c_str();

		sRC = SadOpenDataSource(hDSC, Name_char,
			CIF_NativeSpect,
			ACC_Exclusive | ACC_SysWrite | ACC_ReadWrite,
			FALSE, "");
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error opening file: %lx", ulRC);
		}

		std::cout << "The spectrum " << Name_char << " has been open \n\n";

		dCAMTime = 0.0;
		sRC = SadGetParam(hDSC, mCAM_X_ASTIME, 0, 0, &dCAMTime, sizeof(DOUBLE));
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting parameter: %lx", ulRC);
		}

		spectrum.time = dCAMTime;

		adTimes[0] = 0.0;
		adTimes[1] = 0.0;
		sRC = SadGetListCAM(hDSC, 1, 0, 2, sizeof(DOUBLE), astParams, adTimes);
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting times: %lx", ulRC);
		}
		spectrum.times[0] = adTimes[0];
		spectrum.times[1] = adTimes[1];

		rQuant = 0.0;
		sRC = SadGetParam(hDSC, mCAM_F_SQUANT, 0, 0, &rQuant, sizeof(REAL));
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting quant.: %lx", ulRC);
		}

		spectrum.quant = rQuant;

		for (int j = 0; j < 1024; j++) {
			aulChanData1[j] = 0;
			aulChanData2[j] = 0;
			aulChanData3[j] = 0;
			aulChanData4[j] = 0;
		}
		sRC = SadGetSpectrum(hDSC, 1, 1024, FALSE, aulChanData1);
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting data: %lx", ulRC);
		}

		sRC = SadGetSpectrum(hDSC, 1025, 1024, FALSE, aulChanData2);
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting data: %lx", ulRC);
		}

		sRC = SadGetSpectrum(hDSC, 2049, 1024, FALSE, aulChanData3);
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting data: %lx", ulRC);
		}

		sRC = SadGetSpectrum(hDSC, 3073, 1024, FALSE, aulChanData4);
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting data: %lx", ulRC);
		}

		for (int j = 0; j < 1024; j++) {
			spectrum.data0[j] = aulChanData1[j];
			spectrum.data1025[j] = aulChanData2[j];
			spectrum.data2049[j] = aulChanData3[j];
			spectrum.data3073[j] = aulChanData4[j];
		}

		b_vector.push_back(spectrum);
		std::cout << "The data from spectrum " << Name_char << " have been added to vector\n\n";

		sRC = SadFlush(hDSC);
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error saving changes to file: %lx", ulRC);
		}

		sRC = SadCloseDataSource(hDSC);
	}

	return(b_vector);
}

int SumSpectra(const char* sumfile, std::vector<spectr> buff) {
	spectr sum;

	for (int x = 0; x < 1024; x++) {
		sum.data0[x] = 0;
		sum.data1025[x] = 0;
		sum.data2049[x] = 0;
		sum.data3073[x] = 0;
	}
	sum.quant = 0.0;
	sum.time = 0.0;
	sum.times[0] = 0.0;
	sum.times[1] = 0.0;

	for (size_t file = 0; file < buff.size(); file++) {
		for (int channel = 0; channel < 1024; channel++) {
			sum.data0[channel] = sum.data0[channel] + buff[file].data0[channel];
			sum.data1025[channel] = sum.data1025[channel] + buff[file].data1025[channel];
			sum.data2049[channel] = sum.data2049[channel] + buff[file].data2049[channel];
			sum.data3073[channel] = sum.data3073[channel] + buff[file].data3073[channel];
		}
		sum.times[0] = sum.times[0] + buff[file].times[0];
		sum.times[1] = sum.times[1] + buff[file].times[1];
	}
	sum.quant = buff[0].quant;
	std::cout << "Parameters have been summed in the vector\n\n";

	sRC = SadOpenDataSource(hDSC, sumfile,
		CIF_NativeSpect,
		ACC_Exclusive | ACC_SysWrite | ACC_ReadWrite,
		FALSE, "");
	if (sRC) {
		ulRC = ulSadStat(hDSC, sRC);
		printf("Error opening summed file: %lx", ulRC);
		return(1);
	}

	sRC = SadPutSpectrum(hDSC, 1, 1024, sum.data0);
	sRC = SadPutSpectrum(hDSC, 1025, 1024, sum.data1025);
	sRC = SadPutSpectrum(hDSC, 2049, 1024, sum.data2049);
	sRC = SadPutSpectrum(hDSC, 3073, 1024, sum.data3073);
	sRC = SadPutParam(hDSC, mCAM_F_SQUANT, 0, 0, &sum.quant, sizeof(REAL));
	sRC = SadPutListCAM(hDSC, 1, 0, 2, sizeof(DOUBLE), astParams, sum.times);
	std::cout << "Parameters have been written to the file\n\n";

	sRC = SadFlush(hDSC);
	if (sRC) {
		ulRC = ulSadStat(hDSC, sRC);
		printf("Error saving changes to SUM.CNF: %lx", ulRC);
		return(1);
	}

	sRC = SadCloseDataSource(hDSC);

	return 0;
}

int TimeCheck(std::vector<spectr> buff, std::vector<spectr> working, int index) {
	double diff = 0.0;
	if (buff.empty()) {
		diff = 1.0;
		std::cout << "The buffor is empty \n\n";
	}
	else {
		diff = working[index].time - buff[0].time;
		std::cout << "The time diff between the last spectrum (" << buff[0].path << ") and the new spectrum (" << working[index].path << "): " << diff << " \n\n";
	}

	if (diff <= 0.0) {
		std::cout << "The spectrum has been ignored \n\n";
		return 0;
	}
	else {
		if (diff < ThresholdTimeSec) return 1;
		else {
			buff.clear();
			std::cout << "The buffer has been cleared \n\n";
			return 1;
		}
	};
}

int AnalyzeSpectrum(const char* Spectrum) {
	BSTR File = _bstr_t(Spectrum);
	BSTR AFile = _bstr_t(SqnAn);

	CoInitialize(nullptr);
	HRESULT hr;

	IDataAccessPtr da;
	hr = da.CreateInstance(__uuidof(DataAccess));
	hr = da->Open(File, dReadWrite, 0);
	if (S_OK != hr) std::cout << "Opening error \n";

	ISequenceAnalyzerPtr seqAn;
	hr = seqAn.CreateInstance(__uuidof(SequenceAnalyzer));

	short step = 0;
	hr = seqAn->Analyze(da, &step, AFile, VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, NULL, NULL);
	if (S_OK != hr) std::cout << "Analyzing error \n";
	std::cout << "Last step of analyzis: " << step << " of 5 \n\n";

	hr = da->Flush();
	if (S_OK != hr) std::cout << "Flushing error \n";
	hr = da->Close(dUpdate);
	if (S_OK != hr) std::cout << "Closing error \n";

	return(0);

}

std::vector<nucl> GetNuclides(const char* SumSpectrum) {
	nucl b_Nuclide;
	nucl Sum;
	Sum.name = "TOTAL ACT.";
	Sum.mean = 0.0;
	Sum.mda = 0.0;
	Sum.error = 0.0;
	std::vector<nucl> b_vector;
	sRC = SadOpenDataSource(hDSC, SumSpectrum,
		CIF_NativeSpect,
		ACC_Exclusive | ACC_SysWrite | ACC_ReadWrite,
		FALSE, "");
	if (sRC) {
		ulRC = ulSadStat(hDSC, sRC);
		printf("Error opening NBSSTD.CNF: %lx", ulRC);
	}

	for (int n = 1; n <= NumberOfNuclides; n++) {
		double b_mean = 0.0;
		sRC = SadGetParam(hDSC, mCAM_G_NCLWTMEAN, n, 1, &b_mean, sizeof(DOUBLE));
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting quant.: %lx", ulRC);
		}
		b_Nuclide.mean = b_mean*37;
		
		char b_name[9];
		sRC = SadGetParam(hDSC, mCAM_T_NCLNAME, n, 1, &b_name, 9);
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting quant.: %lx", ulRC);
		}
		std::string b_sname = b_name;
		b_Nuclide.name = b_name;

		double b_error = 0.0;
		sRC = SadGetParam(hDSC, mCAM_G_NCLWTMERR, n, 1, &b_error, sizeof(DOUBLE));
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting quant.: %lx", ulRC);
		}
		b_Nuclide.error = b_error*37;
		
		double b_mda = 0.0;
		sRC = SadGetParam(hDSC, mCAM_G_NCLMDA, n, 1, &b_mda, sizeof(DOUBLE));
		if (sRC) {
			ulRC = ulSadStat(hDSC, sRC);
			printf("Error getting quant.: %lx", ulRC);
		}
		b_Nuclide.mda = b_mda*37;
		if (b_Nuclide.mda < b_Nuclide.mean) {
			Sum.mean = Sum.mean + b_Nuclide.mean;
			Sum.error = Sum.error + b_Nuclide.error;
		}

		b_vector.push_back(b_Nuclide);

	}
	b_vector.push_back(Sum);

	sRC = SadFlush(hDSC);
	if (sRC) {
		ulRC = ulSadStat(hDSC, sRC);
		printf("Error saving changes to SUM.CNF: %lx", ulRC);
	}

	sRC = SadCloseDataSource(hDSC);
	return b_vector;
}

int ShowResults(std::vector<nucl> vVector) {
	std::cout << "Nuclide" << "\t\t" << "Act. [kBq/m3]" << "\t" << "Error [kBq/m3]" << "\t" << "MDA [kBq/m3]" << "\n";
	unsigned k;
	for (k = 0; k < vVector.size()-1; k++) {
		std::cout << vVector[k].name << "\t\t";
		if (vVector[k].mean < vVector[k].mda) std::cout << " <MDA";
		else std::cout << vVector[k].mean;
		std::cout << "\t\t" << vVector[k].error << "\t\t" << vVector[k].mda << "\n";
	}
	std::cout << vVector[k].name << "\t";
	std::cout << vVector[k].mean;
	std::cout << "\t" << vVector[k].error << "\t\t" << "N/A" << "\n";

	return(0);
}

int DisplayResults(std::vector<nucl> v, std::string timestamp) {
	std::ofstream display("C:\\Mochawk\\Display.html");
	if (display.is_open())
	{
	
		display << "<!DOCTYPE html><html><head><style> table, th, td{ border: 1px solid black; text - align: center; } h2{ text - align: center; }</style></head><body><h2> " << timestamp << " </h2><table align = \"center\"><tr><th>Nuclide</th><th>Activity [kBq/m3]</th><th>Error [kBq/m3]</th><th>MDA [kBq/m3]</th></tr>";
		unsigned k;
		
		for (k = 0; k < v.size() - 1; k++) {
			display << "<tr>";
			display << "<td>" << v[k].name << "</td>";
			if (v[k].mean < v[k].mda) display << "<td>&ltMDA</td>";
			else display << "<td>" << v[k].mean << "</td>";
			display << "<td>" << v[k].error << "</td>";
			display << "<td>" << v[k].mda << "</td>";
			display << "</tr>";
		}
		display << "</table></body></html>";

		display.close();
	}
	return 0;
}

int LoadBackup(fs::path backup) {
	fs::remove_all("C:\\Mochawk\\Backup");
	try {
		fs::copy(Flush, "C:\\Mochawk\\Backup", fs::copy_options::overwrite_existing);
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}

	std::ifstream file(backup);
	fs::path p;
	fs::path rootto = "C:\\Mochawk\\Backup";
	fs::path rootfrom = "C:\\Mochawk\\Archive";
	while (file >> p) {
		fs::path to = rootto / p;
		fs::path from = rootfrom / p;
		try {
			fs::copy(from, to, fs::copy_options::overwrite_existing);
		}
		catch (std::exception& e)
		{
			std::cout << e.what();
		}
	}

	return 0;
}

std::vector<std::string> GetTimestamp(std::vector<spectr> v) {
	time_t LastTime = v[0].time - 3506716800u;
	struct tm LocalLastTime;
	LocalLastTime = *localtime(&LastTime);
	char DateTimeDisplay[80];
	char DateTimeExport[80];
	strftime(DateTimeDisplay, sizeof(DateTimeDisplay), "%Y-%m-%d %H:%M:%S", &LocalLastTime);
	strftime(DateTimeExport, sizeof(DateTimeExport), "%Y%m%d%H%M%S", &LocalLastTime);
	std::cout << "Date of analyzis: " << DateTimeDisplay << "\n";
	std::string t_exp = DateTimeExport;
	std::string t_disp = DateTimeDisplay;
	std::vector<std::string> timestamp;
	timestamp.push_back(t_exp);
	timestamp.push_back(t_disp);
	return timestamp;
}

int SaveBackup(std::vector<spectr> buffer) {
	std::ofstream backup("C:\\Mochawk\\backup.txt");
	if (backup.is_open())
	{
		for (unsigned i = 0; i < buffer.size(); i++) {
			backup << buffer[i].path << "\n";
		}
		backup.close();
	}

	return 0;
}

int WriteResults(std::vector<nucl> v, std::string timestamp) {
	std::ofstream results("C:\\Mochawk\\Results.txt");
	if (results.is_open())
	{
		results << timestamp << "\n"
			<< "3KBF11CR700_B33\t" << v[31].mean << "\t0\n" //total
			<< "3KBF11CR700_B06\t" << v[6].mean << "\t0\n" //Kr-85m
			<< "3KBF11CR700_B07\t" << v[7].mean << "\t0\n" //Kr-87
			<< "3KBF11CR700_B08\t" << v[8].mean << "\t0\n" //Kr-88
			<< "3KBF11CR700_B09\t" << v[9].mean << "\t0\n" //Kr-89
			<< "3KBF11CR700_B18\t" << v[15].mean << "\t0\n" //I-131
			<< "3KBF11CR700_B19\t" << v[16].mean << "\t0\n" //I-132
			<< "3KBF11CR700_B20\t" << v[17].mean << "\t0\n" //I-133
			<< "3KBF11CR700_B24\t" << v[21].mean << "\t0\n" //I-134
			<< "3KBF11CR700_B25\t" << v[22].mean << "\t0\n" //I-135
			<< "3KBF11CR700_B22\t" << v[18].mean << "\t0\n" //Xe-133
			<< "3KBF11CR700_B21\t" << v[19].mean << "\t0\n" //Xe-133m
			<< "3KBF11CR700_B26\t" << v[24].mean << "\t0\n" //Xe-135
			<< "3KBF11CR700_B28\t" << v[26].mean << "\t0\n" //Xe-137
			<< "3KBF11CR700_B30\t" << v[28].mean << "\t0\n" //Xe-138
			<< "3KBF11CR700_B23\t" << v[20].mean << "\t0\n" //Cs-134
			<< "3KBF11CR700_B27\t" << v[25].mean << "\t0\n" //Cs-137
			<< "3KBF11CR700_B29\t" << v[27].mean << "\t0\n"; //Cs-138

		results.close();
	}

	return 0;
}

void ExportFtp(LPCWSTR server, LPCWSTR user, LPCWSTR password, LPCWSTR path, LPCWSTR name, int threshold) {
	InternetCloseHandle(hInternet);
	hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	hFtpSession = InternetConnect(hInternet, server, INTERNET_DEFAULT_FTP_PORT, user, password, INTERNET_SERVICE_FTP, 0, 0);
	if (hFtpSession == NULL) {
		dwError = GetLastError();
		std::cout << "Error connection in export: " << dwError << "\n";
		DWORD dwInetError;
		DWORD dwExtLength = 1000;
		TCHAR *szExtErrMsg = NULL;
		TCHAR errmsg[1000];
		szExtErrMsg = errmsg;
		int returned = InternetGetLastResponseInfo(&dwInetError, szExtErrMsg, &dwExtLength);
		printf("dwInetError: %d  Returned: %d\n", dwInetError, returned);
		_tprintf(_T("Buffer: %s\n"), szExtErrMsg);
	}
	bool result = FALSE;
	int protector = 0;
	do {
		result = FtpPutFile(hFtpSession,
			path, name, FTP_TRANSFER_TYPE_ASCII, 0);
		if (result == FALSE) {
			dwError = GetLastError();
			std::cout << "Error sending file: " << dwError << "\n";
			DWORD dwInetError;
			DWORD dwExtLength = 1000;
			TCHAR *szExtErrMsg = NULL;
			TCHAR errmsg[1000];
			szExtErrMsg = errmsg;
			int returned = InternetGetLastResponseInfo(&dwInetError, szExtErrMsg, &dwExtLength);
			printf("dwInetError: %d  Returned: %d\n", dwInetError, returned);
			_tprintf(_T("Buffer: %s\n"), szExtErrMsg);

			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
		protector++;
		if (protector > threshold) break;

	} while (result == FALSE);
}

int main(int argc, char *argv[], char *envp[])
{
	std::chrono::high_resolution_clock::time_point StartPoint = std::chrono::high_resolution_clock::now();
	vG2KEnv();

	iRC = iUtlCreateFileDSC2(&hDSC, 0, 0);
	if (iRC) {
		printf("Error creating a VDM connection: %u", iRC);
		return(1);
	}
	
	fs::path BackupFile = "C:\\Mochawk\\backup.txt";
	LoadBackup(BackupFile);
	std::vector<spectr> buffer = SpectraVector("C:\\Mochawk\\Backup");
	sort(buffer.rbegin(), buffer.rend());
	
	size_t OldNewSpectra = CountSpectra(NewDir);
	if (OldNewSpectra < 5) {
		ImportFtp(server, user, password, 1, 1);
	} else {
		std::cout << "Too many old spectra in New folder, fresh spectra not downloaded \n";
	}
	
	try
	{
		fs::copy(NewDir, ArchiveDir, fs::copy_options::update_existing);

		auto end = std::chrono::system_clock::now();
		std::time_t end_time = std::chrono::system_clock::to_time_t(end);

		std::cout << std::ctime(&end_time) << "Data archived in Archive folder \n" << "\n";
	}
		catch (std::exception& e)
	{
		std::cout << e.what();
	}

	std::vector<spectr> vNewSpectra;
	vNewSpectra = SpectraVector(NewDir);
	sort(vNewSpectra.begin(), vNewSpectra.end());
	std::cout << "The vector has been sorted \n\n";
		
	int NumberOfSpectra = 0;
	NumberOfSpectra = vNewSpectra.size();
	std::cout << "Amount of new spectra: " << NumberOfSpectra << "\n";

	for (int i = 0; i < NumberOfSpectra; i++) {

		if (TimeCheck(buffer, vNewSpectra, i) == 1) {
			buffer.push_back(vNewSpectra[i]);
			std::rotate(buffer.rbegin(), buffer.rbegin() + 1, buffer.rend());
			if (buffer.size() > SpectraLimit) buffer.pop_back();
			if (buffer.size() >= SpectraLimit) {
				SumSpectra(dSumSpectrum, buffer);
				AnalyzeSpectrum(dSumSpectrum);
				std::vector<nucl> vNuclides;
				vNuclides = GetNuclides(dSumSpectrum);
				std::vector<std::string> timestamp;
				timestamp = GetTimestamp(buffer);
				ShowResults(vNuclides);
				DisplayResults(vNuclides, timestamp[1]);
				SaveBackup(buffer);
				WriteResults(vNuclides, timestamp[0]);
				ExportFtp(server, user, password, L"C:\\Mochawk\\Results.txt", L"Results.txt", 1);
				KillVector(vNuclides);
			}
		}
	}

	SaveBackup(buffer);
	KillVector(vNewSpectra);
	try
	{
		fs::remove_all(NewDir);
		std::this_thread::sleep_for(1s);
		fs::copy(Flush, NewDir, fs::copy_options::overwrite_existing);
	
		auto end = std::chrono::system_clock::now();
		std::time_t end_time = std::chrono::system_clock::to_time_t(end);
		std::cout << std::ctime(&end_time) << "The directory New is empty \n\n";
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
	
	if (CountSpectra(ArchiveDir) > 100) {
		std::priority_queue<fs::directory_entry, pathvec, older_file> newestN;
		for (auto entry : fs::directory_iterator(ArchiveDir))
		{
			newestN.push(entry);
			if (newestN.size() > 15)
			{
				fs::remove_all(newestN.top());
				newestN.pop();
			}
		}
		std::cout << "Old spectra in Archive have been removed\n\n";
	}

	std::cout << "*************************** END OF THE MAIN LOOP ***************************\n\n";
	std::chrono::high_resolution_clock::time_point EndPoint = std::chrono::high_resolution_clock::now();
	auto LoopDuration = std::chrono::duration_cast<std::chrono::minutes>(EndPoint - StartPoint);
	std::this_thread::sleep_for(std::chrono::minutes(max(0min, NominalWaitingTime-LoopDuration)));
		
	sRC = SadDeleteDSC(hDSC);
	
	return 0;
}
