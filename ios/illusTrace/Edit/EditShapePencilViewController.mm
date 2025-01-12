//
//  EditShapePencilViewController.mm
//  illusTrace
//
//  Created by abechan on 2016/08/01.
//  Copyright © 2016年 Noriyoshi Abe. All rights reserved.
//

#import "EditShapePencilViewController.h"
#import "EditorObserver.h"
#import "Color.h"

using namespace illustrace;

@interface EditShapePencilViewController () <EditorObserver, PreviewViewDelegate> {
    EditorObserverBridge _editorObserverBridge;
}

@property (weak, nonatomic) IBOutlet UISlider *thicknessSlider;
@property (weak, nonatomic) IBOutlet UIButton *moveButton;
@property (assign, nonatomic) BOOL move;
@end

@implementation EditShapePencilViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _editorObserverBridge.observer = self;
    
    _thicknessSlider.minimumValue = 5.0;
    _thicknessSlider.maximumValue = 100.0;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    _editor->addObserver(&_editorObserverBridge);
    
    _previewView.drawPreprocessedImage = YES;
    self.move = NO;
    
    [self update];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    _editor->removeObserver(&_editorObserverBridge);
    _previewView.delegate = nil;
    _previewView.drawPreprocessedImage = NO;
}

- (void)update
{
    _thicknessSlider.value = _editor->drawThickness();
}

- (void)setMove:(BOOL)move
{
    if (move) {
        _previewView.scrollEnabled = YES;
        _previewView.zoomEnabled = YES;
        _previewView.delegate = nil;
        _moveButton.tintColor = [Color systemBlueColor];
    }
    else {
        _previewView.scrollEnabled = NO;
        _previewView.zoomEnabled = NO;
        _previewView.delegate = self;
        _moveButton.tintColor = [UIColor darkGrayColor];
    }
    
    _move = move;
}

- (IBAction)thicknessSliderAction:(id)sender
{
    _editor->drawThickness(_thicknessSlider.value);
}

- (IBAction)moveButtonAction:(id)sender
{
    self.move = !_move;
}

#pragma mark EditorObserver

- (void)editor:(Editor *)editor notify:(va_list)argList
{
    Editor::Event event = static_cast<Editor::Event>(va_arg(argList, int));
    
    switch (event) {
        case Editor::Event::PreprocessedImageThickness:
            [self update];
            break;
        default:
            break;
    }
}

#pragma mark PreviewViewDelegate

- (void)previewView:(PreviewView *)previewView touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    CGPoint point = [previewView locationInDocumentWithTouch:touches.anyObject];
    _editor->draw(point.x, point.y);
}

- (void)previewView:(PreviewView *)previewView touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    CGPoint point = [previewView locationInDocumentWithTouch:touches.anyObject];
    _editor->draw(point.x, point.y);
}

- (void)previewView:(PreviewView *)previewView touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    _editor->drawFinish();
}

- (void)previewView:(PreviewView *)previewView touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
}

@end
