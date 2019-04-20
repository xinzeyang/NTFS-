
/*! \file ntfs_index.cpp
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
void ntfs_index_add_entry( PINDEX_ALLOCATION_BUFFER p_index, \
                           PADD_CONTEXT p_add_context )
{
    
    /*
     *  首先我们找到first entry
     */

    PINDEX_HEADER p_index_hdr = &p_index->IndexHeader;
    PINDEX_ENTRY  p_first_entry = NtfsFirstIndexEntry( p_index_hdr );
    
    /*
     *  我们再找到indexentry的末尾
     */
    
    PINDEX_ENTRY p_temp = p_first_entry;

    while( TRUE )
    {
        if ( p_temp->Flags == INDEX_ENTRY_END )
        {
            break;
        }
        p_temp = NtfsNextIndexEntry( p_temp );
    }
    
    /*
     *  我们新建一个entry,然后写进磁盘
     */

    PINDEX_ENTRY p_entry_add = p_temp;
    
    /*
     *  开始填充结构体
     */
    
    USHORT u_name_len = ((PFILE_NAME)(&p_add_context->file_name))->FileNameLength;

    p_entry_add->Form.FileReference.SegmentNumberLowPart = p_add_context->mft_no;
    p_entry_add->Form.FileReference.SegmentNumberHighPart = 0;
    p_entry_add->Form.FileReference.SequenceNumber = 0x01;
    p_entry_add->Length = NTFS_INDEX_LENGTH_EXCEPTE_NAME + u_name_len * 2 - 4;
    p_entry_add->AttributeLength = sizeof( FILE_NAME ) + u_name_len * 2 - 2;
    p_entry_add->Flags = 0;
    
    
    /*
     *  filename
     */
    
    memcpy( &p_entry_add->FileName, p_add_context->file_name, \
            u_name_len * 2 + sizeof( FILE_NAME ) );

    /*
     *  其他为0就可以
     */
    
    
    /*
     *  再将下一个的flag设为INDEX_ENTRY_END
     */

    PINDEX_ENTRY p_entry_add_end = NtfsNextIndexEntry( p_entry_add );
    
    
    /*
     *  再设置下总长度
     */
    
    p_index->IndexHeader.FirstFreeByte += p_entry_add->Length;

    p_entry_add_end->Length = 0x10;
    p_entry_add_end->Flags = INDEX_ENTRY_END;

    
    
}
