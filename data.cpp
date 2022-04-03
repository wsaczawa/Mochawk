#include "pch.h"

#include "dataFTP.h"

struct older_file {
	bool operator()(const fs::directory_entry& p, const fs::directory_entry& p2)
	{
		return p.last_write_time() > p2.last_write_time();
	}
};

std::string value(std::string line) {
	return line.substr(line.find(',') + 1, line.length() - line.find(','));
}

folders::folders() {
	signals.clear();
	std::wifstream f;
	std::wstring line;
	f.open("./signals.csv");
	while (std::getline(f, line)) {
		std::pair<std::wstring, std::wstring> signal;
		signal.first = getcell(line, 1);
		signal.second = getcell(line, 2);
		signals.push_back(signal);
	}
}

void folders::manage(std::string file) {
	fs::remove(New / file);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	if (fs::exists(file)) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		fs::copy_file(file, Summed / file, fs::copy_options::update_existing);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		fs::remove(file);
	}
}

void folders::upgrade(fs::path from, fs::path to) {
	try
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		fs::copy(from, to, fs::copy_options::update_existing);
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

void folders::send(ftp exp) {
	for (auto& p : fs::directory_iterator("./export/")) {
		if (exp.put(L"./export/" + p.path().wstring(), p.path().wstring())) {
			fs::remove(p.path());
		}
	}
}

void folders::flush(fs::path d) {
	try
	{
		fs::remove_all(d);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		fs::copy(Flush, d, fs::copy_options::overwrite_existing);
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

void folders::clear(fs::path d, int last) {
	std::priority_queue<fs::directory_entry, pathvec, older_file> newest;
	for (auto entry : fs::directory_iterator(d))
	{
		newest.push(entry);
		if (newest.size() > last)
		{
			fs::remove_all(newest.top());
			newest.pop();
		}
	}
}

std::size_t folders::count(fs::path d) {
	using fs::directory_iterator;
	return std::distance(directory_iterator(d), directory_iterator{});
}

spectrum::spectrum() {
		duration = 0.0;
		start = 0.0;
		time = 0.0;
		preset = 0.0;
		vol = 0.0;
}

bool spectrum::operator<(const spectrum& a) const {
		return time < a.time;
}

CNFset::CNFset() {
		spectra.clear();
		downLimit = 0.0;
		spectraTime = 0.0;
		spectraLimit = 0;
		plus = 0;
		minus = 0;
}

void CNFset::getConst(fs::path csv) {
		std::ifstream f;
		f.open(csv);
		std::string s;
		std::getline(f, s);
		downLimit = stod(value(s));
		std::getline(f, s);
		spectraTime = stod(value(s));
		std::getline(f, s);
		spectraLimit = stoi(value(s));
		std::getline(f, s);
		asf = value(s);
		std::getline(f, s);
		f.close();
}

void CNFset::getSpectra(fs::path folder) {
		spectra.clear();
		for (auto& p : fs::directory_iterator(folder)) {
			spectrum s;
			s.name = p.path().filename().string();
			s.getData(fs::absolute(p).string());
			if(s.time > 0)
			spectra.push_back(s);
		}

}

void CNFset::load(fs::path backup) {
		try {
			fs::remove_all("./Buffer/");
			fs::copy("./Flush/", "./Buffer/", fs::copy_options::overwrite_existing);
		}
		catch (std::exception& e)
		{
			std::cout << e.what();
		}

		std::ifstream f;
		std::string p;
		f.open(backup);
		while (std::getline(f, p)) {
			try {
				fs::copy("./Archive/" + p, "./Buffer/" + p, fs::copy_options::overwrite_existing);
			}
			catch (std::exception& e)
			{
				std::cout << e.what();
			}
		}
		f.close();
}

void CNFset::save(fs::path backup) {
		std::ofstream f(backup);
		if (f.is_open())
		{
			for (unsigned i = 0; i < spectra.size(); i++) {
				f << spectra[i].name << std::endl;
			}
			f.close();
		}
}

void CNFset::append(spectrum s) {
		spectra.push_back(s);
		std::rotate(spectra.rbegin(), spectra.rbegin() + 1, spectra.rend());
		if (spectra.size() > spectraLimit + 1) spectra.pop_back();
	}

bool CNFset::check(spectrum s) {
		if (spectra.empty()) return TRUE;
		if (s.time - spectra[0].time < downLimit) {
			return FALSE;
		}
		else {
			return TRUE;
		}
}

nuclide::nuclide() {
		mean = 0.0;
		error = 0.0;
		mda = 0.0;
	}

analysis::analysis(CNFset set) {
		unsigned i;
		int plus = 0,
			minus = 0;

		for (i = 1; i < set.spectra.size(); i++) {
			if (set.spectra[0].duration < set.spectra[i].duration) {
				if (set.spectra[0].time - set.spectra[i].time < set.spectraTime * ((long long)set.spectraLimit + 0.5)) {
					plus = i;
				}
				break;
			}
		}

		for (i = set.spectra.size() - 1; i >= plus; i--) {
			if (set.spectra[0].time - set.spectra[i].time < set.spectraTime * ((long long)set.spectraLimit + 0.5)) {
				minus = i;
				break;
			}
		}
		if (set.spectra[0].duration < set.spectraTime * ((long long)set.spectraLimit + 0.5) && plus == 0) {
			minus = 0;
		}

		fs::copy_file("./New/" + set.spectra[0].name, set.spectra[0].name, fs::copy_options::overwrite_existing);

	
		s = set.spectra[0];
		if (minus != 0 && plus != minus) {
			if (plus != 0) {
				strip(set.spectra[0].name, set.spectra[plus].name, -1);
				s.duration = +set.spectra[plus].duration;
				s.preset = +set.spectra[plus].preset;
			}
			strip(set.spectra[0].name, set.spectra[minus].name, 1);
			s.duration = -set.spectra[minus].duration;
			s.preset = -set.spectra[plus].preset;
		}
}

void analysis::strip(std::string sumFile, std::string addFile, int f) {
		std::string here = fs::current_path().string() + "/";
		std::string command = "strip.exe \"" + here + sumFile + "\" \"" + here + "Archive\\" + addFile + "\" /FACTOR=" + std::to_string(f);
		std::system(command.c_str());
}

void analysis::getTimestamp() {
	time_t epoch = s.time - 3506716800u;
	struct tm localTime = *gmtime(&epoch);
	std::wostringstream strTime;
	strTime << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S");
	tsDisp = strTime.str();
	strTime.str(std::wstring());
	strTime << std::put_time(&localTime, L"%Y%m%d%H%M%S");
	tsExp = strTime.str();
}

void analysis::process() {
		nuclide t;
		t.name = L"total";
		for (unsigned i = 0; i < nuclides.size(); i++) {
			t.mean = t.mean + nuclides[i].mean;
			t.mda = t.mda + nuclides[i].mda;
			t.error = t.error + nuclides[i].error * nuclides[i].error;
		}
		t.error = sqrt(t.error);
		nuclides.push_back(t);
}

void analysis::writeFTP(std::vector<std::pair<std::wstring, std::wstring>> signals) {
		std::wofstream f;
		f.open(L"./export/" + tsExp + L".txt");
		std::string det = s.name.substr(0, s.name.find("_"));
		std::wstring detector(det.begin(), det.end()); //it works only for ASCII!
		f << tsExp << std::endl;
		for (unsigned i = 0; i < signals.size(); i++) {
			std::wstring signal = signals[i].second;
			auto that_nuclide = [signal](const nuclide& n) {return n.name == signal; };
			int code = 1;
			double value = 0.0;
			auto it = std::find_if(nuclides.begin(), nuclides.end(), that_nuclide);
			if (it != nuclides.end()) {
				code = 0;
				value = it->mean;
				if (it->mean < it->mda) {
					code = 1;
				}
			}
			f << detector << "_" << signals[i].first << "\t" << value << "\t" << code << std::endl;
		}
		f.close();
	}

void analysis::writeHTML() {
		std::wofstream f;
		f.open("./display.html");
		f << L"<!DOCTYPE html><html><head><style> table, th, td{ border: 1px solid black; text - align: center; } h2{ text - align: center; }</style></head><body><h2> ";
		f << tsDisp;
		f << L" </h2><table align = \"center\"><tr><th>Nuclide</th><th>Activity [kBq/m3]</th><th>Error [kBq/m3]</th><th>MDA [kBq/m3]</th></tr>";
		for (unsigned i = 0; i < nuclides.size(); i++) {
			f << L"<tr>";
			f << L"<td>" << nuclides[i].name << L"</td>";
			if (nuclides[i].mean < nuclides[i].mda) {
				f << L"<td>&ltMDA</td>";
			}
			else {
				f << L"<td>" << nuclides[i].mean << L"</td>";
			}
			f << L"<td>" << nuclides[i].error << L"</td>";
			f << L"<td>" << nuclides[i].mda << L"</td>";
			f << L"</tr>";
		}
		f << L"</table></body></html>";
		f.close();
	}