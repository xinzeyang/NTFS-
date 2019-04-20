#include <stdio.h>
#include <windows.h>


#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $OBJECT_ID                       (0x40)
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $SYMBOLIC_LINK                   (0xC0)
#define $EA_INFORMATION                  (0xD0)
#define $EA                              (0xE0)
#define $FIRST_USER_DEFINED_ATTRIBUTE    (0x100)
#define $END                             (0xFFFFFFFF)

#define NTFS_MFT_FILE_SIZE       1024

#define FAT_DIRENT_ATTR_READ_ONLY        (0x01)
#define FAT_DIRENT_ATTR_HIDDEN           (0x02)
#define FAT_DIRENT_ATTR_SYSTEM           (0x04)
#define FAT_DIRENT_ATTR_VOLUME_ID        (0x08)
#define FAT_DIRENT_ATTR_ARCHIVE          (0x20)
#define FAT_DIRENT_ATTR_DEVICE           (0x40)

#define FILE_NAME_NTFS                   (0x01)
#define FILE_NAME_DOS                    (0x02)

#define SIZEOF_RESIDENT_ATTRIBUTE_HEADER (                         \
    FIELD_OFFSET(ATTRIBUTE_RECORD_HEADER,Form.Resident.Reserved)+1 \
)

#define SIZEOF_FULL_NONRES_ATTR_HEADER (    \
    sizeof(ATTRIBUTE_RECORD_HEADER)         \
)

#define SIZEOF_PARTIAL_NONRES_ATTR_HEADER (                                 \
    FIELD_OFFSET(ATTRIBUTE_RECORD_HEADER,Form.Nonresident.TotalAllocated)   \
)


//
// 引导扇区的定义
//

#pragma pack(1)

typedef struct _BIOS_PARAMETERS_BLOCK
{
        USHORT    BytesPerSector;			// 0x0B
        UCHAR     SectorsPerCluster;		// 0x0D
        UCHAR     Unused0[7];				// 0x0E, checked when volume is mounted
        UCHAR     MediaId;				    // 0x15
        UCHAR     Unused1[2];				// 0x16
        USHORT    SectorsPerTrack;		    // 0x18
        USHORT    Heads;				    // 0x1A
        UCHAR     Unused2[4];				// 0x1C
        UCHAR     Unused3[4];				// 0x20, checked when volume is mounted

} BIOS_PARAMETERS_BLOCK, *PBIOS_PARAMETERS_BLOCK;

typedef struct _EXTENDED_BIOS_PARAMETERS_BLOCK
{
        USHORT    Unknown[2];				    // 0x24, always 80 00 80 00
        ULONGLONG SectorCount;			        // 0x28
        ULONGLONG MftLocation;			        // 0x30
        ULONGLONG MftMirrLocation;		        // 0x38
        CHAR      ClustersPerMftRecord;	        // 0x40
        UCHAR     Unused4[3];				    // 0x41
        CHAR      ClustersPerIndexRecord;       // 0x44
        UCHAR     Unused5[3];				    // 0x45
        ULONGLONG SerialNumber;			        // 0x48
        UCHAR     Checksum[4];			        // 0x50

} EXTENDED_BIOS_PARAMETERS_BLOCK, *PEXTENDED_BIOS_PARAMETERS_BLOCK;

typedef struct _BOOT_SECTOR
{
  
          UCHAR     Jump[3];				// 0x00
          UCHAR     OEMID[8];				// 0x03
          BIOS_PARAMETERS_BLOCK BPB;
          EXTENDED_BIOS_PARAMETERS_BLOCK EBPB;
          UCHAR     BootStrap[426];			// 0x54
          USHORT    EndSector;				// 0x1FE

} BOOT_SECTOR, *PBOOT_SECTOR;
#pragma pack()

#pragma pack(4)
//
// 文件记录头部的定义
//

typedef struct _NTFS_RECORD_HEADER
{
        ULONG           Type;                   /* Magic number 'FILE' */
        USHORT          UsaOffset;              /* Offset to the update sequence */
        USHORT          UsaCount;               /* Size in words of Update Sequence Number & Array (S) */
        ULONGLONG       Lsn;                    /* $LogFile Sequence Number (LSN) */

} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

