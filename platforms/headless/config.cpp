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

#include "../../src/gearboy.h"
#define MINI_CASE_SENSITIVE
#include "mINI/ini.h"

#define CONFIG_IMPORT
#include "config.h"
#include <unistd.h>

static bool check_portable(void);
static int read_int(const char* group, const char* key, int default_value);
static void write_int(const char* group, const char* key, int integer);
static float read_float(const char* group, const char* key, float default_value);
static void write_float(const char* group, const char* key, float value);
static bool read_bool(const char* group, const char* key, bool default_value);
static void write_bool(const char* group, const char* key, bool boolean);
static std::string read_string(const char* group, const char* key);
static void write_string(const char* group, const char* key, std::string value);
char cwd[PATH_MAX];

void config_init(void)
{
    getcwd(cwd, sizeof(cwd));
    config_root_path = cwd;

    strcpy(config_emu_file_path, config_root_path);
    strcat(config_emu_file_path, "config.ini");

    strcpy(config_imgui_file_path, config_root_path);
    strcat(config_imgui_file_path, "imgui.ini");

    config_ini_file = new mINI::INIFile(config_emu_file_path);
}

void config_destroy(void)
{
    SafeDelete(config_ini_file)
}

void config_read(void)
{
    if (!config_ini_file->read(config_ini_data))
    {
        Log("Unable to load settings from %s", config_emu_file_path);
        return;
    }

    Log("Loading settings from %s", config_emu_file_path);

    config_debug.debug = read_bool("Debug", "Debug", false);
    config_debug.show_audio = read_bool("Debug", "Audio", false);
    config_debug.show_disassembler = read_bool("Debug", "Disassembler", true);
    config_debug.show_gameboy = read_bool("Debug", "GameBoy", true);
    config_debug.show_iomap = read_bool("Debug", "IOMap", false);
    config_debug.show_memory = read_bool("Debug", "Memory", true);
    config_debug.show_processor = read_bool("Debug", "Processor", true);
    config_debug.show_video = read_bool("Debug", "Video", false);
    config_debug.font_size = read_int("Debug", "FontSize", 0);
    
    config_emulator.ffwd_speed = read_int("Emulator", "FFWD", 1);
    config_emulator.save_slot = read_int("Emulator", "SaveSlot", 0);
    config_emulator.start_paused = read_bool("Emulator", "StartPaused", false);
    config_emulator.force_dmg = read_bool("Emulator", "ForceDMG", false);
    config_emulator.force_gba = read_bool("Emulator", "ForceGBA", false);
    config_emulator.mbc = read_int("Emulator", "MBC", 0);
    config_emulator.dmg_bootrom = read_bool("Emulator", "DMGBootrom", false);
    config_emulator.dmg_bootrom_path = read_string("Emulator", "DMGBootromPath");
    config_emulator.gbc_bootrom = read_bool("Emulator", "GBCBootrom", false);
    config_emulator.gbc_bootrom_path = read_string("Emulator", "GBCBootromPath");
    config_emulator.savefiles_dir_option = read_int("Emulator", "SaveFilesDirOption", 0);
    config_emulator.savefiles_path = read_string("Emulator", "SaveFilesPath");
    config_emulator.savestates_dir_option = read_int("Emulator", "SaveStatesDirOption", 0);
    config_emulator.savestates_path = read_string("Emulator", "SaveStatesPath");

    if (config_emulator.savefiles_path.empty())
    {
        config_emulator.savefiles_path = config_root_path;
    }
    if (config_emulator.savestates_path.empty())
    {
        config_emulator.savestates_path = config_root_path;
    }

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        config_emulator.recent_roms[i] = read_string("Emulator", item.c_str());
    }

    config_video.scale = read_int("Video", "Scale", 0);
    config_video.ratio = read_int("Video", "AspectRatio", 0);
    config_video.fps = read_bool("Video", "FPS", false);
    config_video.bilinear = read_bool("Video", "Bilinear", false);
    config_video.mix_frames = read_bool("Video", "MixFrames", true);
    config_video.mix_frames_intensity = read_float("Video", "MixFramesIntensity", 0.50f);
    config_video.matrix = read_bool("Video", "Matrix", true);
    config_video.matrix_intensity = read_float("Video", "MatrixIntensity", 0.30f);
    config_video.palette = read_int("Video", "Palette", 0);
    for (int i = 0; i < config_max_custom_palettes; i++)
    {
        for (int c = 0; c < 4; c++)
        {
            char pal_label_r[32];
            char pal_label_g[32];
            char pal_label_b[32];
            sprintf(pal_label_r, "CustomPalette%i%iR", i, c);
            sprintf(pal_label_g, "CustomPalette%i%iG", i, c);
            sprintf(pal_label_b, "CustomPalette%i%iB", i, c);
            config_video.color[i][c].red = read_int("Video", pal_label_r, config_video.color[i][c].red);
            config_video.color[i][c].green = read_int("Video", pal_label_g, config_video.color[i][c].green);
            config_video.color[i][c].blue = read_int("Video", pal_label_b, config_video.color[i][c].blue);
        }
    }
    config_video.sync = read_bool("Video", "Sync", true);
    config_video.color_correction = read_bool("Video", "ColorCorrection", true);
    
    config_audio.enable = read_bool("Audio", "Enable", true);
    config_audio.sync = read_bool("Audio", "Sync", true);


    config_input.gamepad = read_bool("Input", "Gamepad", true);
    config_input.gamepad_directional = read_int("Input", "GamepadDirectional", 0);
    config_input.gamepad_invert_x_axis = read_bool("Input", "GamepadInvertX", false);
    config_input.gamepad_invert_y_axis = read_bool("Input", "GamepadInvertY", false);

    Log("Settings loaded");
}

