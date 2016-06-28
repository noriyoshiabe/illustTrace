#include "Editor.h"

using namespace illustrace;

class Editor::Command {
public:
    Command(Editor *editor) : document(editor->document), illustrace(editor->illustrace) {};
    virtual ~Command() {}
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual void redo() { execute(); }

    Document *document;
    Illustrace *illustrace;
};

class DetailCommand : public Editor::Command {
public:
    DetailCommand(Editor *editor) : Command(editor) {}

    double oldValue;
    double newValue;

    void execute() {
        document->detail(newValue);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
    }

    void undo() {
        document->detail(oldValue);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
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

    cv::Mat newCanvas;
    cv::Mat oldCanvas;
};

class PreprocessedImageCommand : public DrawCommand {
public:
    PreprocessedImageCommand(Editor *editor) : DrawCommand(editor) {}

    void execute() {
        document->preprocessedImage(newCanvas, nullptr);
    }

    void undo() {
        auto dirtyRect = cv::Rect(0, 0, oldCanvas.cols, oldCanvas.rows);
        document->preprocessedImage(oldCanvas, &dirtyRect);
        illustrace->buildLines(document);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
    }

    void redo() {
        auto dirtyRect = cv::Rect(0, 0, newCanvas.cols, newCanvas.rows);
        document->preprocessedImage(newCanvas, &dirtyRect);
        illustrace->buildLines(document);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
    }
};

class PaintLayerCommand : public DrawCommand {
public:
    PaintLayerCommand(Editor *editor) : DrawCommand(editor) {}

    void execute() {
        document->paintLayer(newCanvas, nullptr);
    }

    void undo() {
        auto dirtyRect = cv::Rect(0, 0, oldCanvas.cols, oldCanvas.rows);
        document->paintLayer(oldCanvas, &dirtyRect);
        illustrace->buildPaths(document);
    }

    void redo() {
        auto dirtyRect = cv::Rect(0, 0, newCanvas.cols, newCanvas.rows);
        document->paintLayer(newCanvas, &dirtyRect);
        illustrace->buildPaths(document);
    }
};


Editor::Editor(Illustrace *illustrace, Document *document)
    : illustrace(illustrace), document(document),
      _mode(Editor::Mode::Line), _lineState(Editor::LineState::Line),
      _paintState(Editor::PaintState::Brush),
      _clipState(Editor::ClipState::Trimming),
      preprocessedImageRadius(5),
      paintLayerRadius(5),
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

Editor::LineState Editor::lineState()
{
    return _lineState;
}

Editor::PaintState Editor::paintState()
{
    return _paintState;
}

Editor::ClipState Editor::clipState()
{
    return _clipState;
}

int Editor::radius()
{
    switch (_mode) {
    case Mode::Line:
        return preprocessedImageRadius;
    case Mode::Paint:
        return paintLayerRadius;
    default:
        return 0;
    }
}

void Editor::mode(Mode mode)
{
    lastCommand = nullptr;
    _mode = mode;
    notify(this, Event::Mode);
}

void Editor::lineState(LineState state)
{
    lastCommand = nullptr;
    _lineState = state;
    notify(this, Event::LineState);
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

void Editor::radius(int radius)
{
    switch (_mode) {
    case Mode::Line:
        preprocessedImageRadius = radius;
        notify(this, Event::Radius);
        break;
    case Mode::Paint:
        paintLayerRadius = radius;
        notify(this, Event::Radius);
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

    notify(this, Event::Execute);
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
        notify(this, Event::Undo);
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
        notify(this, Event::Redo);
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
    DrawCommand *command;

    if (!(command = dynamic_cast<DrawCommand *>(lastCommand))) {
        switch (_mode) {
        case Mode::Line:
            command = new PreprocessedImageCommand(this);
            command->oldCanvas = document->preprocessedImage();
            break;
        case Mode::Paint:
            command = new PaintLayerCommand(this);
            command->oldCanvas = document->paintLayer();
            break;
        default:
            // Illegal operation
            return;
        }

        command->newCanvas = command->oldCanvas.clone();
        execute(command);
    }

    auto point = cv::Point(x, y);

    switch (_mode) {
    case Mode::Line:
        switch (_lineState) {
        case LineState::PencilBlack:
            illustrace->drawCircleOnPreprocessedImage(point, preprocessedImageRadius, 0, document);
            break;
        case LineState::PencilWhite:
            illustrace->drawCircleOnPreprocessedImage(point, preprocessedImageRadius, 255, document);
            break;
        case LineState::Eraser:
            illustrace->eraseCircleOnPreprocessedImage(point, preprocessedImageRadius, document);
            break;
        default:
            break;
        }
        break;
    case Mode::Paint:
        if (PaintState::Brush == _paintState) {
            illustrace->drawCircleOnPaintLayer(point, paintLayerRadius, paintColor, document);
        }
        break;
    default:
        break;
    }
}

void Editor::drawFinish()
{
    switch (_mode) {
    case Mode::Line:
        illustrace->buildLines(document);
        illustrace->approximateLines(document);
        illustrace->buildPaths(document);
        break;
    case Mode::Paint:
        illustrace->buildPaths(document);
        break;
    default:
        break;
    }

    lastCommand = nullptr;
}

void Editor::fill(float x, float y)
{
    DrawCommand *command = new PaintLayerCommand(this);
    command->oldCanvas = document->paintLayer();
    command->newCanvas = command->oldCanvas.clone();
    execute(command);

    auto point = cv::Point(x, y);

    switch (_paintState) {
    case PaintState::Fill:
        illustrace->fillRegionOnPaintLayer(point, paintColor, document);
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

    illustrace->buildPaths(document);

    lastCommand = nullptr;
}
