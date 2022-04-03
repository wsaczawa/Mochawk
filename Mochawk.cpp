#include "pch.h"

#include "genie.h"
#include "dataFTP.h"

int main(int argc, char* argv[], char* envp[])
{
	folders dir;

	CNFset data;
	data.getConst("./config.csv");
	data.load(dir.backup);
	data.getSpectra(dir.Buffer);
	std::sort(data.spectra.rbegin(), data.spectra.rend());

	dataFTP cred("./ftp.csv");
	if (dir.count(dir.New) < 5) {
		ftp impU3(cred.login[0]);
		impU3.download();
		ftp impU4(cred.login[1]);
		impU4.download();
	}

	dir.upgrade(dir.New, dir.Archive);

	CNFset newCNF;
	newCNF.getSpectra(dir.New);
	std::sort(newCNF.spectra.begin(), newCNF.spectra.end());

	unsigned i = 0;
	while (i < newCNF.spectra.size()) {
		if (data.check(newCNF.spectra[i])) {
			data.append(newCNF.spectra[i]);
			
			analysis sum(data);
			sum.getNuclides(fs::current_path().string() + "/" + newCNF.spectra[i].name, data.asf);
			sum.process();
			sum.getTimestamp();
			sum.writeFTP(dir.signals);
			sum.writeHTML();

			data.save(dir.backup);
		}

		dir.manage(newCNF.spectra[i].name);
		i++;
	}

	ftp expU3(cred.login[2]);
	expU3.cd(expU3.login.dir);
	dir.send(expU3);
	expU3.bye();

	ftp expU4(cred.login[3]);
	expU4.cd(expU4.login.dir);
	dir.send(expU4);
	expU4.bye();

	if (dir.count(dir.Archive) > 100)
		dir.clear(dir.Archive, 15);
	if (dir.count(dir.Summed) > 100)
		dir.clear(dir.Summed, 15);

	return 0;
}