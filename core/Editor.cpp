#include "Editor.h"

#define MINIMUM_CLIPPING_SIDE 50

using namespace illustrace;

class DetailCommand : public Editor::Command {
public:
    DetailCommand(Editor *editor) : Command(editor) {}

    double oldValue;
    double newValue;

    void execute() {
        document->detail(newValue);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
        illustrace->buildPaintMask(document);
    }

    void undo() {
        document->detail(oldValue);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
        illustrace->buildPaintMask(document);
    }
};

class ThicknessCommand : public Editor::Command {
public:
    ThicknessCommand(Editor *editor) : Command(editor) {}

    double oldValue;
    double newValue;

    void execute() {
        document->thickness(newValue);
    }

    void undo() {
        document->thickness(oldValue);
    }
};

class RotationCommand : public Editor::Command {
public:
    RotationCommand(Editor *editor) : Command(editor) {}

    double oldValue;
    double newValue;

    void execute() {
        document->rotation(newValue);
    }

    void undo() {
        document->rotation(oldValue);
    }
};

class BackgroundEnableCommand : public Editor::Command {
public:
    BackgroundEnableCommand(Editor *editor) : Command(editor) {}

    bool newValue;

    void execute() {
        document->backgroundEnable(newValue);
    }

    void undo() {
        document->backgroundEnable(!newValue);
    }
};

class DrawCommand : public Editor::Command {
public:
    DrawCommand(Editor *editor) : Command(editor) {}
    virtual ~DrawCommand() {}
    virtual void apply() = 0;

    cv::Mat newCanvas;
    cv::Mat oldCanvas;

    cv::Point prevPoint;
};

class PreprocessedImageCommand : public DrawCommand {
public:
    PreprocessedImageCommand(Editor *editor) : DrawCommand(editor) {}

    void execute() {
    }

    void apply() {
        illustrace->buildLines(document);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
        illustrace->buildPaintMask(document);
    }

    void undo() {
        document->preprocessedImage(oldCanvas, &document->contentRect());
        apply();
    }

    void redo() {
        document->preprocessedImage(newCanvas, &document->contentRect());
        apply();
    }
};

class PaintLayerCommand : public DrawCommand {
public:
    PaintLayerCommand(Editor *editor) : DrawCommand(editor) {}

    void execute() {
    }

    void apply() {
        illustrace->buildPaintPaths(document);
    }

    void undo() {
        document->paintLayer(oldCanvas, &document->contentRect());
        apply();
    }

    void redo() {
        document->paintLayer(newCanvas, &document->contentRect());
        apply();
    }
};

class ReloadCommand : public Editor::Command {
public:
    ReloadCommand(Editor *editor) : Command(editor) {}

    cv::Mat newCanvas;
    cv::Mat oldCanvas;

    void apply() {
        illustrace->buildLines(document);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
        illustrace->buildPaintMask(document);
    }

    void execute() {
        document->preprocessedImage(newCanvas, &document->contentRect());
        apply();
    }

    void undo() {
        document->preprocessedImage(oldCanvas, &document->contentRect());
        apply();
    }
};

class ColorCommand : public Editor::Command {
public:
    ColorCommand(Editor *editor) : Command(editor) {}
    cv::Scalar oldValue;
    cv::Scalar newValue;
};

class LineColorCommand : public ColorCommand {
public:
    LineColorCommand(Editor *editor) : ColorCommand(editor) {}

    void execute() {
        document->color(newValue);
    }

    void undo() {
        document->color(oldValue);
    }
};

class BackgroundColorCommand : public ColorCommand {
public:
    BackgroundColorCommand(Editor *editor) : ColorCommand(editor) {}

    void execute() {
        document->backgroundColor(newValue);
    }

    void undo() {
        document->backgroundColor(oldValue);
    }
};

class ClippingRectCommand : public Editor::Command {
public:
    ClippingRectCommand(Editor *editor) : Command(editor) {}

    cv::Rect oldValue;
    cv::Rect newValue;

    void execute() {
        document->clippingRect(newValue);
    }

    void undo() {
        document->clippingRect(oldValue);
    }
};


