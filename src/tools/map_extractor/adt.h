/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: http://github.com/azerothcore/azerothcore-wotlk/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef ADT_H
#define ADT_H

#include "loadlib.h"

#define TILESIZE (533.33333f)
#define CHUNKSIZE ((TILESIZE) / 16.0f)
#define UNITSIZE (CHUNKSIZE / 8.0f)

enum LiquidType
{
    LIQUID_TYPE_WATER = 0,
    LIQUID_TYPE_OCEAN = 1,
    LIQUID_TYPE_MAGMA = 2,
    LIQUID_TYPE_SLIME = 3
};

//**************************************************************************************
// ADT file class
//**************************************************************************************
#define ADT_CELLS_PER_GRID    16
#define ADT_CELL_SIZE         8
#define ADT_GRID_SIZE         (ADT_CELLS_PER_GRID*ADT_CELL_SIZE)

//
// Adt file height map chunk
//
class adt_MCVT
{
    union
    {
        uint32 fcc;
        char   fcc_txt[4];
    };
    uint32 size;
public:
    float height_map[(ADT_CELL_SIZE + 1) * (ADT_CELL_SIZE + 1) + ADT_CELL_SIZE * ADT_CELL_SIZE];

    bool  prepareLoadedData();
};

//
// Adt file liquid map chunk (old)
//
class adt_MCLQ
{
    union
    {
        uint32 fcc;
        char   fcc_txt[4];
    };
public:
    uint32 size;
    float height1;
    float height2;
    struct liquid_data
    {
        uint32 light;
        float  height;
    } liquid[ADT_CELL_SIZE + 1][ADT_CELL_SIZE + 1];

    // 1<<0 - ochen
    // 1<<1 - lava/slime
    // 1<<2 - water
    // 1<<6 - all water
    // 1<<7 - dark water
    // == 0x0F - not show liquid
    uint8 flags[ADT_CELL_SIZE][ADT_CELL_SIZE];
    uint8 data[84];
    bool  prepareLoadedData();
};

//
// Adt file cell chunk
//
class adt_MCNK
{
    union
    {
        uint32 fcc;
        char   fcc_txt[4];
    };
public:
    uint32 size;
    uint32 flags;
    uint32 ix;
    uint32 iy;
    uint32 nLayers;
    uint32 nDoodadRefs;
    uint32 offsMCVT;        // height map
    uint32 offsMCNR;        // Normal vectors for each vertex
    uint32 offsMCLY;        // Texture layer definitions
    uint32 offsMCRF;        // A list of indices into the parent file's MDDF chunk
    uint32 offsMCAL;        // Alpha maps for additional texture layers
    uint32 sizeMCAL;
    uint32 offsMCSH;        // Shadow map for static shadows on the terrain
    uint32 sizeMCSH;
    uint32 areaid;
    uint32 nMapObjRefs;
    uint32 holes;
    uint16 s[2];
    uint32 data1;
    uint32 data2;
    uint32 data3;
    uint32 predTex;
    uint32 nEffectDoodad;
    uint32 offsMCSE;
    uint32 nSndEmitters;
    uint32 offsMCLQ;         // Liqid level (old)
    uint32 sizeMCLQ;         //
    float  zpos;
    float  xpos;
    float  ypos;
    uint32 offsMCCV;         // offsColorValues in WotLK
    uint32 props;
    uint32 effectId;

    bool   prepareLoadedData();
    adt_MCVT* getMCVT()
    {
        if (offsMCVT)
            return (adt_MCVT*)((uint8*)this + offsMCVT);
        return nullptr;
    }
    adt_MCLQ* getMCLQ()
    {
        if (offsMCLQ)
            return (adt_MCLQ*)((uint8*)this + offsMCLQ);
        return nullptr;
    }
};

//
// Adt file grid chunk
//
class adt_MCIN
{
    union
    {
        uint32 fcc;
        char   fcc_txt[4];
    };
public:
    uint32 size;
    struct adt_CELLS
    {
        uint32 offsMCNK;
        uint32 size;
        uint32 flags;
        uint32 asyncId;
    } cells[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

    bool   prepareLoadedData();
    // offset from begin file (used this-84)
    adt_MCNK* getMCNK(int x, int y)
    {
        if (cells[x][y].offsMCNK)
            return (adt_MCNK*)((uint8*)this + cells[x][y].offsMCNK - 84);
        return nullptr;
    }
};

#define ADT_LIQUID_HEADER_FULL_LIGHT   0x01
#define ADT_LIQUID_HEADER_NO_HIGHT     0x02

struct adt_liquid_header
{
    uint16 liquidType;             // Index from LiquidType.dbc
    uint16 formatFlags;
    float  heightLevel1;
    float  heightLevel2;
    uint8  xOffset;
    uint8  yOffset;
    uint8  width;
    uint8  height;
    uint32 offsData2a;
    uint32 offsData2b;
};

//
// Adt file liquid data chunk (new)
//
class adt_MH2O
{
public:
    union
    {
        uint32 fcc;
        char   fcc_txt[4];
    };
    uint32 size;

