#ifndef _WINDOWS_PLATFORM_H_
#define _WINDOWS_PLATFORM_H_

#include <stdio.h>
#include "file_io.h"

#if !defined(_GAMING_XBOX)
    #include "shellapi.h"
#endif

void OpenUrl(const char* url) {
#if !defined(_GAMING_XBOX)
    ShellExecute(0, "open", url, 0, 0, SW_SHOWNORMAL);
#endif
}

double SecondsPerCount() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    double secondsPerCount = 1.0 / (double)frequency.QuadPart;
    return secondsPerCount;
}

double CurrentTimeInSeconds() {
    LARGE_INTEGER now, frequency;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&frequency);
    return (double)now.QuadPart / (double)frequency.QuadPart;
}

s64 CurrentTimeInMilliseconds() {
    LARGE_INTEGER now, frequency;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&frequency);

    return (s64)((now.QuadPart * 1000) / frequency.QuadPart);
}

s64 CurrentTimeCount() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (s64)now.QuadPart;
}

// NOTE(roger): length of -1 means its zero-terminated.
char* WideToUTF8(WCHAR* data, int length, Allocator allocator) {
    // Query size of wide string.
    int requiredSize = WideCharToMultiByte(
        CP_UTF8,                // Convert to UTF-8
        0,                      // No special flags
        data, length,           // input                 
        0, 0,                   // Output buffer (data and size)
        0, 0                    // No default char replacement
    );

    if (requiredSize <= 0) {
        return 0;
    }
    
    if (length != -1) {
        requiredSize += 1; //Make room for zero terminator
    }
    
    char* utf8 = (char*)allocator.alloc(requiredSize);
    int result = WideCharToMultiByte(CP_UTF8, 0,
                                     data, length,
                                     utf8, requiredSize,
                                     0, 0);
    
    if (result <= 0) {
        return 0;
    }
    ASSERT_ERROR(result <= requiredSize, "Exceeded required size!");

    if (length != -1) {
        utf8[result] = 0;            
    }
    
    return utf8;
}

// TODO(roger): [XBOX] Does this need to change for _GAMING_XBOX? Probably.
bool GetAppDataFolder(char* directory) {
    char* appdata = getenv("APPDATA");
    if (!appdata) {
        return false;
    }
    
    u32 i = 0;
    while (appdata[i] != '\0') {
        directory[i] = appdata[i];
        i++;
    }
    directory[i] = '\0';
    
    OverwritePathSeparators(directory);
    return true;
}

bool DirectoryExists(const char* directory) {
    DWORD attribute = GetFileAttributes(directory);
    return (attribute != INVALID_FILE_ATTRIBUTES) && (attribute & FILE_ATTRIBUTE_DIRECTORY);
}

void GetAssetDirectory(String* directory) {
    // Get the current path of the EXE
    DWORD n = GetModuleFileNameA(0, directory->buffer, MAX_PATH_LENGTH);
    TruncateLastDirectory(directory->buffer, n);
    
    // Check if the 'data' folder is in the same directory as the EXE
    char dataPath[MAX_PATH_LENGTH];
    sprintf(dataPath, "%s/%s", directory->buffer, "data");

    // If not pick one directory up.
    if (!DirectoryExists(dataPath)) {
        TruncateLastDirectory(directory->buffer, n);
        sprintf(dataPath, "%s/%s", directory->buffer, "data");
    }

    memcpy(directory->buffer, dataPath, MAX_PATH_LENGTH);
    OverwritePathSeparators(directory->buffer);
    directory->length = CStringLength(directory->buffer);

    ASSERT_ERROR(DirectoryExists(directory->buffer), "Failed to locate asset folder.");
}

// NOTE(roger): Called MyCreateDirectory instead of CreateDirectory due to win32 having the same function name.
void MyCreateDirectory(const char* directoryPath) {
    char extracted[MAX_PATH_LENGTH];
    
    const char* current = directoryPath;
    while (current[0] != 0) {
        if (current[0] == '\\' || current[0] == '/') {
            u32 length = current - directoryPath;
            memcpy(extracted, directoryPath, length);
            extracted[length] = 0;
            
            CreateDirectory(extracted, 0);
            int error = GetLastError();
            if (error == ERROR_PATH_NOT_FOUND) {
                ASSERT_DEBUG(false, "Failed to create directory for: %s", directoryPath);
            }
        } 
        current++;
    }
}

