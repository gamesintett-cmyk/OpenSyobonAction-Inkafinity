#ifndef INKAFINITY_INTERACTIVE_H
#define INKAFINITY_INTERACTIVE_H

#include <string>

void Inkafinity_StartServer(int port = 5755);
void Inkafinity_StopServer();
void Inkafinity_ProcessEvents();
void Inkafinity_SetNextViewerName(const std::string& name);
std::string Inkafinity_TakeNextViewerName();
void Inkafinity_SetEnemyViewerName(int slot, const std::string& name);
const char* Inkafinity_GetEnemyViewerName(int slot);
void Inkafinity_DrawHud();

#endif
