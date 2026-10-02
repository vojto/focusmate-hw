#pragma once

/*
 * Development tools that exist only in the `debug` environment; in the normal
 * build both functions do nothing. See debug.cpp and tools/screenshot.py.
 */

#ifdef DEBUG_TOOLS
void debugSetup();
void debugLoop();
#else
inline void debugSetup() {}
inline void debugLoop() {}
#endif
