#pragma once

#include "Observable.h"

#include "opencv2/opencv.hpp"
#include <vector>

namespace illustrace {

enum class LineMode {
    Center,
    Outline,
};

struct Segment {
    enum Type {
        Move,
        Line,
        Curve
    };

    Type type;
    cv::Point2f p[3];

    cv::Point2f& operator[] (const int index) {
        return p[index];
    }

    static Segment M(cv::Point2f p) {
        return (Segment){
            Type::Move,
            {
                {0.0, 0.0},
                {0.0, 0.0},
                {p.x, p.y},
            }
        };
    }

    static Segment M(cv::Point p) {
        return (Segment){
            Type::Move,
            {
                {0.0, 0.0},
                {0.0, 0.0},
                {(float)p.x, (float)p.y},
            }
        };
    }

    static Segment L(cv::Point2f p) {
        return (Segment){
            Type::Line,
            {
                {0.0, 0.0},
                {0.0, 0.0},
                {p.x, p.y},
            }
        };
    }

    static Segment L(cv::Point p) {
        return (Segment){
            Type::Line,
            {
                {0.0, 0.0},
                {0.0, 0.0},
                {(float)p.x, (float)p.y},
            }
        };
    }

    static Segment C(cv::Point2f p1, cv::Point2f p2, cv::Point2f p3) {
        return (Segment){
            Type::Curve,
            {
                {p1.x, p1.y},
                {p2.x, p2.y},
                {p3.x, p3.y},
            }
        };
    }
};

struct Path {
    std::vector<Segment> segments;
    bool closed;
    std::vector<Path *> children;

    Path() : closed(false) {};
};

class Document : public Observable<Document> {
public:
    enum Event : int {
        Mode,
        Brightness,
        Blur,
        Detail,
        Smoothing,
        Thickness,
        Transform,
        Color,
        BackgroundColor,
        BackgroundEnable,
        PaintLayer,
        ClippingRect,
        BoundingRect,
        Paths,
        PaintPaths,
        BinarizedImage,
        PreprocessedImage,
        CenterLines,
        ApproximatedCenterLines,
        OutlineContours,
        ApproximatedOutlineContours,
        OutlineHierarchy,
    };

    Document();
    ~Document();

    LineMode mode();
    double brightness();
    double blur();
    double detail();
    double smoothing();
    double thickness();
    cv::Mat &transform();
    cv::Scalar &color();
    cv::Scalar &backgroundColor();
    bool backgroundEnable();
    cv::Mat &paintLayer();
    cv::Rect &clippingRect();
    cv::Rect &boundingRect();
    std::vector<Path *> *paths();
    std::vector<Path *> *paintPaths();
    cv::Mat &binarizedImage();
    cv::Mat &preprocessedImage();
    std::vector<std::vector<cv::Point2f>> *centerLines();
    std::vector<std::vector<cv::Point2f>> *approximatedCenterLines();
    std::vector<std::vector<cv::Point>> *outlineContours();
    std::vector<std::vector<cv::Point2f>> *approximatedOutlineContours();
    std::vector<cv::Vec4i> *outlineHierarchy();

    void mode(LineMode mode);
    void brightness(double brightness);
    void blur(double blur);
    void detail(double detail);
    void smoothing(double smoothing);
    void thickness(double thickness);
    void transform(cv::Mat &transform);
    void color(cv::Scalar &color);
    void backgroundColor(cv::Scalar &backgroundColor);
    void backgroundEnable(bool enable);
    void paintLayer(cv::Mat &paintLayer);
    void clippingRect(cv::Rect &rect);
    void boundingRect(cv::Rect &rect);
    void paths(std::vector<Path *> *paths);
    void paintPaths(std::vector<Path *> *paintPaths);
    void binarizedImage(cv::Mat &binarizedImage);
    void preprocessedImage(cv::Mat &preprocessedImage);
    void centerLines(std::vector<std::vector<cv::Point2f>> *centerLines);
    void approximatedCenterLines(std::vector<std::vector<cv::Point2f>> *approximatedCenterLines);
    void outlineContours(std::vector<std::vector<cv::Point>> *outlineContours);
    void approximatedOutlineContours(std::vector<std::vector<cv::Point2f>> *approximatedOutlineContours);
    void outlineHierarchy(std::vector<cv::Vec4i> *outlineHierarchy);

private:
    LineMode _mode;
    double _brightness;
    double _blur;
    double _detail;
    double _smoothing;
    double _thickness;
    cv::Mat _transform;
    cv::Scalar _color;
    cv::Scalar _backgroundColor;
    cv::Mat _paintlayer;
    bool _backgroundEnable;
    cv::Rect _clippingRect;
    cv::Rect _boundingRect;
    std::vector<Path *> *_paths;
    std::vector<Path *> *_paintPaths;

    cv::Mat _binarizedImage;
    cv::Mat _preprocessedImage;

    std::vector<std::vector<cv::Point2f>> *_centerLines;
    std::vector<std::vector<cv::Point2f>> *_approximatedCenterLines;
    std::vector<std::vector<cv::Point>> *_outlineContours;
    std::vector<std::vector<cv::Point2f>> *_approximatedOutlineContours;
    std::vector<cv::Vec4i> *_outlineHierarchy;
};

} // namespace illustrace