Editor::Editor(Illustrace *illustrace, Document *document)
    : illustrace(illustrace), document(document),
      _mode(Editor::Mode::Shape), _shapeState(Editor::ShapeState::Line),
      _paintState(Editor::PaintState::Brush),
      _clipState(Editor::ClipState::Trimming),
      preprocessedImageThickness(5),
      paintLayerThickness(5),
      _paintColor(cv::Scalar(255, 255, 255, 255)),
      _clearColor(cv::Scalar(0, 0, 0, 0)),
      lastCommand(nullptr),
      currentPoint(0),
      savedPoint(0)
{
}

Editor::~Editor()
{
    while (!undoStack.empty()) {
        auto *command = undoStack.top();
        undoStack.pop();
        delete command;
    }
}

Editor::Mode Editor::mode()
{
    return _mode;
}

Editor::ShapeState Editor::shapeState()
{
    return _shapeState;
}

Editor::PaintState Editor::paintState()
{
    return _paintState;
}

Editor::ClipState Editor::clipState()
{
    return _clipState;
}

int Editor::drawThickness()
{
    switch (_mode) {
    case Mode::Shape:
        return preprocessedImageThickness;
    case Mode::Paint:
        return paintLayerThickness;
    default:
        return 0;
    }
}

cv::Scalar &Editor::paintColor()
{
    return _paintColor;
}

void Editor::mode(Mode mode)
{
    lastCommand = nullptr;
    _mode = mode;
    notify(this, Event::Mode);
}

void Editor::shapeState(ShapeState state)
{
    lastCommand = nullptr;
    _shapeState = state;
    notify(this, Event::ShapeState);
}

void Editor::paintState(PaintState state)
{
    lastCommand = nullptr;
    _paintState = state;
    notify(this, Event::PaintState);
}

void Editor::clipState(ClipState state)
{
    lastCommand = nullptr;
    _clipState = state;
    notify(this, Event::ClipState);
}

void Editor::drawThickness(int drawThickness)
{
    switch (_mode) {
    case Mode::Shape:
        preprocessedImageThickness = drawThickness;
        notify(this, Event::PreprocessedImageThickness);
        break;
    case Mode::Paint:
        paintLayerThickness = drawThickness;
        notify(this, Event::PaintLayerThickness);
        break;
    default:
        break;
    }
}

void Editor::execute(Command *command)
{
    command->execute();
    if (lastCommand != command) {
        undoStack.push(command);
        ++currentPoint;
    }
    lastCommand = command;

    while (!redoStack.empty()) {
        auto *command = redoStack.top();
        redoStack.pop();
        delete command;
    }

    notify(this, Event::Execute, command);
}

void Editor::undo()
{
    if (canUndo()) {
        auto *command = undoStack.top();
        undoStack.pop();
        command->undo();
        redoStack.push(command);
        lastCommand = nullptr;
        --currentPoint;
        notify(this, Event::Undo, command);
    }
}

void Editor::redo()
{
    if (canRedo()) {
        auto *command = redoStack.top();
        redoStack.pop();
        command->redo();
        undoStack.push(command);
        lastCommand = nullptr;
        ++currentPoint;
        notify(this, Event::Redo, command);
    }
}

bool Editor::canUndo()
{
    return !undoStack.empty();
}

bool Editor::canRedo()
{
    return !redoStack.empty();
}

void Editor::save()
{
    lastCommand = nullptr;
    savedPoint = currentPoint;

    // TODO
    
    notify(this, Event::Save);
}

bool Editor::hasChanged()
{
    return savedPoint != currentPoint;
}

void Editor::detail(double detail)
{
    DetailCommand *command;

    if (!(command = dynamic_cast<DetailCommand *>(lastCommand))) {
        command = new DetailCommand(this);
        command->oldValue = document->detail();
    }

    command->newValue = detail;
    execute(command);
}

void Editor::thickness(double thickness)
{
    ThicknessCommand *command;

    if (!(command = dynamic_cast<ThicknessCommand *>(lastCommand))) {
        command = new ThicknessCommand(this);
        command->oldValue = document->thickness();
    }

    command->newValue = thickness;
    execute(command);
}