#define NRH_FILE_TYPE  0x454C4946  /* 'FILE' */

typedef struct _FILE_RECORD_HEADER
{
        NTFS_RECORD_HEADER      Ntfs;
        USHORT                  SequenceNumber;         /* Sequence number */
        USHORT                  LinkCount;              /* Hard link count */
        USHORT                  AttributeOffset;        /* Offset to the first Attribute */
        USHORT                  Flags;                  /* Flags */
        ULONG                   BytesInUse;             /* Real size of the FILE record */
        ULONG                   BytesAllocated;         /* Allocated size of the FILE record */
        ULONGLONG               BaseFileRecord;         /* File reference to the base FILE record */
        USHORT                  NextAttributeNumber;    /* Next Attribute Id */
        USHORT                  Pading;                 /* Align to 4 UCHAR boundary (XP) */
        ULONG                   MFTRecordNumber;        /* Number of this MFT Record (XP) */

} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

/*typedef struct _ATTRIBUTE_HREADER
{

        ULONG   AttributeType;
        ULONG   AttributeLength;
        UCHAR   ResidentFlag;
        UCHAR   NameLength;
        USHORT  NameOffset;
        USHORT  Flag;
        USHORT  AttributeID;
       

}ATTRIBUTE_HREADER,*PATTRIBUTE_HREADER;*/

//
// 属性头(copy from source code of NT)
//

typedef struct _ATTRIBUTE_RECORD_HEADER {
        ULONG  TypeCode;                                                //  offset = 0x000 //0x00000090
        USHORT RecordLength;                                            //  offset = 0x004 //0x000001B8
        USHORT Resered1;
        UCHAR  FormCode;                                                //  offset = 0x008 //0x00
        UCHAR  NameLength;                                              //  offset = 0x009 //0x04
        USHORT NameOffset;                                              //  offset = 0x00A //0x0018
        USHORT Flags;                                                   //  offset = 0x00C //0x0000
        USHORT Instance;                                                //  offset = 0x00E //0x0006
        
        union {
                
                struct {
                        
                        ULONG ValueLength;                                      //  offset = 0x010 //0x00000198
                        USHORT ValueOffset;                                     //  offset = 0x014 //0x0020
                        UCHAR ResidentFlags;                                    //  offset = 0x016 //0x00
                        UCHAR Reserved;                                         //  offset = 0x017 //0x00
                } Resident;
                
                struct {
                        
                        LONGLONG LowestVcn;                                     //  offset = 0x010
                        LONGLONG HighestVcn;                                    //  offset = 0x018
                        USHORT   MappingPairsOffset;                            //  offset = 0x020
                        UCHAR    CompressionUnit;                               //  offset = 0x022
                        UCHAR    Reserved[5];                                   //  offset = 0x023
                        LONGLONG AllocatedLength;                               //  offset = 0x028
                        LONGLONG FileSize;                                      //  offset = 0x030
                        LONGLONG ValidDataLength;                               //  offset = 0x038
                        LONGLONG TotalAllocated;                                //  offset = 0x040
                } Nonresident;
                
        } Form;
        
} ATTRIBUTE_RECORD_HEADER,*PATTRIBUTE_RECORD_HEADER;

//
// 0x90属性
//

typedef struct _INDEX_HEADER {
    ULONG FirstIndexEntry;                                          //  offset = 0x000 //0x00000010
    ULONG FirstFreeByte;                                            //  offset = 0x004 //0x00000188
    ULONG BytesAvailable;                                           //  offset = 0x008 //0x00000188
    UCHAR Flags;                                                    //  offset = 0x00C //0x01
    UCHAR Reserved[3];                                              //  offset = 0x00D //0x000000
} INDEX_HEADER,*PINDEX_HEADER;                                      //  sizeof = 0x010

