
/* ! \file ntfs_mft.cpp
 *  \brief �����������ntfs��mft�Ĳ���...
 */

#include <stdio.h>
#include <windows.h>
#include "ntfs.h"
#include "ntfs_attribute.h"
#include "ntfs_bitmap.h"

/** \fn 
	\brief 
    ����:
         1, �����ȵõ���һ��mft�Ĵ�С
         2, �ó������Ѿ�ʹ�ù���mft�ĸ���(�������õĺ�δ�õ�)
         3, ��ѯbitmap���Ѿ�δʹ�õ�
         4, ����ҵ����������,û�оͽ���5
         5, �޸�mft�Ĵ�С,���������չ

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
     *  �������ǵõ��׸�mft��80����
     */
    
    p_attri_data = ntfs_attri_lookup_in_file_record( p_mft, $DATA );
    
    if ( p_attri_data == NULL )
    {
        printf( "can't find data attribute!\n" );
        return 0;
    }
    
    /*
     *  $DATA --> 80 �϶���no resident
     */
    
    if ( p_attri_data->FormCode != 1 )
    {
        return 0;
    }
    
    /*
     *  ���ǵõ�mft�ķ���Ĵ�С
     */

    u_mft_size = p_attri_data->Form.Nonresident.AllocatedLength;
    
    /*
     *  ������������˶��ٸ�mft?,����λͼ�����е�allocate sizeҲ�ܱ�ʾ�����˶��ٸ�mft
     */

    u_mft_count = (ULONG)( u_mft_size / NTFS_MFT_FILE_SIZE );

    
    /*
     *  �����������ҵ�λͼ
     */
    
    p_attri_bitmap = ntfs_attri_lookup_in_file_record( p_mft, $BITMAP );

    if ( p_attri_bitmap == NULL )
    {
        printf( "can't find bitmap attribute!\n" );
        return 0;
    }

    /*
     *  ���Ǹ���run,�ҵ�λͼ������
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
         *  ���ǵ��ȶ�ȡλͼ����
         */
        
        DWORD dw_size = (DWORD)p_attri_bitmap->Form.Nonresident.AllocatedLength;
        DWORD dw_read = 0;
        
        p_bitmap_data = new BYTE[dw_size];
        SetFilePointerEx( h_file, DataOffset, NULL, FILE_BEGIN );
        ReadFile( h_file, p_bitmap_data, dw_size, &dw_read, NULL );
      
        /*
         *  ǰ24λ��ϵͳ�����ǲ���
         */
        
        PBYTE p_datda = p_bitmap_data + 3;
        
        /*
         *  ���������ǿ�ʼ��λ��ȡû�õ���λ
         */

        int n_free_bit = ntfs_get_free_bit_and_set( p_datda, (u_mft_count - 24) / 8 );
        
        
        /*
         *  ���ǽ����ù���λͼд�뵽����
         */
        
        SetFilePointerEx( h_file, DataOffset, NULL, FILE_BEGIN );
        WriteFile( h_file, p_bitmap_data, dw_size, &dw_read, NULL );

        /*
         *  ����ϵͳ��24��
         */

         n_free_bit += 24;
        
        /*
         *  Ҳ����˵���ڵ�n_free_bit��mft�ǿյ����ǿ���ʹ��,�����Ƿ������ĵ�ַ
         */

        delete p_bitmap_data;
        
        return  n_free_bit;
        
        
    }
    
    return 0;
    
}


/** \fn void ntfs_mft_init( PFILE_RECORD_HEADER p_mft )
	\brief ��ʼ��mft file record,��������Ҫ���еĹ����������õ�һ��attribute��typeΪ0xffffffff
           ���PFILE_RECORD_HEADER��һЩֵ
	\param p_mft Ҫ����mft file record
	\return none
*/

void ntfs_mft_init( PFILE_RECORD_HEADER p_mft, ULONG mft_number )
{
    
    /*
     *  ���һЩ���õĽṹ��,�������Ƕ����͸�usa����8���ֽ�
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
     *  usa��Ϊ1
     */
    
    *(DWORD*)((char*)p_mft + sizeof( FILE_RECORD_HEADER ) ) = 1;

    /*
     *  ���õ�һ�����Ե�typecodeΪĩβ��0xffffffff
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