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




#include "app_tarcza.h"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

std::wstring const app_tarcza::s_main_class_name{ L"TarczaMainWindow" };
std::wstring const app_tarcza::s_child_class_name{ L"TarczaChildWindow" };

static int child_id_from_hwnd(HWND hwnd)
{
    return static_cast<int>(GetWindowLongPtrW(hwnd, GWLP_ID));
}

double app_tarcza::initial_angle_for_id(int id)
{
    return static_cast<double>(id) * std::numbers::pi / 5.0;
}

double app_tarcza::normalize_angle(double a)
{
    while (a <= -std::numbers::pi)
        a += 2.0 * std::numbers::pi;
    while (a > std::numbers::pi)
        a -= 2.0 * std::numbers::pi;
    return a;
}

app_tarcza::app_tarcza(HINSTANCE instance)
    : m_instance(instance)
{
    if (!register_main_class())
        throw std::runtime_error("Nie udalo sie zarejestrowac klasy glownej.");

    if (!register_child_class())
        throw std::runtime_error("Nie udalo sie zarejestrowac klasy dzieci.");

    m_main = create_main_window();
    if (!m_main)
        throw std::runtime_error("Nie udalo sie utworzyc okna glownego.");
}

bool app_tarcza::register_main_class()
{
    WNDCLASSEXW wc{};
    if (GetClassInfoExW(m_instance, s_main_class_name.c_str(), &wc) != 0)
        return true;

    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = main_window_proc_static;
    wc.hInstance = m_instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    wc.lpszClassName = s_main_class_name.c_str();

    return RegisterClassExW(&wc) != 0;
}

bool app_tarcza::register_child_class()
{
    WNDCLASSEXW wc{};
    if (GetClassInfoExW(m_instance, s_child_class_name.c_str(), &wc) != 0)
        return true;

    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = child_window_proc_static;
    wc.hInstance = m_instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    wc.lpszClassName = s_child_class_name.c_str();

    return RegisterClassExW(&wc) != 0;
}

HWND app_tarcza::create_main_window()
{
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT wr{ 0, 0, s_client_size, s_client_size };
    AdjustWindowRectEx(&wr, style, FALSE, 0);

    int outer_w = wr.right - wr.left;
    int outer_h = wr.bottom - wr.top;

    RECT desktop{};
    GetWindowRect(GetDesktopWindow(), &desktop);

    int screen_cx = (desktop.right - desktop.left) / 2;
    int screen_cy = (desktop.bottom - desktop.top) / 2;

    int client_center_offset_x = -wr.left + s_client_size / 2;
    int client_center_offset_y = -wr.top + s_client_size / 2;

    int x = screen_cx - client_center_offset_x;
    int y = screen_cy - client_center_offset_y;

    return CreateWindowExW(
        0,
        s_main_class_name.c_str(),
        L"Tarcza numeryczna",
        style,
        x,
        y,
        outer_w,
        outer_h,
        nullptr,
        nullptr,
        m_instance,
        this
    );
}

void app_tarcza::create_children()
{
    for (int i = 0; i < s_child_count; ++i)
    {
        std::wstring title = std::to_wstring(i);

        CreateWindowExW(
            0,
            s_child_class_name.c_str(),
            title.c_str(),
            WS_CHILD | WS_VISIBLE | WS_CAPTION,
            0, 0,
            s_child_size, s_child_size,
            m_main,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(i)),
            m_instance,
            nullptr
        );
    }

    layout_children(0.0);
}

POINT app_tarcza::get_main_client_origin_screen() const
{
    POINT p{ 0, 0 };
    ClientToScreen(m_main, &p);
    return p;
}

