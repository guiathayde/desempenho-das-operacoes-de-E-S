Titulo: Desempenho das operações de E/S

Objetivo: criar um programa benchmark para avaliar o desempenho das operações de leitura (e escrita) em arquivos, usando diferentes tamanhos de requisição, em acessos sequenciais e aleatórios.

Programa de teste de desempenho (benchmark) para arquivos

De maneira simplificada, um sistema de arquivos é uma organização provida pelo sistema operacional para o armazenamento de conjuntos de informações de forma não volátil. Para tanto, um sistema de arquivo trata de como utilizar os blocos de armazenamento providos pelos discos para o armazenamento de programas e dados, organizados na forma de uma hierarquia de diretórios e arquivos.
Diferentes atributos e informações de controle podem estar associados aos arquivos e diferentes mecanismos podem ser utilizados para identificar os blocos do disco que contêm seus dados.
Considerando o uso de chamadas de sistema padronizadas para o acesso aos arquivos pelas aplicações, cabe ao SO traduzir essas chamadas em comandos apropriados para os controladores de disco. Também cabe ao SO compatibilizar os tamanhos das requisições feitas com os tamanhos dos blocos de armazenamento que podem ser lidos ou escritos no discos, limitados a esse tipo de transferência.
Assim, a eficiência de um sistema de arquivos depende de diversos fatores, que vão da distribuição física dos blocos de dados de um arquivo em disco, passando pelas estruturas de controle usadas para identificação dos blocos de um arquivo, e chegando aos mecanismos de transferência e armazenamento intermediário providos pelo SO.
O padrão de acesso aos dados também é relevante no acesso a um arquivo. Acessos sequenciais podem ser beneficiados por mecanismos de busca antecipada (prefetching), enquanto acessos aleatórios podem favorecer-se de mecanismos de cache.
Para avaliar a eficiência de sistemas de arquivo, diferentes estudos e programas de teste de desempenho (benchmarks) têm sido criados ao longo do tempo, como pode ser visto em [1]. Uma discussão sobre diferentes aspectos que podem ser avaliados por um benchmark de sistema de arquivos pode ser vista em [2].

Atividade

Criar um programa benchmark simplificado para  sistema de arquivos, que analise o desempenho de diferentes operações:

leitura sequencial com read
leitura aleatória com lseek + read() ou pread()
escrita sequencial com write
escrita aleatória com lseek + write ou pwrite()
Para avaliar o efeito do tamanho dos blocos dos arquivos em disco, o programa deve permitir especificar diferentes tamanhos de requisição, tanto para leitura quanto para escrita (e.g.  1 a 1Mbytes (2^0 a 2^20).
Para tentar evitar que o uso de caches do  sistema de arquivos mascare os resultados, os arquivos utilizados nos testes devem ter ao menos o dobro do tamanho da memória. Assim, uma estratégia poderia ser calcular o número de requisições de forma que NumReq * TamReq >= 2 * RAM_size.
Para testar as operações de leitura é possível criar um arquivo com o tamanho desejado. Em sistemas Linux, isso pode ser feito com o comando dd. No exemplo abaixo, cria-se um arquivo
$ dd if=/dev/zero of=...nome_do_arquivo... bs=4096 count=xxx

Vale saber que, em testes com medições de desempenho, é comum realizar várias avaliações para cada caso, procurando certificar-se que os resultados obtidos são consistentes.

Referências
[1] File and Storage System Benchmarking Portal. http://http://fsbench.filesystems.org.
[2] Tarasov, Vasily, et al. "Benchmarking file system benchmarking: It* is* rocket science." HotOS XIII (2011).