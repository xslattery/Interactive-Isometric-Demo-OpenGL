#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <mach/mach_time.h>

#import "game.hpp"
#import <glm/glm.hpp>

#define HANDMADE_USE_VSYNC_AND_DOUBLE_BUFFER 1
#ifndef BUILD_FOR_APP_BUNDLE
#define BUILD_FOR_APP_BUNDLE 0
#endif

static NSOpenGLContext* GlobalGLContext;
static float GlobalAspectRatio = 16/9.0f;
static bool running;

static bool fullscreen = false;
static float gl_width = 960;
static float gl_height = 540;

extern bool down_keys[256];
extern glm::vec2 gl_viewport_size;

void refresh_after_resize() {
	glViewport( 0, 0, gl_viewport_size.x, gl_viewport_size.y );
	glClearColor( 0, 0, 0, 1 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void hide_cursor() {
	[NSCursor hide];
}

void unhide_cursor() {
	[NSCursor unhide];
}

const char* getApplicationSupportDir () {
	#if BUILD_FOR_APP_BUNDLE
		NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
	#else
		NSString *bundleID = @"_DEBUG-HandmadeCocoa";
	#endif

	if ( bundleID == nil ) { return nullptr; }

	NSFileManager *fm = [NSFileManager defaultManager];
	NSURL *dirPath = nil;

	NSArray *appSupportDir = [fm URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
	if ( [appSupportDir count] > 0 ) {

		dirPath = [[appSupportDir objectAtIndex:0] URLByAppendingPathComponent:bundleID];

		NSError *theError = nil;
		if ( ![fm createDirectoryAtURL:dirPath withIntermediateDirectories:YES attributes:nil error:&theError] ) {
			// Handle the error.
			return (char*)"?";
		}
	}

	return (const char*)[[dirPath.path stringByAppendingString:@"/"] UTF8String];
}

void createDirectoryAt( const char* dir ) {
	NSFileManager *fm = [NSFileManager defaultManager];
	NSString *directory = [NSString stringWithUTF8String:dir];
	NSError	*error = nil;
	[[NSFileManager defaultManager] createDirectoryAtPath:directory withIntermediateDirectories:NO attributes:nil error:&error];
}

void removeFileAt( const char* filename ) {
	NSString* fileremoval = [NSString stringWithUTF8String:filename];
	[[NSFileManager defaultManager] removeItemAtPath:fileremoval error:NULL];
}

const char* getFilePathFromDialog() {
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];

	[openDlg setCanChooseFiles:YES];
	[openDlg setAllowsMultipleSelection:NO];
	[openDlg setCanChooseDirectories:NO];

	if ( [openDlg runModal] == NSModalResponseOK ) {
		NSURL *file = [[openDlg URLs] objectAtIndex:0];
		return (char*)[[file path] UTF8String];
	}

	return nullptr;
}

int alertBox( const char *msg, const char *info ) {
	NSAlert *alert = [[NSAlert alloc] init];
	alert.alertStyle = NSAlertStyleCritical;

	NSImage *picture =  [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:"/Users/xavier/Desktop/2DGame Text Improvements/res/msgbox.png"]];
	alert.icon = picture;

	[alert setMessageText:[NSString stringWithUTF8String:msg]];
	[alert setInformativeText:[NSString stringWithUTF8String:info]];
	
	[alert addButtonWithTitle:[NSString stringWithUTF8String:"Ok"]];
	
	// This will halt the application until the message box is dismissed.
	NSModalResponse output = [alert runModal];
	[alert release];

	switch (output) {
		case NSAlertThirdButtonReturn: { return 3; } break;
		case NSAlertSecondButtonReturn: { return 2; } break;
		default: case NSAlertFirstButtonReturn: { return 1; } break;
	}
}

//////////////////////////////////////////////
// Application Delegate
@interface AppDelegate : NSObject<NSApplicationDelegate, NSWindowDelegate>
{}
@end

@implementation AppDelegate

- (void) applicationDidFinishLaunching:(id)sender {
	#pragma unused(sender)
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
	#pragma unused(sender)
	return YES;
}

- (void) applicationWillTerminate:(NSApplication*)sender {
	#pragma unused(sender)
	printf("applicationWillTerminate\n");
	running = false;
}

///////////////////////////////////////////////////
// Window delegate methods
- (void) windowWillClose:(id)sender {
	printf("Main window will close. Stopping game.\n");
	running = false;
}

- (NSSize) windowWillResize:(NSWindow*)window toSize:(NSSize)frameSize {
	// @NOTE(Xavier): The following will maintain an aspect ratio for the window.
	// NSRect WindowRect = [window frame];
	// NSRect ContentRect = [window contentRectForFrameRect:WindowRect];
	// float WidthAdd = (WindowRect.size.width - ContentRect.size.width);
	// float HeightAdd = (WindowRect.size.height - ContentRect.size.height);
	// float RenderAspect = GlobalAspectRatio; // 16.0f/9.0f
	// float NewCy = (frameSize.width - WidthAdd) / RenderAspect;
	// frameSize.height = NewCy + HeightAdd;
	return frameSize;
}

@end



void OSXCreateMainMenu() {
    NSMenu* menubar = [NSMenu new];

	NSMenuItem* appMenuItem = [NSMenuItem new];
	[menubar addItem:appMenuItem];

	[NSApp setMainMenu:menubar];

	NSMenu* appMenu = [NSMenu new];

    //NSString* appName = [[NSProcessInfo processInfo] processName];
    NSString* appName = @"Handmade Cocoa";

    NSString* toggleFullScreenTitle = @"Toggle Full Screen";
    NSMenuItem* toggleFullScreenMenuItem = [[NSMenuItem alloc] initWithTitle:toggleFullScreenTitle action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
    [appMenu addItem:toggleFullScreenMenuItem];

    NSString* quitTitle = [@"Quit " stringByAppendingString:appName];
    NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];

    [appMenuItem setSubmenu:appMenu];
}



///////////////////////////////////////////////////////////////////////
// OpenGL View
@interface OpenGLView : NSOpenGLView
{}
@end

@implementation OpenGLView

- (id) init {
	self = [super init];
	return self;
}

- (void) prepareOpenGL {
	[super prepareOpenGL];
	[[self openGLContext] makeCurrentContext];
}

- (void) reshape {
	[super reshape];

	// @NOTE(Xavier): This is the fix to enable the conversion for higher resolution.
	// https://developer.apple.com/library/content/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/CapturingScreenContents/CapturingScreenContents.html
	// The common practice of passing the width and height of [self bounds] to glViewport() 
	// will yield incorrect results at high resolution, because the parameters passed to 
	// the glViewport() function must be in pixels. As a result, you’ll get only partial
	// instead of complete coverage of the render surface. Instead, use the backing store bounds:
	[GlobalGLContext makeCurrentContext];
	[GlobalGLContext update];

	NSRect window_bounds = [self bounds];
	NSRect bounds = [self convertRectToBacking:[self bounds]];

	// if ( fullscreen ) glViewport(0, 0, window_bounds.size.width, window_bounds.size.height);
	glViewport(0, 0, bounds.size.width, bounds.size.height);
	resize_view( window_bounds.size.width, window_bounds.size.height, bounds.size.width, bounds.size.height );

	// @NOTE(Xavier): I am unsure if there is any issues with this.
	// Currently I am using this reshape function as a place to render
	// while the window is resizing so that the window is not black during resize.
	update_game();
	render_game();
	
	#if HANDMADE_USE_VSYNC_AND_DOUBLE_BUFFER
		[GlobalGLContext flushBuffer];
	#else
		glFlush();
	#endif
}

- (void) drawRect:(NSRect)bounds {
	#pragma unused(bounds)
	// @NOTE(Xavier): Rendering here causes issues when in fullscreen.
	// My current (28.6.17) idea is not to render from this function.
}

@end

void toggle_fullscreen() {

    if ( !fullscreen ) {
    	fullscreen = true;
	    
	    NSRect screenRect = [[NSScreen mainScreen] frame];
        [[NSApp mainWindow] setStyleMask:NSWindowStyleMaskBorderless];
        [[NSApp mainWindow] setFrame:screenRect display:YES];
        [[NSApp mainWindow] setLevel:NSMainMenuWindowLevel+1];
        [[NSApp mainWindow] setOpaque:YES];
        [[NSApp mainWindow] setHidesOnDeactivate:YES];
		
		// // NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:NO], NSFullScreenModeAllScreens, nil];
		// GLint dim[2] = {1280, 800};
		// gl_width = dim[0]; gl_height = dim[1];
		// CGLSetParameter([GlobalGLContext CGLContextObj], kCGLCPSurfaceBackingSize, dim);
		// CGLEnable ([GlobalGLContext CGLContextObj], kCGLCESurfaceBackingSize);
		// [GLView enterFullScreenMode:[NSScreen mainScreen] withOptions:options];

		// NSRect bounds = [GLView convertRectToBacking:[GLView bounds]];
		// glViewport(0, 0, gl_width, gl_height);
		// resize_view( gl_width, gl_height, gl_width, gl_height );

	} else if ( fullscreen ) {
		fullscreen = false;
		
		NSRect screenRect = [[NSScreen mainScreen] frame];
        unsigned int width = 960;
        unsigned int height = 540;

        NSRect InitialFrame = NSMakeRect((screenRect.size.width - width) * 0.5, (screenRect.size.height - height) * 0.5, width, height);

        [[NSApp mainWindow] setStyleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable ];
        [[NSApp mainWindow] setFrame:InitialFrame display:YES];
        [[NSApp mainWindow] setLevel:NSNormalWindowLevel];
        [[NSApp mainWindow] setOpaque:YES];
        [[NSApp mainWindow] setHidesOnDeactivate:NO];

        // This has to be done a seccond time bacause the title bar is not taken into account.
        float title_bar_height = [[NSApp mainWindow] frame].size.height - [[[NSApp mainWindow] contentView] frame].size.height;
        height = 540 + title_bar_height;
        InitialFrame = NSMakeRect((screenRect.size.width - width) * 0.5, (screenRect.size.height - height) * 0.5, width, height);
        [[NSApp mainWindow] setFrame:InitialFrame display:YES];
	}
}

///////////////////////////////////////////////////////////////////////
// Startup
int main(int argc, const char* argv[]) {

	@autoreleasepool {

		///////////////////////////////////
		// NSApplication
		NSApplication* app = [NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp activateIgnoringOtherApps:YES];

		OSXCreateMainMenu();

		///////////////////////////////
		// Set working directory
		NSString *dir = [[NSFileManager defaultManager] currentDirectoryPath];
		NSLog(@"Working directory: %@", dir);

		NSFileManager* FileManager = [NSFileManager defaultManager];
		NSString* AppPath = [NSString stringWithFormat:@"%@/Contents/Resources", [[NSBundle mainBundle] bundlePath]];

		if ([FileManager changeCurrentDirectoryPath:AppPath] == NO) {
			printf("Directory Error!!!!!!\n");
		} else {
			dir = AppPath;
		}

		AppDelegate* appDelegate = [[AppDelegate alloc] init];
		[app setDelegate:appDelegate];

	    [NSApp finishLaunching];

	    // [NSCursor hide];

		/////////////////////////
		// OpenGL Setup
	    NSOpenGLPixelFormatAttribute openGLAttributes[] = {
	        NSOpenGLPFAAccelerated,
        #if HANDMADE_USE_VSYNC_AND_DOUBLE_BUFFER
	        NSOpenGLPFADoubleBuffer, // Uses vsync
		#endif
			NSOpenGLPFAColorSize, 24,
			NSOpenGLPFAAlphaSize, 8,
	        NSOpenGLPFADepthSize, 24,
	        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
	        0
	    };
		NSOpenGLPixelFormat* PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:openGLAttributes];
	    GlobalGLContext = [[NSOpenGLContext alloc] initWithFormat:PixelFormat shareContext:NULL];

		///////////////////////////////////////////
		// NSWindow and NSOpenGLView:
		// Create the main window and the content view.
		NSRect screenRect = [[NSScreen mainScreen] frame];
		float Width = 960.0;
		float Height = 540.0;

		NSRect InitialFrame = NSMakeRect((screenRect.size.width - Width) * 0.5, (screenRect.size.height - Height) * 0.5, Width, Height);
		NSWindow* window = [[NSWindow alloc] initWithContentRect:InitialFrame
											 styleMask:NSWindowStyleMaskTitled
											 | NSWindowStyleMaskClosable
											 | NSWindowStyleMaskMiniaturizable
											 // | NSWindowStyleMaskResizable
											 // @NOTE(Xavier): This is What makes the title bar overlap with the opengl view:
											 // @BUG(Xavier): For some reason this plays with VSYNC???
											 // | NSWindowStyleMaskFullSizeContentView 
											 backing:NSBackingStoreBuffered defer:NO];
		[window setDelegate:appDelegate];

		float title_bar_height = [window frame].size.height - [[window contentView] frame].size.height;

		// [window setMinSize:NSMakeSize(960, 540 + title_bar_height)];
		[window setTitle:@""];
		[window makeKeyAndOrderFront:nil];

		// [window setHasShadow:NO];

		// @NOTE(Xavier): When the title bar is set to overlap the opengl view
		// this should be set so that you can no longer see the title bar, only its buttons.
		// window.titlebarAppearsTransparent = true;
		// @NOTE(Xavier): This hides the title text.
		// window.titleVisibility = NSWindowTitleHidden;

		NSView* cv = [window contentView];
		[cv setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		[cv setAutoresizesSubviews:YES];
		OpenGLView* GLView = [[OpenGLView alloc] init];
		[GLView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		[GLView setPixelFormat:PixelFormat];
		[GLView setWantsBestResolutionOpenGLSurface:YES];
		[GLView setOpenGLContext:GlobalGLContext];
		[GLView setFrame:[cv bounds]];
		[cv addSubview:GLView];
	    
	    [PixelFormat release];
		
		////////////////////////////
		// OpenGL setup with Cocoa
		[[GLView openGLContext] makeCurrentContext];
	    #if HANDMADE_USE_VSYNC_AND_DOUBLE_BUFFER
		    const GLint swapInt = 1;
		#else
		    const GLint swapInt = 0;
		#endif
		[[GLView openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
		[[GLView openGLContext] setView:[window contentView]];

		//////////////////////////////
		// Initialising the game:
		// resize_view( [GLView bounds].size.width, [GLView bounds].size.height, [GLView bounds].size.width, [GLView bounds].size.height );
		init_game();

		////////////////////
		// Run loop
		float scroll_value = 0;
		uint64_t startTime = mach_absolute_time();
		uint64_t deltaTime = 0;
		running = true;
		while ( running ) {

			////////////////////////////
			// Event Processing:
			NSEvent* Event;
			do {
				Event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];

				switch ( [Event type] ) {
					case NSEventTypeKeyDown: {
						unichar C = [[Event charactersIgnoringModifiers] characterAtIndex:0];
						int ModifierFlags = [Event modifierFlags];
						int CommandKeyFlag = ModifierFlags & NSEventModifierFlagCommand;
						int ControlKeyFlag = ModifierFlags & NSEventModifierFlagControl;
						int AlternateKeyFlag = ModifierFlags & NSEventModifierFlagOption;
						int KeyDownFlag = 1;
						// OSXKeyProcessing(KeyDownFlag, C, CommandKeyFlag, ControlKeyFlag, AlternateKeyFlag, GameData->NewInput, GameData);
						if ( C == 'q' && KeyDownFlag && CommandKeyFlag ) { running = false; }
						if ( C == 'w' && KeyDownFlag && CommandKeyFlag ) { running = false; }

						if ( C < 256 && C >= 0 ) down_keys[C] = true;

					} break;

					case NSEventTypeKeyUp: {
						unichar C = [[Event charactersIgnoringModifiers] characterAtIndex:0];
						int ModifierFlags = [Event modifierFlags];
						int CommandKeyFlag = ModifierFlags & NSEventModifierFlagCommand;
						int ControlKeyFlag = ModifierFlags & NSEventModifierFlagControl;
						int AlternateKeyFlag = ModifierFlags & NSEventModifierFlagOption;
						int KeyDownFlag = 0;
						// OSXKeyProcessing(KeyDownFlag, C, CommandKeyFlag, ControlKeyFlag, AlternateKeyFlag, GameData->NewInput, GameData);
						if ( C == 'f' && CommandKeyFlag && ControlKeyFlag  ) {
							// @TODO(Xavier): I am not compleetly sure how this works or if it 
							// is the way I want to go fullscreen.
							[window performSelector: @selector(toggleFullScreen:) withObject:window];
						}

						if ( C == 'g' ) {
							toggle_fullscreen();
						}

						if ( C < 256 && C >= 0 ) down_keys[C] = false;
					} break;

					case NSEventTypeScrollWheel: {
						// printf( "%f\n", [Event deltaY] );
						scroll_value = [Event scrollingDeltaY];
					} break;

					default: {
						[NSApp sendEvent:Event];
					} break;
				}
			} while (Event != nil);

			//////////////////////////////
			// Mouse Position Logic:
			CGPoint MouseLocationInScreen = [NSEvent mouseLocation];
			NSRect RectInWindow = [window convertRectFromScreen:NSMakeRect(MouseLocationInScreen.x, MouseLocationInScreen.y, 1, 1)];
			NSPoint PointInWindow = RectInWindow.origin;
			NSPoint MouseLocationInView = [GLView convertPoint:PointInWindow fromView:nil];
			BOOL MouseInWindowFlag = NSPointInRect(MouseLocationInScreen, [window frame]);

			/////////////////////////
			// @NOTE(Xavier): This is the value of the mouse button currently pressed:
			unsigned int MouseButtonMask = [NSEvent pressedMouseButtons];

			// @NOTE(Xavier) This is the size of the opengl frame.
			// It may be better to assertain it using:
			// NSRect bounds = [self convertRectToBacking:[self bounds]];
			// This is found in the reshape method for the opengl view.
			CGRect ContentViewFrame = [[window contentView] frame];

			///////////////////////
			// Game Input:
			set_mouse_position( MouseLocationInView.x, MouseLocationInView.y );
			set_mouse_state( MouseButtonMask );
			set_mouse_scroll_value( scroll_value );
			input_game();

			///////////////////
			// Game Update:
			update_game();

			/////////////////////////////
			// @TODO(Xavier): process logic and render here...
			[GlobalGLContext makeCurrentContext];
			
			/////////////////
			// Game Render:
			render_game();

		    // @NOTE(Xavier) Currently (30.6.17) I am unable to disable vsync
		    // and the refresh rate is locked at 60Hz reguardless of
		    // whether this is enabled or not.
			#if HANDMADE_USE_VSYNC_AND_DOUBLE_BUFFER
				[GlobalGLContext flushBuffer];
			#else
				glFlush();
			#endif

			// @NOTE(Xavier): This is what I am using for calculating the frame time.
			uint64_t endTime = mach_absolute_time();
			uint64_t elapsedTime = endTime - startTime;
			startTime = mach_absolute_time();
			mach_timebase_info_data_t info;
			if (mach_timebase_info (&info) != KERN_SUCCESS) { printf ("mach_timebase_info failed\n"); }
			uint64_t nanosecs = elapsedTime * info.numer / info.denom;
			uint64_t millisecs = nanosecs / 1000000;

			// @TEMPOARY(Xavier): This is just to see what the frame timing is.
			// [window setTitle:[@((float)nanosecs/1000000) stringValue]];
			// [window setTitle:dir];
		}
	}

	// [NSCursor unhide];

	printf("Handmade Cocoa Exited.\n");
}

