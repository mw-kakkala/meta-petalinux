#include <cardano.h>
#include "config.h"

int a[NUM_A_ELMNTS_PER_TILE];

void OneInput(input_window_int32 * dataIn,
		output_window_int32 * dataOut,
		output_window_int32 * result) {

	int currentCol = (get_coreid() & 0x7F0000) >> 16;
	int b[NUM_ROWS], intrmdtResult[NUM_ROWS / NUM_HW_COLS];

	for(unsigned i = 0; i < NUM_A_ELMNTS_PER_TILE / WIN_SIZE; i++) {
		window_acquire(dataIn);
		for(unsigned w = 0; w < WIN_SIZE; w++) {
			a[i * WIN_SIZE + w] = window_readincr(dataIn);
		}
		window_release(dataIn);
	}

	for(unsigned i = 0;
		i < NUM_A_ELMNTS_PER_TILE * (NUM_HW_COLS - currentCol - 1) / WIN_SIZE;
		i++) {

		window_acquire(dataOut);
		window_acquire(dataIn);
		for(unsigned w = 0; w < WIN_SIZE; w++)
			window_writeincr(dataOut, window_readincr(dataIn));
		window_release(dataIn);
		window_release(dataOut);
	}

	/**
	 * read one column of b, pass the same to the next core,
	 * compute matrix multiplication of 'a' rows x 'b' col and
	 * finally output the result
	 */
	for(unsigned i = 0; i < NUM_COLS; i++) {
		/* read 1 entire col of b */
		window_acquire(dataOut);
		window_acquire(dataIn);
		for(unsigned w = 0; w < WIN_SIZE; w++) {
			b[w] = window_readincr(dataIn);
			window_writeincr(dataOut, b[w]);
		}
		window_release(dataIn);
		window_release(dataOut);

		window_acquire(result);
		for(unsigned k = 0; k < NUM_ROWS / NUM_HW_COLS; k++) {
			intrmdtResult[k] = 0;
			for(unsigned l = 0; l < NUM_COLS; l++) {
				intrmdtResult[k] += a[k * NUM_COLS + l] * b[l];
			}
			window_writeincr(result, intrmdtResult[k]);
		}
		window_release(result);
	}
}