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

#if __ANDROID__

#include <mobile/android/jni/hl_android.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>

#include <SDL_system.h>
#include <sys/stat.h>

// TODO use SDL_RWFromFile with SDL Android Hints to use OBB files
const char* hl_mobile_get_resource_path(const char* file)
{
	const char* path = SDL_AndroidGetExternalStoragePath();
	char* full_path = (char*)malloc(sizeof(char)*(strlen(path)+1+strlen(file)+1));
	strcpy(full_path, path);
	strcat(full_path, "/");
	strcat(full_path, file);
	return full_path;
}

const char* hl_mobile_get_document_path(const char* file)
{
	const char* path = SDL_AndroidGetInternalStoragePath();
	char* full_path = (char*)malloc(sizeof(char)*(strlen(path)+1+strlen(file)+1));
	strcpy(full_path, path);
	strcat(full_path, "/");
	strcat(full_path, file);
	return full_path;
}

bool hl_mobile_file_exists(const char* file)
{
	struct stat buffer;
	bool result1, result2;
	const char* path = SDL_AndroidGetInternalStoragePath();
	char* full_path = (char*)malloc(sizeof(char)*(strlen(path)+1+strlen(file)+1));
	strcpy(full_path, path);
	strcat(full_path, "/");
	strcat(full_path, file);
	result1 = (stat (hl_mobile_get_document_path(file), &buffer) == 0);
	result2 = (stat (hl_mobile_get_resource_path(file), &buffer) == 0);
	return result1 || result2;
}

bool hl_mobile_create_directory(const char* dir, int mode)
{
	struct stat buffer;
	bool result;
	const char* path = SDL_AndroidGetInternalStoragePath();
	char* full_path = (char*)malloc(sizeof(char)*(strlen(path)+1+strlen(dir)+1));
	strcpy(full_path, path);
	strcat(full_path, "/");
	strcat(full_path, dir);
	return mkdir(full_path, mode);
}

#endif
