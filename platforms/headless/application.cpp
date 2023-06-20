/*
 * Gearboy - Nintendo Game Boy Emulator
 * Copyright (C) 2012  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */
#include "emu.h"
#include "config.h"

#define APPLICATION_IMPORT
#include "application.h"
#include "runner.h"

static bool running = true;
static bool config_initialized = false;
static bool emu_initialized = false;
static runner_t* runner;

static void run_emulator(void);

static Cartridge::CartridgeTypes get_mbc(int index)
{
    switch (index)
    {
        case 0:
            return Cartridge::CartridgeNotSupported;
        case 1:
            return Cartridge::CartridgeNoMBC;
        case 2:
            return Cartridge::CartridgeMBC1;
        case 3:
            return Cartridge::CartridgeMBC2;
        case 4:
            return Cartridge::CartridgeMBC3;
        case 5:
            return Cartridge::CartridgeMBC5;
        case 6:
            return Cartridge::CartridgeMBC1Multi;
        default:
            return Cartridge::CartridgeNotSupported;
    }
}

void load_rom(const char* path)
{
    emu_resume();
    emu_load_rom(path, config_emulator.force_dmg, get_mbc(config_emulator.mbc), config_emulator.force_gba);
    emu_clear_cheats();


    std::string str(path);
    str = str.substr(0, str.find_last_of("."));
    str += ".sym";

    if (config_emulator.start_paused)
    {
        emu_pause();
        
        for (int i=0; i < (GAMEBOY_WIDTH * GAMEBOY_HEIGHT); i++)
        {
            emu_frame_buffer[i].red = 0;
            emu_frame_buffer[i].green = 0;
            emu_frame_buffer[i].blue = 0;
        }
    }
}

int application_init(const char* rom, const char* runner_so)
{
    Log ("<·> %s %s Desktop App <·>", GEARBOY_TITLE, GEARBOY_VERSION);

    if (IsValidPointer(rom) && (strlen(rom) > 0))
    {
        Log ("Loading with argv: %s");
    }

    runner = runner_create(runner_so);
    if(!runner)
    {
        Log ("Couldn't load runner");
        return -1;
    }

    application_fullscreen = false;
    
    config_init();
    config_initialized = true;
    config_read();

    emu_init();
    emu_initialized = true;

    strcpy(emu_savefiles_path, config_emulator.savefiles_path.c_str());
    strcpy(emu_savestates_path, config_emulator.savestates_path.c_str());
    emu_savefiles_dir_option = config_emulator.savefiles_dir_option;
    emu_savestates_dir_option = config_emulator.savestates_dir_option;

    if (IsValidPointer(rom) && (strlen(rom) > 0))
    {
        load_rom(rom);
    }

    return 0;
}

void application_destroy(void)
{
    if(config_initialized)
    {
        config_write();
        config_destroy();
    }
    if(emu_initialized)
    {
        emu_destroy();
    }
}

void application_mainloop(void)
{
    while (running)
    {
        run_emulator();
    }
}

static void run_emulator(void)
{
    if (!emu_is_empty())
    {
        static int i = 0;
        i++;

        if (i > 20)
        {
            i = 0;

            char title[256];
            sprintf(title, "%s %s - %s", GEARBOY_TITLE, GEARBOY_VERSION, emu_get_core()->GetCartridge()->GetFileName());
        }
    }
    config_emulator.paused = emu_is_paused();
    emu_audio_sync = config_audio.sync;
    runner_pre_frame(runner, emu_get_core());
    emu_update();
    runner_post_frame(runner, emu_get_core());
}
