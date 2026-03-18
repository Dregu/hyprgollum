#include "gollum.hpp"

#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
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
    bool fs    = target->fullscreenMode() & FSMODE_FULLSCREEN;
    if (target->window() && !fs)
        fs = target->window()->m_ruleApplicator->static_.fullscreen.value_or(false) ||
            (target->window()->m_ruleApplicator->static_.fullscreenStateInternal.value_or(0) & FSMODE_FULLSCREEN) || target->window()->m_ruleApplicator->m_tagKeeper.isTagged("fs");
    if (target->window() && m_next.empty()) {
        if (m_gollumData.empty()) {
            m_gollumData.emplace_back(makeShared<SGollumData>(target, fs));
            found = true;
        } else if (target->window()->m_ruleApplicator->m_tagKeeper.isTagged("top")) {
            auto it = std::ranges::find_if(m_gollumData, [](const auto& data) { return !data->target.lock()->window()->m_ruleApplicator->m_tagKeeper.isTagged("top"); });
            m_gollumData.insert(it, makeShared<SGollumData>(target, fs));
            found = true;
        } else if (target->window()->m_ruleApplicator->m_tagKeeper.isTagged("bottom")) {
            auto it = std::ranges::find_if(m_gollumData.rbegin(), m_gollumData.rend(),
                                           [](const auto& data) { return !data->target.lock()->window()->m_ruleApplicator->m_tagKeeper.isTagged("bottom"); });
            m_gollumData.insert(it.base(), makeShared<SGollumData>(target, fs));
            found = true;
        }
    }

    if (!found) {
        if (NEW.starts_with("n") && WIN) {
            auto t  = WIN->layoutTarget();
            auto it = std::ranges::find_if(m_gollumData, [t](const auto& data) { return data->target.lock() == t; });
            if (++it != m_gollumData.end()) {
                m_gollumData.insert(it, makeShared<SGollumData>(target, fs));
                found = true;
            }
        } else if (NEW.starts_with("p") && WIN) {
            auto t  = WIN->layoutTarget();
            auto it = std::ranges::find_if(m_gollumData, [t](const auto& data) { return data->target.lock() == t; });
            if (it != m_gollumData.end()) {
                m_gollumData.insert(it, makeShared<SGollumData>(target, fs));
                found = true;
            }
        }
    }

    if (!found) {
        if (NEW.starts_with("t"))
            m_gollumData.emplace_front(makeShared<SGollumData>(target, fs));
        else
            m_gollumData.emplace_back(makeShared<SGollumData>(target, fs));
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
    auto FIT   = getStrOpt("fit");
    auto ORDER = getStrOpt("order");
    auto DIR   = getStrOpt("dir");
    auto FS    = getIntOpt("fs");
    auto MONO  = getIntOpt("mono");
    if (!ORDER.empty() && !std::all_of(ORDER.begin(), ORDER.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        Log::logger->log(Log::ERR, "[hyprgollum] order = {} is not a number", ORDER);
        ORDER = "";
    }
    const auto  N           = m_gollumData.size();
    const auto  AREA        = m_parent->space()->workArea();
    static auto PBORDERSIZE = CConfigValue<Hyprlang::INT>("general:border_size");

    if (MONO && m_parent->space()->workspace()->m_fullscreenMode & MONO) {
        for (size_t i = 0; i < N; ++i) {
            const auto& DATA   = m_gollumData[i];
            const auto  TARGET = DATA->target.lock();
            if (!TARGET)
                continue;
            const int BORDER = TARGET->window() ? TARGET->window()->getRealBorderSize() : 0;
            if (m_parent->space()->workspace()->m_fullscreenMode == FSMODE_MAXIMIZED) {
                DATA->box = AREA;
                TARGET->setPositionGlobal(AREA);
            } else {
                const auto MON = m_parent->space()->workspace()->m_monitor->logicalBox().expand(BORDER);
                const auto BOX = STargetBox{.logicalBox = MON, .visualBox = MON};
                DATA->box      = MON;
                TARGET->setPositionGlobal(BOX);
            }
        }
        return;
    }

    int FW = GRID.x;
    int H  = GRID.y;
    int OW = 0;
    if (!ORDER.empty()) {
        FW = 0;
        for (size_t i = 0; i < ORDER.size(); ++i) {
            FW = std::max(ORDER[i] - '0' + 1, FW);
            if (i < N)
                OW = std::max(ORDER[i] - '0' + 1, OW);
        }
    }

    const auto MFS = !FS ? 0 : std::count_if(m_gollumData.begin(), m_gollumData.end(), [FS](const auto& DATA) -> bool {
        return (DATA->target->fullscreenMode() & FSMODE_FULLSCREEN) && (FS == 1 || DATA->fs);
    });
    int        NFS = 0;
    if ((FW < 2 && H < 2 && ORDER.empty()) /*|| N == 1*/) {
        for (size_t i = 0; i < N; ++i) {
            const auto& DATA   = m_gollumData[i];
            const auto  TARGET = DATA->target.lock();
            if (!TARGET)
                continue;
            if (FS && (TARGET->fullscreenMode() & FSMODE_FULLSCREEN) && (FS == 1 || DATA->fs)) {
                ++NFS;
                continue;
            }
            double w = AREA.w;
            double m = 0;
            if (FIT.starts_with("c")) {
                w = AREA.w / FW;
                m = (FW - 1) * w / 2;
            }
            const auto BOX = CBox{AREA.x + m, AREA.y + (i - NFS) * AREA.h / (N - MFS), w, AREA.h / (N - MFS)};
            DATA->box      = BOX;
            TARGET->setPositionGlobal(BOX);
        }
        return;
    }

    std::map<size_t, std::vector<SP<SGollumData>>> cols;
    int                                            x = 0;
    for (size_t i = 0; i < N; ++i) {
        const auto& DATA = m_gollumData[i];
        if (FS && (DATA->target->fullscreenMode() & FSMODE_FULLSCREEN) && (FS == 1 || DATA->fs)) {
            ++NFS;
            continue;
        }
        auto ri = i - NFS;
        if (!ORDER.empty())
            x = ORDER[ri % ORDER.size()] - '0';
        else {
            if ((ri < FW * H - 1 && cols[x].size() >= H) || (ri >= FW * H - 1))
                --x;
            if ((ri == 0 || ri == FW * H - (H - 1)) && cols[0].size() < H)
                x = 0;
            else if (x <= 0)
                x = FW - 1;
        }
        cols[x].emplace_back(DATA);
    }

    int                     W = cols.size();
    size_t                  n = 0;
    std::unordered_set<int> used;
    for (auto& [x, col] : cols) {
        for (size_t y = 0; y < col.size(); ++y) {
            const auto& DATA   = col[y];
            const auto  TARGET = DATA->target.lock();
            if (!TARGET)
                continue;
            double rx = x;
            double w  = AREA.w / FW;
            double m  = 0;
            if (FIT.starts_with("c")) {
                m  = (FW - W) * w / 2;
                rx = n;
                if (DIR.starts_with("r"))
                    m = -m;
            } else if (FIT.starts_with("f") && ORDER.empty()) {
                w  = AREA.w / W;
                rx = n;
                m  = 0;
            } else if (FIT.starts_with("f") && !ORDER.empty()) {
                w = AREA.w / OW;
                m = 0;
            }
            auto   rw = w;
            int    o  = 0;
            double h  = AREA.h / col.size();

            for (int i = x + 1; i < FW; ++i) {
                if ((FIT.starts_with("t") && !cols.contains(i)) ||
                    (!ORDER.empty() && !FIT.starts_with("c") && !FIT.starts_with("f") && !FIT.starts_with("t") && !ORDER.contains(std::format("{}", i)))) {
                    rw += w;
                    if (DIR.starts_with("r"))
                        --o;
                } else
                    break;
            }

            if (!ORDER.empty() && FIT.starts_with("f")) {
                for (int i = x + 1; i < OW; ++i) {
                    if (!cols.contains(i)) {
                        rw += w;
                        used.insert(i);
                        if (DIR.starts_with("r"))
                            --o;
                    } else
                        break;
                }
                for (int i = x - 1; i >= 0; --i) {
                    if (!cols.contains(i) && !used.contains(i)) {
                        rw += w;
                        --rx;
                        if (DIR.starts_with("r"))
                            --o;
                    } else
                        break;
                }
            }

            auto BOX = CBox{AREA.x + rx * w + m, AREA.y + y * h, rw, h};
            if (DIR.starts_with("r"))
                BOX = CBox{AREA.x + AREA.w - w - rx * w + m + o * w, AREA.y + y * h, rw, h};
            DATA->box = BOX;
            TARGET->setPositionGlobal(BOX);
        }
        ++n;
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
    auto     WRAP = getIntOpt("wrap");
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
            return {};
        } else if (args[1].starts_with("b")) {
            Desktop::focusState()->fullWindowFocus(m_gollumData.back()->target.lock()->window(), Desktop::FOCUS_REASON_KEYBIND);
            return {};
        }
        auto WIN = Desktop::focusState()->window();
        if (!WIN)
            return {};
        auto target = WIN->layoutTarget();
        auto it     = std::ranges::find_if(m_gollumData, [target](const auto& data) { return data->target.lock() == target; });
        if (args[1].starts_with("p")) {
            if (it == m_gollumData.end() || it == m_gollumData.begin()) {
                if (!WRAP && it == m_gollumData.begin())
                    return {};
                it = --m_gollumData.end();
            } else {
                --it;
            }
            Desktop::focusState()->fullWindowFocus((*it)->target.lock()->window(), Desktop::FOCUS_REASON_KEYBIND);
        } else if (args[1].starts_with("n")) {
            if (it == m_gollumData.end() || it + 1 == m_gollumData.end()) {
                if (!WRAP && it + 1 == m_gollumData.end())
                    return {};
                it = m_gollumData.begin();
            } else {
                ++it;
            }
            Desktop::focusState()->fullWindowFocus((*it)->target.lock()->window(), Desktop::FOCUS_REASON_KEYBIND);
        }
    } else if (args[0].starts_with("move")) {
        if (m_gollumData.empty())
            return {};
        const auto WIN = Desktop::focusState()->window();
        if (!WIN)
            return {};
        auto target = WIN->layoutTarget();
        auto it     = std::ranges::find_if(m_gollumData, [target](const auto& data) { return data->target.lock() == target; });
        auto other  = it;
        if (it != m_gollumData.end()) {
            if (args[1].starts_with("t")) {
                std::rotate(m_gollumData.begin(), it, it + 1);
            } else if (args[1].starts_with("b")) {
                std::rotate(it, it + 1, m_gollumData.end());
            } else if (args[1].starts_with("p")) {
                if (it == m_gollumData.begin()) {
                    if (!WRAP)
                        return {};
                    std::rotate(it, it + 1, m_gollumData.end());
                } else {
                    other = it - 1;
                    swapTargets((*it)->target.lock(), (*other)->target.lock());
                }
            } else if (args[1].starts_with("n")) {
                if (it + 1 == m_gollumData.end()) {
                    if (!WRAP)
                        return {};
                    std::rotate(m_gollumData.begin(), it, it + 1);
                } else {
                    other = it + 1;
                    swapTargets((*it)->target.lock(), (*other)->target.lock());
                }
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
        auto other  = it;
        if (it != m_gollumData.end()) {
            if (args[1].starts_with("t")) {
                swapTargets(target, m_gollumData.front()->target.lock());
            } else if (args[1].starts_with("b")) {
                swapTargets(target, m_gollumData.back()->target.lock());
            } else if (args[1].starts_with("p")) {
                if (it == m_gollumData.begin()) {
                    if (!WRAP)
                        return {};
                    other = m_gollumData.end() - 1;
                } else {
                    other = it - 1;
                }
                swapTargets((*it)->target.lock(), (*other)->target.lock());

            } else if (args[1].starts_with("n")) {
                if (it + 1 == m_gollumData.end()) {
                    if (!WRAP)
                        return {};
                    other = m_gollumData.begin();
                } else {
                    other = it + 1;
                }
                swapTargets((*it)->target.lock(), (*other)->target.lock());
            }
        }
    } else if (args[0].starts_with("roll")) {
        if (m_gollumData.empty())
            return {};
        const auto WIN = Desktop::focusState()->window();
        if (args[1].starts_with("p")) {
            std::rotate(m_gollumData.begin(), m_gollumData.end() - 1, m_gollumData.end());
            if (WIN)
                layoutMsg("focus p");
        } else if (args[1].starts_with("n")) {
            std::rotate(m_gollumData.begin(), m_gollumData.begin() + 1, m_gollumData.end());
            if (WIN)
                layoutMsg("focus n");
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
    static auto PMONITORFALLBACK = CConfigValue<Hyprlang::INT>("binds:window_direction_monitor_fallback");
    if (!t || !t->space() || !t->space()->workspace())
        return;
    if (t->window())
        t->window()->setAnimationsToMove();
    const auto POS = t->position().middle();
    auto       NEW = g_pCompositor->getWindowInDirection(t->window(), dir);
    if (!NEW || !dataFor(NEW->layoutTarget())) {
        const auto PMONINDIR = g_pCompositor->getMonitorInDirection(t->space()->workspace()->m_monitor.lock(), dir);
        if (*PMONITORFALLBACK && PMONINDIR && PMONINDIR != t->space()->workspace()->m_monitor.lock()) {
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