typedef struct _INDEX_ROOT {
        
        ULONG IndexedAttributeType;                                     //  offset = 0x000 //0x00000030
        ULONG CollationRule;                                            //  offset = 0x004 //0x00000001
        ULONG BytesPerIndexBuffer;                                      //  offset = 0x008 //0x00000010
        UCHAR BlocksPerIndexBuffer;                                     //  offset = 0x00C //0x01
        UCHAR Reserved[3];                                              //  offset = 0x00D //0x000000
        INDEX_HEADER IndexHeader;                                       //  offset = 0x010 //0x00000010

} INDEX_ROOT,*PINDEX_ROOT;                                              //  sizeof = 0x020

//
// 0x30属性.结构体
//

typedef struct _DUPLICATED_INFORMATION {
        
        LONGLONG CreationTime;                                          //  offset = 0x000
        LONGLONG LastModificationTime;                                  //  offset = 0x008
        LONGLONG LastChangeTime;                                        //  offset = 0x010
        LONGLONG LastAccessTime;                                        //  offset = 0x018
        LONGLONG AllocatedLength;                                       //  offset = 0x020
        LONGLONG FileSize;                                              //  offset = 0x028
        ULONG FileAttributes;                                           //  offset = 0x030
        USHORT PackedEaSize;                                            //  offset = 0x034
        USHORT Reserved;                                                //  offset = 0x036

} DUPLICATED_INFORMATION, *PDUPLICATED_INFORMATION;                     //  sizeof = 0x038

typedef struct _FILE_NAME {
        
        ULONGLONG ParentDirectory;                                      //  offset = 0x000
        DUPLICATED_INFORMATION Info;                                    //  offset = 0x008
        UCHAR FileNameLength;                                           //  offset = 0x040
        UCHAR Flags;                                                    //  offset = 0x041
        WCHAR FileName[1];                                              //  offset = 0x042
        
} FILE_NAME, *PFILE_NAME;

typedef struct _MFT_SEGMENT_REFERENCE {
        ULONG  SegmentNumberLowPart;                                    //  offset = 0x000 //0x00007AD6
        USHORT SegmentNumberHighPart;                                   //  offset = 0x004 //0x0000
        USHORT SequenceNumber;                                          //  offset = 0x006 //0x0003
} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;                       //  sizeof = 0x008

typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, *PFILE_REFERENCE;

typedef struct _INDEX_ENTRY {
        union {
                FILE_REFERENCE FileReference;                                   //  offset = 0x000
                struct 
                {
                        USHORT DataOffset;                                      //  offset = 0x000  
                        USHORT DataLength;                                      //  offset = 0x002
                        ULONG  ReservedForZero;                                 //  offset = 0x004
                };
        }Form;
        
        USHORT Length;                                                  //  offset = 0x008 //0x0070
        USHORT AttributeLength;                                         //  offset = 0x00A //0x0052
        USHORT Flags;                                                   //  offset = 0x00C //0x0001
        USHORT Reserved;                                                //  offset = 0x00E //0x0000
        FILE_NAME FileName;
        
} INDEX_ENTRY;                                                      //  sizeof = 0x010
typedef INDEX_ENTRY *PINDEX_ENTRY;

#define NTFS_INDEX_LENGTH_EXCEPTE_NAME   0x56
#define FILE_NAME_NTFS                   (0x01)
#define FILE_NAME_DOS                    (0x02)

typedef struct _NTFS_CONTEXT
{
    HANDLE       h_device;
    ULONGLONG    u_first_mft_offset;
    USHORT       u_byte_per_sector;
    UCHAR        u_sector_per_cluster;
    ULONG        u_byte_per_cluster;

}NTFS_CONTEXT, *PNTFS_CONTEXT;

typedef struct _STANDARD_INFORMATION {
    
    LONGLONG CreationTime;                                          //  offset = 0x000
    LONGLONG LastModificationTime;                                  //  offset = 0x008
    LONGLONG LastChangeTime;                                        //  offset = 0x010
    LONGLONG LastAccessTime;                                        //  offset = 0x018
    ULONG FileAttributes;                                           //  offset = 0x020
    ULONG MaximumVersions;                                          //  offset = 0x024
    ULONG VersionNumber;                                            //  offset = 0x028
    ULONG ClassId;                                                  //  offset = 0x02c
    ULONG OwnerId;                                                  //  offset = 0x030
    ULONG SecurityId;                                               //  offset = 0x034   
    ULONGLONG QuotaCharged;                                         //  offset = 0x038
    ULONGLONG Usn;                                                  //  offset = 0x040  
    
} STANDARD_INFORMATION;                                             //  sizeof = 0x048

