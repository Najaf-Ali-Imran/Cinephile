#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QString>

struct AppTheme
{
    static inline const QString BACKGROUND_DARK = "#1B1717";
    static inline const QString PANEL_LIGHT = "#EEEBDD";
    static inline const QString PRIMARY_RED = "#CE1212";
    static inline const QString DARK_RED = "#810000";
    static inline const QString GRADIENT_MID_DARK = "#222222";
    static inline const QString SKELETON_BG = "#333333";
    static inline const QString CONTENT_AREA_BG = "#1F1F1F";
    static inline const QString SIDEBAR_BG = "#101010";

    static inline const QString TEXT_ON_LIGHT = "#1B1717";
    static inline const QString TEXT_ON_DARK = "#EEEBDD";
    static inline const QString TEXT_ON_DARK_SECONDARY = "#A0A0A0";
    static inline const QString TEXT_PLACEHOLDER = "#999999";
    static inline const QString TEXT_DISABLED = "#666666";

    static inline const QString BUTTON_HOVER_LIGHT_BG = "#D13030";
    static inline const QString BUTTON_HOVER_DARK_BG = "#A12020";
    static inline const QString BUTTON_WHITE_BG = "#FFFFFF";
    static inline const QString BUTTON_DISABLED_BG = "#CCCCCC";
    static inline const QString BUTTON_BORDER_LIGHT = "#CCCCCC";
    static inline const QString BUTTON_HOVER_GENERAL_BG = "#332F2F";
    static inline const QString BUTTON_PRESSED_GENERAL_BG = "#555050";
    static inline const QString BUTTON_CLOSE_HOVER_BG = "#CE1212";
    static inline const QString BUTTON_CLOSE_PRESSED_BG = "#810000";

    static inline const QString OVERLAY_BUTTON_HOVER_BG = "rgba(238, 235, 221, 0.1)";
    static inline const QString OVERLAY_BUTTON_PRESSED_BG = "rgba(238, 235, 221, 0.2)";
    static inline const QString PRIMARY_RED_ALPHA_15 = "rgba(206, 18, 18, 0.15)";
    static inline const QString PRIMARY_RED_ALPHA_30 = "rgba(206, 18, 18, 0.3)";
    static inline const QString INTERACTIVE_HOVER_BG = "#2A2A2A";

    static inline const QString INPUT_BG = "#252525";
    static inline const QString INPUT_BG_HOVER = "#333333";
    static inline const QString INPUT_BORDER = "#404040";
    static inline const QString INPUT_BORDER_FOCUS = "#CE1212";
    static inline const QString BORDER_COLOR = "#404040";

    static inline const QColor SHADOW_COLOR = QColor(0, 0, 0, 65);

    static inline const QString MAIN_WINDOW_BG = "#000000";
    static inline const QString WINDOW_CONTROL_HOVER_BG
        = "#333333";


    static inline const QString BUTTON_SECONDARY_BG
        = "#555555";
    static inline const QString BUTTON_SECONDARY_HOVER_BG = "#666666";
    static inline const QString BUTTON_SECONDARY_PRESSED_BG = "#444444";


    static inline const QString DARK_GREY
        = "#2C2C2C";

    static inline const QString CATEGORY_DOT_BLUE = "#4287f5";
    static inline const QString CATEGORY_DOT_GREEN = "#32a852";
    static inline const QString CATEGORY_DOT_ORANGE = "#f5a142";
    static inline const QString CATEGORY_DOT_PURPLE = "#a842f5";
    static inline const QString CATEGORY_DOT_CYAN = "#42f5ef";
    static inline const QString CATEGORY_DOT_PINK = "#f5429e";
    static inline const QString CATEGORY_DOT_WHITE = "#E0E0E0";
};

#endif
