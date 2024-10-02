# O que eu quero testar

- Tempo do algoritmo sequencial
- Tempo do OpenMP variando o número de threads para 2, 4, 8 e 16
- Tempo do MPI variando o número de processos para 2, 4, 8, 16
  - Em 16 processos, variar o número de nós entre 1, 2 e 4

# Como testar

- Executar todos os programas pelo menos 3 vezes
- As máquinas tem 24 núcleos físicos, devo fazer o melhor uso possível disso

# Submissões

### 01.pbs

- 3 execuções do algoritmo sequencial (ocuparia 3, faltariam 21)
- 3 execuções do OpenMP com 2 threads (ocuparia 6, faltariam 15)
- 3 execuções do MPI com 2 processos (ocuparia 6, faltariam 9)
- 2 execuções do OpenMP com 4 threads (ocuparia 8, faltariam 1)

### 02.pbs

- 1 execução do OpenMP com 4 threads (ocuparia 4, faltariam 20)
- 3 execuções do MPI com 4 processos (ocuparia 12, faltariam 8)
- 1 execução do OpenMP com 8 threads (ocuparia 8, faltariam 0)

### 03.pbs

- 2 execuções do OpenMP com 8 threads (ocuparia 16, faltariam 8)
- 1 execução do MPI com 8 processos (ocuparia 8, faltariam 0)

### 04.pbs

- 2 execuções do MPI com 8 processos (ocuparia 16, faltariam 8)

### 05.pbs, 06.pbs, 07.pbs

- 1 execução do MPI com 16 processos (ocuparia 16, faltariam 8)

### 08.pbs, 09.pbs, 10.pbs

- 1 execução do OpenMP com 16 threads (ocuparia 16, faltariam 8)

### 11.pbs

- 3 execuções do MPI com 16 processos em 2 máquinas (ocuparia 24 em cada, faltariam 0 em cada)

### 12.pbs

- 3 execuções do MPI com 16 processos em 4 máquinas (ocuparia 12 em cada, faltariam 12 em cada)
