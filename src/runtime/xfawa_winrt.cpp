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

// Animation data structure
struct XRAnimation {
    int initialValue = 0;     // Initial value (pixels)
    int targetValue = 0;      // Target value (pixels)
    int currentValue = 0;     // Current value
    bool active = false;      // Is active
    float progress = 0.0f;    // Animation progress (0.0 - 1.0)
    int duration = 1000;      // Animation duration (milliseconds)
    DWORD startTime = 0;      // Start time
};

// Animation trigger structure
struct XRAnimationTrigger {
    std::string type;  // "window-click", "key-click", "button-click"
    
    // window-click parameters
    int windowClickX = 0;
    int windowClickY = 0;
    int windowClickWidth = 0;
    int windowClickHeight = 0;
    bool windowClickFullArea = true;  // Trigger on any window click
    
    // key-click parameters
    std::string keyName;
    
    // button-click parameters
    std::string buttonNamespace;
};

struct XRButtonStyle {
    COLORREF backgroundColor = RGB(128, 128, 128);
    COLORREF textColor = RGB(0, 0, 0);
    COLORREF borderColor = RGB(0, 0, 0);
    int borderRadius = 0;
    int borderWidth = 0;
    XRAnimation xAnimation;      // X-axis animation
    XRAnimation yAnimation;      // Y-axis animation
    XRAnimation widthAnimation;  // Width animation
    XRAnimation heightAnimation; // Height animation
    std::vector<XRAnimationTrigger> triggers;  // Animation triggers
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
    XRAnimation xAnimation;
    XRAnimation yAnimation;
};

struct XRButtonCommand {
    int x;
    int y;
    int width;
    int height;
    int initialX;  // Initial position (for identification)
    int initialY;  // Initial position (for identification)
    std::string text;
    std::string namespace_;  // Button namespace for trigger identification
    XRButtonStyle style;
    void (*handler)() = nullptr;
    XRAnimation xAnimation;
    XRAnimation yAnimation;
    XRAnimation widthAnimation;
    XRAnimation heightAnimation;
    std::vector<XRAnimationTrigger> triggers;  // Animation triggers
};

struct XRBoxCommand {
    std::string id;
    int x;
    int y;
    int width;
    int height;
    std::string text;
    int scrollOffset = 0;
    XRAnimation xAnimation;
    XRAnimation yAnimation;
    XRAnimation widthAnimation;
    XRAnimation heightAnimation;
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
    XRAnimation xAnimation;
    XRAnimation yAnimation;
    XRAnimation widthAnimation;
    XRAnimation heightAnimation;
    std::string animationTrigger;  // "window-click" or "button-click"
    bool animationOnWindowClick = false;
    bool animationOnButtonClick = false;
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

// Parse animation syntax: animation(initialValue, targetValue)
XRAnimation parseAnimation(const std::string& value) {
    XRAnimation anim;
    
    // Find animation keyword
    if (value.find("animation(") == std::string::npos) {
        return anim;
    }
    
    // Extract parameters inside parentheses
    size_t start = value.find('(');
    size_t end = value.find(')');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return anim;
    }
    
    std::string params = value.substr(start + 1, end - start - 1);
    
    // Split parameters (separated by comma)
    size_t comma = params.find(',');
    if (comma == std::string::npos) {
        return anim;
    }
    
    std::string initVal = trim(params.substr(0, comma));
    std::string targetVal = trim(params.substr(comma + 1));
    
    // Parse values
    anim.initialValue = std::stoi(initVal);
    anim.targetValue = std::stoi(targetVal);
    anim.currentValue = anim.initialValue;
    anim.active = false;  // Don't auto-activate - wait for trigger
    anim.progress = 0.0f;
    
    return anim;
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
    bool inTriggerBlock = false;
    std::string line;
    
