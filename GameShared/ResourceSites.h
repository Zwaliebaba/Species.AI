#pragma once

#include "TerrainField.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

struct RegionLayout;

// WLD-080: the classes of resource site the MVP's region has; it has no crossing, so no crossing resource site (MVP-010).
enum class ResourceSiteClass : std::uint8_t
{
  Home,
  Border,
  Central
};

/// One resource site of a region (WLD-080 to WLD-085): world data, placed by the world generator from the region's
/// layout and never moved (WLD-084). Its units are its yield in home resource sites (WLD-081): 1 for a home resource
/// site, wld.resource.border_factor for a border one and wld.resource.central_factor for the central one. It faces the
/// centre of the site it belongs to, and the central resource site faces site 0.
struct ResourceSitePlace
{
  ResourceSiteClass siteClass = ResourceSiteClass::Home;
  std::optional<std::uint8_t> site; // the local site whose own it is (WLD-085); none for the central resource site
  Neuron::Vec3 position;            // on the terrain
  Neuron::Vec3 front;               // horizontal, of unit length
  std::uint32_t units = 1;
};

// WLD-080 to WLD-085 for the MVP's one region: for each local site in order, its home resource sites at the inner
// radius and then at the outer radius, each ring in rising bearing from the site's outward diagonal, then the ninth
// (no site of the MVP has a crossing) at the extra radius and wld.resource.extra_bearing (DEC-1000), then the resource
// sites at the centres of its two valley border areas, towards the neighbour across x first; and last the central
// resource site at the valley's centre. A ring of n sites stands at bearings 180°/n apart, symmetric about the
// diagonal: WLD-082's −60°, 0°, +60° for 3 and −72° to +72° for 5. A bearing turns from +x towards +z, as the layout's
// rotation does. A border area's centre is the centroid of the half of the lens of WLD-062 that lies nearer the site.
// Each stands at the terrain's height.
[[nodiscard]] std::vector<ResourceSitePlace> PlaceResourceSites(const RegionLayout& _layout, const Neuron::TerrainField& _field,
                                                                const TuningMvp& _tuning);

// MVP-053 (ADR-167): the centre of a claim's area on the ground, where PlaceResourceSites puts its resource site: for a
// half of a valley border area (VALLEY_PAIRS index × 2 + side) the centroid of the half that lies nearer its side's
// site, and for the central resource site (CLAIM_AREA_CENTRAL) the valley's centre. None for an area beyond the central
// one, or a half whose two discs do not overlap.
[[nodiscard]] std::optional<Neuron::Vec3> ClaimAreaCenter(const RegionLayout& _layout, const Neuron::TerrainField& _field,
                                                          const TuningMvp& _tuning, std::uint8_t _area);

// WLD-163 check 7 with the MVP's one resource kind (O57): the home resource sites of every local site yield the same
// within wld.resource.home_yield_tolerance, the difference between the largest and the smallest total at most that
// share of the largest. A resource site that stands at or below the shore height or on ground steeper than walkable
// yields nothing, since no Worker gathers there (WLD-002, DEC-1000).
[[nodiscard]] bool HomeYieldsAgree(std::span<const ResourceSitePlace> _places, const Neuron::TerrainField& _field,
                                   const TuningMvp& _tuning);

} // namespace Species
