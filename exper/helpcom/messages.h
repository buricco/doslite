/*
 * Copyright 2024 S. V. Nickolas.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the Software), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _H_MESSAGES_
#define _H_MESSAGES_
/* 
 * Strings affected by localization.
 * Things to note about how this is designed:
 * 
 *   1. Accelerator keys.  These are highlighted under certain circumstances
 *      and can be pressed to drop down a menu or initiate an action; they
 *      need to be marked in the strings.  The way they are marked here is
 *      the same way Windows and Visual Basic do it.
 *   2. w_status needs to be treated as three hot spots, where clicking the
 *      mouse should cause an action.
 */
static char *w_file     = "&File",
            *w_search   = "&Search",
            *w_help     = "&Help",
            *w_print    = "&Print...",
            *w_exit     = "E&xit",
            *w_find     = "&Find...",
            *w_refind   = "&Repeat Last Find     F3",
            *w_howto    = "&How to Use Help",
            *w_about    = "&About...",
            *w_status   = "<Alt+C=Contents> <Alt+N=Next> <Alt+B=Back>",
            *w_alt      = "F1=Help   Enter=Display Menu   Esc=Cancel   Arrow=Next Item",
            *w_f1help   = "F1=Help",
            *w_sthelp   = "Prints specified text",
            *w_stexit   = "Exits Help and returns to DOS",
            *w_stfind   = "Finds specified text",
            *w_strefind = "Finds next occurrence of text specified in previous search",
            *w_sthowto  = "Displays information on using Help",
            *w_stabout  = "Displays project version and copyright information",
            *d_ok       = "OK",
            *d_cancel   = "Cancel",
            *d_help     = "&Help",
            *d_print1   = "Print",
            *d_print2   = "Print the current topic to:",
            *d_print3   = "&Printer on",
            *d_print4   = "&File",
            *d_print5   = "Printer &Setup...",
            *d_setup1   = "Printer Setup",
            *d_setup2   = "Use the printer connected to:",
            *d_find1    = "Find",
            *d_find2    = "&Find What:",
            *d_find3    = "&Match Upper/Lowercase",
            *d_find4    = "&Whole Word",
            *d_about1   = "PC DOS/RE Help Viewer",
            *d_about2   = "Version " HELP_VERSION,
            *d_about3   = "Copyright (C) PC DOS/RE Project 2024.",
            *e_long     = "Topic name too long",
            *e_ram      = "Insufficient memory",
            *e_file     = "Cannot open help file",
            *e_404      = "Topic not found";
#endif /* _H_MESSAGES_ */
