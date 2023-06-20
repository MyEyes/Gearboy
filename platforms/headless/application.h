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

#ifndef APPLICATION_H
#define	APPLICATION_H


#ifdef APPLICATION_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN int application_gamepad_mappings;
EXTERN float application_display_scale;
EXTERN bool application_fullscreen;

EXTERN int application_init(const char* rom, const char* runner);
EXTERN void application_destroy(void);
EXTERN void application_mainloop(void);
EXTERN void application_trigger_quit(void);
EXTERN void application_trigger_fullscreen(bool fullscreen);

#undef APPLICATION_IMPORT
#undef EXTERN
#endif	/* APPLICATION_H */