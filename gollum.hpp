#pragma once

#include <string_view>
#include <expected>
#include <deque>
#include <unordered_map>
#include <map>

#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/managers/HookSystemManager.hpp>
#include <hyprland/src/config/ConfigValue.hpp>

namespace Layout {
    class CAlgorithm;
}

namespace Layout::Tiled {

    struct SGollumData {
        SGollumData(SP<ITarget> t) : target(t) {
            ;
        }

        WP<ITarget> target;
        CBox        box;
        Vector2D    resize;
        Vector2D    pos;
    };

    struct SGollumSize {
        double                             w;
        std::unordered_map<size_t, double> h;
    };

    class CGollumAlgorithm : public ITiledAlgorithm {
      public:
        CGollumAlgorithm()          = default;
        virtual ~CGollumAlgorithm() = default;

        virtual void                             newTarget(SP<ITarget> target);
        virtual void                             movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint = std::nullopt);
        virtual void                             removeTarget(SP<ITarget> target);

        virtual void                             resizeTarget(const Vector2D& delta, SP<ITarget> target, eRectCorner corner = CORNER_NONE);
        virtual void                             recalculate();

        virtual void                             swapTargets(SP<ITarget> a, SP<ITarget> b);
        virtual void                             moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent);

        virtual std::expected<void, std::string> layoutMsg(const std::string_view& sv);
        virtual std::optional<Vector2D>          predictSizeForNewTarget();

        virtual SP<ITarget>                      getNextCandidate(SP<ITarget> old);

      private:
        std::deque<SP<SGollumData>>                    m_gollumData;
        std::unordered_map<std::string, std::string>   m_gollumOpt;
        std::map<size_t, std::vector<SP<SGollumData>>> m_gollumCol;
        std::string                                    m_gollumNext;
        std::unordered_map<size_t, SGollumSize>        m_gollumSize;

        SP<SGollumData>                                dataFor(SP<ITarget> t);
        SP<SGollumData>                                getClosestNode(const Vector2D&);

        std::string                                    getStrOpt(const std::string& opt);
        Hyprlang::INT                                  getIntOpt(const std::string& opt);
        Hyprlang::VEC2                                 getVec2Opt(const std::string& opt);
    };
}
