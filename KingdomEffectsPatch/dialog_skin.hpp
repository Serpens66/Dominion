#pragma once

#include <windows.h>
#include <commctrl.h>
#include <objidl.h>
#include <gdiplus.h>
#include "game_resources.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace kingdom {

class DialogSkin {
public:
    explicit DialogSkin(HWND dialog);
    ~DialogSkin();
    DialogSkin(const DialogSkin&) = delete;
    DialogSkin& operator=(const DialogSkin&) = delete;

    RECT units(RECT logical) const;
    void place(int control, RECT logical) const;
    INT_PTR handle(UINT message, WPARAM wparam, LPARAM lparam) const;
    ResourceImage art(std::string_view name) const;
    bool draw_icon(HDC dc, std::string_view name, RECT bounds) const;
    int add_icon(HIMAGELIST images, std::string_view name) const;
    HIMAGELIST checks() const;
    static constexpr COLORREF paper = RGB(248, 238, 212);
    static constexpr COLORREF panel = RGB(229, 209, 165);
    static constexpr COLORREF ink = RGB(56, 35, 20);

private:
    void paint_background(HDC dc) const;
    void paint_button(const DRAWITEMSTRUCT& item) const;
    static LRESULT CALLBACK checkbox_proc(HWND button, UINT message, WPARAM wparam, LPARAM lparam,
                                         UINT_PTR id, DWORD_PTR data);
    void paint_checkbox(HWND button, HDC dc) const;

    HWND dialog_;
    double scale_ = 1;
    int header_height_ = 0;
    HBRUSH paper_brush_ = nullptr;
    HFONT body_ = nullptr;
    HFONT section_ = nullptr;
    HFONT heading_ = nullptr;
    ULONG_PTR graphics_ = 0;
};

}
