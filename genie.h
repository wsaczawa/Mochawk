#pragma once

#include "dataFTP.h"

#include <vector>
#include <string>
#include <windows.h>
#include "crackers.h"

#include "citypes.h"

#include "spasst.h"
#include "sad.h"
#include "ci_files.h"
#include "Sad_nest.h"
#include "utility.h"

#pragma comment(lib, "User32.lib")

#import  "Analyze.tlb" raw_interfaces_only, raw_native_types, named_guids
#import  "DataAccess.tlb" raw_interfaces_only, raw_native_types, named_guids

#define mCAM_X_ASTIME 0X20000015L

using namespace CanberraSequenceAnalyzerLib;
using namespace CanberraDataAccessLib;