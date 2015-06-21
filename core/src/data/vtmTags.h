#include <type_traits>

namespace oscim {
    const char* keys[] = {
        "access", "addr:housename", "addr:housenumber", "addr:interpolation",
        "admin_level", "aerialway", "aeroway", "amenity", "area", "barrier",
        "bicycle", "brand", "bridge", "boundary", "building", "construction",
        "covered", "culvert", "cutting", "denomination", "disused",
        "embankment", "foot", "generator:source", "harbour", "highway",
        "historic", "horse", "intermittent", "junction", "landuse", "layer",
        "leisure", "lock", "man_made", "military", "motorcar", "name",
        "natural", "oneway", "operator", "population", "power", "power_source",
        "place", "railway", "ref", "religion", "route", "service", "shop",
        "sport", "surface", "toll", "tourism", "tower:type", "tracktype",
        "tunnel", "water", "waterway", "wetland", "width", "wood", "height",
        "min_height", "roof:shape", "roof:height", "rank"};

    const char* values[] = {
        "yes", "residential", "service", "unclassified", "stream", "track",
        "water", "footway", "tertiary", "private", "tree", "path", "forest",
        "secondary", "house", "no", "asphalt", "wood", "grass", "paved",
        "primary", "unpaved", "bus_stop", "parking", "parking_aisle", "rail",
        "driveway", "8", "administrative", "locality", "turning_circle",
        "crossing", "village", "fence", "grade2", "coastline", "grade3",
        "farmland", "hamlet", "hut", "meadow", "wetland", "cycleway", "river",
        "school", "trunk", "gravel", "place_of_worship", "farm", "grade1",
        "traffic_signals", "wall", "garage", "gate", "motorway",
        "living_street", "pitch", "grade4", "industrial", "road", "ground",
        "scrub", "motorway_link", "steps", "ditch", "swimming_pool", "grade5",
        "park", "apartments", "restaurant", "designated", "bench",
        "survey_point", "pedestrian", "hedge", "reservoir", "riverbank",
        "alley", "farmyard", "peak", "level_crossing", "roof", "dirt", "drain",
        "garages", "entrance", "street_lamp", "deciduous", "fuel", "trunk_link",
        "information", "playground", "supermarket", "primary_link", "concrete",
        "mixed", "permissive", "orchard", "grave_yard", "canal", "garden",
        "spur", "paving_stones", "rock", "bollard", "convenience", "cemetery",
        "post_box", "commercial", "pier", "bank", "hotel", "cliff", "retail",
        "construction", "-1", "fast_food", "coniferous", "cafe", "6",
        "kindergarten", "tower", "hospital", "yard", "sand", "public_building",
        "cobblestone", "destination", "island", "abandoned", "vineyard",
        "recycling", "agricultural", "isolated_dwelling", "pharmacy",
        "post_office", "motorway_junction", "pub", "allotments", "dam",
        "secondary_link", "lift_gate", "siding", "stop", "main",
        "farm_auxiliary", "quarry", "10", "station", "platform", "taxiway",
        "limited", "sports_centre", "cutline", "detached", "storage_tank",
        "basin", "bicycle_parking", "telephone", "terrace", "town", "suburb",
        "bus", "compacted", "toilets", "heath", "works", "tram", "beach",
        "culvert", "fire_station", "recreation_ground", "bakery", "police",
        "atm", "clothes", "tertiary_link", "waste_basket", "attraction",
        "viewpoint", "bicycle", "church", "shelter", "drinking_water", "marsh",
        "picnic_site", "hairdresser", "bridleway", "retaining_wall",
        "buffer_stop", "nature_reserve", "village_green", "university", "1",
        "bar", "townhall", "mini_roundabout", "camp_site", "aerodrome", "stile",
        "9", "car_repair", "parking_space", "library", "pipeline", "true",
        "cycle_barrier", "4", "museum", "spring", "hunting_stand", "disused",
        "car", "tram_stop", "land", "fountain", "hiking", "manufacture",
        "vending_machine", "kiosk", "swamp", "unknown", "7", "islet", "shed",
        "switch", "rapids", "office", "bay", "proposed", "common", "weir",
        "grassland", "customers", "social_facility", "hangar", "doctors",
        "stadium", "give_way", "greenhouse", "guest_house", "viaduct",
        "doityourself", "runway", "bus_station", "water_tower", "golf_course",
        "conservation", "block", "college", "wastewater_plant", "subway",
        "halt", "forestry", "florist", "butcher"};

    const size_t ATTRIB_OFFSET = 256;
    const size_t MAX_VAL = std::extent<decltype(values)>::value;
    const size_t MAX_KEY = std::extent<decltype(keys)>::value;
}
