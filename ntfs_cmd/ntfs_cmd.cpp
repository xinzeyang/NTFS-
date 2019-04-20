

/*! \file ntfs_cmd.cpp
 *  \brief
 */

#include <stdio.h>
#include "ntfs.h"
#include "ntfs_cmd.h"
#include "ntfs_mft.h"
#include "ntfs_attribute.h"
#include "ntfs_bitmap.h"
#include "ntfs_index.h"

#pragma warning ( disable : 4101 )

 /** \var
	 \brief
 */

int main()
{
	NTFS_CONTEXT ntfs_context;
	//     char sz_cmd[MAX_CMD_LEN] = {0};
	// 
	//     printf( "C:>" );
	//     scanf( "%s", sz_cmd );    
	//     parse_cmd( sz_cmd ); 

		/*
		 *  初始化
		 */

	ntfs_init_context(&ntfs_context);
	/*WCHAR *p = (WCHAR*)L"777.txt";*/
	create_new_file(&ntfs_context,(WCHAR*)L"777.txt");
	//printf( "%x\n", SIZEOF_RESIDENT_ATTRIBUTE_HEADER );
	Sleep(5000);
	return 0;

}

void create_new_file(PNTFS_CONTEXT p_context, WCHAR* sz_file_name)
{
	
	HANDLE h_file = p_context->h_device;
	ADD_CONTEXT  add_context;

	memset(&add_context, 0, sizeof(ADD_CONTEXT));

	/*
	 *  首先我们读取第一个mft
	 */
	printf("首先我们读取第一个mft\n");
	PBYTE p_mft = new BYTE[NTFS_MFT_FILE_SIZE];
	DWORD dw_read = 0;
	LARGE_INTEGER lg_move;

	/*
	 *  设置偏移
	 */
	printf("设置偏移\n");
	lg_move.QuadPart = p_context->u_first_mft_offset;
	SetFilePointerEx(h_file, lg_move, NULL, FILE_BEGIN);
	ReadFile(h_file, p_mft, NTFS_MFT_FILE_SIZE, &dw_read, NULL);

	/*
	 *  我们先找到一个空的mft
	 */
	printf("我们先找到一个空的mft\n");
	int n_free_mft_no = ntfs_get_free_mft_record((PFILE_RECORD_HEADER)p_mft, p_context);

	printf("the number of free mft is %d\n", n_free_mft_no);

	/*
	 *  保存到context中来
	 */
	printf("保存到context中来\n");
	add_context.mft_no = n_free_mft_no - 1;

	/*
	 *  这里我们初始化这mft,然后填充数据,最后将其写入磁盘
	 */
	printf("这里我们初始化这mft,然后填充数据,最后将其写入磁盘\n");
	PBYTE p_mft_record = new BYTE[NTFS_MFT_FILE_SIZE];

	memset(p_mft_record, 0, NTFS_MFT_FILE_SIZE);

	/*
	 *  初始化mft
	 */
	printf("初始化mft\n");
	ntfs_mft_init((PFILE_RECORD_HEADER)p_mft_record, n_free_mft_no - 1);

	/*
	 *  先加个10属性
	 */
	printf("先加个10属性\n");
	ntfs_attri_add_resident((PFILE_RECORD_HEADER)p_mft_record, \
		$STANDARD_INFORMATION, NULL, &add_context);

	/*
	 *  再加个30属性
	 */

	ntfs_attri_add_resident((PFILE_RECORD_HEADER)p_mft_record, \
		$FILE_NAME, sz_file_name, &add_context);

	/*
	 *  再加个80属性
	 */

	ntfs_attri_add_resident((PFILE_RECORD_HEADER)p_mft_record, \
		$DATA, NULL, &add_context);

	ntfs_resize_mft_header((PFILE_RECORD_HEADER)p_mft_record);

	/*
	 *  首先在磁盘中写入这个mft
	 */
	printf("%s",p_mft_record);
	int n_ret = 0;
	LARGE_INTEGER offset;
	DWORD dw_write;
	offset.QuadPart = p_context->u_first_mft_offset + ((n_free_mft_no - 1) * NTFS_MFT_FILE_SIZE);
	//offset.QuadPart = 172702720;
	n_ret = SetFilePointerEx(h_file, offset, NULL, FILE_BEGIN);
	FILE *p;
	p = fopen("t.txt", "wb");
	fwrite(p_mft_record, 1, 1024,p);
	fclose(p);
	/*n_ret = WriteFile(h_file, p_mft_record, NTFS_MFT_FILE_SIZE, &dw_write, NULL);
	if (n_ret == 0)
	{
		printf("Get error is %d\n", GetLastError());
		return;
	}*/
	/*
	 *  接下来就是在根目录的index_entry中添加此项
	 */


	 /*
	  *  首先我们先找到根目录的mft
	  */

	PBYTE p_mft_dir = new BYTE[NTFS_MFT_FILE_SIZE];
	offset.QuadPart = p_context->u_first_mft_offset + 0x1400;
	SetFilePointerEx(h_file, offset, NULL, FILE_BEGIN);
	ReadFile(h_file, p_mft_dir, NTFS_MFT_FILE_SIZE, &dw_read, NULL);

	PATTRIBUTE_RECORD_HEADER p_attri = ntfs_attri_lookup_in_file_record((PFILE_RECORD_HEADER)p_mft_dir, \
		$INDEX_ALLOCATION);

	/*
	 *  我们去找索引根
	 */

	PBYTE   pRun = (PBYTE)p_attri + \
		p_attri->Form.Nonresident.MappingPairsOffset;

	/*
	 *  这里我测试的时候就是一个512M的空盘,所以我这解析一个run就行了
	 */

	ULONGLONG Lcv = 0;
	Lcv = RunLCN(pRun);
	offset.QuadPart = Lcv * p_context->u_byte_per_cluster;

	PBYTE p_index_buff = new BYTE[4096];
	SetFilePointerEx(h_file, offset, NULL, FILE_BEGIN);
	ReadFile(h_file, p_index_buff, 4096, &dw_read, NULL);

	ntfs_index_add_entry((PINDEX_ALLOCATION_BUFFER)p_index_buff, &add_context);


	/*
	 *  写入磁盘
	 */

	SetFilePointerEx(h_file, offset, NULL, FILE_BEGIN);
	WriteFile(h_file, p_index_buff, 4096, &dw_write, NULL);
	printf("Get error is %d\n", GetLastError());
	FILE *p1;
	p1 = fopen("p_index_buff.txt", "wb");
	fwrite(p_index_buff, 1, 4096, p1);
	fclose(p1);

	/*
	 *  修改位图
	 */
	//ntfs_get_free_bit_and_set(172538368)

	 /*
	  *  释放内存
	  */

	delete p_mft;
	delete p_mft_record;
	delete p_index_buff;

}

