#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#include <string_view>
#include <expected>
#include <deque>
#include <unordered_map>

#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/config/ConfigValue.hpp>

namespace Layout {
    class CAlgorithm;
}

namespace Layout::Tiled {

    struct SGollumData {
        SGollumData(SP<ITarget> t) : target(t) {
            ;
        }
        SGollumData(SP<ITarget> t, bool f) : target(t), fs(f) {
            ;
        }

        WP<ITarget> target;
        CBox        box;
        bool        fs{false};
    };

    class CGollumAlgorithm : public ITiledAlgorithm {
      public:
        CGollumAlgorithm(HANDLE h) : PHANDLE(h) {};
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
        HANDLE                                       PHANDLE;
        std::deque<SP<SGollumData>>                  m_gollumData;
        std::unordered_map<std::string, std::string> m_gollumOpt;
        std::string                                  m_next;

        SP<SGollumData>                              dataFor(SP<ITarget> t);
        SP<SGollumData>                              getClosestNode(const Vector2D&);

        std::string                                  getStrOpt(const std::string& opt);
        Hyprlang::INT                                getIntOpt(const std::string& opt);
        Hyprlang::VEC2                               getVec2Opt(const std::string& opt);
    };
}
