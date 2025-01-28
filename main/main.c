#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
    #endif
    #define open   _open
    #define read   _read
    #define write  _write
    #define close  _close
    #define lseek  _lseek
    #ifndef S_IRUSR
    #define S_IRUSR 0
    #define S_IWUSR 0
    #endif
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>
#else
    #define _XOPEN_SOURCE 700
    #include <stdio.h>
    #include <stdlib.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <string.h>
    #include <time.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

// 1 MB em bytes
#define MB (1024.0 * 1024.0)

// Quatro tamanhos de bloco (exemplo)
#define NUM_BLOCK_SIZES 4

// Máximo de requisições permitidas (para não travar)
#define MAX_REQUESTS 10000000LL

// Função que pega tempo atual (segundos) no Windows/Linux
double get_time_sec(void)
{
#ifdef _WIN32
    static int freq_init = 0;
    static LARGE_INTEGER freq;
    if (!freq_init) {
        if (!QueryPerformanceFrequency(&freq)) {
            fprintf(stderr, "Erro QueryPerformanceFrequency.\n");
            return 0.0;
        }
        freq_init = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
#endif
}

// Retorna RAM total (bytes)
long long get_total_ram(void)
{
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return (long long)status.ullTotalPhys;
    }
    return -1;
#else
    long long pages = sysconf(_SC_PHYS_PAGES);
    long long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
#endif
}

