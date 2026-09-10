#include "ecu/can_profile_registry.h"

#include <cstring>

#include "ecu/profiles/profile_declarations.h"

namespace {
const CanProfile* const kProfiles[] = {
    &kEcumasterEmuBlackProfile,
    &kRusefiVerboseProfile,
    &kMaxxEcuDefaultProfile,
    &kHaltechBroadcastProfile,
    &kSpeeduinoHaltechProfile,
    &kBmwMs43StockProfile,
    &kLinkGenericDashProfile,
    &kPsaC2VtsProfile,
};
}  // namespace

std::size_t CanProfileRegistry::count() {
    return sizeof(kProfiles) / sizeof(kProfiles[0]);
}

const CanProfile* CanProfileRegistry::at(std::size_t index) {
    return index < count() ? kProfiles[index] : nullptr;
}

const CanProfile* CanProfileRegistry::find(const char* id) {
    if (id == nullptr || id[0] == '\0') {
        return nullptr;
    }
    for (const CanProfile* profile : kProfiles) {
        if (std::strcmp(profile->id, id) == 0) {
            return profile;
        }
    }
    return nullptr;
}