    struct adt_LIQUID
    {
        uint32 offsData1;
        uint32 used;
        uint32 offsData2;
    } liquid[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

    bool   prepareLoadedData();

    adt_liquid_header* getLiquidData(int x, int y)
    {
        if (liquid[x][y].used && liquid[x][y].offsData1)
            return (adt_liquid_header*)((uint8*)this + 8 + liquid[x][y].offsData1);
        return nullptr;
    }

    float* getLiquidHeightMap(adt_liquid_header* h)
    {
        if (h->formatFlags & ADT_LIQUID_HEADER_NO_HIGHT)
            return nullptr;
        if (h->offsData2b)
            return (float*)((uint8*)this + 8 + h->offsData2b);
        return nullptr;
    }

    uint8* getLiquidLightMap(adt_liquid_header* h)
    {
        if (h->formatFlags & ADT_LIQUID_HEADER_FULL_LIGHT)
            return nullptr;
        if (h->offsData2b)
        {
            if (h->formatFlags & ADT_LIQUID_HEADER_NO_HIGHT)
                return (uint8*)((uint8*)this + 8 + h->offsData2b);
            return (uint8*)((uint8*)this + 8 + h->offsData2b + (h->width + 1) * (h->height + 1) * 4);
        }
        return nullptr;
    }

    uint32* getLiquidFullLightMap(adt_liquid_header* h)
    {
        if (!(h->formatFlags & ADT_LIQUID_HEADER_FULL_LIGHT))
            return nullptr;
        if (h->offsData2b)
        {
            if (h->formatFlags & ADT_LIQUID_HEADER_NO_HIGHT)
                return (uint32*)((uint8*)this + 8 + h->offsData2b);
            return (uint32*)((uint8*)this + 8 + h->offsData2b + (h->width + 1) * (h->height + 1) * 4);
        }
        return nullptr;
    }

    uint64 getLiquidShowMap(adt_liquid_header* h)
    {
        if (h->offsData2a)
            return *((uint64*)((uint8*)this + 8 + h->offsData2a));
        else
            return 0xFFFFFFFFFFFFFFFFuLL;
    }

};

// Adt file min/max height chunk
//
class adt_MFBO
{
    union
    {
        uint32 fcc;
        char   fcc_txt[4];
    };
public:
    uint32 size;
    struct plane
    {
        int16 coords[9];
    };
    plane max;
    plane min;

    bool prepareLoadedData();
};

//
// Adt file header chunk
//
class adt_MHDR
{
    union
    {
        uint32 fcc;
        char   fcc_txt[4];
    };
public:
    uint32 size;

    uint32 flags;
    uint32 offsMCIN;           // MCIN
    uint32 offsTex;            // MTEX
    uint32 offsModels;         // MMDX
    uint32 offsModelsIds;      // MMID
    uint32 offsMapObejcts;     // MWMO
    uint32 offsMapObejctsIds;  // MWID
    uint32 offsDoodsDef;       // MDDF
    uint32 offsObjectsDef;     // MODF
    uint32 offsMFBO;           // MFBO
    uint32 offsMH2O;           // MH2O
    uint32 data1;
    uint32 data2;
    uint32 data3;
    uint32 data4;
    uint32 data5;
public:
    bool prepareLoadedData();
    adt_MCIN* getMCIN()
    {
        return reinterpret_cast<adt_MCIN*>(reinterpret_cast<uint8*>(&flags) + offsMCIN);
    }
    adt_MH2O* getMH2O()
    {
        if (offsMH2O)
            return reinterpret_cast<adt_MH2O*>(reinterpret_cast<uint8*>(&flags) + offsMH2O);
        return nullptr;
    }
    adt_MFBO* getMFBO()
    {
        if (flags & 1 && offsMFBO)
            return reinterpret_cast<adt_MFBO*>(reinterpret_cast<uint8*>(&flags) + offsMFBO);
        return nullptr;
    }
};

class ADT_file : public FileLoader
{
public:
    bool prepareLoadedData() override;
    ADT_file();
    ~ADT_file();
    void free() override;

    adt_MHDR* a_grid;
};

#endif
