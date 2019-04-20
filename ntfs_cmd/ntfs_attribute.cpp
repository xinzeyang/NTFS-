

/*! \file ntfs_attribute.cpp
 *  \brief 
 */

#include <stdio.h>
#include <windows.h>
#include "ntfs.h"

/** \fn 
	\brief 
	\param 
	\return
*/

PATTRIBUTE_RECORD_HEADER 
ntfs_attri_lookup_in_file_record( PFILE_RECORD_HEADER p_file, BYTE n_attri_type )
{
    
    /*
     *  first attribute
     */
    
    PATTRIBUTE_RECORD_HEADER     pAttribute = (PATTRIBUTE_RECORD_HEADER)\
                                        ( (DWORD)p_file + (DWORD)(p_file->AttributeOffset));
    
    /*
     *  lookup the attribute we want
     */
    
    while ( pAttribute->TypeCode != $END )
    {
        if ( pAttribute->TypeCode == 0 || pAttribute->TypeCode == 0xff )
        {
            return NULL;
        }
        if ( pAttribute->TypeCode == n_attri_type )
        {
            return pAttribute;
        }
        pAttribute = (PATTRIBUTE_RECORD_HEADER)( (DWORD)pAttribute + \
            pAttribute->RecordLength );

    }
    return NULL;
}

PATTRIBUTE_RECORD_HEADER 
ntfs_attri_find_end( PFILE_RECORD_HEADER p_file )
{
    
    /*
     *  first attribute
     */
    
    PATTRIBUTE_RECORD_HEADER     pAttribute = (PATTRIBUTE_RECORD_HEADER)\
                                        ( (DWORD)p_file + (DWORD)(p_file->AttributeOffset));
    
    /*
     *  lookup the attribute we want
     */
    
    while ( TRUE )
    {   
        
        /*
         *  加上这句避免死循环
         */
        
        if ( ((DWORD)pAttribute - (DWORD)p_file) >= 1024 )
        {
            return NULL;
        }
        if ( pAttribute->TypeCode == $END )
        {
            return pAttribute;
        }
        pAttribute = (PATTRIBUTE_RECORD_HEADER)( (DWORD)pAttribute + \
            pAttribute->RecordLength );

    }
    return NULL;
}

/** \fn 
	\brief 
	\param 
	\return
*/

BOOL ntfs_attri_add_resident( PFILE_RECORD_HEADER p_mft, \
                              ULONG type_code, WCHAR* sz_name, PADD_CONTEXT p_add_context )
{
    BOOL b_have_name = ( sz_name == NULL ) ? 0 : 1;
    
    /*
     *  查找这个属性
     */
    
    if( ntfs_attri_lookup_in_file_record( p_mft, (BYTE)type_code ) != NULL )
    {
        printf( "attribute already existing\n" );
        return FALSE;
    }
    
    /*
     *  找到属性尾
     */
    
    PATTRIBUTE_RECORD_HEADER p_add = ntfs_attri_find_end( p_mft );

    if ( p_add == NULL )
    {
        printf( "can't find the end of attribute!\n" );
        return FALSE;
    }
    

    /*
     *  我们填充这个属性头
     */
    
    p_add->TypeCode         = type_code;
    if ( b_have_name )
       p_add->RecordLength = (USHORT)(SIZEOF_RESIDENT_ATTRIBUTE_HEADER + sizeof( FILE_NAME ) + wcslen(sz_name)*2) - 2;
    else
       p_add->RecordLength = SIZEOF_RESIDENT_ATTRIBUTE_HEADER + SIZEOF_STANDERD_INFORMATION;
    p_add->FormCode = 0;
    p_add->NameLength = 0;
    p_add->NameOffset = 0;
    p_add->Flags = 0;
    p_add->Form.Resident.ValueOffset = SIZEOF_RESIDENT_ATTRIBUTE_HEADER;
    p_add->Instance = 0;
    
    /*
     *  p_add->Form.Resident.ValueLength表示的常驻属性的长度也就是body的长度
     */

    if ( b_have_name )
        p_add->Form.Resident.ValueLength = sizeof( FILE_NAME ) +  wcslen(sz_name)*2 - 2;
    else
        p_add->Form.Resident.ValueLength = SIZEOF_STANDERD_INFORMATION;
    
    /*
     *  接着填充standerd information
     */
    
    if ( type_code == $STANDARD_INFORMATION )
    {
        PSTANDARD_INFORMATION p_std_info = (PSTANDARD_INFORMATION)((char*)p_add + \
                                            SIZEOF_RESIDENT_ATTRIBUTE_HEADER );
        p_std_info->CreationTime = 0x01ca50a2c98f0d80;
        p_std_info->LastAccessTime = 0x01ca50a2c98f0d80;
        p_std_info->LastModificationTime = 0x01ca50a2c98f0d80;
        p_std_info->LastChangeTime = 0x01ca50a2c98f0d80;
        p_std_info->FileAttributes = FAT_DIRENT_ATTR_ARCHIVE;
        p_std_info->MaximumVersions = 0;
        p_std_info->VersionNumber   = 0;
        p_std_info->ClassId = 0;
        p_std_info->OwnerId = 0;
        p_std_info->SecurityId = 0x102;
        p_std_info->QuotaCharged = 0;
        p_std_info->Usn = 0;
    }
    else if ( type_code == $FILE_NAME )
    {
        p_add->Form.Resident.ResidentFlags = 0x01;
        PFILE_NAME p_name = (PFILE_NAME)((char*)p_add + \
                            SIZEOF_RESIDENT_ATTRIBUTE_HEADER );
        p_name->ParentDirectory = 0x0005000000000005;
        p_name->Info.CreationTime = 0x01ca50a2c98f0d80;
        p_name->Info.LastAccessTime = 0x01ca50a2c98f0d80;
        p_name->Info.LastModificationTime = 0x01ca50a2c98f0d80;
        p_name->Info.LastChangeTime = 0x01ca50a2c98f0d80;
        p_name->Info.AllocatedLength = 0;
        p_name->Info.FileSize = 0;
        p_name->Info.FileAttributes = FAT_DIRENT_ATTR_ARCHIVE;
        p_name->Info.PackedEaSize = 0;
        p_name->FileNameLength = wcslen( sz_name );
        p_name->Flags = FILE_NAME_NTFS;
        
        wcscpy( p_name->FileName, sz_name );
        
        /*
         *  将filename拷贝到context中
         */

        memcpy( &p_add_context->file_name, p_name, \
                 p_add->Form.Resident.ValueLength );
        

    }
    if ( type_code == $DATA )
    {
        p_add->RecordLength = SIZEOF_RESIDENT_ATTRIBUTE_HEADER;
        p_add->Form.Resident.ValueLength = 0;
        p_add->NameOffset = 0x18;
    }
    
    
    /*
     *  我们再设置属性结尾
     */
    
    PATTRIBUTE_RECORD_HEADER p_end = (PATTRIBUTE_RECORD_HEADER)( (char*)p_add + p_add->RecordLength );
    p_end->TypeCode = $END;

    return TRUE;
}
