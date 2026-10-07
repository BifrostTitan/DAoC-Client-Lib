#pragma once

#include "daoc_client_lib/mpk_archive.h"
#include "niflib/ni_file.h"

#include <memory>
#include <string>

namespace daoc_client_lib
{
    std::unique_ptr<niflib::NiFile> load_nif(const MpkArchive& archive, const std::string& entry_name);
}