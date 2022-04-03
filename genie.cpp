#include "pch.h"

#include "genie.h"

void analysis::getNuclides(std::string summFile, std::string asf) {
	CoInitialize(nullptr);
	HRESULT hr;
	IDataAccessPtr dat;
	ISequenceAnalyzerPtr seq;
	short step = 0;

	hr = dat.CreateInstance(__uuidof(DataAccess));
	hr = seq.CreateInstance(__uuidof(SequenceAnalyzer));

	hr = dat->Open((_bstr_t)summFile.c_str(), dReadWrite, 0);
	hr = dat->put_Param(CanberraDataAccessLib::CAM_X_EREAL, 0, 0, (_variant_t) s.duration);
	hr = dat->put_Param(CanberraDataAccessLib::CAM_X_ELIVE, 0, 0, (_variant_t) s.preset);
	hr = dat->Flush();
	hr = seq->Analyze(dat, &step, (_bstr_t)asf.c_str(), VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE, NULL, NULL);
	hr = dat->Flush();
	int i = 1;
	_variant_t vVar;
	while (S_OK == dat->get_Param(CanberraDataAccessLib::CAM_T_NCLNAME, i, 0, &vVar))
	{
		nuclide nuc;

		nuc.name = vVar.bstrVal;
		hr = dat->get_Param(CanberraDataAccessLib::CAM_G_NCLWTMEAN, i, 0, &vVar);
		nuc.mean = vVar;
		nuc.mean = nuc.mean * 37; // 37 factor for kBq/m3
		hr = dat->get_Param(CanberraDataAccessLib::CAM_G_NCLWTMERR, i, 0, &vVar);
		nuc.error = vVar;
		nuc.error = nuc.mean * 37; // 37 factor for kBq/m3
		hr = dat->get_Param(CanberraDataAccessLib::CAM_G_NCLMDA, i, 0, &vVar);
		nuc.mda = vVar;
		nuc.mda = nuc.mean * 37; // 37 factor for kBq/m3

		nuclides.push_back(nuc);
		i++;
	}

	hr = dat->Close(dUpdate);
}

void spectrum::getData(std::string cnf) {

	HMEM hDSC;
	iUtlCreateFileDSC2(&hDSC, 0, 0);
	SadOpenDataSource(hDSC, cnf.c_str(), CIF_NativeSpect, ACC_ReadOnly, FALSE, "");
	SadGetParam(hDSC, mCAM_X_ASTIME, 0, 0, &start, sizeof(DOUBLE));
	SadCloseDataSource(hDSC);
	SadDeleteDSC(hDSC);

	CoInitialize(nullptr);
	HRESULT hr;
	IDataAccessPtr dat;
	_variant_t vVar;

	hr = dat.CreateInstance(__uuidof(DataAccess));

	hr = dat->Open((_bstr_t)cnf.c_str(), dReadWrite, 0);
	hr = dat->get_Param(CanberraDataAccessLib::CAM_X_EREAL, 0, 0, &vVar);
	duration = vVar;
	hr = dat->get_Param(CanberraDataAccessLib::CAM_X_ELIVE, 0, 0, &vVar);
	preset = vVar;
	hr = dat->get_Param(CanberraDataAccessLib::CAM_F_SQUANT, 0, 0, &vVar);
	vol = vVar;
	hr = dat->Close(dNoUpdate);
	time = start + duration;
}