#ifndef PATTERN_GRID_H
#define PATTERN_GRID_H

#include "PatternBoxes.h"
#include <vector>
#include <string>

class PatternGrid {
public:
    PatternGrid();
    ~PatternGrid();

    // Reset all squares to default calibrated states (all visible, default white color)
    void ResetToDefaults();

    // Turn an individual square ON or OFF by integer ID (0 to 8)
    void SetSquareVisible(int id, bool visible);

    // Turn an individual square ON or OFF by BoxId enum
    void SetSquareVisible(BoxId id, bool visible);

    // Toggle an individual square's visibility state
    void ToggleSquare(int id);
    void ToggleSquare(BoxId id);

    // Turn ALL squares ON or OFF simultaneously
    void SetAllVisible(bool visible);

    // Query visibility of a square
    bool IsSquareVisible(int id) const;
    bool IsSquareVisible(BoxId id) const;

    // Set color of an individual square (for future custom patterns/color changes)
    void SetSquareColor(int id, COLORREF color);
    void SetSquareColor(BoxId id, COLORREF color);

    // Set color of ALL squares
    void SetAllColor(COLORREF color);

    // Accessors
    const std::vector<SquareData>& GetSquares() const { return m_squares; }
    std::vector<SquareData>& GetSquares() { return m_squares; }
    size_t GetCount() const { return m_squares.size(); }

    // Pointer access by ID (returns nullptr if invalid)
    SquareData* GetSquare(int id);
    const SquareData* GetSquare(int id) const;

private:
    std::vector<SquareData> m_squares;
};

#endif // PATTERN_GRID_H
