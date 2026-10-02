#pragma once

class Framework;

namespace sailfish
{
// The single Framework instance owned by OrganicMapsMain(); valid while QML types exist.
Framework & GetFramework();
}  // namespace sailfish