typedef STANDARD_INFORMATION *PSTANDARD_INFORMATION;
#define SIZEOF_STANDERD_INFORMATION         0x48

typedef struct _MULTI_SECTOR_HEADER {

    UCHAR Signature[4];
    USHORT UpdateSequenceArrayOffset;
    USHORT UpdateSequenceArraySize;
    
} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER;


typedef USHORT UPDATE_SEQUENCE_NUMBER, *PUPDATE_SEQUENCE_NUMBER;
typedef LARGE_INTEGER LSN, *PLSN;
typedef LONGLONG VCN;
typedef UPDATE_SEQUENCE_NUMBER UPDATE_SEQUENCE_ARRAY[1];
typedef UPDATE_SEQUENCE_ARRAY *PUPDATE_SEQUENCE_ARRAY;

typedef struct _INDEX_ALLOCATION_BUFFER 
{    
    MULTI_SECTOR_HEADER MultiSectorHeader;                          //  offset = 0x000
    LSN Lsn;                                                        //  offset = 0x008
    VCN ThisBlock;                                                  //  offset = 0x010
    INDEX_HEADER IndexHeader;                                       //  offset = 0x018
    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;                      //  offset = 0x028
    
} INDEX_ALLOCATION_BUFFER;
typedef INDEX_ALLOCATION_BUFFER *PINDEX_ALLOCATION_BUFFER;


#define INDEX_ENTRY_NODE                 (0x0001)

#define INDEX_ENTRY_END                  (0x0002)

#define NtfsIndexEntryBlock(IE) (                                       \
    *(PLONGLONG)((PCHAR)(IE) + (ULONG)(IE)->Length - sizeof(LONGLONG))  \
)

#define NtfsSetIndexEntryBlock(IE,IB) {                                         \
    *(PLONGLONG)((PCHAR)(IE) + (ULONG)(IE)->Length - sizeof(LONGLONG)) = (IB);  \
}

#define NtfsFirstIndexEntry(IH) (                       \
    (PINDEX_ENTRY)((PCHAR)(IH) + (IH)->FirstIndexEntry) \
)

#define NtfsNextIndexEntry(IE) (                        \
    (PINDEX_ENTRY)((PCHAR)(IE) + (ULONG)(IE)->Length)   \
    )



/*
 *  add context
 */

typedef struct tag_add_context
{

    ULONG mft_no;
    BYTE file_name[512];
    LARGE_INTEGER now_pointer;

}ADD_CONTEXT,*PADD_CONTEXT;


#pragma pack()


void   AnalyseFileRecord( HANDLE  hNTFS, PFILE_RECORD_HEADER pFileRecord );
void   AnalyseAttribute ( HANDLE  hNTFS, LPVOID  lpAttribute );

inline ULONG
RunLength(PUCHAR run)
{
        return(*run & 0x0f) + ((*run >> 4) & 0x0f) + 1;
}


inline LONGLONG
RunLCN(PUCHAR run)
{
        UCHAR n1 = *run & 0x0f;
        UCHAR n2 = (*run >> 4) & 0x0f;
        LONGLONG lcn = (n2 == 0) ? 0 : (CHAR)(run[n1 + n2]);
        LONG i = 0;
        
        for (i = n1 +n2 - 1; i > n1; i--)
                lcn = (lcn << 8) + run[i];
        return lcn;
}



inline ULONGLONG
RunCount(PUCHAR run)
{
        UCHAR n =  *run & 0xf;
        ULONGLONG count = 0;
        ULONG i = 0;
        
        for (i = n; i > 0; i--)
                count = (count << 8) + run[i];
        return count;
}