POINT app_tarcza::get_main_client_center_screen() const
{
    RECT rc{};
    GetClientRect(m_main, &rc);

    POINT p{ (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
    ClientToScreen(m_main, &p);
    return p;
}

RECT app_tarcza::child_window_rect_on_circle(int id, double delta) const
{
    POINT center = get_main_client_center_screen();
    POINT client_origin = get_main_client_origin_screen();

    double angle = initial_angle_for_id(id) + delta;

    int cx = static_cast<int>(std::lround(center.x + s_radius * std::cos(angle)));
    int cy = static_cast<int>(std::lround(center.y + s_radius * std::sin(angle)));

    int left = cx - client_origin.x - s_child_size / 2;
    int top  = cy - client_origin.y - s_child_size / 2;

    RECT r{};
    r.left = left;
    r.top = top;
    r.right = left + s_child_size;
    r.bottom = top + s_child_size;
    return r;
}

void app_tarcza::layout_children(double delta)
{
    if (!m_main)
        return;

    m_internal_move = true;

    for (int i = 0; i < s_child_count; ++i)
    {
        HWND child = GetDlgItem(m_main, i);
        if (!child)
            continue;

        RECT r = child_window_rect_on_circle(i, delta);

        SetWindowPos(
            child,
            nullptr,
            r.left,
            r.top,
            r.right - r.left,
            r.bottom - r.top,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
    }

    m_internal_move = false;
}

void app_tarcza::begin_drag(HWND child, POINT cursor_screen)
{
    KillTimer(m_main, s_return_timer);

    m_dragging = true;
    m_dragging_id = child_id_from_hwnd(child);

    RECT rc{};
    GetWindowRect(child, &rc);

    m_drag_offset_screen.x = cursor_screen.x - rc.left;
    m_drag_offset_screen.y = cursor_screen.y - rc.top;

    SetCapture(child);
}

void app_tarcza::update_drag(HWND child, POINT cursor_screen)
{
    if (!m_dragging || m_internal_move)
        return;

    RECT child_rc{};
    GetWindowRect(child, &child_rc);

    int width = child_rc.right - child_rc.left;
    int height = child_rc.bottom - child_rc.top;

    int proposed_left = cursor_screen.x - m_drag_offset_screen.x;
    int proposed_top = cursor_screen.y - m_drag_offset_screen.y;

    int proposed_cx = proposed_left + width / 2;
    int proposed_cy = proposed_top + height / 2;

    POINT parent_center = get_main_client_center_screen();

    double angle = std::atan2(
        static_cast<double>(proposed_cy - parent_center.y),
        static_cast<double>(proposed_cx - parent_center.x)
    );

    m_delta = normalize_angle(angle - initial_angle_for_id(m_dragging_id));
    layout_children(m_delta);
}

void app_tarcza::end_drag()
{
    if (!m_dragging)
        return;

    m_dragging = false;
    m_dragging_id = -1;
    ReleaseCapture();

    SetTimer(m_main, s_return_timer, s_return_interval_ms, nullptr);
}

void app_tarcza::animate_back_step()
{
    constexpr double step = std::numbers::pi / 60.0;

    if (std::abs(m_delta) <= step)
    {
        m_delta = 0.0;
        layout_children(m_delta);
        KillTimer(m_main, s_return_timer);
        return;
    }

    if (m_delta > 0.0)
        m_delta -= step;
    else
        m_delta += step;

    layout_children(m_delta);
}

LRESULT CALLBACK app_tarcza::main_window_proc_static(
    HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    app_tarcza* app = nullptr;

    if (message == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        app = static_cast<app_tarcza*>(cs->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
        app = reinterpret_cast<app_tarcza*>(
            GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (app)
        return app->main_window_proc(window, message, wparam, lparam);

    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT CALLBACK app_tarcza::child_window_proc_static(
    HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(window);
    if (!parent)
        return DefWindowProcW(window, message, wparam, lparam);

    auto* app = reinterpret_cast<app_tarcza*>(
        GetWindowLongPtrW(parent, GWLP_USERDATA));

    if (app)
        return app->child_window_proc(window, message, wparam, lparam);

    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT app_tarcza::main_window_proc(
    HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_CREATE:
        create_children();
        return 0;

    case WM_TIMER:
        if (wparam == s_return_timer)
        {
            animate_back_step();
            return 0;
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT app_tarcza::child_window_proc(
    HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_NCLBUTTONDOWN:
    {
        POINT p{
            GET_X_LPARAM(lparam),
            GET_Y_LPARAM(lparam)
        };
        begin_drag(window, p);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        POINT p{
            GET_X_LPARAM(lparam),
            GET_Y_LPARAM(lparam)
        };
        ClientToScreen(window, &p);
        begin_drag(window, p);
        return 0;
    }

    case WM_MOUSEMOVE:
        if (m_dragging && GetCapture() == window)
        {
            POINT p{
                GET_X_LPARAM(lparam),
                GET_Y_LPARAM(lparam)
            };
            ClientToScreen(window, &p);
            update_drag(window, p);
            return 0;
        }
        break;

    case WM_LBUTTONUP:
    case WM_NCLBUTTONUP:
        if (m_dragging && GetCapture() == window)
        {
            end_drag();
            return 0;
        }
        break;

    case WM_CAPTURECHANGED:
        if (m_dragging && reinterpret_cast<HWND>(lparam) != window)
        {
            end_drag();
            return 0;
        }
        break;
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

int app_tarcza::run(int show_command)
{
    ShowWindow(m_main, show_command);
    UpdateWindow(m_main);

    MSG msg{};
    while (true)
    {
        BOOL result = GetMessageW(&msg, nullptr, 0, 0);
        if (result == -1)
            return 1;
        if (result == 0)
            break;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