    // Debug: log file parsing
    static FILE* parseLog = nullptr;
    if (!parseLog) {
        parseLog = fopen("xss_parse_debug.log", "w");
        if (parseLog) {
            fprintf(parseLog, "=== XSS Parse Debug ===\n");
            fprintf(parseLog, "Parsing file: %s\n", path.string().c_str());
            fflush(parseLog);
        }
    }
    
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
                if (parseLog) {
                    fprintf(parseLog, "\nFound button block\n");
                    fflush(parseLog);
                }
            }
            continue;
        }

        // Handle trigger block
        if (line == "animation-trigger{" || line == "animation-trigger {") {
            inTriggerBlock = true;
            if (parseLog) {
                fprintf(parseLog, "  Found animation-trigger block\n");
                fflush(parseLog);
            }
            continue;
        }

        if (inTriggerBlock && line == "}") {
            inTriggerBlock = false;
            continue;
        }

        if (inButtonBlock && !inTriggerBlock && line == "}") {
            inButtonBlock = false;
            continue;
        }

        // Parse trigger conditions inside trigger block
        if (inTriggerBlock) {
            // Parse window-click(x, y, width, height) or window-click()
            if (line.find("window-click") == 0) {
                XRAnimationTrigger trigger;
                trigger.type = "window-click";
                
                size_t start = line.find('(');
                size_t end = line.find(')');
                if (start != std::string::npos && end != std::string::npos && end > start) {
                    std::string params = line.substr(start + 1, end - start - 1);
                    params = trim(params);
                    
                    if (!params.empty()) {
                        // Parse x, y, width, height
                        std::vector<int> values;
                        std::istringstream iss(params);
                        std::string token;
                        while (std::getline(iss, token, ',')) {
                            values.push_back(std::stoi(trim(token)));
                        }
                        if (values.size() >= 4) {
                            trigger.windowClickX = values[0];
                            trigger.windowClickY = values[1];
                            trigger.windowClickWidth = values[2];
                            trigger.windowClickHeight = values[3];
                            trigger.windowClickFullArea = false;
                        }
                    }
                }
                
                g_styles.button.triggers.push_back(trigger);
                if (parseLog) {
                    fprintf(parseLog, "    Added trigger: window-click\n");
                    fflush(parseLog);
                }
            }
            // Parse key-click: "keyName"
            else if (line.find("key-click:") == 0) {
                XRAnimationTrigger trigger;
                trigger.type = "key-click";
                
                size_t colon = line.find(':');
                std::string value = trim(line.substr(colon + 1));
                // Remove quotes
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                trigger.keyName = value;
                
                g_styles.button.triggers.push_back(trigger);
                if (parseLog) {
                    fprintf(parseLog, "    Added trigger: key-click \"%s\"\n", trigger.keyName.c_str());
                    fflush(parseLog);
                }
            }
            // Parse button-click: "namespace"
            else if (line.find("button-click:") == 0) {
                XRAnimationTrigger trigger;
                trigger.type = "button-click";
                
                size_t colon = line.find(':');
                std::string value = trim(line.substr(colon + 1));
                // Remove quotes
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                trigger.buttonNamespace = value;
                
                g_styles.button.triggers.push_back(trigger);
                if (parseLog) {
                    fprintf(parseLog, "    Added trigger: button-click \"%s\"\n", trigger.buttonNamespace.c_str());
                    fflush(parseLog);
                }
            }
            // Parse animation properties inside trigger block (not auto-activated)
            else {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = trim(line.substr(0, colon));
                    std::string value = trim(line.substr(colon + 1));
                    if (!value.empty() && value.back() == ';') {
                        value.pop_back();
                        value = trim(value);
                    }
                    
                    if (key == "x-animation") {
                        g_styles.button.xAnimation = parseAnimation(value);
                        // Don't auto-activate - wait for trigger
                        if (parseLog) {
                            fprintf(parseLog, "    x-animation: %d -> %d (triggered)\n", 
                                    g_styles.button.xAnimation.initialValue, 
                                    g_styles.button.xAnimation.targetValue);
                            fflush(parseLog);
                        }
                    } else if (key == "y-animation") {
                        g_styles.button.yAnimation = parseAnimation(value);
                        if (parseLog) {
                            fprintf(parseLog, "    y-animation: %d -> %d (triggered)\n", 
                                    g_styles.button.yAnimation.initialValue, 
                                    g_styles.button.yAnimation.targetValue);
                            fflush(parseLog);
                        }
                    } else if (key == "width-animation") {
                        g_styles.button.widthAnimation = parseAnimation(value);
                        if (parseLog) {
                            fprintf(parseLog, "    width-animation: %d -> %d (triggered)\n", 
                                    g_styles.button.widthAnimation.initialValue, 
                                    g_styles.button.widthAnimation.targetValue);
                            fflush(parseLog);
                        }
                    } else if (key == "height-animation") {
                        g_styles.button.heightAnimation = parseAnimation(value);
                        if (parseLog) {
                            fprintf(parseLog, "    height-animation: %d -> %d (triggered)\n", 
                                    g_styles.button.heightAnimation.initialValue, 
                                    g_styles.button.heightAnimation.targetValue);
                            fflush(parseLog);
                        }
                    }
                }
            }
            continue;
        }

        // Parse normal properties outside trigger block
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
        } else if (key == "x-animation") {
            g_styles.button.xAnimation = parseAnimation(value);
            // Animation outside trigger block = auto-trigger on load
            g_styles.button.xAnimation.active = true;
            if (parseLog) {
                fprintf(parseLog, "  x-animation: %d -> %d (auto-trigger)\n", 
                        g_styles.button.xAnimation.initialValue, 
                        g_styles.button.xAnimation.targetValue);
                fflush(parseLog);
            }
        } else if (key == "y-animation") {
            g_styles.button.yAnimation = parseAnimation(value);
            g_styles.button.yAnimation.active = true;
            if (parseLog) {
                fprintf(parseLog, "  y-animation: %d -> %d (auto-trigger)\n", 
                        g_styles.button.yAnimation.initialValue, 
                        g_styles.button.yAnimation.targetValue);
                fflush(parseLog);
            }
        } else if (key == "width-animation") {
            g_styles.button.widthAnimation = parseAnimation(value);
            g_styles.button.widthAnimation.active = true;
            if (parseLog) {
                fprintf(parseLog, "  width-animation: %d -> %d (auto-trigger)\n", 
                        g_styles.button.widthAnimation.initialValue, 
                        g_styles.button.widthAnimation.targetValue);
                fflush(parseLog);
            }
        } else if (key == "height-animation") {
            g_styles.button.heightAnimation = parseAnimation(value);
            g_styles.button.heightAnimation.active = true;
            if (parseLog) {
                fprintf(parseLog, "  height-animation: %d -> %d (auto-trigger)\n", 
                        g_styles.button.heightAnimation.initialValue, 
                        g_styles.button.heightAnimation.targetValue);
                fflush(parseLog);
            }
        }
    }

    return true;
}

