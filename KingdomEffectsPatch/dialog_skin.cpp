#define WIN32_LEAN_AND_MEAN
#include "dialog_skin.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>

#include "effect_editor_ids.h"

namespace kingdom {
namespace {

constexpr int header_units = 24;
constexpr COLORREF wood = RGB(63, 36, 23);
constexpr COLORREF gold = RGB(164, 107, 38);
constexpr COLORREF muted = RGB(116, 91, 60);
constexpr COLORREF accent = RGB(117, 48, 25);

void fill(HDC dc, const RECT& rect, COLORREF color) {
    const auto brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void frame(HDC dc, const RECT& rect, COLORREF color) {
    const auto brush = CreateSolidBrush(color);
    FrameRect(dc, &rect, brush);
    DeleteObject(brush);
}

void image_at(HDC dc, Gdiplus::Image& image, const RECT& rect) {
    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.DrawImage(&image, static_cast<INT>(rect.left), static_cast<INT>(rect.top),
                       static_cast<INT>(rect.right - rect.left), static_cast<INT>(rect.bottom - rect.top));
}

void tile(HDC dc, Gdiplus::Image& image, const RECT& rect, float image_scale) {
    if (!image.GetWidth() || !image.GetHeight()) return;
    Gdiplus::Graphics graphics(dc);
    Gdiplus::TextureBrush brush(&image, Gdiplus::WrapModeTile);
    brush.ScaleTransform(image_scale, image_scale);
    graphics.FillRectangle(&brush, static_cast<INT>(rect.left), static_cast<INT>(rect.top),
                           static_cast<INT>(rect.right - rect.left), static_cast<INT>(rect.bottom - rect.top));
}

std::array<wchar_t, 256> caption(HWND window) {
    std::array<wchar_t, 256> text{};
    GetWindowTextW(window, text.data(), static_cast<int>(text.size()));
    return text;
}

}

DialogSkin::DialogSkin(HWND dialog) : dialog_(dialog) {
    Gdiplus::GdiplusStartupInput startup;
    if (Gdiplus::GdiplusStartup(&graphics_, &startup, nullptr) != Gdiplus::Ok) graphics_ = 0;
    const auto owner = GetWindow(dialog, GW_OWNER);
    MONITORINFO monitor{sizeof monitor};
    RECT client{};
    GetClientRect(dialog, &client);
    const auto width = client.right;
    const auto height = client.bottom;
    const auto window_style = GetWindowLongPtrW(dialog, GWL_STYLE);
    SetWindowLongPtrW(dialog, GWL_STYLE, (window_style & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU)) | WS_CLIPCHILDREN);
    const auto extended = GetWindowLongPtrW(dialog, GWL_EXSTYLE);
    SetWindowLongPtrW(dialog, GWL_EXSTYLE, extended & ~(WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME));
    if (width > 0 && height > 0 &&
        GetMonitorInfoW(MonitorFromWindow(owner ? owner : dialog, MONITOR_DEFAULTTONEAREST), &monitor)) {
        const auto& work = monitor.rcWork;
        scale_ = (std::min)({1.0, static_cast<double>(work.right - work.left) / width,
                            static_cast<double>(work.bottom - work.top) / height});
        RECT anchor = work;
        if (owner) GetWindowRect(owner, &anchor);
        const auto w = static_cast<LONG>(width * scale_);
        const auto h = static_cast<LONG>(height * scale_);
        const auto x = std::clamp((anchor.left + anchor.right - w) / 2, work.left, work.right - w);
        const auto y = std::clamp((anchor.top + anchor.bottom - h) / 2, work.top, work.bottom - h);
        SetWindowPos(dialog, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    } else {
        SetWindowPos(dialog, nullptr, 0, 0, width, height,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    header_height_ = units({0, 0, 0, header_units}).bottom;
    paper_brush_ = CreateSolidBrush(paper);

    LOGFONTW font{};
    const auto original_font = reinterpret_cast<HFONT>(SendMessageW(dialog, WM_GETFONT, 0, 0));
    if (GetObjectW(original_font, sizeof font, &font)) {
        if (scale_ < 1) {
            font.lfHeight = static_cast<LONG>(std::lround(font.lfHeight * scale_));
            body_ = CreateFontIndirectW(&font);
        }
        font.lfWeight = FW_BOLD;
        section_ = CreateFontIndirectW(&font);
        font.lfHeight = static_cast<LONG>(std::lround(font.lfHeight * 1.45));
        wcscpy_s(font.lfFaceName, L"Georgia");
        heading_ = CreateFontIndirectW(&font);
    }

    for (auto child = GetWindow(dialog, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        wchar_t name[24]{};
        GetClassNameW(child, name, static_cast<int>(std::size(name)));
        if (scale_ < 1) {
            RECT rect{};
            GetWindowRect(child, &rect);
            MapWindowPoints(HWND_DESKTOP, dialog, reinterpret_cast<POINT*>(&rect), 2);
            RECT dropped{};
            if (_wcsicmp(name, L"ComboBox") == 0 &&
                SendMessageW(child, CB_GETDROPPEDCONTROLRECT, 0, reinterpret_cast<LPARAM>(&dropped)) != CB_ERR &&
                dropped.bottom > dropped.top) rect.bottom = rect.top + dropped.bottom - dropped.top;
            SetWindowPos(child, nullptr, static_cast<int>(rect.left * scale_), static_cast<int>(rect.top * scale_),
                         static_cast<int>((rect.right - rect.left) * scale_),
                         static_cast<int>((rect.bottom - rect.top) * scale_), SWP_NOZORDER | SWP_NOACTIVATE);
            if (body_) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(body_), FALSE);
        }
        if (_wcsicmp(name, L"Button") != 0) continue;
        const auto style = GetWindowLongPtrW(child, GWL_STYLE);
        const auto type = style & BS_TYPEMASK;
        if (type == BS_AUTOCHECKBOX) {
            SetWindowSubclass(child, checkbox_proc, 1, reinterpret_cast<DWORD_PTR>(this));
        } else if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) {
            SetWindowLongPtrW(child, GWL_STYLE, (style & ~BS_TYPEMASK) | BS_OWNERDRAW);
        }
    }
    if (section_) {
        for (const auto id : {IDC_EFFECT_LABEL, IDC_ACTIVE_LABEL, IDC_CARD_EXPANSIONS, IDC_CARD_TITLE}) {
            if (const auto label = GetDlgItem(dialog, id))
                SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(section_), FALSE);
        }
    }
    SendMessageW(dialog, DM_SETDEFID, IDOK, 0);
}

DialogSkin::~DialogSkin() {
    if (graphics_) Gdiplus::GdiplusShutdown(graphics_);
    if (paper_brush_) DeleteObject(paper_brush_);
    if (body_) DeleteObject(body_);
    if (section_) DeleteObject(section_);
    if (heading_) DeleteObject(heading_);
}

RECT DialogSkin::units(RECT rect) const {
    MapDialogRect(dialog_, &rect);
    rect.left = static_cast<LONG>(std::lround(rect.left * scale_));
    rect.top = static_cast<LONG>(std::lround(rect.top * scale_));
    rect.right = static_cast<LONG>(std::lround(rect.right * scale_));
    rect.bottom = static_cast<LONG>(std::lround(rect.bottom * scale_));
    return rect;
}

void DialogSkin::place(int control, RECT logical) const {
    const auto rect = units(logical);
    SetWindowPos(GetDlgItem(dialog_, control), nullptr, rect.left, rect.top, rect.right - rect.left,
                 rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

ResourceImage DialogSkin::art(std::string_view name) const {
    return game_art(name);
}

bool DialogSkin::draw_icon(HDC dc, std::string_view name, RECT bounds) const {
    auto icon = art(name);
    if (!icon || bounds.right <= bounds.left || bounds.bottom <= bounds.top) return false;
    const double ratio = (std::min)(static_cast<double>(bounds.right - bounds.left) / icon->GetWidth(),
                                    static_cast<double>(bounds.bottom - bounds.top) / icon->GetHeight());
    const auto width = static_cast<LONG>(std::lround(icon->GetWidth() * ratio));
    const auto height = static_cast<LONG>(std::lround(icon->GetHeight() * ratio));
    bounds.left += (bounds.right - bounds.left - width) / 2;
    bounds.top += (bounds.bottom - bounds.top - height) / 2;
    bounds.right = bounds.left + width;
    bounds.bottom = bounds.top + height;
    if (name.substr(0, 4) == "exp-") fill(dc, bounds, wood);
    image_at(dc, *icon, bounds);
    return true;
}

int DialogSkin::add_icon(HIMAGELIST images, std::string_view name) const {
    int width = 0, height = 0;
    if (!images || !art(name) || !ImageList_GetIconSize(images, &width, &height)) return I_IMAGENONE;
    Gdiplus::Bitmap tile_image(width, height, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics graphics(&tile_image);
        graphics.Clear(name.substr(0, 4) == "exp-" ? Gdiplus::Color(255, 63, 36, 23) :
                       Gdiplus::Color(0, 0, 0, 0));
        auto icon = art(name);
        const double scale = (std::min)(static_cast<double>(width) / icon->GetWidth(),
                                       static_cast<double>(height) / icon->GetHeight());
        const auto w = static_cast<int>(std::lround(icon->GetWidth() * scale));
        const auto h = static_cast<int>(std::lround(icon->GetHeight() * scale));
        graphics.DrawImage(icon.get(), (width - w) / 2, (height - h) / 2, w, h);
    }
    HBITMAP bitmap{};
    if (tile_image.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &bitmap) != Gdiplus::Ok) return I_IMAGENONE;
    const auto result = ImageList_Add(images, bitmap, nullptr);
    DeleteObject(bitmap);
    return result >= 0 ? result : I_IMAGENONE;
}

HIMAGELIST DialogSkin::checks() const {
    if (!art("checkbox") || !art("check")) return nullptr;
    const auto size = units({0, 0, 10, 9}).right;
    const auto images = ImageList_Create(size, size, ILC_COLOR32, 3, 0);
    if (!images) return nullptr;
    // The tree draws no image for state 0 and uses image indices 1 and 2 for checkboxes.
    if (add_icon(images, "checkbox") == 0 && add_icon(images, "checkbox") == 1 &&
        add_icon(images, "check") == 2) return images;
    ImageList_Destroy(images);
    return nullptr;
}

void DialogSkin::paint_background(HDC dc) const {
    RECT full{};
    GetClientRect(dialog_, &full);
    fill(dc, full, panel);
    RECT body = full;
    body.top = header_height_;
    if (auto texture = art("panel")) {
        tile(dc, *texture, body, static_cast<float>(header_height_) / 42.0f);
        Gdiplus::Graphics graphics(dc);
        Gdiplus::SolidBrush tint(Gdiplus::Color(214, 234, 216, 178));
        graphics.FillRectangle(&tint, static_cast<int>(body.left), static_cast<int>(body.top),
                               static_cast<int>(body.right - body.left), static_cast<int>(body.bottom - body.top));
    }
    const auto margin = units({0, 0, 5, 4});
    RECT content{margin.right, header_height_ + margin.bottom, full.right - margin.right, full.bottom - margin.bottom};
    fill(dc, content, panel);
    RECT header = full;
    header.bottom = header_height_;
    fill(dc, header, wood);
    if (auto texture = art("wood")) tile(dc, *texture, header, static_cast<float>(header_height_) / texture->GetHeight());
    frame(dc, full, gold);
    RECT line{0, header.bottom - 1, full.right, header.bottom + 1};
    fill(dc, line, gold);
    const auto inset = units({0, 0, 8, 0}).right;
    const auto side = (std::max)(1L, header_height_ - 2 * inset / 3);
    RECT emblem{inset, (header_height_ - side) / 2, inset + side, (header_height_ + side) / 2};
    draw_icon(dc, "seal", emblem);
    header.left = emblem.right + inset / 2;
    header.right -= inset;
    const auto text = caption(dialog_);
    SetBkMode(dc, TRANSPARENT);
    const auto previous = heading_ ? SelectObject(dc, heading_) : nullptr;
    SetTextColor(dc, RGB(251, 229, 182));
    DrawTextW(dc, text.data(), -1, &header, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    if (previous) SelectObject(dc, previous);
}

void DialogSkin::paint_button(const DRAWITEMSTRUCT& item) const {
    RECT rect = item.rcItem;
    const bool enabled = !(item.itemState & ODS_DISABLED);
    const bool pressed = item.itemState & ODS_SELECTED;
    const bool primary = item.CtlID == IDOK || item.CtlID == IDC_ADD || item.CtlID == IDC_TARGET ||
                         item.CtlID == IDC_SECOND_TARGET;
    auto texture = primary && enabled ? art("gold") : nullptr;
    fill(item.hDC, rect, enabled ? (primary ? RGB(215, 168, 81) : paper) : RGB(220, 201, 162));
    if (texture) image_at(item.hDC, *texture, rect);
    if (pressed) {
        Gdiplus::Graphics graphics(item.hDC);
        Gdiplus::SolidBrush shade(Gdiplus::Color(48, 63, 36, 23));
        graphics.FillRectangle(&shade, static_cast<int>(rect.left), static_cast<int>(rect.top),
                               static_cast<int>(rect.right - rect.left), static_cast<int>(rect.bottom - rect.top));
    }
    frame(item.hDC, rect, enabled ? gold : muted);
    const auto inset = units({0, 0, 2, 0}).right;
    InflateRect(&rect, -inset, -2);
    if (enabled && (item.CtlID == IDOK || item.CtlID == IDC_ADD)) {
        RECT icon = rect;
        icon.right = icon.left + (rect.bottom - rect.top);
        if (draw_icon(item.hDC, item.CtlID == IDOK ? "check" : "plus", icon)) rect.left = icon.right + 2;
    }
    const auto text = caption(item.hwndItem);
    const auto font = reinterpret_cast<HFONT>(SendMessageW(item.hwndItem, WM_GETFONT, 0, 0));
    const auto previous = font ? SelectObject(item.hDC, font) : nullptr;
    SetBkMode(item.hDC, TRANSPARENT);
    if (texture) {
        auto shadow = rect;
        OffsetRect(&shadow, 0, 1);
        SetTextColor(item.hDC, RGB(41, 25, 15));
        DrawTextW(item.hDC, text.data(), -1, &shadow, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    SetTextColor(item.hDC, texture ? RGB(255, 240, 201) : enabled ? ink : muted);
    DrawTextW(item.hDC, text.data(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (previous) SelectObject(item.hDC, previous);
    if (item.itemState & ODS_FOCUS) {
        RECT focus = item.rcItem;
        InflateRect(&focus, -3, -3);
        DrawFocusRect(item.hDC, &focus);
    }
}

void DialogSkin::paint_checkbox(HWND button, HDC dc) const {
    RECT rect{};
    GetClientRect(button, &rect);
    fill(dc, rect, panel);
    const auto size = (std::min)(rect.bottom, units({0, 0, 10, 0}).right);
    RECT box{0, (rect.bottom - size) / 2, size, (rect.bottom + size) / 2};
    const bool checked = SendMessageW(button, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (!draw_icon(dc, checked ? "check" : "checkbox", box))
        DrawFrameControl(dc, &box, DFC_BUTTON, DFCS_BUTTONCHECK | (checked ? DFCS_CHECKED : 0));
    rect.left = box.right + 4;
    const auto label = caption(button);
    const auto font = reinterpret_cast<HFONT>(SendMessageW(button, WM_GETFONT, 0, 0));
    const auto previous = font ? SelectObject(dc, font) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, ink);
    DrawTextW(dc, label.data(), -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    if (previous) SelectObject(dc, previous);
    if (GetFocus() == button) DrawFocusRect(dc, &rect);
}

LRESULT CALLBACK DialogSkin::checkbox_proc(HWND button, UINT message, WPARAM wparam, LPARAM lparam,
                                          UINT_PTR id, DWORD_PTR data) {
    const auto& skin = *reinterpret_cast<DialogSkin*>(data);
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        skin.paint_checkbox(button, BeginPaint(button, &paint));
        EndPaint(button, &paint);
        return 0;
    }
    if (message == WM_PRINTCLIENT) {
        skin.paint_checkbox(button, reinterpret_cast<HDC>(wparam));
        return 0;
    }
    if (message == WM_NCDESTROY) RemoveWindowSubclass(button, checkbox_proc, id);
    const auto result = DefSubclassProc(button, message, wparam, lparam);
    if (message == BM_SETCHECK || message == WM_SETFOCUS || message == WM_KILLFOCUS)
        InvalidateRect(button, nullptr, FALSE);
    return result;
}

INT_PTR DialogSkin::handle(UINT message, WPARAM wparam, LPARAM lparam) const {
    if (message == WM_ERASEBKGND) {
        paint_background(reinterpret_cast<HDC>(wparam));
        return TRUE;
    }
    if (message == WM_DRAWITEM) {
        const auto& item = *reinterpret_cast<const DRAWITEMSTRUCT*>(lparam);
        if (item.CtlType == ODT_BUTTON) {
            paint_button(item);
            SetWindowLongPtrW(dialog_, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
    }
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX) {
        const auto dc = reinterpret_cast<HDC>(wparam);
        const auto control = GetDlgCtrlID(reinterpret_cast<HWND>(lparam));
        SetTextColor(dc, control == IDC_STATUS || control == IDC_CARD_STATUS ? accent : ink);
        SetBkColor(dc, paper);
        if (message == WM_CTLCOLORSTATIC && control != IDC_EFFECT_EVIDENCE &&
            control != IDC_CARD_RULES) {
            SetBkColor(dc, panel);
            SetDCBrushColor(dc, panel);
            return reinterpret_cast<INT_PTR>(GetStockObject(DC_BRUSH));
        }
        return reinterpret_cast<INT_PTR>(paper_brush_);
    }
    if (message == WM_LBUTTONDOWN && static_cast<short>(HIWORD(lparam)) < header_height_) {
        POINT point{static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam))};
        ClientToScreen(dialog_, &point);
        ReleaseCapture();
        SendMessageW(dialog_, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
        return TRUE;
    }
    return FALSE;
}

}
