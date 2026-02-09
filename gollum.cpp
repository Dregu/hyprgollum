#include "gollum.hpp"

#include <hyprland/src/helpers/math/Direction.hpp>
#include <hyprland/src/layout/algorithm/Algorithm.hpp>
#include <hyprland/src/layout/space/Space.hpp>
#include "hyprland/src/layout/target/WindowTarget.hpp"
#include <hyprland/src/desktop/state/FocusState.hpp>

#include <algorithm>

using namespace Layout;
using namespace Layout::Tiled;

void CGollumAlgorithm::newTarget(SP<ITarget> target) {
    const auto DATA = m_gollumData.emplace_back(makeShared<SGollumData>(target));
    recalculate();
}

void CGollumAlgorithm::movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint) {
    newTarget(target);
}

void CGollumAlgorithm::removeTarget(SP<ITarget> target) {
    auto it = std::ranges::find_if(m_gollumData, [target](const auto& data) { return data->target.lock() == target; });
    if (it == m_gollumData.end())
        return;
    m_gollumData.erase(it);
    recalculate();
}

void CGollumAlgorithm::resizeTarget(const Vector2D& Δ, SP<ITarget> target, eRectCorner corner) {}

void CGollumAlgorithm::recalculate() {
    if (m_gollumData.empty())
        return;
    const auto N    = m_gollumData.size();
    const auto AREA = m_parent->space()->workArea();
    for (size_t i = 0; i < N; ++i) {
        const auto& DATA   = m_gollumData[i];
        const auto  TARGET = DATA->target.lock();
        if (!TARGET)
            continue;
        const auto WINDOW = TARGET->window();
        if (!WINDOW)
            continue;
        const auto BOX = CBox{AREA.x, AREA.y + i * AREA.h / N, AREA.w, AREA.h / N};
        DATA->box      = BOX;
        TARGET->setPositionGlobal(BOX);
    }
}

SP<ITarget> CGollumAlgorithm::getNextCandidate(SP<ITarget> old) {
    if (m_gollumData.empty())
        return nullptr;
    const auto MIDDLE = old->position().middle();
    if (const auto NODE = getClosestNode(MIDDLE); NODE)
        return NODE->target.lock();
    return nullptr;
}

std::expected<void, std::string> CGollumAlgorithm::layoutMsg(const std::string_view& sv) {
    return {};
}

std::optional<Vector2D> CGollumAlgorithm::predictSizeForNewTarget() {
    const auto N    = m_gollumData.size() + 1;
    const auto AREA = m_parent->space()->workArea();
    return {{AREA.w, AREA.h / N}};
}

void CGollumAlgorithm::swapTargets(SP<ITarget> a, SP<ITarget> b) {
    auto nodeA = dataFor(a);
    auto nodeB = dataFor(b);
    if (nodeA)
        nodeA->target = b;
    if (nodeB)
        nodeB->target = a;
}

void CGollumAlgorithm::moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent) {
    if (m_gollumData.size() < 2)
        return;
    const auto POS = t->position().middle();
    auto       it  = std::ranges::find_if(m_gollumData, [t](const auto& data) { return data->target.lock() == t; });
    if (dir == Math::DIRECTION_UP && it != m_gollumData.begin()) {
        auto next = std::prev(it);
        swapTargets(t, next->get()->target.lock());
    } else if (dir == Math::DIRECTION_DOWN && it != --m_gollumData.end()) {
        auto next = std::next(it);
        swapTargets(t, next->get()->target.lock());
    }
    if (silent) {
        const auto OLD = getClosestNode(POS);
        if (OLD && OLD->target)
            Desktop::focusState()->fullWindowFocus(OLD->target->window(), Desktop::FOCUS_REASON_KEYBIND);
    }
    recalculate();
}

SP<SGollumData> CGollumAlgorithm::dataFor(SP<ITarget> t) {
    for (auto& data : m_gollumData) {
        if (data->target.lock() == t)
            return data;
    }
    return nullptr;
}

SP<SGollumData> CGollumAlgorithm::getClosestNode(const Vector2D& point) {
    SP<SGollumData> res         = nullptr;
    double          distClosest = -1;
    for (auto& n : m_gollumData) {
        if (n->target && Desktop::View::validMapped(n->target->window())) {
            auto distAnother = vecToRectDistanceSquared(point, n->box.pos(), n->box.pos() + n->box.size());
            if (!res || distAnother < distClosest) {
                res         = n;
                distClosest = distAnother;
            }
        }
    }
    return res;
}