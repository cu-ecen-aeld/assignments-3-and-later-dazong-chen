/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn     is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    int        cur_total_size = 0;
    int        prev_total_size = 0;
    uint8_t    position = buffer->out_offs;
    
    
    if(buffer == NULL)
    {
        return NULL;
    }
    
    // check if ring buffer is empty
    if( (position == buffer->in_offs) && (buffer->full == false) )
    {
        return NULL;
    }
    
    
    do   // search through the circular buffer
    {
        if(char_offset > cur_total_size)
        {
            cur_total_size += buffer->entry[position].size;
        }
        
        else
        {
            *entry_offset_byte_rtn = char_offset - prev_total_size;
            
            return buffer->entry[position];
        }
        
        position = (position+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;  // next entry[i]
        prev_total_size += buffer->entry[position].size;
        
    }while(position != buffer->in_offs) 
    
    
    return NULL;    // searched through whole buffer, char offset is greater than total size of buffer string
    
    
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{

    if(buffer == NULL)
    {
        return NULL;
    }
       
    if(buffer->full == true)
    {   
        buffer->entry[buff->in_offs] = *add_entry;    // overwrites the oldest data
        
        buffer->out_offs = (buffer->out_offs+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;    //advances buffer->out_offs to the new start location.
    }
    
    else
    {
        buffer->entry[buffer->in_offs] = *add_entry;
    }
    
    buffer->in_offs = (buffer->in_offs+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;    // implement in_offs to next location after new entry is written
    
    
    if(buffer->in_offs == buffer->out_offs)    // check the full conditions
    {
        buffer->full = true;
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
