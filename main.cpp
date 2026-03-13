#include <windows.h>
#include "app_combined.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int show_command)
{
    app_combined app{ instance };
    return app.run(show_command);
}
