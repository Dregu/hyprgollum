#pragma once

#include <string_view>
#include <expected>

#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/managers/HookSystemManager.hpp>

namespace Layout {
    class CAlgorithm;
}

namespace Layout::Tiled {
    struct SGollumData;

    class CGollumAlgorithm : public ITiledAlgorithm {
      public:
        CGollumAlgorithm()          = default;
        virtual ~CGollumAlgorithm() = default;

        virtual void                             newTarget(SP<ITarget> target);
        virtual void                             movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint = std::nullopt);
        virtual void                             removeTarget(SP<ITarget> target);

        virtual void                             resizeTarget(const Vector2D& Δ, SP<ITarget> target, eRectCorner corner = CORNER_NONE);
        virtual void                             recalculate();

        virtual void                             swapTargets(SP<ITarget> a, SP<ITarget> b);
        virtual void                             moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent);

        virtual std::expected<void, std::string> layoutMsg(const std::string_view& sv);
        virtual std::optional<Vector2D>          predictSizeForNewTarget();

        virtual SP<ITarget>                      getNextCandidate(SP<ITarget> old);

      private:
        //std::vector<SP<SGollumData>> m_gollumData;
    };
}
