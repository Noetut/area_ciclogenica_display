#include "PatternGrid.h"
#include <iostream>

PatternGrid::PatternGrid() {
    ResetToDefaults();
}

PatternGrid::~PatternGrid() {
}

void PatternGrid::ResetToDefaults() {
    m_squares = DEFAULT_PATTERN_BOXES;
}

void PatternGrid::SetSquareVisible(int id, bool visible) {
    if (id >= 0 && id < static_cast<int>(m_squares.size())) {
        m_squares[id].isVisible = visible;
    }
}

void PatternGrid::SetSquareVisible(BoxId id, bool visible) {
    SetSquareVisible(static_cast<int>(id), visible);
}

void PatternGrid::ToggleSquare(int id) {
    if (id >= 0 && id < static_cast<int>(m_squares.size())) {
        m_squares[id].isVisible = !m_squares[id].isVisible;
    }
}

void PatternGrid::ToggleSquare(BoxId id) {
    ToggleSquare(static_cast<int>(id));
}

void PatternGrid::SetAllVisible(bool visible) {
    for (auto& sq : m_squares) {
        sq.isVisible = visible;
    }
}

bool PatternGrid::IsSquareVisible(int id) const {
    if (id >= 0 && id < static_cast<int>(m_squares.size())) {
        return m_squares[id].isVisible;
    }
    return false;
}

bool PatternGrid::IsSquareVisible(BoxId id) const {
    return IsSquareVisible(static_cast<int>(id));
}

void PatternGrid::SetSquareColor(int id, COLORREF color) {
    if (id >= 0 && id < static_cast<int>(m_squares.size())) {
        m_squares[id].color = color;
    }
}

void PatternGrid::SetSquareColor(BoxId id, COLORREF color) {
    SetSquareColor(static_cast<int>(id), color);
}

void PatternGrid::SetAllColor(COLORREF color) {
    for (auto& sq : m_squares) {
        sq.color = color;
    }
}

SquareData* PatternGrid::GetSquare(int id) {
    if (id >= 0 && id < static_cast<int>(m_squares.size())) {
        return &m_squares[id];
    }
    return nullptr;
}

const SquareData* PatternGrid::GetSquare(int id) const {
    if (id >= 0 && id < static_cast<int>(m_squares.size())) {
        return &m_squares[id];
    }
    return nullptr;
}
