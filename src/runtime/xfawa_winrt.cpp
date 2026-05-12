#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct XRButtonStyle {
    COLORREF backgroundColor = RGB(128, 128, 128);
    COLORREF textColor = RGB(0, 0, 0);
    COLORREF borderColor = RGB(0, 0, 0);
    int borderRadius = 0;
    int borderWidth = 0;
};

struct XRStyleTable {
    XRButtonStyle button;
};

struct XRRectCommand {
    int x;
    int y;
    int width;
    int height;
    COLORREF color;
};

struct XRTextCommand {
    int x;
    int y;
    std::string text;
    COLORREF color;
};

struct XRButtonCommand {
    int x;
    int y;
    int width;
    int height;
    std::string text;
    XRButtonStyle style;
    void (*handler)() = nullptr;
};

struct XRBoxCommand {
    std::string id;
    int x;
    int y;
    int width;
    int height;
    std::string text;
    int scrollOffset = 0;
};

struct XRInputCommand {
    std::string id;
    std::string varName;
    int x;
    int y;
    int width;
    int height;
    std::string text;
    bool focused = false;
    HWND hwnd = nullptr;
};

HWND g_window = nullptr;
HINSTANCE g_instance = nullptr;
bool g_shouldClose = false;
bool g_classRegistered = false;
COLORREF g_clearColor = RGB(255, 255, 255);
XRStyleTable g_styles;
std::vector<XRRectCommand> g_rects;
std::vector<XRTextCommand> g_texts;
std::vector<XRButtonCommand> g_buttons;
std::vector<XRBoxCommand> g_boxes;
std::vector<XRInputCommand> g_inputs;

std::string trim(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(start, end - start);
}

std::string stripQuotes(const std::string& input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

COLORREF parseColor(const std::string& raw, COLORREF fallback) {
    std::string value = trim(raw);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (value == "red") return RGB(255, 0, 0);
    if (value == "green") return RGB(0, 255, 0);
    if (value == "blue") return RGB(0, 0, 255);
    if (value == "white") return RGB(255, 255, 255);
    if (value == "black") return RGB(0, 0, 0);
    if (value == "gray" || value == "grey") return RGB(128, 128, 128);
    if (value == "yellow") return RGB(255, 255, 0);
    if (value == "cyan") return RGB(0, 255, 255);
    if (value == "magenta") return RGB(255, 0, 255);
    if (!value.empty() && value[0] == '#' && value.size() == 7) {
        int red = std::stoi(value.substr(1, 2), nullptr, 16);
        int green = std::stoi(value.substr(3, 2), nullptr, 16);
        int blue = std::stoi(value.substr(5, 2), nullptr, 16);
        return RGB(red, green, blue);
    }

    return fallback;
}

std::filesystem::path resolveStylePath(const std::string& input) {
    std::filesystem::path requested(input);
    if (std::filesystem::exists(requested)) {
        return requested;
    }

    char modulePath[MAX_PATH] = {};
    DWORD length = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        std::filesystem::path exeDir(modulePath);
        exeDir = exeDir.parent_path();
        std::filesystem::path candidate = exeDir / requested;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return requested;
}

void applyDefaultStyles() {
    g_styles = XRStyleTable{};
    g_styles.button.backgroundColor = RGB(128, 128, 128);
    g_styles.button.textColor = RGB(0, 0, 0);
    g_styles.button.borderColor = RGB(0, 0, 0);
    g_styles.button.borderRadius = 0;
    g_styles.button.borderWidth = 0;
}

bool parseXssFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    applyDefaultStyles();

    bool inButtonBlock = false;
    std::string line;
    while (std::getline(file, line)) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line.erase(commentPos);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        if (!inButtonBlock) {
            if (line == "button{" || line == "button {") {
                inButtonBlock = true;
            }
            continue;
        }

        if (line == "}") {
            inButtonBlock = false;
            continue;
        }

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        if (!value.empty() && value.back() == ';') {
            value.pop_back();
            value = trim(value);
        }

        if (key == "background-color") {
            g_styles.button.backgroundColor = parseColor(value, g_styles.button.backgroundColor);
        } else if (key == "text-color") {
            g_styles.button.textColor = parseColor(value, g_styles.button.textColor);
        } else if (key == "border-radius") {
            g_styles.button.borderRadius = std::max(0, std::stoi(value));
        } else if (key == "border-width") {
            g_styles.button.borderWidth = std::max(0, std::stoi(value));
        } else if (key == "border-color") {
            g_styles.button.borderColor = parseColor(value, g_styles.button.borderColor);
        }
    }

    return true;
}

