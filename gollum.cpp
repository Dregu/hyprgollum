#include "gollum.hpp"

#include <hyprland/src/helpers/math/Direction.hpp>
#include <hyprland/src/layout/algorithm/Algorithm.hpp>
#include <hyprland/src/layout/space/Space.hpp>
#include "hyprland/src/layout/target/WindowTarget.hpp"
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/config/ConfigManager.hpp>

using namespace Layout;
using namespace Layout::Tiled;

void CGollumAlgorithm::newTarget(SP<ITarget> target) {
    const auto DATA = m_gollumData.emplace_back(makeShared<SGollumData>(target));
    recalculate();
}

void CGollumAlgorithm::movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint) {
    if (g_layoutManager->dragController()->wasDraggingWindow()) {
        const auto MOUSECOORDS = g_pInputManager->getMouseCoordsInternal();
        if (const auto NODE = getClosestNode(MOUSECOORDS); NODE) {
            auto       t      = NODE->target.lock();
            auto       it     = std::ranges::find_if(m_gollumData, [t](const auto& data) { return data->target.lock() == t; });
            const auto MIDDLE = t->position().middle();
            if (MIDDLE.y < MOUSECOORDS.y)
                ++it;
            const auto DATA = m_gollumData.insert(it, makeShared<SGollumData>(target));
            recalculate();
            return;
        }
    }
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

    static auto PGRID = CConfigValue<Hyprlang::VEC2>("plugin:gollum:grid");
    const int   W     = (*PGRID).x;
    const int   H     = (*PGRID).y;
    const auto  N     = m_gollumData.size();
    const auto  AREA  = m_parent->space()->workArea();

    if ((W < 2 && H < 2) || N == 1) {
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
        return;
    }

    std::map<size_t, std::vector<SP<SGollumData>>> cols;

    size_t                                         x = 0;
    size_t                                         y = 0;
    for (size_t i = 0; i < N; ++i) {
        const auto& DATA = m_gollumData[i];
        cols[x].emplace_back(DATA);
        if (++y >= H) {
            y = 0;
            if (++x >= W)
                x = 0;
        }
    }
    size_t xmax = cols.size();
    size_t ymax = 0;
    for (auto& [x, col] : cols) {
        if (col.size() > ymax)
            ymax = col.size();
    }
    double w = AREA.w / xmax;

    for (auto& [x, col] : cols) {
        for (size_t y = 0; y < col.size(); ++y) {
            const auto& DATA   = col[y];
            const auto  TARGET = DATA->target.lock();
            if (!TARGET)
                continue;
            const auto WINDOW = TARGET->window();
            if (!WINDOW)
                continue;
            double     h   = AREA.h / col.size();
            const auto BOX = CBox{AREA.x + x * w, AREA.y + y * h, w, h};
            DATA->box      = BOX;
            TARGET->setPositionGlobal(BOX);
        }
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