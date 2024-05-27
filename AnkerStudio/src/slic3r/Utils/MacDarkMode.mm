#import "MacDarkMode.hpp"

#import <algorithm>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <AppKit/NSScreen.h>


@interface MacDarkMode : NSObject {}
@end

/* edit column for wxCocoaOutlineView */

#include <wx/dataview.h>
#include <wx/osx/cocoa/dataview.h>
#include <wx/osx/dataview.h>

@implementation wxCocoaOutlineView (Edit)

bool addObserver = false;

- (BOOL)outlineView: (NSOutlineView*) view shouldEditTableColumn:(nullable NSTableColumn *)tableColumn item:(nonnull id)item
{
    NSClipView * clipView = [[self enclosingScrollView] contentView];
    if (!addObserver) {
        addObserver = true;
        clipView.postsBoundsChangedNotifications = YES;
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(synchronizedViewContentBoundsDidChange:)
                                                     name:NSViewBoundsDidChangeNotification
                                                   object:clipView];
    }

    wxDataViewColumn* const col((wxDataViewColumn *)[tableColumn getColumnPointer]);
    wxDataViewItem item2([static_cast<wxPointerObject *>(item) pointer]);

    wxDataViewCtrl* const dvc = implementation->GetDataViewCtrl();
    // Before doing anything we send an event asking if editing of this item is really wanted.
    wxDataViewEvent event(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, dvc, col, item2);
    dvc->GetEventHandler()->ProcessEvent( event );
    if( !event.IsAllowed() )
        return NO;

    return YES;
}

- (void)synchronizedViewContentBoundsDidChange:(NSNotification *)notification
{
    wxDataViewCtrl* const dvc = implementation->GetDataViewCtrl();
    wxDataViewCustomRenderer * r = dvc->GetCustomRendererPtr();
    if (r)
        r->FinishEditing();
}

@end

@implementation MacDarkMode

namespace Slic3r {
namespace GUI {

bool mac_dark_mode()
{
    NSString *style = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
    return style && [style isEqualToString:@"Dark"];

}

double mac_max_scaling_factor()
{
    double scaling = 1.;
    if ([NSScreen screens] == nil) {
        scaling = [[NSScreen mainScreen] backingScaleFactor];
    } else {
	    for (int i = 0; i < [[NSScreen screens] count]; ++ i)
	    	scaling = std::max<double>(scaling, [[[NSScreen screens] objectAtIndex:0] backingScaleFactor]);
	}
    return scaling;
}

}
}

@end

