#pragma once

#include "daoc_client_lib/zone_data.h"
#include "daoc_client_lib/zone_geometry.h"

#include <map>
#include <optional>
#include <set>
#include <vector>

namespace daoc_client_lib
{
    class ZoneGeometryChooser
    {
    public:
        ZoneGeometryChooser(const ClientDataDirectory& client, const std::vector<ZoneData>& zones);
        static ZoneGeometryChooser load(const ClientDataDirectory& client);

        std::optional<ZoneGeometry> find(int zone_id) const;

    private:
        ClientDataDirectory client_;
        std::map<int, ZoneType> zone_types_;
        std::set<int> available_zone_ids_;
    };
}