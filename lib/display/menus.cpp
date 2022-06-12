#include "display.h"

#include <Arduino.h>

#include "bg.c"

#include "global.h"
#include "point.h"

namespace Display
{
namespace Internal
{
Point gaugeCenter(320 - 52, 240 - 82, Point::CARTESIAN);
int16_t gaugeRadius = 140;
uint16_t GAUGE_BG = DISPLAY_BG;

extern ILI9341_t3n tft;

uint8_t numSize(int n)
{
    uint8_t s = (n < 0);
    n = abs(n);
    if (n < 100) // small binary-search-like optimisation
    {
        if (n < 10) // n between 0 & 9
            s += 1;
        else // n between 10 & 99
            s += 2;
    }
    else if (n < 1000) // n between 100 & 999
        s += 3;
    else if (n < 10000) // n between 1000 & 9999
        s += 4;
    else // n between 10000 & 65535, maximum value for 16-bit integer
        s += 5;
    return s;
}

void initGauge()
{
    //    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 12, DISPLAY_ACCENT2);
    //    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 11, GAUGE_BG);
    //    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, GAUGE_BG);
    tft.writeRect(0, 0, gimp_image.width, gimp_image.height, (uint16_t*)gimp_image.pixel_data);
    //    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, DISPLAY_ACCENT1);

    tft.setTextSize(2);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setTextColor(0x7fe2);

    int16_t a = -17;
    for (int i = 0; i < 9; i++)
    {
        Point c = gaugeCenter - Point(a, gaugeRadius, Point::POLAR);
        tft.setCursor(c.x() - 4, c.y() - 7);
        tft.print(8 - i);
        a += 17;
    }
}

void updateGauge()
{
    static Point needle[2];
    static bool wasAlert = false;

    if (GV.alert != wasAlert)
    {
        GAUGE_BG = GV.alert ? DISPLAY_ALERT : DISPLAY_BG;
        tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 14, GAUGE_BG);
        wasAlert = GV.alert;
    }

    Point c1(-(GV.ms.rpm * 17 / 1000) + 119, gaugeRadius - 16, Point::POLAR);
    Point c2(-(GV.ms.rpm * 17 / 1000) + 119, -30, Point::POLAR);

    tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), GAUGE_BG);
    tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), GAUGE_BG);
    tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, GAUGE_BG);

    needle[0] = gaugeCenter - c2;
    needle[1] = gaugeCenter - c1;

    tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), DISPLAY_FG1);
    tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), DISPLAY_FG1);
    tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, DISPLAY_FG1);

    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 9, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 8, DISPLAY_ACCENT1);

    tft.setTextSize(4);
    tft.setTextColor(DISPLAY_FG1, GAUGE_BG);
    tft.setCursor(220, 205);

    for (int i = 0; i < 4 - numSize(GV.ms.rpm); i++)
    {
        tft.print(' ');
    }
    tft.print(GV.ms.rpm);
}

void updateAccelGauge(uint16_t center_x, uint16_t center_y, uint16_t radius)
{
    static int16_t lastx, lasty;
    static bool init = false;

    if (init)
    {
        // remove last point
        tft.fillCircle(lastx, lasty, 4, DISPLAY_BG);
    }

    // re-draw gauge background
    tft.drawCircle(center_x, center_y, radius, DISPLAY_FG1);
    tft.drawCircle(center_x, center_y, radius / 2, DISPLAY_FG1);
    tft.drawFastHLine(center_x - radius, center_y, radius * 2, DISPLAY_FG1);
    tft.drawFastVLine(center_x, center_y - radius, radius * 2, DISPLAY_FG1);

    // update accel marker position
    lastx = (-GV.accel.x * radius) + center_x;
    lasty = (GV.accel.z * radius) + center_y;

    // draw accel marker
    tft.fillCircle(lastx, lasty, 4, DISPLAY_ACCENT1);
    // tft.drawLine(center_x, center_y, lastx, lasty, DISPLAY_FG2);
    init = true;
}

