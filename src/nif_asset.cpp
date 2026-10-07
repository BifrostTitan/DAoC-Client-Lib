#include "daoc_client_lib/nif_asset.h"

#include <sstream>
#include <stdexcept>

namespace daoc_client_lib
{
    std::unique_ptr<niflib::NiFile> load_nif(const MpkArchive &archive, const std::string &entry_name)
    {
        const auto *entry = archive.find(entry_name);
        if (entry == nullptr)
        {
            throw std::out_of_range("NIF entry not found in MPK archive: " + entry_name);
        }

        const std::string bytes(entry->data.begin(), entry->data.end());
        std::istringstream input(bytes, std::ios::in | std::ios::binary);
        return std::make_unique<niflib::NiFile>(input);
    }
}