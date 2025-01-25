#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#define NUM_BLOCK_SIZES 4
#define RAM_SIZE 16 * 1024
#define NUM_REQUESTS 2 * RAM_SIZE

double get_time_sec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

int main(int argc, char *argv[])
{
    // Tamanhos dos blocos (ex.: 1B, 1KB, 32KB, 1MB)
    int block_sizes[NUM_BLOCK_SIZES] = {1, 1024, 32768, 1048576};

    // Arquivo criado para testes
    char *filename = "testfile.bin";
    // Cria arquivo grande (simples, sem dupla do tamanho da RAM)
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        perror("Erro ao criar arquivo");
        return 1;
    }
    int mb = 32;
    char *random_data = (char *)malloc(mb * 1024 * 1024);
    if (!random_data)
    {
        perror("Erro ao alocar buffer");
        fclose(f);
        return 1;
    }
    srand(time(NULL));
    for (int i = 0; i < mb * 1024 * 1024; i++)
    {
        random_data[i] = rand() % 256;
    }
    fwrite(random_data, 1, mb * 1024 * 1024, f);
    fclose(f);
    free(random_data);

    // Tabela de resultados
    printf("BlockSize (bytes), SeqRead (s), RandRead (s), SeqWrite (s), RandWrite (s)\n");

    for (int i = 0; i < NUM_BLOCK_SIZES; i++)
    {
        int bs = block_sizes[i];
        int num_requests = NUM_REQUESTS / bs;
        
        double start, end;
        double seq_read_time = 0.0, rand_read_time = 0.0;
        double seq_write_time = 0.0, rand_write_time = 0.0;

        // Leitura sequencial
        start = get_time_sec();
        int fd = open(filename, O_RDONLY);
        if (fd < 0)
        {
            perror("Erro seq read");
        }
        else
        {
            char *buf = (char *)malloc(bs);
            for (int r = 0; r < num_requests; r++)
            {
                ssize_t rsz = read(fd, buf, bs);
                if (rsz <= 0)
                    break;
            }
            free(buf);
            close(fd);
        }
        end = get_time_sec();
        seq_read_time = end - start;

        // Leitura aleatória
        start = get_time_sec();
        fd = open(filename, O_RDONLY);
        if (fd < 0)
        {
            perror("Erro rand read");
        }
        else
        {
            off_t file_size = lseek(fd, 0, SEEK_END);
            char *buf2 = (char *)malloc(bs);
            for (int r = 0; r < num_requests; r++)
            {
                off_t offset = rand() % (file_size > bs ? (file_size - bs) : 1);
                lseek(fd, offset, SEEK_SET);
                read(fd, buf2, bs);
            }
            free(buf2);
            close(fd);
        }
        end = get_time_sec();
        rand_read_time = end - start;

        // Escrita sequencial
        start = get_time_sec();
        fd = open("temp_seq_write.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0)
        {
            perror("Erro seq write");
        }
        else
        {
            char *buf3 = (char *)malloc(bs);
            memset(buf3, 'x', bs);
            for (int r = 0; r < num_requests; r++)
            {
                write(fd, buf3, bs);
            }
            free(buf3);
            close(fd);
        }
        end = get_time_sec();
        seq_write_time = end - start;

        // Escrita aleatória
        start = get_time_sec();
        int file_size = bs * num_requests;
        fd = open("temp_rand_write.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd >= 0)
        {
            lseek(fd, file_size - 1, SEEK_SET);
            write(fd, "", 1);
            close(fd);
        }
        fd = open("temp_rand_write.bin", O_WRONLY);
        if (fd < 0)
        {
            perror("Erro rand write");
        }
        else
        {
            char *buf4 = (char *)malloc(bs);
            memset(buf4, 'y', bs);
            for (int r = 0; r < num_requests; r++)
            {
                off_t offset = rand() % (file_size > bs ? (file_size - bs) : 1);
                lseek(fd, offset, SEEK_SET);
                write(fd, buf4, bs);
            }
            free(buf4);
            close(fd);
        }
        end = get_time_sec();
        rand_write_time = end - start;

        printf("%d, %.6f, %.6f, %.6f, %.6f\n",
               bs, seq_read_time, rand_read_time, seq_write_time, rand_write_time);
    }
    return 0;
}