bool RemoveFile(const char* filePath) {
    return remove(filePath) == 0;
}

File FileOpen(const char* filePath, FileMode fileMode) {
    File file = {0};
    
    DWORD access = 0;
    switch (fileMode) {
        case FileMode_Write:      access = GENERIC_WRITE; break;
        case FileMode_Read:       access = GENERIC_READ; break;
        case FileMode_WriteRead:  access = GENERIC_READ | GENERIC_WRITE; break;
        default: ASSERT_DEBUG(false, "Invalid file mode.");
    }
    
    file.handle = (FileHandle)CreateFileA(
        filePath,
        access,
        FILE_SHARE_WRITE,
        0,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        0
    );
    
    ASSERT_DEBUG(file.handle != INVALID_HANDLE_VALUE, "Error opening file: %s\n", filePath);

    SetFilePointer((HANDLE)file.handle, 0, 0, FILE_BEGIN);
    file.mode = fileMode;
    if (fileMode == FileMode_Write || fileMode == FileMode_WriteRead) { 
        //ALLOC(roger)
        file.writeBuffer = (u8*)malloc(WriteBufferSize);
    }
    return file;
}

void FileClose(File& file) {
    if (file.mode == FileMode_Write || file.mode == FileMode_WriteRead) {
        if (file.bufferPosition != 0) {
            u32 bytesWritten = 0;
            BOOL success = WriteFile((HANDLE)file.handle, file.writeBuffer, file.bufferPosition, (LPDWORD)&bytesWritten, 0);
            DWORD error = GetLastError();
            ASSERT_ERROR(success != 0, "Failed to write fileContents.");
            file.bufferPosition = 0;
        }
        free(file.writeBuffer);

        SetEndOfFile((HANDLE)file.handle);
    }

    CloseHandle((HANDLE)file.handle);
}

u64 FileWrite(File& file, void* buffer, u64 size) {
    if (file.mode != FileMode_Write && file.mode != FileMode_WriteRead) {
        ASSERT_DEBUG(false, "Cannot write to a file opened in Read mode.");
        return 0;
    }
    u64 totalWritten = 0;
    u32 bytesWritten = 0;
    
    BOOL success = true;
    
    while (size >= 0xFFFFFFFF) {
        if (file.bufferPosition != 0) {
            success = WriteFile((HANDLE)file.handle, file.writeBuffer, file.bufferPosition, (LPDWORD)&bytesWritten, 0);
            file.bufferPosition = 0;
        }

        success = WriteFile((HANDLE)file.handle, buffer, 0xFFFFFFFF, (LPDWORD)&bytesWritten, 0);

        buffer = (u8*)buffer + bytesWritten;
        size -= bytesWritten;
        totalWritten += bytesWritten;
    }

    while (size >= (1 << 24)) {
        if (file.bufferPosition != 0) {
            success = WriteFile((HANDLE)file.handle, file.writeBuffer, file.bufferPosition, (LPDWORD)&bytesWritten, 0);
            file.bufferPosition = 0;
        }
        
        success = WriteFile((HANDLE)file.handle, buffer, (1 << 24), (LPDWORD)&bytesWritten, 0);
        buffer = (u8*)buffer + bytesWritten;
        size -= bytesWritten;
        totalWritten += bytesWritten;
    }

    if (file.bufferPosition + size >= (1 << 24)) {
        success = WriteFile((HANDLE)file.handle, file.writeBuffer, file.bufferPosition, (LPDWORD)&bytesWritten, 0);
        file.bufferPosition = 0;
    }

    if (size > 0) {
        memcpy(file.writeBuffer + file.bufferPosition, buffer, size);
        file.bufferPosition += size;
        totalWritten += size;
    }
    
    ASSERT_ERROR(success != 0, "Failed to write fileContents.");

    return totalWritten;
}

#endif //_WINDOWS_PLATFORM_H_