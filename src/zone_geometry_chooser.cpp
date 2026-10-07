#include "daoc_client_lib/zone_geometry_chooser.h"

#include "daoc_client_lib/zone_data.h"

#include <charconv>
#include <string>

namespace daoc_client_lib
{
    namespace
    {
        bool zone_directory_id(const std::filesystem::path &path, int &id)
        {
            auto directory = path.parent_path().filename().string();
            for (auto &character : directory)
            {
                if (character >= 'A' && character <= 'Z')
                {
                    character = static_cast<char>(character + ('a' - 'A'));
                }
            }
            if (directory.size() <= 4 || directory.size() > 7 || directory.substr(0, 4) != "zone")
            {
                return false;
            }
            const auto result = std::from_chars(directory.data() + 4, directory.data() + directory.size(), id);
            return result.ec == std::errc{} && result.ptr == directory.data() + directory.size();
        }
    }

    ZoneGeometryChooser::ZoneGeometryChooser(
        const ClientDataDirectory &client,
        const std::vector<ZoneData> &zones)
        : client_(client)
    {
        for (const auto &zone : zones)
        {
            zone_types_.emplace(zone.id, static_cast<ZoneType>(zone.type));
        }
        for (const auto &file : client_.files())
        {
            int id = 0;
            if (zone_directory_id(file, id))
            {
                available_zone_ids_.insert(id);
            }
        }
    }

    ZoneGeometryChooser ZoneGeometryChooser::load(const ClientDataDirectory &client)
    {
        return ZoneGeometryChooser(client, read_zones(client));
    }

    std::optional<ZoneGeometry> ZoneGeometryChooser::find(int zone_id) const
    {
        const auto type = zone_types_.find(zone_id);
        if (type == zone_types_.end() || available_zone_ids_.find(zone_id) == available_zone_ids_.end())
        {
            return std::nullopt;
        }
        return ZoneGeometry(zone_id, client_, type->second);
    }
}