// Animation execution engine - update animation state
void updateAnimation(XRAnimation& anim) {
    if (!anim.active) {
        return;
    }
    
    // If first time starting, record start time
    if (anim.startTime == 0) {
        anim.startTime = GetTickCount();
        // Debug output when animation starts
        char debugMsg[256];
        snprintf(debugMsg, sizeof(debugMsg), 
                 "[Animation] Started: %d -> %d (duration: %dms)\n", 
                 anim.initialValue, anim.targetValue, anim.duration);
        OutputDebugStringA(debugMsg);
    }
    
    // Calculate elapsed time
    DWORD currentTime = GetTickCount();
    DWORD elapsed = currentTime - anim.startTime;
    
    // Calculate progress (0.0 - 1.0)
    anim.progress = static_cast<float>(elapsed) / static_cast<float>(anim.duration);
    
    // Limit progress to 0.0-1.0 range
    if (anim.progress >= 1.0f) {
        anim.progress = 1.0f;
        anim.currentValue = anim.targetValue;
        anim.active = false; // Animation complete
        // Debug output when animation completes
        char debugMsg[256];
        snprintf(debugMsg, sizeof(debugMsg), 
                 "[Animation] Completed: final value = %d\n", 
                 anim.currentValue);
        OutputDebugStringA(debugMsg);
    } else {
        // Linear interpolation to calculate current value
        anim.currentValue = anim.initialValue + 
            static_cast<int>((anim.targetValue - anim.initialValue) * anim.progress);
    }
}

// Update all button animations
void updateButtonAnimations() {
    static int frameCount = 0;
    static FILE* logFile = nullptr;
    
    // Open log file on first call
    if (!logFile) {
        logFile = fopen("animation_debug.log", "w");
        if (logFile) {
            fprintf(logFile, "=== Animation Debug Log ===\n");
            fflush(logFile);
        }
    }
    
    frameCount++;
    
    // Log button count every 10 frames
    if (frameCount % 10 == 0 && logFile) {
        fprintf(logFile, "[Frame %d] Button count: %zu\n", frameCount, g_buttons.size());
        fflush(logFile);
    }
    
    for (auto& button : g_buttons) {
        // Update all animations
        updateAnimation(button.xAnimation);
        updateAnimation(button.yAnimation);
        updateAnimation(button.widthAnimation);
        updateAnimation(button.heightAnimation);
        
        // Log animation progress every 10 frames
        if (frameCount % 10 == 0 && logFile) {
            fprintf(logFile, "[Frame %d] x-anim: %d->%d prog=%.2f%% active=%d, y-anim: %d->%d prog=%.2f%% active=%d\n",
                   frameCount, 
                   button.xAnimation.initialValue, button.xAnimation.targetValue, 
                   button.xAnimation.progress * 100.0f, button.xAnimation.active,
                   button.yAnimation.initialValue, button.yAnimation.targetValue,
                   button.yAnimation.progress * 100.0f, button.yAnimation.active);
            fprintf(logFile, "  position: x=%d, y=%d, w=%d, h=%d\n",
                   button.x, button.y, button.width, button.height);
            fflush(logFile);
        }
    }
}

