
#ifndef __NTFS__MFT__
#define __NTFS__MFT__

void ntfs_mft_init( PFILE_RECORD_HEADER p_mft, ULONG mft_number );
DWORD ntfs_get_free_mft_record( PFILE_RECORD_HEADER p_mft, PNTFS_CONTEXT p_context );
void ntfs_resize_mft_header( PFILE_RECORD_HEADER p_mft );
#endif