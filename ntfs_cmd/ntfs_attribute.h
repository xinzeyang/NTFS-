
#ifndef __NTFS_ATTTRIBUTE__
#define __NTFS_ATTTRIBUTE__


PATTRIBUTE_RECORD_HEADER 
ntfs_attri_lookup_in_file_record( PFILE_RECORD_HEADER p_file, BYTE n_attri_type );

BOOL ntfs_attri_add_resident( PFILE_RECORD_HEADER p_mft, \
                              ULONG type_code, WCHAR* sz_name, PADD_CONTEXT p_add_context );

#endif //__NTFS_ATTTRIBUTE__