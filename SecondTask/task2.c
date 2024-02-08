#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "omp.h"

void prodottoMatriceVettore(int righe, int colonne, int **matrix, int *vector, int *vectorFinal, int threads);
void creaMatrice(int righe, int colonne, int **matrix);
void creaVettore(int dimensione, int *vettore);
void stampaMatrice(int righe, int colonne, int **matrix);
void stampaVettore(int dimensione, int *vettore, int flag);

int main(int argc, char **argv){

	if ( argc!= 4 ){
		
		printf("Argomenti non sufficienti!\n");
		return 0;
	}
	
	int threads=atoi(argv[1]), righe=atoi(argv[2]), colonne=atoi(argv[3]);
	int *vector, *vectorFinal, **matrix;
	
	//alloco il vettore
	vettore = (int*)calloc(colonne,sizeof(int));
	
	//alloco il vettore finale
	vectorFinal = (int*)calloc(righe,sizeof(int));
	
	//alloco la matrice
	matrix = (int**)calloc(righe,sizeof(int *));
	
	for ( index1 = 0; index1<righe; index1++ ) matrix[index1] = (int* )calloc(colonne,sizeof(int)); //allocazione completa matrice
	
	creaMatrice(righe,colonne,matrix);
	creaVettore(colonne,vector);
	
	stampaMatrice(righe,colonne,matrix);
	stampaVettore(colonne,vector,0);
	
	prodottoMatriceVettore(righe,colonne,matrix,vector,vectorFinal,threads);
	
	return 0;

}

void prodottoMatriceVettore(int righe, int colonne, int **matrix, int *vector, int *vectorFinal, int threads){

	int index1=0, index2=0, index=0;
	double inizio=0, fine=0;
	struct timeval time;
	
	//prendo i tempi prima della regione parallela
	gettimeofday(&time,NULL);
	inizio = time.tv_sec + (time.tv_usec/1000000.0);

	//direttive
	#pragma omp parallel for num_threads(threads) default(none) shared(righe,colonne,vector,matrix,vectorFinal) private(index1,index2)
	for ( index1=0; index1<righe; index1++ ){
		
		for ( index2=0; index2<colonne; index2++ ){
			
			vectorFinal[index1]+=matrix[index1][index2]*vector[index2];
		}
	}
	
	//tempo calcolato dopo l'esecuzione del prodotto
	gettimeofday(&time,NULL);
	fine = time.tv_sec + (time.tv_usec/1000000.0);
	
	printf("TEMPO IMPIEGATO: %e \n", fine-inizio);
	stampaVettore(righe,vectorFinal,1);	
}

//stampo i vettori
void stampaVettore(int dimensione, int *vettore, int flag){
	
	int index=0;
	
	if( flag == 1 ) printf("VETTORE RISULTANTE\n");
	else printf("VETTORE\n");
	
	for( index=0; index<dimensione; index++ ) printf("[%d]", vettore[index]);
	printf("\n");
	
}

//stampo la matrice
void stampaMatrice(int righe, int colonne, int **matrix){

	int index1=0, index2=0;
	
	printf("MATRICE\n");
	for( index1=0; index1<righe; index1++ ){
		
		for ( index2=0; index2<colonne; index2++ ){
			
				printf("[%d]", matrix[index1][index2]);
		}
		printf("\n");
	}
}
//creo vettore moltiplicativo con interi casuali
void creaVettore(int dimensione, int *vettore){
	
	int index=0;
	
	for ( index=0; index<dimensione; index++ ) vettore[index]=rand()%50;
	
}

//creo la matrice con interi casuali
void creaMatrice(int righe, int colonne, int **matrix){
	
	int index1=0, index2=0;

	for( index1=0; index1<righe; index1++ ){
		
		for ( index2=0; index2<colonne; index2++ ){
			
				matrix[index1][index2]=rand()%50;
		}
	}
}