/** \fn
	\brief
	\param
	\return
*/

void ntfs_init_context(PNTFS_CONTEXT p_ntfs_context)
{

	memset(p_ntfs_context, 0, sizeof(NTFS_CONTEXT));

	/*
	 *  首先我们打开一个ntfs盘
	 */

	/*\\\\.\\g:*/
	p_ntfs_context->h_device = CreateFile(L"\\\\.\\e:", \
		GENERIC_READ | GENERIC_WRITE, \
		FILE_SHARE_READ | FILE_SHARE_WRITE, \
		NULL, \
		OPEN_EXISTING, \
		0, \
		NULL
	);

	if (INVALID_HANDLE_VALUE == p_ntfs_context->h_device)
	{
		printf("Open Disk Error!  error code is %d\n", GetLastError());
		return;
	}


	/*
	 *  首先读取启动扇区
	 */

	PBYTE    pByteRead = new BYTE[512];
	DWORD    dwRead;

	ReadFile(p_ntfs_context->h_device, (LPVOID)pByteRead, 512, &dwRead, \
		NULL);

	PBOOT_SECTOR    pBootSector = (PBOOT_SECTOR)pByteRead;

	/*printf( "%s\n", pBootSector->OEMID );*/

	/*
	 *  记录一些常用的参数
	 */

	p_ntfs_context->u_byte_per_sector = pBootSector->BPB.BytesPerSector;
	p_ntfs_context->u_sector_per_cluster = pBootSector->BPB.SectorsPerCluster;
	p_ntfs_context->u_byte_per_cluster = pBootSector->BPB.BytesPerSector * \
		pBootSector->BPB.SectorsPerCluster;

	/*
	 *  计算第一个MFT的位置,并记录
	 */

	p_ntfs_context->u_first_mft_offset = pBootSector->EBPB.MftLocation   * \
		pBootSector->BPB.BytesPerSector * \
		pBootSector->BPB.SectorsPerCluster;
	delete pByteRead;
}