void paintRect(HDC dc, const XRRectCommand& rect) {
    HBRUSH brush = CreateSolidBrush(rect.color);
    RECT area{rect.x, rect.y, rect.x + rect.width, rect.y + rect.height};
    FillRect(dc, &area, brush);
    DeleteObject(brush);
}

void paintText(HDC dc, const XRTextCommand& text) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, text.color);
    TextOutA(dc, text.x, text.y, text.text.c_str(), static_cast<int>(text.text.size()));
}

void paintButton(HDC dc, const XRButtonCommand& button) {
    RECT rect{button.x, button.y, button.x + button.width, button.y + button.height};

    HBRUSH brush = CreateSolidBrush(button.style.backgroundColor);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, button.style.borderWidth), button.style.borderColor);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);

    if (button.style.borderRadius > 0) {
        RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, button.style.borderRadius, button.style.borderRadius);
    } else {
        Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
    }

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, button.style.textColor);
    DrawTextA(dc, button.text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void paintBox(HDC dc, const XRBoxCommand& box) {
    RECT outer{box.x, box.y, box.x + box.width, box.y + box.height};
    HBRUSH brush = CreateSolidBrush(RGB(240, 240, 240));
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, GetStockObject(BLACK_PEN));
    Rectangle(dc, outer.left, outer.top, outer.right, outer.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);

    HRGN oldClipRegion = CreateRectRgn(0, 0, 0, 0);
    int hadClipRegion = GetClipRgn(dc, oldClipRegion);
    
    HRGN boxClipRegion = CreateRectRgn(outer.left + 1, outer.top + 1, outer.right - 1, outer.bottom - 1);
    SelectClipRgn(dc, boxClipRegion);

    RECT inner{
        outer.left + 8,
        outer.top + 8 - box.scrollOffset,
        std::max(outer.left + 8, outer.right - 8),
        outer.top + 8 - box.scrollOffset + 10000
    };
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    DrawTextA(dc, box.text.c_str(), -1, &inner, DT_LEFT | DT_TOP | DT_WORDBREAK);

    if (hadClipRegion == 1) {
        SelectClipRgn(dc, oldClipRegion);
    } else {
        SelectClipRgn(dc, nullptr);
    }
    DeleteObject(oldClipRegion);
    DeleteObject(boxClipRegion);
}