int main(void)
{
    // Leitura do valor de GB desejado
    double requested_gb = 0.0;
    printf("Digite quantos GB de RAM deseja testar (0 = usar a RAM do sistema): ");
    if (scanf("%lf", &requested_gb) != 1) {
        fprintf(stderr, "Entrada invalida.\n");
        return 1;
    }

    // Se 0, pega RAM real; senão, converte GB -> bytes
    long long ram_size = 0;
    if (requested_gb == 0.0) {
        ram_size = get_total_ram();
        if (ram_size <= 0) {
            fprintf(stderr, "Falha ao detectar RAM.\n");
            return 1;
        }
    } else {
        ram_size = (long long)(requested_gb * 1024.0 * 1024.0 * 1024.0);
        if (ram_size <= 0) {
            fprintf(stderr, "Valor de GB invalido.\n");
            return 1;
        }
    }

    // Tamanho do arquivo = 2 * RAM
    long long file_size = 2LL * ram_size;

    // Tamanhos de bloco
    int block_sizes[NUM_BLOCK_SIZES] = {1, 1024, 32768, 1048576};

    const char *filename = "testfile.bin";

    // Cria arquivo grande em chunks de 1 MB
    const size_t CHUNK_SIZE = 1024 * 1024;
    char *chunk_buffer = (char *)malloc(CHUNK_SIZE);
    if (!chunk_buffer) {
        perror("Erro ao alocar chunk_buffer");
        return 1;
    }

    // Preenche chunk com dados aleatórios
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < CHUNK_SIZE; i++) {
        chunk_buffer[i] = (char)(rand() % 256);
    }

    printf("Criando arquivo de teste com ~%lld bytes...\n", file_size);

    int fd_test = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_test < 0) {
        perror("Erro ao criar testfile.bin");
        free(chunk_buffer);
        return 1;
    }

    long long bytes_escritos = 0;
    while (bytes_escritos < file_size) {
        long long faltam = file_size - bytes_escritos;
        size_t to_write = (faltam > CHUNK_SIZE) ? CHUNK_SIZE : (size_t)faltam;
        ssize_t ret = write(fd_test, chunk_buffer, to_write);
        if (ret < 0) {
            perror("Erro escrevendo no arquivo");
            close(fd_test);
            free(chunk_buffer);
            return 1;
        }
        bytes_escritos += ret;
    }
    close(fd_test);
    free(chunk_buffer);

    printf("Arquivo '%s' criado.\n\n", filename);

    // Abre CSV
    FILE *output = fopen("results.csv", "w");
    if (!output) {
        perror("Erro ao abrir results.csv");
        return 1;
    }
    fprintf(output, "BlockSize,SeqRead(s),RandRead(s),SeqWrite(s),RandWrite(s),"
                    "SeqRead(MB/s),RandRead(MB/s),SeqWrite(MB/s),RandWrite(MB/s)\n");

    printf("========================================================================\n");
    printf("BlockSize |  SeqRead(s)  RandRead(s)  SeqWrite(s)  RandWrite(s)  [MB/s]\n");
    printf("========================================================================\n");

    // Teste para cada tamanho de bloco
    for (int i = 0; i < NUM_BLOCK_SIZES; i++) {
        int bs = block_sizes[i];

        // Numero (ideal) de reqs = file_size / bs
        long long num_requests = file_size / bs;
        // Limite para não travar
        if (num_requests > MAX_REQUESTS) {
            num_requests = MAX_REQUESTS;
        }

        if (num_requests <= 0) {
            printf("Bloco %d grande demais ou arquivo inexistente.\n", bs);
            continue;
        }

        double seq_read_time   = 0.0;
        double rand_read_time  = 0.0;
        double seq_write_time  = 0.0;
        double rand_write_time = 0.0;

        // ---------------- LEITURA SEQUENCIAL ----------------
        {
            double start = get_time_sec();
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                perror("Erro ao abrir para leitura seq");
            } else {
                char *buf = (char *)malloc(bs);
                if (buf) {
                    for (long long r = 0; r < num_requests; r++) {
                        ssize_t rsz = read(fd, buf, bs);
                        if (rsz < 0) {
                            perror("Erro lendo seq");
                            break;
                        }
                        if (rsz < bs) {
                            // chegou ao fim cedo
                            break;
                        }
                    }
                    free(buf);
                }
                close(fd);
            }
            double end = get_time_sec();
            seq_read_time = end - start;
        }

        // ---------------- LEITURA ALEATÓRIA ----------------
        {
            double start = get_time_sec();
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                perror("Erro ao abrir para leitura rand");
            } else {
#ifdef _WIN32
                long long fsize = _lseeki64(fd, 0, SEEK_END);
                _lseeki64(fd, 0, SEEK_SET);
#else
                off_t fsize = lseek(fd, 0, SEEK_END);
                lseek(fd, 0, SEEK_SET);
#endif
                if (fsize > 0 && (long long)fsize >= bs) {
                    char *buf2 = (char *)malloc(bs);
                    if (buf2) {
                        for (long long r = 0; r < num_requests; r++) {
                            double fraction = (double)rand() / RAND_MAX;
                            long long range = (long long)(fsize - bs + 1);
                            if (range < 1) range = 1;
                            long long offset = (long long)(fraction * range);

#ifdef _WIN32
                            _lseeki64(fd, offset, SEEK_SET);
#else
                            lseek(fd, offset, SEEK_SET);
#endif
                            read(fd, buf2, bs);
                        }
                        free(buf2);
                    }
                }
                close(fd);
            }
            double end = get_time_sec();
            rand_read_time = end - start;
        }

        // ---------------- ESCRITA SEQUENCIAL ----------------
        {
            double start = get_time_sec();
            int fd = open("temp_seq_write.bin", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd < 0) {
                perror("Erro ao criar temp_seq_write.bin");
            } else {
                char *buf3 = (char *)malloc(bs);
                if (buf3) {
                    memset(buf3, 'X', bs);
                    for (long long r = 0; r < num_requests; r++) {
                        write(fd, buf3, bs);
                    }
                    free(buf3);
                }
                close(fd);
            }
            double end = get_time_sec();
            seq_write_time = end - start;
        }

        // ---------------- ESCRITA ALEATÓRIA ----------------
        {
            double start = get_time_sec();
            long long test_size = bs * num_requests;

            // Cria arquivo do tamanho desejado
            {
                int fd = open("temp_rand_write.bin", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                if (fd >= 0) {
#ifdef _WIN32
                    _lseeki64(fd, test_size - 1, SEEK_SET);
#else
                    lseek(fd, test_size - 1, SEEK_SET);
#endif
                    write(fd, "", 1); 
                    close(fd);
                }
            }

            // Agora faz escrita aleatória
            {
                int fd = open("temp_rand_write.bin", O_WRONLY);
                if (fd < 0) {
                    perror("Erro ao abrir temp_rand_write.bin");
                } else {
                    char *buf4 = (char *)malloc(bs);
                    if (buf4) {
                        memset(buf4, 'Y', bs);
                        for (long long r = 0; r < num_requests; r++) {
                            double fraction = (double)rand() / RAND_MAX;
                            long long range = test_size - bs + 1;
                            if (range < 1) range = 1;
                            long long offset = (long long)(fraction * range);

#ifdef _WIN32
                            _lseeki64(fd, offset, SEEK_SET);
#else
                            lseek(fd, offset, SEEK_SET);
#endif
                            write(fd, buf4, bs);
                        }
                        free(buf4);
                    }
                    close(fd);
                }
            }
            double end = get_time_sec();
            rand_write_time = end - start;
        }

        // ---------------- Cálculo de throughput (MB/s) ----------------
        double total_data_mb = (double)num_requests * (double)bs / MB;

        // Se tempo <= 0, define 0.000001 para evitar dividir por zero
        if (seq_read_time < 1e-9)   seq_read_time   = 1e-9;
        if (rand_read_time < 1e-9)  rand_read_time  = 1e-9;
        if (seq_write_time < 1e-9)  seq_write_time  = 1e-9;
        if (rand_write_time < 1e-9) rand_write_time = 1e-9;

        double seq_read_bw   = total_data_mb / seq_read_time;
        double rand_read_bw  = total_data_mb / rand_read_time;
        double seq_write_bw  = total_data_mb / seq_write_time;
        double rand_write_bw = total_data_mb / rand_write_time;

        // Impressão
        printf("%9d | %10.3f | %10.3f | %10.3f | %10.3f |  [%.2f / %.2f / %.2f / %.2f]\n",
               bs, seq_read_time, rand_read_time, seq_write_time, rand_write_time,
               seq_read_bw, rand_read_bw, seq_write_bw, rand_write_bw);

        // Salva no CSV
        fprintf(output, "%d,%.6f,%.6f,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f\n",
                bs,
                seq_read_time,
                rand_read_time,
                seq_write_time,
                rand_write_time,
                seq_read_bw,
                rand_read_bw,
                seq_write_bw,
                rand_write_bw
               );
        fflush(output); // força a gravação imediato
    }

    fclose(output);
    printf("\nTeste concluido. Resultados em 'results.csv'.\n");

    // Remove arquivos temporários
    remove("temp_seq_write.bin");
    remove("temp_rand_write.bin");
    remove("testfile.bin");

    return 0;
}
