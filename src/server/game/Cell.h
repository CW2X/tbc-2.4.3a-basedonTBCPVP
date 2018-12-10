
#ifndef TRINITY_CELL_H
#define TRINITY_CELL_H

#include "TypeContainerVisitor.h"
#include "GridDefines.h"
#include <cmath>

class Map;
class WorldObject;

enum District
{
    UPPER_DISTRICT = 1,
    LOWER_DISTRICT = 1 << 1,
    LEFT_DISTRICT  = 1 << 2,
    RIGHT_DISTRICT = 1 << 3,
    CENTER_DISTRICT = 1 << 4,
    UPPER_LEFT_DISTRICT = (UPPER_DISTRICT | LEFT_DISTRICT),
    UPPER_RIGHT_DISTRICT = (UPPER_DISTRICT | RIGHT_DISTRICT),
    LOWER_LEFT_DISTRICT = (LOWER_DISTRICT | LEFT_DISTRICT),
    LOWER_RIGHT_DISTRICT = (LOWER_DISTRICT | RIGHT_DISTRICT),
    ALL_DISTRICT = (UPPER_DISTRICT | LOWER_DISTRICT | LEFT_DISTRICT | RIGHT_DISTRICT | CENTER_DISTRICT)
};

struct CellArea
{
	CellArea() { }
	CellArea(CellCoord low, CellCoord high) : low_bound(low), high_bound(high) { }

	bool operator!() const { return low_bound == high_bound; }

    void ResizeBorders(CellCoord& begin_cell, CellCoord& end_cell) const
    {
		begin_cell = low_bound;
		end_cell = high_bound;
    }

	CellCoord low_bound;
	CellCoord high_bound;
};

struct Cell
{
    Cell() { data.All = 0; }
    Cell(const Cell &cell) { data.All = cell.data.All; }
    explicit Cell(CellCoord const& p);
	explicit Cell(float x, float y);

    void Compute(uint32 &x, uint32 &y) const
    {
        x = data.Part.grid_x*MAX_NUMBER_OF_CELLS + data.Part.cell_x;
        y = data.Part.grid_y*MAX_NUMBER_OF_CELLS + data.Part.cell_y;
    }

    inline bool DiffCell(const Cell &cell) const
    {
        return( data.Part.cell_x != cell.data.Part.cell_x ||
            data.Part.cell_y != cell.data.Part.cell_y );
    }

    inline bool DiffGrid(const Cell &cell) const
    {
        return( data.Part.grid_x != cell.data.Part.grid_x ||
            data.Part.grid_y != cell.data.Part.grid_y );
    }

    uint32 CellX() const { return data.Part.cell_x; }
    uint32 CellY() const { return data.Part.cell_y; }
    uint32 GridX() const { return data.Part.grid_x; }
    uint32 GridY() const { return data.Part.grid_y; }
    bool NoCreate() const { return data.Part.nocreate; }
    void SetNoCreate() { data.Part.nocreate = 1; }

    CellCoord GetCellCoord() const
    {
        return CellCoord(
            data.Part.grid_x*MAX_NUMBER_OF_CELLS+data.Part.cell_x,
            data.Part.grid_y*MAX_NUMBER_OF_CELLS+data.Part.cell_y);
    }

    Cell& operator=(const Cell &cell)
    {
        this->data.All = cell.data.All;
        return *this;
    }

    bool operator==(const Cell &cell) const { return (data.All == cell.data.All); }
    bool operator!=(const Cell &cell) const { return !operator==(cell); }
    union
    {
        struct
        {
            unsigned grid_x : 6;
            unsigned grid_y : 6;
            unsigned cell_x : 6;
            unsigned cell_y : 6;
            unsigned nocreate : 1;
            unsigned reserved : 7;
        } Part;
        uint32 All;
    } data;

    template<class T, class CONTAINER> void Visit(CellCoord const&, TypeContainerVisitor<T, CONTAINER>& visitor, Map&, WorldObject const& obj, float radius) const;
    template<class T, class CONTAINER> void Visit(CellCoord const&, TypeContainerVisitor<T, CONTAINER>& visitor, Map&, float x, float y, float radius) const;

    template<class T> static void VisitGridObjects(WorldObject const* obj, T& visitor, float radius, bool dont_load = true);
    template<class T> static void VisitWorldObjects(WorldObject const* obj, T& visitor, float radius, bool dont_load = true);
    template<class T> static void VisitAllObjects(WorldObject const* obj, T& visitor, float radius, bool dont_load = true);
    
    template<class T> static void VisitGridObjects(float x, float y, Map* map, T& visitor, float radius, bool dont_load = true);
    template<class T> static void VisitWorldObjects(float x, float y, Map* map, T& visitor, float radius, bool dont_load = true);
    template<class T> static void VisitAllObjects(float x, float y, Map* map, T& visitor, float radius, bool dont_load = true);

    static CellArea CalculateCellArea(const WorldObject &obj, float radius);
    static CellArea CalculateCellArea(float x, float y, float radius);

private:
    template<class T, class CONTAINER> void VisitCircle(TypeContainerVisitor<T, CONTAINER> &, Map &, const CellCoord&, const CellCoord&) const;
};

#endif

