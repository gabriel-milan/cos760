# O que eu quero testar

- Tempo do algoritmo sequencial
- Tempo do OpenMP variando o número de threads para 2, 4, 8, 24 e 48
- Tempo do MPI variando o número de processos para 2, 4, 8, 16
  - Em 16 processos, variar o número de nós entre 1, 2 e 4

# Como testar

- Executar todos os programas pelo menos 3 vezes
- As máquinas tem 24 núcleos físicos e 48 threads, devo fazer o melhor uso possível disso

# Submissões

### 01.pbs

- 3 execuções do algoritmo sequencial (ocuparia 3, faltariam 45)
- 3 execuções do OpenMP com 2 threads (ocuparia 6, faltariam 39)
- 3 execuções do MPI com 2 processos (ocuparia 6, faltariam 33)
- 3 execuções do OpenMP com 4 threads (ocuparia 12, faltariam 21)
- 3 execuções do MPI com 4 processos (ocuparia 12, faltariam 9)
- 1 execução do OpenMP com 8 threads (ocuparia 8, faltariam 1)

### 02.pbs

- 2 execuções do OpenMP com 8 threads (ocuparia 8, faltariam 40)
- 3 execuções do MPI com 8 processos (ocuparia 24, faltariam 16)
- 1 execução do MPI com 16 processos (ocuparia 16, faltariam 0)

### 03.pbs

- 2 execuções do MPI com 16 processos (ocuparia 32, faltariam 16)

### 04.pbs

- 3 execuções do MPI com 16 processos em 2 máquinas (ocuparia 24 em cada, faltariam 24 em cada)

### 05.pbs

- 3 execuções do MPI com 16 processos em 4 máquinas (ocuparia 12 em cada, faltariam 36 em cada)

### 06.pbs

- 2 execuções do OpenMP com 24 threads (ocuparia 48, faltariam 0)

### 07.pbs

- 1 execução do OpenMP com 24 threads (ocuparia 24, faltariam 24)

### 08.pbs, 09.pbs, 10.pbs

- 1 execução do OpenMP com 48 threads (ocuparia 48, faltariam 0)
