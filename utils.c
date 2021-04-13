#include "utils.h"

BOOL IsElevated() {
    BOOL ret = FALSE;
    HANDLE tokenHandle = NULL;
    BOOL result;
    result = OpenProcessToken(GetCurrentProcess(),
                              TOKEN_QUERY,
                              &tokenHandle);

    if(result == TRUE) {
        TOKEN_ELEVATION Elevation;
        DWORD returnLength;
        result = GetTokenInformation(tokenHandle,
                                     TokenElevation,
                                     &Elevation,
                                     sizeof(TOKEN_ELEVATION),
                                     &returnLength);
        if(result == TRUE) {
        	if(Elevation.TokenIsElevated != 0){
            	ret = TRUE;
        	}else{
        		ret = FALSE;
        	}
        }
    }
    if(tokenHandle) {
        CloseHandle(tokenHandle);
    }
    return(ret);
}