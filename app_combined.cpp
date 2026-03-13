#define _USE_MATH_DEFINES
#include "app_combined.h"
#include <cmath>
#include <string>

std::wstring const app_combined::s_class_name{ L"RotarySpringWindow" };

app_combined::app_combined(HINSTANCE instance)
    : m_instance{ instance }, m_main{}, m_is_spring_mode{ false },
      m_mouse_pos{ 200, 200 }, m_is_dragging{ false }, m_current_angle{ 0.0 }
{
    // Inicjalizacja tablic fizyki
    for (int i = 0; i < 10; ++i) {
        m_x[i] = 200.0; m_y[i] = 200.0;
        m_vx[i] = 0.0;  m_vy[i] = 0.0;
    }

    register_class(); // Rejestracja klasy wg wykładu
    m_main = create_window();
}

bool app_combined::register_class() // [cite: 102-112]
{
    WNDCLASSEXW desc{};
    if (GetClassInfoExW(m_instance, s_class_name.c_str(), &desc) != 0)
        return true;

    desc = {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = window_proc_static,
        .hInstance = m_instance,
        .hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW),
        .hbrBackground = CreateSolidBrush(RGB(230, 230, 240)),
        .lpszClassName = s_class_name.c_str()
    };
    return RegisterClassExW(&desc) != 0;
}

HWND app_combined::create_window() // [cite: 118-129]
{
    HWND window = CreateWindowExW(
        0, s_class_name.c_str(), L"Tarcza i Sprężynka (Prawy Przycisk: Zmiana Trybu)",
        WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 500, 500,
        nullptr, nullptr, m_instance, this);

    // Tworzenie 10 okienek potomnych. Styl WS_DISABLED przekazuje kliknięcia do okna głównego [cite: 342-355]
    for (int i = 0; i < 10; ++i)
    {
        std::wstring text = std::to_wstring((i + 1) % 10);
        m_digits[i] = CreateWindowExW(
            0, L"STATIC", text.c_str(),
            WS_CHILD | WS_VISIBLE | SS_CENTER | WS_DISABLED,
            200, 200, 30, 30, 
            window, nullptr, m_instance, nullptr);
    }
    
    return window;
}

LRESULT app_combined::window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // [cite: 136-161]
{
    app_combined* app = nullptr;
    if (message == WM_NCCREATE)
    {
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        app = static_cast<app_combined*>(p->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
        app = reinterpret_cast<app_combined*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (app != nullptr) return app->window_proc(window, message, wparam, lparam);
    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT app_combined::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    const int center_x = 250;
    const int center_y = 250;

    switch (message)
    {
    case WM_CREATE:
        SetTimer(window, s_timer, 16, nullptr); // Timer działający w ok. 60 FPS [cite: 642]
        return 0;

    case WM_RBUTTONDOWN: // Przełączanie trybów (Prawy Przycisk Myszy)
        m_is_spring_mode = !m_is_spring_mode;
        m_is_dragging = false;
        return 0;

    case WM_LBUTTONDOWN: // Łapanie tarczy
        if (!m_is_spring_mode) {
            m_mouse_pos.x = LOWORD(lparam); m_mouse_pos.y = HIWORD(lparam);
            m_is_dragging = true;
            m_start_drag_angle = m_current_angle;
            m_start_drag_mouse_angle = std::atan2(m_mouse_pos.y - center_y, m_mouse_pos.x - center_x);
            SetCapture(window);
        }
        return 0;

    case WM_MOUSEMOVE:
        m_mouse_pos.x = LOWORD(lparam); 
        m_mouse_pos.y = HIWORD(lparam);

        if (m_is_dragging && !m_is_spring_mode) {
            double current_mouse_angle = std::atan2(m_mouse_pos.y - center_y, m_mouse_pos.x - center_x);
            double delta = current_mouse_angle - m_start_drag_mouse_angle;
            
            if (delta > M_PI) delta -= 2 * M_PI;
            if (delta < -M_PI) delta += 2 * M_PI;

            m_current_angle = m_start_drag_angle + delta;
            if (m_current_angle < 0) m_current_angle = 0;
            if (m_current_angle > M_PI * 1.5) m_current_angle = M_PI * 1.5; // Limit obrotu
        }
        return 0;

    case WM_LBUTTONUP:
        if (m_is_dragging) {
            m_is_dragging = false;
            ReleaseCapture();
        }
        return 0;

    case WM_TIMER: // Główna pętla fizyki [cite: 649-651]
        if (wparam == s_timer) {
            // Animacja powrotu tarczy
            if (!m_is_spring_mode && !m_is_dragging && m_current_angle > 0) {
                m_current_angle -= 0.05;
                if (m_current_angle < 0) m_current_angle = 0;
            }

            // WSPÓLNA FIZYKA SPRĘŻYNY
            double k = 0.2;     // Siła napięcia sprężyny (sztywność)
            double damp = 0.65; // Tłumienie (im niższe, tym mniej oscylacji)

            for (int i = 0; i < 10; ++i) {
                double target_x, target_y;

                if (m_is_spring_mode) {
                    // Tryb sprężynki: pierwsza cyfra goni kursor, reszta goni poprzednika
                    if (i == 0) { target_x = m_mouse_pos.x; target_y = m_mouse_pos.y; }
                    else { target_x = m_x[i - 1]; target_y = m_y[i - 1]; }
                } else {
                    // Tryb tarczy: celem jest konkretny punkt na okręgu
                    double base_angle = (i * 30.0) * M_PI / 180.0;
                    double total_angle = base_angle + m_current_angle;
                    target_x = center_x + 120 * std::cos(total_angle);
                    target_y = center_y + 120 * std::sin(total_angle);
                }

                // Obliczanie siły i aktualizacja pozycji (prymitywne całkowanie Verleta/Eulera)
                m_vx[i] = (m_vx[i] + (target_x - m_x[i]) * k) * damp;
                m_vy[i] = (m_vy[i] + (target_y - m_y[i]) * k) * damp;
                
                m_x[i] += m_vx[i];
                m_y[i] += m_vy[i];

                SetWindowPos(m_digits[i], nullptr, (int)m_x[i] - 15, (int)m_y[i] - 15, 0, 0, 
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        return 0;

    case WM_CLOSE: // [cite: 171-173]
        DestroyWindow(window);
        return 0;
    case WM_DESTROY: // [cite: 174-177]
        PostQuitMessage(EXIT_SUCCESS);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int app_combined::run(int show_command) // [cite: 194-202]
{
    ShowWindow(m_main, show_command);
    MSG msg{};
    BOOL result = TRUE;
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0)
    {
        if (result == -1) return EXIT_FAILURE;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return EXIT_SUCCESS;
}