void config_write(void)
{
    Log("Saving settings to %s", config_emu_file_path);

    write_bool("Debug", "Debug", config_debug.debug);
    write_bool("Debug", "Audio", config_debug.show_audio);
    write_bool("Debug", "Disassembler", config_debug.show_disassembler);
    write_bool("Debug", "GameBoy", config_debug.show_gameboy);
    write_bool("Debug", "IOMap", config_debug.show_iomap);
    write_bool("Debug", "Memory", config_debug.show_memory);
    write_bool("Debug", "Processor", config_debug.show_processor);
    write_bool("Debug", "Video", config_debug.show_video);
    write_int("Debug", "FontSize", config_debug.font_size);

    write_int("Emulator", "FFWD", config_emulator.ffwd_speed);
    write_int("Emulator", "SaveSlot", config_emulator.save_slot);
    write_bool("Emulator", "StartPaused", config_emulator.start_paused);
    write_bool("Emulator", "ForceDMG", config_emulator.force_dmg);
    write_bool("Emulator", "ForceGBA", config_emulator.force_gba);
    write_int("Emulator", "MBC", config_emulator.mbc);
    write_bool("Emulator", "DMGBootrom", config_emulator.dmg_bootrom);
    write_string("Emulator", "DMGBootromPath", config_emulator.dmg_bootrom_path);
    write_bool("Emulator", "GBCBootrom", config_emulator.gbc_bootrom);
    write_string("Emulator", "GBCBootromPath", config_emulator.gbc_bootrom_path);
    write_int("Emulator", "SaveFilesDirOption", config_emulator.savefiles_dir_option);
    write_string("Emulator", "SaveFilesPath", config_emulator.savefiles_path);
    write_int("Emulator", "SaveStatesDirOption", config_emulator.savestates_dir_option);
    write_string("Emulator", "SaveStatesPath", config_emulator.savestates_path);

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        write_string("Emulator", item.c_str(), config_emulator.recent_roms[i]);
    }

    write_int("Video", "Scale", config_video.scale);
    write_int("Video", "AspectRatio", config_video.ratio);
    write_bool("Video", "FPS", config_video.fps);
    write_bool("Video", "Bilinear", config_video.bilinear);
    write_bool("Video", "MixFrames", config_video.mix_frames);
    write_float("Video", "MixFramesIntensity", config_video.mix_frames_intensity);
    write_bool("Video", "Matrix", config_video.matrix);
    write_float("Video", "MatrixIntensity", config_video.matrix_intensity);
    write_int("Video", "Palette", config_video.palette);
    for (int i = 0; i < config_max_custom_palettes; i++)
    {
        for (int c = 0; c < 4; c++)
        {
            char pal_label_r[32];
            char pal_label_g[32];
            char pal_label_b[32];
            sprintf(pal_label_r, "CustomPalette%i%iR", i, c);
            sprintf(pal_label_g, "CustomPalette%i%iG", i, c);
            sprintf(pal_label_b, "CustomPalette%i%iB", i, c);
            write_int("Video", pal_label_r, config_video.color[i][c].red);
            write_int("Video", pal_label_g, config_video.color[i][c].green);
            write_int("Video", pal_label_b, config_video.color[i][c].blue);
        }
    }
    write_bool("Video", "Sync", config_video.sync);
    write_bool("Video", "ColorCorrection", config_video.color_correction);

    write_bool("Audio", "Enable", config_audio.enable);
    write_bool("Audio", "Sync", config_audio.sync);

    write_bool("Input", "Gamepad", config_input.gamepad);
    write_int("Input", "GamepadDirectional", config_input.gamepad_directional);
    write_bool("Input", "GamepadInvertX", config_input.gamepad_invert_x_axis);
    write_bool("Input", "GamepadInvertY", config_input.gamepad_invert_y_axis);

    if (config_ini_file->write(config_ini_data, true))
    {
        Log("Settings saved");
    }
}

static bool check_portable(void)
{
    return false;
}

static int read_int(const char* group, const char* key, int default_value)
{
    int ret = 0;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = std::stoi(value);

    Log("Load setting: [%s][%s]=%d", group, key, ret);
    return ret;
}

static void write_int(const char* group, const char* key, int integer)
{
    std::string value = std::to_string(integer);
    config_ini_data[group][key] = value;
    Log("Save setting: [%s][%s]=%s", group, key, value.c_str());
}

static float read_float(const char* group, const char* key, float default_value)
{
    float ret = 0.0f;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = strtof(value.c_str(), NULL);

    Log("Load setting: [%s][%s]=%.2f", group, key, ret);
    return ret;
}

static void write_float(const char* group, const char* key, float value)
{
    std::string value_str = std::to_string(value);
    config_ini_data[group][key] = value_str;
    Log("Save setting: [%s][%s]=%s", group, key, value_str.c_str());
}

static bool read_bool(const char* group, const char* key, bool default_value)
{
    bool ret;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        std::istringstream(value) >> std::boolalpha >> ret;

    Log("Load setting: [%s][%s]=%s", group, key, ret ? "true" : "false");
    return ret;
}

static void write_bool(const char* group, const char* key, bool boolean)
{
    std::stringstream converter;
    converter << std::boolalpha << boolean;
    std::string value;
    value = converter.str();
    config_ini_data[group][key] = value;
    Log("Save setting: [%s][%s]=%s", group, key, value.c_str());
}

static std::string read_string(const char* group, const char* key)
{
    std::string ret = config_ini_data[group][key];
    Log("Load setting: [%s][%s]=%s", group, key, ret.c_str());
    return ret;
}

static void write_string(const char* group, const char* key, std::string value)
{
    config_ini_data[group][key] = value;
    Log("Save setting: [%s][%s]=%s", group, key, value.c_str());
}