void paintInput(HDC dc, const XRInputCommand& input) {
    RECT outer{input.x, input.y, input.x + input.width, input.y + input.height};
    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN pen = CreatePen(PS_SOLID, 1, input.focused ? RGB(0, 0, 0) : RGB(128, 128, 128));
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    Rectangle(dc, outer.left, outer.top, outer.right, outer.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    RECT inner{outer.left + 4, outer.top + 4, outer.right - 4, outer.bottom - 4};
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    std::string displayText = input.text;
    if (input.focused) {
        displayText += "|";
    }
    DrawTextA(dc, displayText.c_str(), -1, &inner, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

XRBoxCommand* findBoxById(const std::string& id) {
    for (auto& box : g_boxes) {
        if (box.id == id) {
            return &box;
        }
    }
    return nullptr;
}

bool pointInButton(const XRButtonCommand& button, int x, int y) {
    return x >= button.x &&
           x < button.x + button.width &&
           y >= button.y &&
           y < button.y + button.height;
}

bool pointInBox(const XRBoxCommand& box, int x, int y) {
    int effectiveHeight = std::max(box.height, 60);
    return x >= box.x &&
           x < box.x + box.width &&
           y >= box.y &&
           y < box.y + effectiveHeight;
}

int calculateTextHeight(HDC dc, const std::string& text, int width) {
    RECT rect{0, 0, width, 0};
    DrawTextA(dc, text.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_CALCRECT);
    return rect.bottom - rect.top;
}

LRESULT CALLBACK XrWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_LBUTTONDOWN: {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);
            
            for (auto& input : g_inputs) {
                input.focused = false;
            }
            
            for (auto it = g_inputs.rbegin(); it != g_inputs.rend(); ++it) {
                if (mouseX >= it->x && mouseX < it->x + it->width &&
                    mouseY >= it->y && mouseY < it->y + it->height) {
                    it->focused = true;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            
            for (auto it = g_buttons.rbegin(); it != g_buttons.rend(); ++it) {
                if (pointInButton(*it, mouseX, mouseY)) {
                    if (it->handler) {
                        it->handler();
                    }
                    return 0;
                }
            }
            break;
        }
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hwnd, &pt);
            
            HDC dc = GetDC(hwnd);
            for (auto& box : g_boxes) {
                if (pointInBox(box, pt.x, pt.y)) {
                    int textHeight = calculateTextHeight(dc, box.text, box.width - 16);
                    int visibleHeight = box.height - 16;
                    
                    if (textHeight > visibleHeight) {
                        int scrollAmount = delta / WHEEL_DELTA * 20;
                        box.scrollOffset -= scrollAmount;
                        if (box.scrollOffset < 0) {
                            box.scrollOffset = 0;
                        }
                        int maxScroll = textHeight - visibleHeight;
                        if (box.scrollOffset > maxScroll) {
                            box.scrollOffset = maxScroll;
                        }
                        InvalidateRect(hwnd, nullptr, FALSE);
                        ReleaseDC(hwnd, dc);
                        return 0;
                    }
                }
            }
            ReleaseDC(hwnd, dc);
            break;
        }
        case WM_CHAR: {
            char ch = static_cast<char>(wParam);
            for (auto& input : g_inputs) {
                if (input.focused) {
                    if (ch == '\b' && !input.text.empty()) {
                        input.text.pop_back();
                    } else if (ch >= 32 && ch < 127) {
                        input.text += ch;
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            break;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_BACK) {
                for (auto& input : g_inputs) {
                    if (input.focused && !input.text.empty()) {
                        input.text.pop_back();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_CLOSE:
            g_shouldClose = true;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            g_shouldClose = true;
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);

            RECT clientRect{};
            GetClientRect(hwnd, &clientRect);
            int clientWidth = clientRect.right - clientRect.left;
            int clientHeight = clientRect.bottom - clientRect.top;
            HDC memoryDc = CreateCompatibleDC(dc);
            HBITMAP backBuffer = CreateCompatibleBitmap(dc, clientWidth, clientHeight);
            HGDIOBJ oldBitmap = SelectObject(memoryDc, backBuffer);

            HBRUSH background = CreateSolidBrush(g_clearColor);
            FillRect(memoryDc, &clientRect, background);
            DeleteObject(background);

            for (const auto& rect : g_rects) {
                paintRect(memoryDc, rect);
            }
            for (const auto& box : g_boxes) {
                paintBox(memoryDc, box);
            }
            for (const auto& input : g_inputs) {
                paintInput(memoryDc, input);
            }
            for (const auto& button : g_buttons) {
                paintButton(memoryDc, button);
            }
            for (const auto& text : g_texts) {
                paintText(memoryDc, text);
            }

            BitBlt(dc, 0, 0, clientWidth, clientHeight, memoryDc, 0, 0, SRCCOPY);
            SelectObject(memoryDc, oldBitmap);
            DeleteObject(backBuffer);
            DeleteDC(memoryDc);

            EndPaint(hwnd, &ps);
            return 0;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

bool ensureWindowClass() {
    if (g_classRegistered) {
        return true;
    }

    g_instance = GetModuleHandleA(nullptr);

    WNDCLASSA wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = XrWindowProc;
    wc.hInstance = g_instance;
    wc.hCursor = LoadCursorA(nullptr, MAKEINTRESOURCEA(32512));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = "XfawaXraphicsWindow";

    if (RegisterClassA(&wc) == 0) {
        return false;
    }

    g_classRegistered = true;
    return true;
}

} // namespace

extern "C" int xr_create_window(int width, int height, const char* title) {
    applyDefaultStyles();
    g_shouldClose = false;

    if (!ensureWindowClass()) {
        MessageBoxA(nullptr, "Failed to register Xraphics window class", "Xraphics", MB_ICONERROR);
        return 0;
    }

    g_window = CreateWindowExA(
        0,
        "XfawaXraphicsWindow",
        title ? title : "xfawa",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        g_instance,
        nullptr);

    if (!g_window) {
        MessageBoxA(nullptr, "Failed to create Xraphics window", "Xraphics", MB_ICONERROR);
        return 0;
    }

    return 1;
}

extern "C" int xr_show_window() {
    if (!g_window) {
        return 0;
    }

    ShowWindow(g_window, SW_SHOWNORMAL);
    UpdateWindow(g_window);
    return 1;
}

extern "C" int xr_poll_events() {
    MSG msg{};
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_shouldClose = true;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    Sleep(1);
    return g_shouldClose ? 0 : 1;
}

extern "C" int xr_should_close() {
    return g_shouldClose ? 1 : 0;
}

extern "C" int xr_begin_frame() {
    g_rects.clear();
    g_texts.clear();
    g_buttons.clear();
    return 1;
}

extern "C" int xr_end_frame() {
    if (!g_window) {
        return 0;
    }

    InvalidateRect(g_window, nullptr, FALSE);
    UpdateWindow(g_window);
    return 1;
}

extern "C" int xr_draw_rect(int x, int y, int width, int height, unsigned int color) {
    COLORREF fill = RGB(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
    g_rects.push_back(XRRectCommand{x, y, width, height, fill});
    return 1;
}

extern "C" int xr_draw_text(int x, int y, const char* text, unsigned int color) {
    COLORREF value = RGB(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
    g_texts.push_back(XRTextCommand{x, y, text ? text : "", value});
    return 1;
}

extern "C" int xr_draw_button(int x, int y, int width, int height, const char* text, void (*handler)()) {
    XRButtonCommand command{};
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.text = text ? text : "";
    command.style = g_styles.button;
    command.handler = handler;
    g_buttons.push_back(std::move(command));
    return 1;
}

extern "C" int xr_draw_box(int x, int y, int width, int height, const char* id, const char* text) {
    const std::string boxId = id ? id : "";
    const std::string boxText = text ? text : "";
    if (boxId.empty()) {
        return 0;
    }

    if (XRBoxCommand* existing = findBoxById(boxId)) {
        existing->x = x;
        existing->y = y;
        existing->width = width;
        existing->height = height;
        if (existing->text.empty() && !boxText.empty()) {
            existing->text = boxText;
        }
        return 1;
    }

    XRBoxCommand command{};
    command.id = boxId;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.text = boxText;
    g_boxes.push_back(std::move(command));
    return 1;
}

XRInputCommand* findInputById(const std::string& id) {
    for (auto& input : g_inputs) {
        if (input.id == id) {
            return &input;
        }
    }
    return nullptr;
}

extern "C" const char* xr_draw_input(int x, int y, int width, int height, const char* id, const char* varName) {
    const std::string inputId = id ? id : "";
    const std::string inputVar = varName ? varName : "";
    if (inputId.empty()) {
        return "";
    }

    XRInputCommand* existing = findInputById(inputId);
    if (existing) {
        existing->x = x;
        existing->y = y;
        existing->width = width;
        existing->height = height;
        return existing->text.c_str();
    }

    XRInputCommand command{};
    command.id = inputId;
    command.varName = inputVar;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.text = "";
    g_inputs.push_back(std::move(command));
    return g_inputs.back().text.c_str();
}

extern "C" int xr_append_box(const char* id, const char* text) {
    const std::string boxId = id ? id : "";
    const std::string appendText = text ? text : "";
    if (boxId.empty()) {
        return 0;
    }

    XRBoxCommand* box = findBoxById(boxId);
    if (!box) {
        return 0;
    }

    if (!box->text.empty()) {
        box->text += "\r\n";
    }
    box->text += appendText;

    if (g_window) {
        InvalidateRect(g_window, nullptr, FALSE);
        UpdateWindow(g_window);
    }

    return 1;
}

extern "C" int xr_load_style(const char* path) {
    applyDefaultStyles();

    if (!path || *path == '\0') {
        return 0;
    }

    std::filesystem::path resolved = resolveStylePath(path);
    return parseXssFile(resolved) ? 1 : 0;
}

extern "C" int xr_set_clear_color(unsigned int color) {
    g_clearColor = RGB(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
    return 1;
}
