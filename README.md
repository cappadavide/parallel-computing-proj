# parallel-computing-proj
This project is based on implementing algorithms for an environment parallel computing on MIMD Architecture distributed memory, using the MPI Library.
In particular, it is divided into 3 parts, each of which differs in difficulty.


## First task

## Second task
The problem addressed in this document is as follows:

### Calculation of the product between a matrix $M \in \mathbb{R}^{nxm}$ and a vector $x \in \mathbb{R}^{m}$  on a parallel computer of the MIMD type with p processors and shared memory.

The task involves calculating the sum of N numbers (a0+a1+ … + an) on a parallel computer of MIMD type with p processors and distributed memory.

On a single-processor computer, the sum is computed by performing N-1 additions one at a time in a predetermined order:

```plaintext
for i := 0 to n-1 do
  yi := 0
  for j := 0 to n-1 do
    yi := yi + aij * xj
  endfor
endfor
```
where $y \in \mathbb{R}^{n}$ is the result vector

The shared memory, facilitated through the use of threads, undoubtedly streamlines the resolution of the problem, as there is no need to distribute data through explicit communication mechanisms as is the case with separate processes. However, it is imperative to employ synchronization mechanisms to prevent unexpected results or issues. 
To address the problem, OpenMP (Open MultiProcessing) was employed—a cross-platform API designed for creating parallel applications on shared memory systems. It is supported by various programming languages such as C/C++ and Fortran. OpenMP comprises a set of compilation directives, library routines, and environment variables that define its runtime behavior.

## Third task
The problem addressed in this document is as follows:

### Calculation of the product between a matrix  $A \in \mathbb{R}^{nxn}$ and $B \in \mathbb{R}^{nxn}$ on a parallel computer of the MIMD type with p processors and distributed memory.

The sequential algorithm for solving this problem is as follows:

```plaintext
for i=0,n-1 do
  for j=0,n-1 do
    cij = 0
    for k=0,n-1 do
      cij = cij + aik * bkj
    endfor
  endfor
endfor
```
Thanks to a parallel algorithm, it becomes possible to decompose the problem of size N into P-independent subproblems of size N/P and solve them simultaneously on multiple computers.

One approach is to decompose the problem into square blocks, combining the decomposition into row blocks with that into column blocks.

\\foto: Suddivisione matrice 4x4 in blocchi 2x2 da distribuire a 4 processi

This type of decomposition is utilized by the Fox algorithm, also known as Broadcast-Multiply-Roll, which will be detailed in the following sections.

The algorithm is founded on the concept that goes beyond mere decomposition into square blocks. It also entails a virtual arrangement of processes within a two-dimensional grid to facilitate the resolution of the given problem.

//foto: Distribuzione dei dati

The grid created for the given problem has a dimension equal to (p, p), where p represents the number of processes. It incorporates periodicity along both dimensions to facilitate smoother communication between processes located at opposite positions.

Each process is then assigned coordinates to determine its position within the grid, establishing an intrinsic connection with the problem at hand. Specifically, processor 𝑃_ij will be assigned the blocks 𝐴_ij and 𝐵_ij.

The desired condition is for each processor 𝑃_ij to compute the block 𝐶_ij of the resulting matrix C. However, with the currently possessed blocks, processor 𝑃_ij cannot calculate this particular block of C.

The proposed approach is as follows:
-  Identify the diagonal (starting from the main diagonal).
-  Conduct a horizontal broadcast of the diagonal block of A, wherein each processor transmits its A block to other processors in the same row (Broadcast).
-  Multiply the copied block with the B block already in possession of each processor (Multiply).
-  Send the current block of B to the processor immediately positioned above the current one within the grid (Rolling).
-  Perform another horizontal broadcast of the blocks above the main diagonal.
-  Multiply the new A block with the current B block.

This algorithm needs to be iterated sqrt(number of blocks) times.

\\foto: Distribuzione matrice 6x6
