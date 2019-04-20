

/*! \file ntfs_bitmap.cpp
 *  \brief 
 */

#include <stdio.h>
#include <windows.h>


/** \fn 
	\brief 
	\param 
	\return
*/

int ntfs_get_free_bit_and_set( PBYTE bitmap, int n_count )
{
    /*register i, j;*/
	int i = 0, j = 0;
    for ( i = 1; i <= n_count; i++ )
    {
        BYTE b_bit = bitmap[i-1];
        for ( j = 0; j < 8; j++ )
        {
            if ( ( b_bit & ( 1 << j ) ) == 0 )
            {
                
                /*
                 *  我们设置这一位为可用
                 */

                b_bit |= ( 1 << j );
                bitmap[i-1] = b_bit;                
                return ( (i - 1) * 8 ) + (j + 1) ;
            }
        }
    }
    return -1;
}