void Editor::rotation(double rotation)
{
    RotationCommand *command;

    if (!(command = dynamic_cast<RotationCommand *>(lastCommand))) {
        command = new RotationCommand(this);
        command->oldValue = document->rotation();
    }

    command->newValue = rotation;
    execute(command);
}

void Editor::backgroundEnable(bool enable)
{
    BackgroundEnableCommand *command;

    if (!(command = dynamic_cast<BackgroundEnableCommand *>(lastCommand))) {
        command = new BackgroundEnableCommand(this);
    }

    command->newValue = enable;
    execute(command);
}

void Editor::draw(float x, float y)
{
    cv::Point point = cv::Point(x, y);
    cv::Point *prevPoint = nullptr;

    DrawCommand *command;

    if (!(command = dynamic_cast<DrawCommand *>(lastCommand))) {
        switch (_mode) {
        case Mode::Shape:
            command = new PreprocessedImageCommand(this);
            command->oldCanvas = document->preprocessedImage();
            command->newCanvas = command->oldCanvas.clone();
            document->preprocessedImage(command->newCanvas, nullptr);
            break;
        case Mode::Paint:
            command = new PaintLayerCommand(this);
            command->oldCanvas = document->paintLayer();
            command->newCanvas = command->oldCanvas.clone();
            document->paintLayer(command->newCanvas, nullptr);
            break;
        default:
            // Illegal operation
            return;
        }
    }
    else {
        prevPoint = &command->prevPoint;
    }

    switch (_mode) {
    case Mode::Shape:
        switch (_shapeState) {
        case ShapeState::Pencil:
            illustrace->drawLineOnPreprocessedImage(prevPoint ? *prevPoint : point, point, preprocessedImageThickness, 255, document);
            break;
        case ShapeState::Eraser:
            illustrace->drawLineOnPreprocessedImage(prevPoint ? *prevPoint : point, point, preprocessedImageThickness, 0, document);
            break;
        default:
            break;
        }
        break;
    case Mode::Paint:
        switch (_paintState) {
        case PaintState::Brush:
            illustrace->drawLineOnPaintLayer(prevPoint ? *prevPoint : point, point, paintLayerThickness, _paintColor, document);
            break;
        case PaintState::Eraser:
            illustrace->drawLineOnPaintLayer(prevPoint ? *prevPoint : point, point, paintLayerThickness, _clearColor, document);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    execute(command);
    command->prevPoint = point;
}

void Editor::drawFinish()
{
    DrawCommand *command;

    if ((command = dynamic_cast<DrawCommand *>(lastCommand))) {
        command->apply();
    }

    lastCommand = nullptr;
}

void Editor::reload()
{
    ReloadCommand *command = new ReloadCommand(this);
    command->oldCanvas = document->preprocessedImage();
    command->newCanvas = document->negativeImage().clone();
    execute(command);
}

void Editor::fill(float x, float y)
{
    DrawCommand *command = new PaintLayerCommand(this);
    command->oldCanvas = document->paintLayer();
    command->newCanvas = command->oldCanvas.clone();
    document->paintLayer(command->newCanvas, nullptr);
    execute(command);

    auto point = cv::Point(x, y);

    switch (_paintState) {
    case PaintState::Fill:
        illustrace->fillRegionOnPaintLayer(point, _paintColor, document);
        break;
    case PaintState::Eraser:
        {
            auto transparent = cv::Scalar(0, 0, 0, 0);
            illustrace->fillRegionOnPaintLayer(point, transparent, document);
        }
        break;
    default:
        break;
    }

    command->apply();

    lastCommand = nullptr;
}

void Editor::R(double red)
{
    color(0, red);
}

void Editor::G(double green)
{
    color(1, green);
}

void Editor::B(double blue)
{
    color(2, blue);
}

void Editor::color(int colorIndex, double value)
{
    if (Mode::Paint == _mode) {
        _paintColor[colorIndex] = cv::saturate_cast<uchar>(value * 255.0);
        notify(this, Event::PaintColor);
    }
    else {
        ColorCommand *command;

        if (!(command = dynamic_cast<ColorCommand *>(lastCommand))) {
            switch (_mode) {
            case Mode::Shape:
                command = new LineColorCommand(this);
                command->oldValue = document->color();
                break;
            case Mode::BG:
                command = new BackgroundColorCommand(this);
                command->oldValue = document->backgroundColor();
                break;
            default:
                // Illegal operation
                return;
            }

            command->newValue = command->oldValue;
        }

        command->newValue[colorIndex] = cv::saturate_cast<uchar>(value * 255.0);
        execute(command);
    }
}

void Editor::trimmingTopLeft(float x, float y)
{
    trimming([=](cv::Rect &rect) {
        rect.x = MIN(x, rect.x + rect.width - 1 - MINIMUM_CLIPPING_SIDE);
        rect.y = MIN(y, rect.y + rect.height - 1 - MINIMUM_CLIPPING_SIDE);
    });
}

void Editor::trimmingTop(float y)
{
    trimming([=](cv::Rect &rect) {
        rect.y = MIN(y, rect.y + rect.height - 1 - MINIMUM_CLIPPING_SIDE);
    });
}

void Editor::trimmingTopRight(float x, float y)
{
    trimming([=](cv::Rect &rect) {
        rect.y = MIN(y, rect.y + rect.height - 1 - MINIMUM_CLIPPING_SIDE);
        rect.width = MAX(x - rect.x + 1, rect.x + MINIMUM_CLIPPING_SIDE - 1);
    });
}

void Editor::trimmingRight(float x)
{
    trimming([=](cv::Rect &rect) {
        rect.width = MAX(x - rect.x + 1, rect.x + MINIMUM_CLIPPING_SIDE - 1);
    });
}

void Editor::trimmingBottomRight(float x, float y)
{
    trimming([=](cv::Rect &rect) {
        rect.width = MAX(x - rect.x + 1, rect.x + MINIMUM_CLIPPING_SIDE - 1);
        rect.height = MAX(y - rect.y + 1, rect.y + MINIMUM_CLIPPING_SIDE - 1);
    });
}

void Editor::trimmingBottom(float y)
{
    trimming([=](cv::Rect &rect) {
        rect.height = MAX(y - rect.y + 1, rect.y + MINIMUM_CLIPPING_SIDE - 1);
    });
}

void Editor::trimmingBottomLeft(float x, float y)
{
    trimming([=](cv::Rect &rect) {
        rect.height = MAX(y - rect.y + 1, rect.y + MINIMUM_CLIPPING_SIDE - 1);
        rect.x = MIN(x, rect.x + rect.width - 1 - MINIMUM_CLIPPING_SIDE);
    });
}

void Editor::trimmingLeft(float x)
{
    trimming([=](cv::Rect &rect) {
        rect.x = MIN(x, rect.x + rect.width - 1 - MINIMUM_CLIPPING_SIDE);
    });
}

template<typename Func>
void Editor::trimming(Func func)
{
    ClippingRectCommand *command;

    if (!(command = dynamic_cast<ClippingRectCommand *>(lastCommand))) {
        command = new ClippingRectCommand(this);
        command->oldValue = document->clippingRect();
        command->newValue = command->oldValue;
    }

    func(command->newValue);
    execute(command);
}

namespace illustrace {

std::ostream &operator<<(std::ostream &os, Editor const &self)
{
    os << "<Editor ";

    os << "mode: " << Editor::Mode2CString(self._mode) << ", ";
    os << "ShapeState: " << Editor::ShapeState2CString(self._shapeState) << ", ";
    os << "paintState: " << Editor::PaintState2CString(self._paintState) << ", ";
    os << "clipState: " << Editor::ClipState2CString(self._clipState) << ", ";
    os << "preprocessedImageThickness: " << self.preprocessedImageThickness << ", ";
    os << "paintLayerThickness: " << self.paintLayerThickness << ", ";
    os << "paintColor: " << self._paintColor << ", ";
    os << "undoStack: " << self.undoStack.size() << ", ";
    os << "redoStack: " << self.redoStack.size() << ", ";
    os << "lastCommand: " << self.lastCommand << ", ";
    os << "currentPoint: " << self.currentPoint << ", ";
    os << "savedPoint: " << self.savedPoint << "";
    os << ">";

    return os;
}

} // namespace illustrace
