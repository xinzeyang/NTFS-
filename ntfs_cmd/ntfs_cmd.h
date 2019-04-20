


#define MAX_CMD_LEN          256
#define NTFS_CMD_DIR         "dir"
#define NTFS_CMD_CD          "cd"
#define NTFS_CMD_MKDIR       "mkdir"
#define NTFS_CMD_MKFILE      "mkfile"
#define NTFS_CMD_HELP        "help"


void create_new_file( PNTFS_CONTEXT p_context, WCHAR* sz_file_name );
void ntfs_init_context( PNTFS_CONTEXT p_ntfs_context );