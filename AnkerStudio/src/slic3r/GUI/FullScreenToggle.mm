#import <Cocoa/Cocoa.h>
#include <wx/wx.h>
#include <wx/osx/core/private.h>

extern "C" void ToggleFullScreen(wxWindow* window) {

    NSView* nsView = (NSView*)window->GetHandle();
    NSWindow* nsWindow = [nsView window];
    [nsWindow toggleFullScreen:nil];
}