// Apply animation to button position and size
void applyButtonAnimation(XRButtonCommand& button) {
    if (button.xAnimation.active || button.xAnimation.progress > 0.0f) {
        button.x = button.xAnimation.currentValue;
    }
    if (button.yAnimation.active || button.yAnimation.progress > 0.0f) {
        button.y = button.yAnimation.currentValue;
    }
    if (button.widthAnimation.active || button.widthAnimation.progress > 0.0f) {
        button.width = button.widthAnimation.currentValue;
    }
    if (button.heightAnimation.active || button.heightAnimation.progress > 0.0f) {
        button.height = button.heightAnimation.currentValue;
    }
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
    
    // Convert to wide string for Unicode support
    int len = MultiByteToWideChar(CP_UTF8, 0, text.text.c_str(), -1, nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> wstr(len);
        MultiByteToWideChar(CP_UTF8, 0, text.text.c_str(), -1, wstr.data(), len);
        TextOutW(dc, text.x, text.y, wstr.data(), len - 1);
    }
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
    
    // Convert to wide string for Unicode support
    int len = MultiByteToWideChar(CP_UTF8, 0, button.text.c_str(), -1, nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> wstr(len);
        MultiByteToWideChar(CP_UTF8, 0, button.text.c_str(), -1, wstr.data(), len);
        DrawTextW(dc, wstr.data(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
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
    
    // Convert to wide string for Unicode support
    int len = MultiByteToWideChar(CP_UTF8, 0, box.text.c_str(), -1, nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> wstr(len);
        MultiByteToWideChar(CP_UTF8, 0, box.text.c_str(), -1, wstr.data(), len);
        DrawTextW(dc, wstr.data(), -1, &inner, DT_LEFT | DT_TOP | DT_WORDBREAK);
    }

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
    
    // Convert to wide string for Unicode support
    int len = MultiByteToWideChar(CP_UTF8, 0, displayText.c_str(), -1, nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> wstr(len);
        MultiByteToWideChar(CP_UTF8, 0, displayText.c_str(), -1, wstr.data(), len);
        DrawTextW(dc, wstr.data(), -1, &inner, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
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
            
            // Check button clicks
            bool buttonClicked = false;
            for (auto it = g_buttons.rbegin(); it != g_buttons.rend(); ++it) {
                if (pointInButton(*it, mouseX, mouseY)) {
                    // Check button-click triggers
                    for (auto& trigger : it->triggers) {
                        if (trigger.type == "button-click") {
                            // Activate animations
                            it->xAnimation.active = true;
                            it->xAnimation.startTime = 0;
                            it->yAnimation.active = true;
                            it->yAnimation.startTime = 0;
                            it->widthAnimation.active = true;
                            it->widthAnimation.startTime = 0;
                            it->heightAnimation.active = true;
                            it->heightAnimation.startTime = 0;
                            break;
                        }
                    }
                    
                    if (it->handler) {
                        it->handler();
                    }
                    buttonClicked = true;
                    return 0;
                }
            }
            
            // If no button clicked, check window-click triggers
            if (!buttonClicked) {
                for (auto& btn : g_buttons) {
                    for (auto& trigger : btn.triggers) {
                        if (trigger.type == "window-click") {
                            // Check if click is in trigger area
                            bool inArea = false;
                            if (trigger.windowClickFullArea) {
                                inArea = true;  // Any window click
                            } else {
                                // Check specific area
                                inArea = (mouseX >= trigger.windowClickX && 
                                         mouseX < trigger.windowClickX + trigger.windowClickWidth &&
                                         mouseY >= trigger.windowClickY && 
                                         mouseY < trigger.windowClickY + trigger.windowClickHeight);
                            }
                            
                            if (inArea) {
                                // Activate animations
                                btn.xAnimation.active = true;
                                btn.xAnimation.startTime = 0;
                                btn.yAnimation.active = true;
                                btn.yAnimation.startTime = 0;
                                btn.widthAnimation.active = true;
                                btn.widthAnimation.startTime = 0;
                                btn.heightAnimation.active = true;
                                btn.heightAnimation.startTime = 0;
                                break;
                            }
                        }
                    }
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
            // Check key-click triggers
            int vkCode = static_cast<int>(wParam);
            char keyChar = 0;
            
            // Convert VK code to character (A-Z)
            if (vkCode >= 0x41 && vkCode <= 0x5A) {
                keyChar = static_cast<char>('A' + (vkCode - 0x41));
            }
            
            if (keyChar != 0) {
                std::string keyStr(1, keyChar);
                for (auto& btn : g_buttons) {
                    for (auto& trigger : btn.triggers) {
                        if (trigger.type == "key-click" && trigger.keyName == keyStr) {
                            // Activate animations
                            btn.xAnimation.active = true;
                            btn.xAnimation.startTime = 0;
                            btn.yAnimation.active = true;
                            btn.yAnimation.startTime = 0;
                            btn.widthAnimation.active = true;
                            btn.widthAnimation.startTime = 0;
                            btn.heightAnimation.active = true;
                            btn.heightAnimation.startTime = 0;
                            break;
                        }
                    }
                }
            }
            
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

            // Update all animations (execute from top to bottom)
            updateButtonAnimations();

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
            for (auto& button : g_buttons) {
                // Apply animation to button
                applyButtonAnimation(button);
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
    // Don't clear g_buttons - keep animation state
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
    // Check if button already exists (by initial position)
    XRButtonCommand* existingButton = nullptr;
    for (auto& btn : g_buttons) {
        if (btn.initialX == x && btn.initialY == y) {
            existingButton = &btn;
            break;
        }
    }
    
    if (existingButton) {
        // Update existing button (keep animation state)
        existingButton->width = width;
        existingButton->height = height;
        existingButton->text = text ? text : "";
        existingButton->handler = handler;
        return 1;
    }
    
    // Create new button
    XRButtonCommand command{};
    command.x = x;
    command.y = y;
    command.initialX = x;  // Save initial position
    command.initialY = y;  // Save initial position
    command.width = width;
    command.height = height;
    command.text = text ? text : "";
    command.style = g_styles.button;
    command.handler = handler;
    
    // Copy animation data from style
    command.xAnimation = g_styles.button.xAnimation;
    command.yAnimation = g_styles.button.yAnimation;
    command.widthAnimation = g_styles.button.widthAnimation;
    command.heightAnimation = g_styles.button.heightAnimation;
    command.triggers = g_styles.button.triggers;  // Copy triggers
    
    // Debug: log button creation
    static FILE* buttonLog = nullptr;
    static int createCount = 0;
    if (!buttonLog) {
        buttonLog = fopen("button_create_debug.log", "w");
        if (buttonLog) {
            fprintf(buttonLog, "=== Button Creation Debug ===\n");
            fflush(buttonLog);
        }
    }
    
    createCount++;
    if (buttonLog && createCount <= 5) {
        fprintf(buttonLog, "\n[Button Created #%d] at (%d, %d) size %dx%d\n", createCount, x, y, width, height);
        fprintf(buttonLog, "  xAnimation: %d -> %d (active=%d)\n", 
                command.xAnimation.initialValue, command.xAnimation.targetValue, command.xAnimation.active);
        fprintf(buttonLog, "  yAnimation: %d -> %d (active=%d)\n", 
                command.yAnimation.initialValue, command.yAnimation.targetValue, command.yAnimation.active);
        fprintf(buttonLog, "  widthAnimation: %d -> %d (active=%d)\n", 
                command.widthAnimation.initialValue, command.widthAnimation.targetValue, command.widthAnimation.active);
        fprintf(buttonLog, "  heightAnimation: %d -> %d (active=%d)\n", 
                command.heightAnimation.initialValue, command.heightAnimation.targetValue, command.heightAnimation.active);
        fprintf(buttonLog, "  triggers: %zu\n", command.triggers.size());
        fflush(buttonLog);
    }
    
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
