
/* ! \file ntfs_mft.cpp
 *  \brief 这里面包含了ntfs对mft的操作...
 */

#include <stdio.h>
#include <windows.h>
#include "ntfs.h"
#include "ntfs_attribute.h"
#include "ntfs_bitmap.h"

/** \fn 
	\brief 
    步骤:
         1, 我们先得到第一个mft的大小
         2, 得出现在已经使用过的mft的个数(包括以用的和未用的)
         3, 查询bitmap中已经未使用的
         4, 如果找到就是用这个,没有就进行5
         5, 修改mft的大小,接着向后扩展

	\param 
	\return
*/

DWORD ntfs_get_free_mft_record( PFILE_RECORD_HEADER p_mft, PNTFS_CONTEXT p_context )
{
    
    ULONGLONG u_mft_size  = 0;
    ULONG     u_mft_count = 0;
    HANDLE    h_file = p_context->h_device; 

    PATTRIBUTE_RECORD_HEADER p_attri_data   = NULL;
    PATTRIBUTE_RECORD_HEADER p_attri_bitmap = NULL;

    /*
     *  首先我们得到首个mft的80属性
     */
    
    p_attri_data = ntfs_attri_lookup_in_file_record( p_mft, $DATA );
    
    if ( p_attri_data == NULL )
    {
        printf( "can't find data attribute!\n" );
        return 0;
    }
    
    /*
     *  $DATA --> 80 肯定是no resident
     */
    
    if ( p_attri_data->FormCode != 1 )
    {
        return 0;
    }
    
    /*
     *  我们得到mft的分配的大小
     */

    u_mft_size = p_attri_data->Form.Nonresident.AllocatedLength;
    
    /*
     *  计算里面包含了多少个mft?,发现位图属性中的allocate size也能表示包含了多少个mft
     */

    u_mft_count = (ULONG)( u_mft_size / NTFS_MFT_FILE_SIZE );

    
    /*
     *  我们我们再找到位图
     */
    
    p_attri_bitmap = ntfs_attri_lookup_in_file_record( p_mft, $BITMAP );

    if ( p_attri_bitmap == NULL )
    {
        printf( "can't find bitmap attribute!\n" );
        return 0;
    }

    /*
     *  我们根据run,找到位图的数据
     */

    PBYTE   pRun = (PBYTE)p_attri_bitmap + \
                            p_attri_bitmap->Form.Nonresident.MappingPairsOffset;
    
    ULONGLONG               nCount = 0;
    ULONGLONG               Lcv = 0;
    LARGE_INTEGER           DataOffset;
    PBYTE p_bitmap_data;

    for ( pRun; *pRun != 0; pRun += RunLength( pRun ) )
    {
        Lcv += RunLCN( pRun ) ;
        DataOffset.QuadPart = Lcv * 512;
        nCount = RunCount( pRun );
        
        /*
         *  我们到先读取位图数据
         */
        
        DWORD dw_size = (DWORD)p_attri_bitmap->Form.Nonresident.AllocatedLength;
        DWORD dw_read = 0;
        
        p_bitmap_data = new BYTE[dw_size];
        SetFilePointerEx( h_file, DataOffset, NULL, FILE_BEGIN );
        ReadFile( h_file, p_bitmap_data, dw_size, &dw_read, NULL );
      
        /*
         *  前24位是系统的我们不管
         */
        
        PBYTE p_datda = p_bitmap_data + 3;
        
        /*
         *  接下来就是开始按位读取没用到的位
         */

        int n_free_bit = ntfs_get_free_bit_and_set( p_datda, (u_mft_count - 24) / 8 );
        
        
        /*
         *  我们将设置过多位图写入到磁盘
         */
        
        SetFilePointerEx( h_file, DataOffset, NULL, FILE_BEGIN );
        WriteFile( h_file, p_bitmap_data, dw_size, &dw_read, NULL );

        /*
         *  加上系统的24个
         */

         n_free_bit += 24;
        
        /*
         *  也就是说现在第n_free_bit号mft是空的我们可以使用,那我们返回他的地址
         */

        delete p_bitmap_data;
        
        return  n_free_bit;
        
        
    }
    
    return 0;
    
}


/** \fn void ntfs_mft_init( PFILE_RECORD_HEADER p_mft )
	\brief 初始化mft file record,这里我们要进行的工作就是设置第一个attribute的type为0xffffffff
           填充PFILE_RECORD_HEADER的一些值
	\param p_mft 要填充的mft file record
	\return none
*/

void ntfs_mft_init( PFILE_RECORD_HEADER p_mft, ULONG mft_number )
{
    
    /*
     *  填充一些常用的结构体,这里我们定死就给usa分配8个字节
     */
    
    p_mft->Ntfs.Type      = 0x454C4946;
    p_mft->Ntfs.UsaOffset = 0x30;
    p_mft->Ntfs.UsaCount  = 0x03;
    p_mft->Ntfs.Lsn       = 0;
    p_mft->SequenceNumber = 0x01;
    p_mft->LinkCount      = 0x01;
    p_mft->AttributeOffset = sizeof( FILE_RECORD_HEADER ) + 8;
    p_mft->Flags          = 0x01;
    p_mft->BytesInUse     = p_mft->AttributeOffset;
    p_mft->BytesAllocated = NTFS_MFT_FILE_SIZE;
    p_mft->BaseFileRecord = 0;
    p_mft->NextAttributeNumber = 0;
    p_mft->Pading = 0;
    p_mft->MFTRecordNumber = mft_number;
    
    /*
     *  usa设为1
     */
    
    *(DWORD*)((char*)p_mft + sizeof( FILE_RECORD_HEADER ) ) = 1;

    /*
     *  设置第一个属性的typecode为末尾即0xffffffff
     */
    
    PATTRIBUTE_RECORD_HEADER p_attri = (PATTRIBUTE_RECORD_HEADER)   \
                                       ( (char*)p_mft + p_mft->AttributeOffset);
    p_attri->TypeCode = $END;
}

void ntfs_resize_mft_header( PFILE_RECORD_HEADER p_mft )
{
    ULONG u_size = p_mft->BytesInUse;;
    PATTRIBUTE_RECORD_HEADER     pAttribute = (PATTRIBUTE_RECORD_HEADER)\
        ( (DWORD)p_mft + (DWORD)(p_mft->AttributeOffset));
    while ( pAttribute->TypeCode != $END )
    {
        u_size += pAttribute->RecordLength;
        pAttribute = (PATTRIBUTE_RECORD_HEADER)( (DWORD)pAttribute + \
            pAttribute->RecordLength );
    }
    u_size += 8;
    p_mft->BytesInUse = u_size;
}