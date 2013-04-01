void fft_init(int sample_rate_in);
void fft_free(void);
void fft_do(double *in, double *output_mag);
void fft_stop(void);
