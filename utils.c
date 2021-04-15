#include "utils.h"

BOOL IsElevated(void){
    BOOL isElevated = FALSE;
    BOOL result;
    HANDLE tokenHandle;
    result = OpenProcessToken(GetCurrentProcess(),
                              TOKEN_QUERY,
                              &tokenHandle);
    if(result == FALSE){
        goto IsElevated_cleanup;
    }
    TOKEN_ELEVATION tokenElevation;
    DWORD bytesReturned;
    result = GetTokenInformation(tokenHandle,
                                 TokenElevation,
                                 &tokenElevation,
                                 sizeof(TOKEN_ELEVATION),
                                 &bytesReturned);
    if(result == TRUE){
        if(tokenElevation.TokenIsElevated != 0){
            isElevated = TRUE;
        }
    }
    IsElevated_cleanup:
    if(tokenHandle != INVALID_HANDLE_VALUE){
        CloseHandle(tokenHandle);
    }
    return(isElevated);
}

HANDLE CreateDummyFile(LPTSTR filePath, LONGLONG size){
    DWORD result;
    HANDLE fileHandle = CreateFile(filePath,
                                   GENERIC_WRITE | GENERIC_READ,
                                   0,
                                   NULL,
                                   CREATE_NEW,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
    if(fileHandle == INVALID_HANDLE_VALUE){
        goto CreateDummyFile_error_cleanup;
    }
    LONG distanceHigh = (LONG)(size >> 0x20);
    LONG distanceLow = (LONG)(size &0xffffffff);
    result = SetFilePointer(fileHandle,
                            distanceLow,
                            &distanceHigh,
                            FILE_BEGIN);
    if(result == INVALID_SET_FILE_POINTER){
        goto CreateDummyFile_error_cleanup;
    }
    result = SetEndOfFile(fileHandle);
    if(result == FALSE){
        goto CreateDummyFile_error_cleanup;
    }
    DWORD bytesWritten;
    result = WriteFile(fileHandle,
                       NULL,
                       0,
                       &bytesWritten,
                       NULL);
    if(result == FALSE){
        goto CreateDummyFile_error_cleanup;
    }
    result = SetFilePointer(fileHandle,
                            0,
                            NULL,
                            FILE_BEGIN);
    if(result == INVALID_SET_FILE_POINTER){
        goto CreateDummyFile_error_cleanup;
    }
    return(fileHandle);
    CreateDummyFile_error_cleanup:
    result = GetLastError();
    if(fileHandle != INVALID_HANDLE_VALUE){
        CloseHandle(fileHandle);
        DeleteFile(filePath); //TODO: deletefile may file
    }
    SetLastError(result);
    return(INVALID_HANDLE_VALUE);
}