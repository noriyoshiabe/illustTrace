#pragma once

#include "Observer.h"
#include "Illustrace.h"

#include "opencv2/highgui.hpp"
#include "cairo/cairo.h"

namespace illustrace {
namespace cli {

class View : public lib::Observer<core::Illustrace, core::IllustraceEvent> {
public:
    View();
    ~View();

    int wait;
    bool step;
    bool plot;

    void waitKey();
    void waitKeyIfNeeded();

private:
    cv::Mat preview;
    cairo_surface_t *surface;
    cairo_t *cr;

    void notify(core::Illustrace *sender, core::IllustraceEvent event, va_list argList);

    void clearPreview();
    void copyFrom(cv::Mat &image);
    template <class T>
    void drawLines(std::vector<std::vector<T>> &lines, double thickness, bool closePath = false);
    void drawBezierLines(std::vector<std::vector<core::BezierVertex<cv::Point2f>>> &bezierLines, double thickness, bool withPlot = false);
    template <class T>
    void plotPoints(std::vector<T> &points);
    template <class T>
    void plotPoints(std::vector<std::vector<T>> &lines);
    void plotGraph(core::Graph &graph);
    void plotBezierHandle(std::vector<std::vector<core::BezierVertex<cv::Point2f>>> &bezierLines);
};

} // namespace cli
} // namespace illustrace
