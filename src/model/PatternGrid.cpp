#include "PatternGrid.h"

namespace {

// Geometry of an area created with [N] in calibration mode.
const int kNewAreaWidth  = 200;
const int kNewAreaHeight = 150;

// Offset applied to a copy made with [D], so it does not hide under the source.
const int kDuplicateOffset = 20;

} // namespace

PatternGrid::PatternGrid() {
}

PatternGrid::~PatternGrid() {
}

void PatternGrid::Clear() {
    m_areas.clear();
}

void PatternGrid::SetAreas(const std::vector<ProjectionArea>& areas) {
    m_areas = areas;
}

void PatternGrid::SetAreaVisible(int index, bool visible) {
    if (IsValidIndex(index)) {
        m_areas[index].isVisible = visible;
    }
}

void PatternGrid::ToggleArea(int index) {
    if (IsValidIndex(index)) {
        m_areas[index].isVisible = !m_areas[index].isVisible;
    }
}

void PatternGrid::SetAllVisible(bool visible) {
    for (auto& area : m_areas) {
        area.isVisible = visible;
    }
}

bool PatternGrid::IsAreaVisible(int index) const {
    return IsValidIndex(index) ? m_areas[index].isVisible : false;
}

void PatternGrid::SetAreaColor(int index, COLORREF color) {
    if (IsValidIndex(index)) {
        m_areas[index].color = color;
    }
}

void PatternGrid::SetAllColor(COLORREF color) {
    for (auto& area : m_areas) {
        area.color = color;
    }
}

int PatternGrid::CreateArea(int canvasWidth, int canvasHeight) {
    ProjectionArea area;
    area.id = NextFreeId();
    area.name = "area_" + std::to_string(area.id);
    area.description = "Created in calibration mode";
    area.quad = Quad::FromRect((canvasWidth  - kNewAreaWidth)  / 2,
                               (canvasHeight - kNewAreaHeight) / 2,
                               kNewAreaWidth,
                               kNewAreaHeight);
    area.color = RGB(255, 255, 255);
    area.isVisible = true;

    m_areas.push_back(area);
    return static_cast<int>(m_areas.size()) - 1;
}

int PatternGrid::DuplicateArea(int index) {
    if (!IsValidIndex(index)) return -1;

    ProjectionArea copy = m_areas[index];
    copy.id = NextFreeId();
    copy.name = "area_" + std::to_string(copy.id);
    copy.description = "Duplicated from " + m_areas[index].name;
    copy.quad.Translate(kDuplicateOffset, kDuplicateOffset);

    m_areas.push_back(copy);
    return static_cast<int>(m_areas.size()) - 1;
}

bool PatternGrid::RemoveArea(int index) {
    if (!IsValidIndex(index)) return false;

    m_areas.erase(m_areas.begin() + index);
    return true;
}

int PatternGrid::NextFreeId() const {
    int maxId = -1;
    for (const auto& area : m_areas) {
        if (area.id > maxId) maxId = area.id;
    }
    return maxId + 1;
}

int PatternGrid::FindIndexById(int id) const {
    for (size_t i = 0; i < m_areas.size(); ++i) {
        if (m_areas[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int PatternGrid::FindIndexByName(const std::string& name) const {
    for (size_t i = 0; i < m_areas.size(); ++i) {
        if (m_areas[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

ProjectionArea* PatternGrid::GetArea(int index) {
    return IsValidIndex(index) ? &m_areas[index] : nullptr;
}

const ProjectionArea* PatternGrid::GetArea(int index) const {
    return IsValidIndex(index) ? &m_areas[index] : nullptr;
}
