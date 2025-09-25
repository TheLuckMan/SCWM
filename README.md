# SCWM
    SCWM - Simple & Chill Window Manager
By TheLuckMan (aka. TheLikeMan) (Mark Platniek)
================================================
SCWM - Is Ultra-Minimalist hybrid X11 window manager which follows UNIX-way
philosphy - Do one thing and do it good. SCWM Inspired by DWM and CWM

It is compatible with GNU/Linux and BSD systems

It uses X11 and "sxhkd" daemon for keyboard combinations, and maybe you want
use statusbar like "lemonbar" or "dzen2". Also if you want to disable
tirring, then you can use compositor like "xcompmgr" or "picom"

- Why?
Just for fun.

- What means Hybrid Window Manager?
SCWM works well as tile or floating (classic) 

- How to configure this window manager?
Just edit configure "sxhkdrc" for sxhkd config file if you want to configure key
combinations (see sxhkdrc.example to understand work with scwmctl), or configure source file "scwm.c"
=================================================

Note: use "make" command to compile it, and "make install" as root to install
in /usr/local/bin, and "make clean" to clean scwm files

- Have made:
1. Core of WM
2. Modes such as - floating, tile like dwm, fullscreen
3. tags (or workspaces)
- Work in process
1. Multimonitor configuration (Xinerama)
2. fix bugs
3. better support EWMH

5. bundle with "sxhkd"
- Work in process
1. Multimonitor configuration
2. fix bugs
3. better support EWMH
