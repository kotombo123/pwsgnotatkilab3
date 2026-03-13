#pragma once
#include <windows.h>
#include <string>
#include <array>

class app_combined
{
private:
    bool register_class();
    static std::wstring const s_class_name;
    
    // Procedury okna oparte na przykładzie z wykładu
    static LRESULT CALLBACK window_proc_static(
        HWND window, UINT message,
        WPARAM wparam, LPARAM lparam);
        
    LRESULT window_proc(
        HWND window, UINT message,
        WPARAM wparam, LPARAM lparam);
        
    HWND create_window();

    HINSTANCE m_instance;
    HWND m_main;
    std::array<HWND, 10> m_digits; 

    // Stan aplikacji
    bool m_is_spring_mode;
    POINT m_mouse_pos;

    // Logika obracania tarczy
    bool m_is_dragging;
    double m_current_angle;
    double m_start_drag_angle;
    double m_start_drag_mouse_angle;
    
    // Fizyka sprężynki
    std::array<double, 10> m_x, m_y, m_vx, m_vy;

    static constexpr UINT_PTR s_timer = 1;

public:
    app_combined(HINSTANCE instance);
    int run(int show_command);
};
