/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once

#ifndef APIPRIVATE_H_INCLUDED
#define APIPRIVATE_H_INCLUDED

namespace DFHack
{
    class Module;

    class Creatures;
    class Engravings;
    class Maps;
    class Gui;
    class World;
    class Materials;
    class Items;
    class Translation;
    class Vegetation;
    class Buildings;
    class Constructions;
    class WindowIO;

    class ProcessEnumerator;
    class Process;
    class VersionInfo;
    struct t_name;
    class DFContextShared
    {
    public:
        DFContextShared();
        ~DFContextShared();

        // names, used by a few other modules.
        void readName(t_name & name, uint32_t address);
        void copyName(uint32_t address, uint32_t target);
        // get the name offsets
        bool InitReadNames();
        uint32_t name_firstname_offset;
        uint32_t name_nickname_offset;
        uint32_t name_words_offset;
        uint32_t name_parts_offset;
        uint32_t name_language_offset;
        uint32_t name_set_offset;
        bool namesInited;

        ProcessEnumerator* pm;
        Process* p;
        char * shm_start;
        VersionInfo* offset_descriptor;
        string xml;

        // Modules
        struct
        {
            Creatures * pCreatures;
            Engravings * pEngravings;
            Maps * pMaps;
            Gui * pPosition; // blerp
            Gui * pGui;
            World * pWorld;
            Materials * pMaterials;
            Items * pItems;
            Translation * pTranslation;
            Vegetation * pVegetation;
            Buildings * pBuildings;
            Constructions * pConstructions;
            WindowIO * pWindowIO;
        } s_mods;
        std::vector <Module *> allModules;
        /*
        uint32_t item_material_offset;

        uint32_t note_foreground_offset;
        uint32_t note_background_offset;
        uint32_t note_name_offset;
        uint32_t note_xyz_offset;

        uint32_t settlement_name_offset;
        uint32_t settlement_world_xy_offset;
        uint32_t settlement_local_xy_offset;

        uint32_t dwarf_lang_table_offset;

        DfVector *p_effect;
        DfVector *p_itm;
        DfVector *p_notes;
        DfVector *p_settlements;
        DfVector *p_current_settlement;
        */
    };
}
#endif

