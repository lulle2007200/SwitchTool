#ifndef MESSAGES_H
#define MESSAGES_H

#include <Windows.h>

#define MSG_ERR_NOT_ELEVATED TEXT("This application requires admin privileges.\n")
#define MSG_H_SEPERATOR TEXT("----------------------------------------------\n")
#define MSG_MORE_INFO TEXT("More information on https://github.com/lulle2007200/SwitchTool\n")
#define MSG_NO_DEVICES TEXT("No storage devices found.")

#define MSG_REFRESH_DEVICE_LIST TEXT("Refresh device list")
#define MSG_QUIT TEXT("Quit")
#define MSG_CONTINUE TEXT("Continue")
#define MSG_BACK TEXT("Back")

#define MSG_SELECT_OPTION TEXT("Select option: ")
#define MSG_INVALID_OPTION TEXT("Invalid option. Please select one of the above options. ")
#define MSG_NO_HEKATE_DEVICES TEXT("No heaktae UMS device found.")
#define MSG_SHOW_ALL TEXT("List other storage devices (careful, be sure to select the right device)")
#define MSG_SELECT_DEVICE TEXT("Select storage device.")
#define MSG_MULTIPLE_HEKATE_DEVICES TEXT("Found multiple hekate UMS devices, select device.")
#define MSG_SINGLE_HEKATE_DEVICE TEXT("Found %s.")
#define MSG_SELECT_ACTION TEXT("Select option for %s.")
#define MSG_RESTORE_FROM_FILE TEXT("Restore from file")

#define MSG_BACKUP_TO_FILE TEXT("Backup to file")




#define MSG_BOOT0_BACKUP_FILENAME TEXT("BOOT0.img")
#define MSG_BOOT1_BACKUP_FILENAME TEXT("BOOT1.img")

#endif