#include <fftw3.h>
 
// 定义输入音频文件名和采样率
#define INPUT_FILENAME "input.pcm"
#define SAMPLE_RATE 44100
 
// 定义 FFTW 参数
#define FFT_SIZE 1024
 
int main() {
    // 定义输入音频缓冲区
    double input_buffer[FFT_SIZE];
 
    // 读取输入音频数据，这里假设从文件读取，你可以根据需要进行修改
    FILE *input_file = fopen(INPUT_FILENAME, "rb");
 
    // 创建 FFTW 输入和输出数组
    fftw_complex *fft_input = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_complex *fft_output = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
 
    // 创建 FFTW 3.0 的计划
    fftw_plan plan = fftw_plan_dft_1d(FFT_SIZE, fft_input, fft_output, FFTW_FORWARD, FFTW_ESTIMATE);
 
    // 读取音频数据并进行频谱计算
    while (!feof(input_file)) {
        // 读取输入音频数据到缓冲区
        size_t read_size = fread(input_buffer, sizeof(double), FFT_SIZE, input_file);
 
        // 将输入音频数据复制到 FFTW 输入数组
        for (int i = 0; i < FFT_SIZE; i++) {
            fft_input[i][0] = input_buffer[i];
            fft_input[i][1] = 0.0;  // 虚部设置为零
        }
 
        // 执行 FFT 变换
        fftw_execute(plan);
 
        // 计算频谱
        for (int i = 0; i < FFT_SIZE; i++) {
            double magnitude = sqrt(fft_output[i][0] * fft_output[i][0] + fft_output[i][1] * fft_output[i][1]);
            double frequency = ((double)i / FFT_SIZE) * SAMPLE_RATE;
 
            // 在这里可以对频谱数据做进一步处理，例如绘图、输出等
            printf("Frequency: %.2f Hz, Magnitude: %.4f\n", frequency, magnitude);
        }
    }
 
    // 销毁 FFTW 相关资源
    fftw_destroy_plan(plan);
    fftw_free(fft_input);
    fftw_free(fft_output);
 
    // 关闭文件
    fclose(input_file);
 
    return 0;
}
