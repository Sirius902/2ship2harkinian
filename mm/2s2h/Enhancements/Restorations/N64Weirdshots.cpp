#include <libultraship/libultraship.h>
#include <libultraship/bridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "Context.h"

#include <spdlog/spdlog.h>

extern "C" {
#include "functions.h"
#include "objects/gameplay_keep/gameplay_keep.h"

int ResourceMgr_OTRSigCheck(char* imgData);
AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path);
}

#define CVAR_NAME "gEnhancements.Restorations.N64Weirdshots"
#define CVAR CVarGetInteger(CVAR_NAME, 0)

void DoIt(PlayerAnimationHeader* animation, std::int32_t frame, std::int32_t limbCount, Vec3s* frameTable, bool* should) {
    if (BEN_ANIM_EQUAL(reinterpret_cast<const char*>(animation), gPlayerAnim_link_bow_side_walk)) {
        if (ResourceMgr_OTRSigCheck(reinterpret_cast<char*>(animation)) != 0)
            animation = reinterpret_cast<PlayerAnimationHeader*>(
                ResourceMgr_LoadAnimByName(reinterpret_cast<const char*>(animation)));

        auto playerAnimHeader =
            static_cast<PlayerAnimationHeader*>(Lib_SegmentedToVirtual(static_cast<void*>(animation)));

        if (frame >= playerAnimHeader->common.frameCount) {
            frame -= playerAnimHeader->common.frameCount;
            
            const auto weirdshotAnimation = Ship::Context::GetInstance()->GetResourceManager()->LoadResource("__OTR__misc/weirdshot/gAnimation");

            if ((sizeof(Vec3s) * limbCount + sizeof(std::int16_t)) * frame < weirdshotAnimation->GetPointerSize()) {
                *should = false;

                SPDLOG_DEBUG("Weirdshot: frame {}", frame + playerAnimHeader->common.frameCount);

                std::memcpy(frameTable,
                            reinterpret_cast<const std::uint8_t*>(weirdshotAnimation->GetRawPointer()) +
                                (((sizeof(Vec3s) * limbCount + sizeof(std::int16_t)) * frame)),
                            sizeof(Vec3s) * limbCount + sizeof(std::int16_t));
            } else {
                SPDLOG_WARN("Weirdshot frame {} not included in data, more frames may need to be dumped", frame);
            }
        }
    }
}

void RegisterN64Weirdshots() {
    COND_VB_SHOULD(VB_LOAD_PLAYER_ANIMATION_FRAME, CVAR, {
        auto animation = va_arg(args, PlayerAnimationHeader*);
        auto frame = va_arg(args, std::int32_t);
        const auto limbCount = va_arg(args, std::int32_t);
        const auto frameTable = va_arg(args, Vec3s*);

        DoIt(animation, frame, limbCount, frameTable, should);
    });
}

static RegisterShipInitFunc initFunc(RegisterN64Weirdshots, { CVAR_NAME });
