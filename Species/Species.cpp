#include "pch.h"

// The client's entry point (ADR-090 item 2). Its composition root, the client session and the integrated server,
// arrives with M0 task T7; until then the program starts and ends.
int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
  return 0;
}
