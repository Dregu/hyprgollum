#include "gollum.hpp"

using namespace Layout;
using namespace Layout::Tiled;

void        CGollumAlgorithm::newTarget(SP<ITarget> target) {}

void        CGollumAlgorithm::movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint) {}

void        CGollumAlgorithm::removeTarget(SP<ITarget> target) {}

void        CGollumAlgorithm::resizeTarget(const Vector2D& Δ, SP<ITarget> target, eRectCorner corner) {}

void        CGollumAlgorithm::recalculate() {}

SP<ITarget> CGollumAlgorithm::getNextCandidate(SP<ITarget> old) {
    return old;
}

std::expected<void, std::string> CGollumAlgorithm::layoutMsg(const std::string_view& sv) {
    return {};
}

std::optional<Vector2D> CGollumAlgorithm::predictSizeForNewTarget() {
    return std::nullopt;
}

void CGollumAlgorithm::swapTargets(SP<ITarget> a, SP<ITarget> b) {}

void CGollumAlgorithm::moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent) {}