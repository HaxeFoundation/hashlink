/*
 * Copyright (C)2005-2017 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef __APPLE__
#include <TargetConditionals.h>

#if TARGET_OS_IOS

#include <mobile/ios/hl_ios.h>
#include <sys/utsname.h>
#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>

static NSString* getNSResourcePath(const char* file)
{
	NSString *relPathNS = [[NSString alloc] initWithUTF8String:file];
	NSString *absPathNS = [[NSBundle mainBundle] pathForResource: [relPathNS stringByDeletingPathExtension] ofType: [relPathNS pathExtension]];
	return absPathNS;
}

static NSString* getNSDocumentPath(const char* file)
{
	NSString *relPathNS = [[NSString alloc] initWithUTF8String:file];
	NSString *absPathNS = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] stringByAppendingPathComponent:relPathNS];
	return absPathNS;
}

const char* hl_mobile_get_resource_path(const char* file)
{
	return [getNSResourcePath(file) UTF8String];
}

const char* hl_mobile_get_document_path(const char* file)
{
	return [getNSDocumentPath(file) fileSystemRepresentation];
}

bool hl_mobile_file_exists(const char* file)
{
	BOOL isDir;
	BOOL exists = NO;
	NSString *pathRes = getNSResourcePath(file);
	
	if (pathRes != nil)
	{
		exists = [[NSFileManager defaultManager] fileExistsAtPath:pathRes isDirectory:&isDir];
	}
	if (!exists)
	{
		NSString *pathDoc = [[NSString alloc] initWithUTF8String:hl_mobile_get_document_path(file)];
		exists = [[NSFileManager defaultManager] fileExistsAtPath:pathDoc isDirectory:&isDir];
	}
	return exists;
}

bool hl_mobile_create_directory(const char* dir, int mode)
{
	NSString *path = getNSDocumentPath(dir);
	NSError *error;
	BOOL ret = false;
	
  // Check if directory already exists
	if (![[NSFileManager defaultManager] fileExistsAtPath:path])    
	{
		if (!(ret = [[NSFileManager defaultManager] createDirectoryAtPath:path
									   withIntermediateDirectories:NO
														attributes:nil
															 error:&error]))
		{
			NSLog(@"hl_ios_create_directory error: %@", error);
		}
	}
	
	return ret;
}

const char* hl_ios_get_device_name()
{
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString* code = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
    return [code UTF8String];
}

int hl_ios_get_retina_scale_factor()
{
	return [[UIScreen mainScreen] scale];
}

#endif
#endif
