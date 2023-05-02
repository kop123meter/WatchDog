
/**************************************************************************//**
* @file        circular_buffer library
* @ingroup 	   Serial Console
* @brief       This library for a circular buffer is taken from https://github.com/embeddedartistry/embedded-resources/blob/master/examples/c/circular_buffer/circular_buffer.c
*				Author: Phillips Johnston
* @details     Please refer to the github code above and to "https://embeddedartistry.com/blog/2017/4/6/circular-buffers-in-cc" for more information
*
*				See "https://github.com/embeddedartistry/embedded-resources/blob/master/examples/c/circular_buffer_test.c" for examples on how to use the buffer
*
* @copyright
* @author		Phillips Johnston
* @date        Aug 6, 2018
* @version		0.1
*****************************************************************************/


 #include <stdlib.h>
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 #include <assert.h>

 #include "circular_buffer.h"


 // The definition of our circular buffer structure is hidden from the user
 struct circular_buf_t {
	 uint8_t * buffer;
	 size_t head;
	 size_t tail;
	 size_t max; //of the buffer
	 bool full;
 };

 #pragma mark - Private Functions -

 static void advance_pointer(cbuf_handle_t cbuf)
 {
	 //assert(cbuf);

	 if(cbuf->full)
	 {
		 cbuf->tail = (cbuf->tail + 1) % cbuf->max;
	 }

	 cbuf->head = (cbuf->head + 1) % cbuf->max;

	 // We mark full because we will advance tail on the next time around
	 cbuf->full = (cbuf->head == cbuf->tail);
 }

 static void retreat_pointer(cbuf_handle_t cbuf)
 {
	 //assert(cbuf);

	 cbuf->full = false;
	 cbuf->tail = (cbuf->tail + 1) % cbuf->max;
 }

 #pragma mark - APIs -

 cbuf_handle_t circular_buf_init(uint8_t* buffer, size_t size)
 {
	// assert(buffer && size);

	 cbuf_handle_t cbuf = malloc(sizeof(circular_buf_t));
	 //assert(cbuf);

	 cbuf->buffer = buffer;
	 cbuf->max = size;
	 circular_buf_reset(cbuf);

	// assert(circular_buf_empty(cbuf));

	 return cbuf;
 }

 void circular_buf_free(cbuf_handle_t cbuf)
 {
	// assert(cbuf);
	 free(cbuf);
 }

 void circular_buf_reset(cbuf_handle_t cbuf)
 {
	// assert(cbuf);

	 cbuf->head = 0;
	 cbuf->tail = 0;
	 cbuf->full = false;
 }

 size_t circular_buf_size(cbuf_handle_t cbuf)
 {
	// assert(cbuf);

	 size_t size = cbuf->max;

	 if(!cbuf->full)
	 {
		 if(cbuf->head >= cbuf->tail)
		 {
			 size = (cbuf->head - cbuf->tail);
		 }
		 else
		 {
			 size = (cbuf->max + cbuf->head - cbuf->tail);
		 }

	 }

	 return size;
 }

 size_t circular_buf_capacity(cbuf_handle_t cbuf)
 {
	 //assert(cbuf);

	 return cbuf->max;
 }

 void circular_buf_put(cbuf_handle_t cbuf, uint8_t data)
 {
	 //assert(cbuf && cbuf->buffer);

	 cbuf->buffer[cbuf->head] = data;

	 advance_pointer(cbuf);
 }

 int circular_buf_put2(cbuf_handle_t cbuf, uint8_t data)
 {
	 int r = -1;

	 //assert(cbuf && cbuf->buffer);

	 if(!circular_buf_full(cbuf))
	 {
		 cbuf->buffer[cbuf->head] = data;
		 advance_pointer(cbuf);
		 r = 0;
	 }

	 return r;
 }

 int circular_buf_get(cbuf_handle_t cbuf, uint8_t * data)
 {
	 //assert(cbuf && data && cbuf->buffer);
	 
	 int r = -1;

	 if(!circular_buf_empty(cbuf))
	 {
		 *data = cbuf->buffer[cbuf->tail];
		 retreat_pointer(cbuf);

		 r = 0;
	 }

	 return r;
 }

 bool circular_buf_empty(cbuf_handle_t cbuf)
 {
	 //assert(cbuf);

	 return (!cbuf->full && (cbuf->head == cbuf->tail));
 }

 bool circular_buf_full(cbuf_handle_t cbuf)
 {
	// assert(cbuf);

	 return cbuf->full;
 }