void updateAFRgraph(int16_t x, int16_t y, int16_t w, int16_t h)
{
    static int16_t cursor;
    static uint32_t last_update = 0;
    static uint8_t afrmin, afrmax;
    static uint16_t cormin, cormax;
    static uint16_t tgtmin, tgtmax;

    tft.drawRect(x, y, w, h, ILI9341_WHITE);

    const bool doUpdate = millis() - last_update >= 100 && GV.connected;
    if (doUpdate)
    {
        const int16_t middle = y + h / 2 - 1;
        const int16_t cursor_x = cursor + x + 1;

        tft.drawLine(cursor_x, y + 1, cursor_x, y - 2 + h, ILI9341_BLACK);

        int16_t h1 = middle + 147 - max(min(tgtmax, 167), 127);
        int16_t h2 = middle + 147 - max(min(tgtmin, 167), 127);
        tft.drawLine(cursor_x, h1, cursor_x, h2, ILI9341_YELLOW);

        h1 = middle + max(min(1000 - cormin, 100), -100) / 5;
        h2 = middle + max(min(1000 - cormax, 100), -100) / 5;
        tft.drawLine(cursor_x, h1, cursor_x, h2, ILI9341_CYAN);

        h1 = middle + 147 - max(min(afrmax, 167), 127);
        h2 = middle + 147 - max(min(afrmin, 167), 127);
        tft.drawLine(cursor_x, h1, cursor_x, h2, ILI9341_GREEN);

        uint16_t tmp = afrmax;
        afrmax = afrmin;
        afrmin = tmp;

        tmp = cormax;
        cormax = cormin;
        cormin = tmp;

        tmp = tgtmax;
        tgtmax = tgtmin;
        tgtmin = tmp;

        last_update = millis();
        cursor = (cursor < w - 2) ? cursor + 1 : 0;
        tft.drawLine(x + 1 + cursor, y + 1, x + 1 + cursor, y + h - 2, ILI9341_WHITE);
    }
    afrmin = min(afrmin, GV.ms.afr);
    afrmax = max(afrmax, GV.ms.afr);
    cormin = min(cormin, GV.ms.egocor);
    cormax = max(cormax, GV.ms.egocor);
    tgtmin = min(tgtmin, GV.ms.afrtgt);
    tgtmax = max(tgtmax, GV.ms.afrtgt);
}

void drawNumber(int number, int scale, int offset, int x, int y)
{
    const int fontsize = 3;
    const int charsize = fontsize * 6; // a char is 6 pixel wide

    tft.setTextSize(fontsize);
    tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);
    tft.setCursor(x, y);

    const int whole = number / scale; // count negative sign
    const int decimal = abs(number) % scale;

    // Print left padding with "space" character
    for (int i = numSize(whole); i < offset; i++)
    {
        tft.print(' ');
    }

    // print whole part of number
    tft.print(whole);

    if (scale > 1)
    {
        // print decimal part of number
        const int scale_size = numSize(scale) - 1;
        const int cursor_x = (offset * charsize) + x;

        tft.setCursor(cursor_x + (fontsize * 3), y);
        for (int i = numSize(decimal); i < scale_size; i++)
        {
            tft.print('0');
        }
        tft.print(decimal);

        // print dot
        tft.setTextColor(DISPLAY_FG1, DISPLAY_FG1);
        tft.setCursor(cursor_x - (fontsize * 2), y);
        tft.print('.');
    }
}

void initMenu0()
{
    tft.fillScreen(DISPLAY_BG);

    initGauge();

    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setCursor(5, 75 + 0);
    tft.print("x");
    tft.setCursor(5, 75 + 50);
    tft.print("y");
    tft.setCursor(5, 75 + 100);
    tft.print("map");
}
void updateMenu0()
{
    updateGauge();

    drawNumber(abs(GV.accel.z) * 100, 100, 1, 5, 92 + 0);
    drawNumber(abs(GV.accel.x) * 100, 100, 1, 5, 92 + 50);
    drawNumber(GV.ms.map, 10, 3, 5, 92 + 100);
}

void initMenu1()
{
    tft.fillScreen(DISPLAY_BG);
    initGauge();
}
void updateMenu1()
{
    updateGauge();

    tft.setTextSize(10);
    tft.setCursor(30, 100);

    if (GV.gear > 0)
    {
        tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);
        tft.print(GV.gear);
    }
    else
    {
        tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
        tft.print('?');
    }
}

void initMenu2()
{
    tft.fillScreen(DISPLAY_BG);

    initGauge();

    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setCursor(5, 60 + 0);
    tft.print("VSS");

    tft.setTextSize(1);
    tft.setCursor(5, 105);
    tft.setTextColor(ILI9341_GREEN, DISPLAY_BG);
    tft.print("  AFR  ");
    tft.setTextColor(ILI9341_YELLOW, DISPLAY_BG);
    tft.print("TGT  ");
    tft.setTextColor(ILI9341_CYAN, DISPLAY_BG);
    tft.print("COR  ");
}
void updateMenu2()
{
    updateGauge();
    updateAccelGauge(68, 240 - 50, 32);
    updateAFRgraph(5, 114, 102, 42);

    tft.fillRect(5, 158, 20, 64, ILI9341_BLACK);
    tft.fillRect(5, 158 + 64 - GV.ms.map / 1000.0 * 64.0, 20, GV.ms.map / 1000.0 * 64.0, ILI9341_GREEN);
    tft.drawRect(5, 158, 20, 64, ILI9341_WHITE);

    drawNumber(GV.vss, 1, 5, 5, 77 + 0);
    drawNumber(GV.temperature * 10, 10, 4, 5, 32);
}

} // namespace Internal
} // namespace Display