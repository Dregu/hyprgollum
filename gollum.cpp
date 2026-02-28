#include "gollum.hpp"

#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/helpers/math/Direction.hpp>
#include <hyprland/src/layout/algorithm/Algorithm.hpp>
#include <hyprland/src/layout/space/Space.hpp>
#include "hyprland/src/layout/target/WindowTarget.hpp"
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprutils/string/String.hpp>

#include <cstring>

using namespace Layout;
using namespace Layout::Tiled;

void CGollumAlgorithm::newTarget(SP<ITarget> target) {
    auto NEW = getStrOpt("new");
    if (!m_next.empty())
        NEW = m_next;
    auto       WIN         = Desktop::focusState()->window();
    const auto MOUSECOORDS = g_pInputManager->getMouseCoordsInternal();
    if (const auto DATA = getClosestNode(MOUSECOORDS); DATA) {
        if (const auto TARGET = DATA->target.lock(); TARGET) {
            if (!WIN || NEW.starts_with("s"))
                WIN = TARGET->window();
            if (NEW.starts_with("s")) {
                const auto MID = TARGET->position().middle();
                if (MOUSECOORDS.y < MID.y)
                    NEW = "prev";
                else
                    NEW = "next";
            }
        }
    }

    bool found = false;
    if (target->window() && m_next.empty()) {
        if (m_gollumData.empty()) {
            m_gollumData.emplace_back(makeShared<SGollumData>(target));
            found = true;
        } else if (target->window()->m_ruleApplicator->m_tagKeeper.isTagged("top")) {
            auto it = std::ranges::find_if(m_gollumData, [](const auto& data) { return !data->target.lock()->window()->m_ruleApplicator->m_tagKeeper.isTagged("top"); });
            m_gollumData.insert(it, makeShared<SGollumData>(target));
            found = true;
        } else if (target->window()->m_ruleApplicator->m_tagKeeper.isTagged("bottom")) {
            auto it = std::ranges::find_if(m_gollumData.rbegin(), m_gollumData.rend(),
                                           [](const auto& data) { return !data->target.lock()->window()->m_ruleApplicator->m_tagKeeper.isTagged("bottom"); });
            m_gollumData.insert(it.base(), makeShared<SGollumData>(target));
            found = true;
        }
    }

    if (!found) {
        if (NEW.starts_with("n") && WIN) {
            auto t  = WIN->layoutTarget();
            auto it = std::ranges::find_if(m_gollumData, [t](const auto& data) { return data->target.lock() == t; });
            if (++it != m_gollumData.end()) {
                m_gollumData.insert(it, makeShared<SGollumData>(target));
                found = true;
            }
        } else if (NEW.starts_with("p") && WIN) {
            auto t  = WIN->layoutTarget();
            auto it = std::ranges::find_if(m_gollumData, [t](const auto& data) { return data->target.lock() == t; });
            if (it != m_gollumData.end()) {
                m_gollumData.insert(it, makeShared<SGollumData>(target));
                found = true;
            }
        }
    }

    if (!found) {
        if (NEW.starts_with("t"))
            m_gollumData.emplace_front(makeShared<SGollumData>(target));
        else
            m_gollumData.emplace_back(makeShared<SGollumData>(target));
    }

    m_next.clear();
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
            m_gollumData.insert(it, makeShared<SGollumData>(target));
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

    auto GRID  = getVec2Opt("grid");
    auto FIT   = getIntOpt("fit");
    auto ORDER = getStrOpt("order");
    auto DIR   = getStrOpt("dir");
    if (!ORDER.empty() && !std::all_of(ORDER.begin(), ORDER.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        Log::logger->log(Log::ERR, "[hyprgollum] order = {} is not a number", ORDER);
        ORDER = "";
    }
    int        W    = GRID.x;
    int        H    = GRID.y;
    const auto N    = m_gollumData.size();
    const auto AREA = m_parent->space()->workArea();

    if (!ORDER.empty()) {
        for (size_t i = 0; i < ORDER.size(); ++i)
            W = std::max((ORDER)[i] - '0', W);
    }

    if ((W < 2 && H < 2 && ORDER.empty()) || N == 1) {
        for (size_t i = 0; i < N; ++i) {
            const auto& DATA   = m_gollumData[i];
            const auto  TARGET = DATA->target.lock();
            if (!TARGET)
                continue;
            const auto WINDOW = TARGET->window();
            if (!WINDOW)
                continue;
            double w = AREA.w;
            double m = 0;
            if (FIT == 0) {
                w = AREA.w / W;
                m = (W - 1) * w / 2;
            }
            const auto BOX = CBox{AREA.x + m, AREA.y + i * AREA.h / N, w, AREA.h / N};
            DATA->box      = BOX;
            TARGET->setPositionGlobal(BOX);
        }
        return;
    }

    std::map<size_t, std::vector<SP<SGollumData>>> cols;
    int                                            FW = W;
    int                                            x  = 0;
    for (size_t i = 0; i < N; ++i) {
        const auto& DATA = m_gollumData[i];
        if (!ORDER.empty())
            x = (ORDER)[i % ORDER.size()] - '0' - 1;
        else {
            if ((i < W * H - 1 && cols[x].size() >= H) || (i >= W * H - 1))
                --x;
            if ((i == 0 || i == W * H - (H - 1)) && cols[0].size() < H)
                x = 0;
            else if (x <= 0)
                x = W - 1;
        }
        cols[x].emplace_back(DATA);
    }

    if (FIT < 2 && cols.size() < W) {
        size_t empty = W - cols.size();
        for (size_t i = 0; i < W; ++i) {
            if (cols[i + 1].empty()) {
                cols[i + 1] = cols[i + 1 + empty];
                cols[i + 1 + empty].clear();
            }
        }
        W = W - empty;
    }

    for (auto& [x, col] : cols) {
        for (size_t y = 0; y < col.size(); ++y) {
            const auto& DATA   = col[y];
            const auto  TARGET = DATA->target.lock();
            if (!TARGET)
                continue;
            const auto WINDOW = TARGET->window();
            if (!WINDOW)
                continue;
            double w = AREA.w / W;
            double m = 0;
            if (FIT == 0) {
                w = AREA.w / FW;
                m = (FW - W) * w / 2;
                if (DIR.starts_with("r"))
                    m = -m;
            }
            auto   rw = w;
            int    o  = 0;
            double h  = AREA.h / col.size();
            if (FIT == 2) {
                for (int i = x + 1; i < W; ++i) {
                    if (cols[i].empty()) {
                        rw += w;
                        if (DIR.starts_with("r"))
                            --o;
                    } else
                        break;
                }
            }
            auto BOX = CBox{AREA.x + x * w + m, AREA.y + y * h, rw, h};
            if (DIR.starts_with("r"))
                BOX = CBox{AREA.x + AREA.w - w - x * w + m + o * w, AREA.y + y * h, rw, h};
            DATA->box = BOX;
            TARGET->setPositionGlobal(BOX);
        }
    }
}

SP<ITarget> CGollumAlgorithm::getNextCandidate(SP<ITarget> old) {
    if (m_gollumData.empty())
        return nullptr;
    const auto MIDDLE = old->position().middle();
    if (const auto NODE = getClosestNode(MIDDLE - Vector2D{1, 1}); NODE)
        return NODE->target.lock();
    return nullptr;
}

std::expected<void, std::string> CGollumAlgorithm::layoutMsg(const std::string_view& sv) {
    CVarList args(std::string{sv}, 3, ' ', false);
    if (args[0] == "reset") {
        m_gollumOpt.clear();
        m_next.clear();
    } else if (args[0] == "toggle") {
        if (m_gollumOpt.contains(std::string{args[1]}))
            m_gollumOpt.erase(std::string{args[1]});
        else
            m_gollumOpt[std::string{args[1]}] = args[2];
    } else if (args[0] == "set") {
        m_gollumOpt[std::string{args[1]}] = args[2];
    } else if (args[0] == "unset") {
        if (m_gollumOpt.contains(std::string{args[1]}))
            m_gollumOpt.erase(std::string{args[1]});
    } else if (args[0] == "cycle") {
        CVarList list(std::string{args[2]}, 0, ',', false);
        if (m_gollumOpt.contains(std::string{args[1]})) {
            auto target = m_gollumOpt[std::string{args[1]}];
            auto it     = std::ranges::find_if(list, [target](const auto& data) { return data == target; });
            if (it != list.end())
                ++it;
            if (it == list.end())
                it = list.begin();
            m_gollumOpt[std::string{args[1]}] = *it;
        } else {
            m_gollumOpt[std::string{args[1]}] = list[0];
        }
    } else if (args[0].starts_with("focus")) {
        if (m_gollumData.empty())
            return {};
        if (args[1].starts_with("t")) {
            Desktop::focusState()->fullWindowFocus(m_gollumData.front()->target.lock()->window(), Desktop::FOCUS_REASON_KEYBIND);
        } else if (args[1].starts_with("b")) {
            Desktop::focusState()->fullWindowFocus(m_gollumData.back()->target.lock()->window(), Desktop::FOCUS_REASON_KEYBIND);
        }
    } else if (args[0].starts_with("move")) {
        if (m_gollumData.empty())
            return {};
        const auto WIN = Desktop::focusState()->window();
        if (!WIN)
            return {};
        auto target = WIN->layoutTarget();
        auto it     = std::ranges::find_if(m_gollumData, [target](const auto& data) { return data->target.lock() == target; });
        if (it != m_gollumData.end()) {
            if (args[1].starts_with("t")) {
                std::rotate(m_gollumData.begin(), it, it + 1);
            } else if (args[1].starts_with("b")) {
                std::rotate(it, it + 1, m_gollumData.end());
            }
        }
    } else if (args[0].starts_with("swap")) {
        if (m_gollumData.empty())
            return {};
        const auto WIN = Desktop::focusState()->window();
        if (!WIN)
            return {};
        auto target = WIN->layoutTarget();
        auto it     = std::ranges::find_if(m_gollumData, [target](const auto& data) { return data->target.lock() == target; });
        if (it != m_gollumData.end()) {
            if (args[1].starts_with("t")) {
                swapTargets(target, m_gollumData.front()->target.lock());
            } else if (args[1].starts_with("b")) {
                swapTargets(target, m_gollumData.back()->target.lock());
            }
        }
    } else if (args[0].starts_with("next")) {
        m_next = args[1];
    } else {
        Log::logger->log(Log::ERR, "[hyprgollum] Unknown layoutmsg: {}", std::string{sv});
        return std::unexpected("nope");
    }
    recalculate();
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
    if (!t || !t->space() || !t->space()->workspace())
        return;
    if (t->window())
        t->window()->setAnimationsToMove();
    const auto POS = t->position().middle();
    auto       NEW = g_pCompositor->getWindowInDirection(t->window(), dir);
    if (!NEW || !dataFor(NEW->layoutTarget())) {
        const auto PMONINDIR = g_pCompositor->getMonitorInDirection(t->space()->workspace()->m_monitor.lock(), dir);
        if (PMONINDIR && PMONINDIR != t->space()->workspace()->m_monitor.lock()) {
            const auto TARGETWS = PMONINDIR->m_activeWorkspace;
            t->assignToSpace(TARGETWS->m_space);
        }
    } else
        swapTargets(t, NEW->layoutTarget());
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

std::string CGollumAlgorithm::getStrOpt(const std::string& opt) {
    const auto WSRULE = g_pConfigManager->getWorkspaceRuleFor(m_parent->space()->workspace());
    if (WSRULE.layoutopts.contains(opt) || m_gollumOpt.contains(opt)) {
        auto VALUE = m_gollumOpt.contains(opt) ? m_gollumOpt[opt] : WSRULE.layoutopts.at(opt);
        Log::logger->log(Log::DEBUG, "[hyprgollum] layoutopt:{} = {}", opt, VALUE);
        return VALUE.c_str();
    }
    return *CConfigValue<Hyprlang::STRING>("plugin:gollum:" + opt);
}

Hyprlang::INT CGollumAlgorithm::getIntOpt(const std::string& opt) {
    const auto WSRULE = g_pConfigManager->getWorkspaceRuleFor(m_parent->space()->workspace());
    if (WSRULE.layoutopts.contains(opt) || m_gollumOpt.contains(opt)) {
        auto VALUE = m_gollumOpt.contains(opt) ? m_gollumOpt[opt] : WSRULE.layoutopts.at(opt);
        Log::logger->log(Log::DEBUG, "[hyprgollum] layoutopt:{} = {}", opt, VALUE);
        Hyprlang::INT x;
        if (VALUE.starts_with("true") || VALUE.starts_with("on") || VALUE.starts_with("yes"))
            return 1;
        else if (VALUE.starts_with("false") || VALUE.starts_with("off") || VALUE.starts_with("no"))
            return 0;
        try {
            x = std::stol(std::string{VALUE});
            return x;
        } catch (std::exception& e) { Log::logger->log(Log::ERR, "[hyprgollum] layoutopt:{} = {} is not INT: {}", opt, VALUE, e.what()); }
    }
    return *CConfigValue<Hyprlang::INT>("plugin:gollum:" + opt);
}

Hyprlang::VEC2 CGollumAlgorithm::getVec2Opt(const std::string& opt) {
    const auto WSRULE = g_pConfigManager->getWorkspaceRuleFor(m_parent->space()->workspace());
    if (WSRULE.layoutopts.contains(opt) || m_gollumOpt.contains(opt)) {
        auto VALUE = m_gollumOpt.contains(opt) ? m_gollumOpt[opt] : WSRULE.layoutopts.at(opt);
        Log::logger->log(Log::DEBUG, "[hyprgollum] layoutopt:{} = {}", opt, VALUE);
        Hyprlang::INT x;
        Hyprlang::INT y;
        CVarList2     args(std::string{VALUE}, 0, ' ', false);
        try {
            x = std::stol(std::string(args[0]));
            y = std::stol(std::string(args[1]));
            return Hyprlang::VEC2(x, y);
        } catch (std::exception& e) { Log::logger->log(Log::ERR, "[hyprgollum] layoutopt:{} = {} is not VEC2: {}", opt, VALUE, e.what()); }
    }
    return *CConfigValue<Hyprlang::VEC2>("plugin:gollum:" + opt);
}
