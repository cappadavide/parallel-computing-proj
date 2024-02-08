#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h" 

void firstStrategy(int nproc, int menum, int processorID, int nloc, double *xloc);
void secondStrategy(int nproc, int menum, int processorID, double logarithm, int nloc, double *xloc);
void thirdStrategy(int nproc, int menum, int processorID, double logarithm, int nloc, double *xloc);
double pseudorand(double max);
long* computePowersOfTwo(int nproc);

int main(int argc, char *argv[]){

	int menum=0, nproc=0, numbers=0, strategyNumber=0, processorID=0, indice=0, nloc=0, tag=0, rest=0, tmp=0, start=0, i=0, count=0;
	double *numeriInput=NULL, *xloc=NULL;
	MPI_Status status;

	/*
	Inizializza l'ambiente di esecuzione MPI
	Inizializza il comunicator MPI_COMMON_WORLD
	I due dati di input: argc e argv sono argomenti del main
	Questa routine inizializza l'ambiente di esecuzione MPI
	Deve essere chiamata una sola volta, prima di ogni altra routine MPI.
	Definisce l'insiemte dei processori attivati(communicator)
	*/
	MPI_Init(&argc,&argv);

	/*
	Questa routine permette al processore chiamante, appartenente al communicator
	MPI_COMM_WORLD, di memorizzare il proprio identificativo nella variabile menum.
	Fornisce ad ogni processore del communcator comm l'identificativo menum.
	*/
	MPI_Comm_rank(MPI_COMM_WORLD,&menum);

	/*
	Questa routine permette al processore chiamante di memorizzare nella variabile
	nproc il numero totale di processoi concorrenti appartenenti al
	communicator MPI_COMM_WORLD
	Ad ogni processore del communicator comm, restituisce in nproc il numero
	totale di processori che costituiscono comm.
	Permette di conoscere quanti processori concorrenti possono essere
	utilizzati per una determinata operazione.
	*/
	MPI_Comm_size(MPI_COMM_WORLD,&nproc);
	double logarithm = log10(nproc)/log10(2);

	numbers = atoi(argv[1]); 
	strategyNumber = atoi(argv[2]);
	processorID = atoi(argv[3]);

	if( numbers < 0 ){
		
		printf("Primo argomento non valido. Terminazione programma...\n");
		MPI_Finalize();
		return 0;
	}

	if ( strategyNumber < 0 || strategyNumber > 3){
		
		printf("Secondo argomento non valido. Terminazione programma...\n");
		MPI_Finalize();
		return 0;
	}

	if( processorID < -1 ){
		
		printf("Terzo argomento non valido. Terminazione programma...\n");
		MPI_Finalize();
		return 0;
	}

	if( menum == 0 ){
		
		
		numeriInput = (double *)malloc(sizeof(double)*numbers);
		
		if( numbers > 20 ) {
		  
		  while( indice < numbers ){
			
			numeriInput[indice] = pseudorand(100); 
			indice++;
		  }
		}
		else{
			
			while( indice < numbers ){
				
				numeriInput[indice] = (double)atoi(argv[indice+4]);
				count++;
				indice++;
			}
			
			if( count != numbers ){
		
				printf("Numero di argomenti da sommare non coincidente con primo argomento. Terminazione programma...\n");
				MPI_Finalize();
				return 0;
			}
		}
	}

	/*LETTURA E DISTRIBUZIONE DATI*/

	//MPI_Bcast invia a tutti i processori il primo argomento, cio� numbers sul communicator MPI_COMM_WORLD
	MPI_Bcast(&numbers,1,MPI_DOUBLE,0,MPI_COMM_WORLD);

	nloc = numbers/nproc; //numeri di addendi che ogni processore deve manipolare
	rest = numbers%nproc;

	/*
	se ci sono addendi rimanenti non distribuiti, il numero totale associato al processore
	viene incrementato di 1
	*/
	if( menum < rest ) nloc = nloc+1;

	if(menum != 0 ) xloc = (double *)malloc(sizeof(double)*nloc); //allocazione del vettore di appoggio xloc

	/*
	Distribuzione dei dati tramite MPI_Send
	*/
	if( menum == 0 ){ 
		
		xloc = numeriInput;
		tmp = nloc;
		start = 0;
		
		for( i=1; i<nproc; i++ ){
			
			start = start+tmp;
			tag = 22+i;
			if( i == rest ) tmp--;
			MPI_Send(&numeriInput[start],tmp,MPI_DOUBLE,i,tag,MPI_COMM_WORLD);
		}
	}

	/*
	Ricezione dei dati tramite MPI_Recv
	*/
	else{
		
		tag = 22+menum;
		MPI_Recv(xloc,nloc,MPI_DOUBLE,0,tag,MPI_COMM_WORLD,&status);
	}

	/*
	Se log2(nproc) non e' un numero intero e viene scelta la strategia II o III, allora viene utilizzata la I
	*/
	if ( strategyNumber == 2 && floor(logarithm) == ceil(logarithm) ){
		
		printf("Ho eseguito la seconda strategia.\n");			
		secondStrategy(nproc, menum, processorID, logarithm, nloc, xloc);
	}
	else if ( strategyNumber == 3 && floor(logarithm) == ceil(logarithm) ){
		
		printf("Ho eseguito la terza strategia.\n");			
		thirdStrategy(nproc, menum, processorID, logarithm, nloc, xloc);
	}
	else if( strategyNumber == 1 || floor(logarithm) != ceil(logarithm) ){
		
		printf("Ho eseguito la prima strategia.\n");
		firstStrategy(nproc, menum, processorID, nloc, xloc);
	}

	/*
	Questa routine determina la fine del programma MPI.
	Dopo questa routine non � possibile richiamare nessun'altra routine di MPI.
	*/
	MPI_Finalize();
	return 0;
}

/*
Genera numeri casuali double compresi tra -max e max.
*/
double pseudorand(double max){
	
	 srand((unsigned)time(0));
	 return( rand() > RAND_MAX / 2 ? -1 : 1 ) * (max/RAND_MAX) * rand();
}

/*
Ogni processore calcola la propria somma parziale.
Ad ogni passo, ciascun processore invia tale somma ad un unico processore
prestabilito che conterr� la somma totale.
*/
void firstStrategy(int nproc, int menum, int processorID, int nloc, double *xloc){

	double sum_parz=0, t1=0, myTime=0, timeTot=0,t0=0,sum=0;
	int tag=0, i=0;
	MPI_Status status;
	
	/*
	Sincronizza tutti i processori per calcolare i tempi di calcolo della somma
	*/	
	MPI_Barrier(MPI_COMM_WORLD);
	t0=MPI_Wtime(); //calcolo del tempo di inizio, uguale per tutti per via della MPI_Barrier
	
	/*
	Ogni processore somma gli addendi che ha a disposizione
	*/
	for( i=0; i<nloc; i++ ) sum = sum+xloc[i];

	if( menum == 0 ){
		
		for( i=1; i<nproc; i++ ){
			
			tag = 80+i;
			MPI_Recv(&sum_parz,1,MPI_DOUBLE,i,tag,MPI_COMM_WORLD,&status);
			sum = sum+sum_parz;
		}
	}
	else{
	
		tag = menum+80;
		MPI_Send(&sum,1,MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
	}
	
	t1=MPI_Wtime(); //calcolo del tempo finale
	myTime=t1-t0; //calcolo del tempo effettivo
	
	if( processorID > -1 ){
		
		if( menum == processorID) printf("Sono il processore [%d] - la somma e' : %.2lf - ho impiegato %lf secondi\n", processorID, sum, myTime);
	}
	else if( processorID == -1 && menum != 0 ) printf("Sono il processore [%d] - la somma e' : %.2lf - ho impiegato %lf secondi\n", menum, sum, myTime);
	
	/*
	Calcola il tempo totale impiegato dall'algoritmo nell'effettuare la somma parallelizzata
	*/
	MPI_Reduce(&myTime,&timeTot,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
	if( menum == 0 ) printf("Sono il processore [%d] - la somma totale e' %.2lf - il tempo totale e' %lf secondi\n", menum, sum, timeTot);
}

/*
Ogni processore calcola la propria somma parziale.
Ad ogni passo, coppie distinte di processori comunicano contemporaneamente.
In ogni coppia, un processore invia all'altro la propria somma parziale per
poi aggiornare la somma. Il risultato � in un unico processore prestabilito.
*/
void secondStrategy(int nproc, int menum, int processorID, double logarithm, int nloc, double *xloc){

	double sum_parz=0, t1=0, myTime=0, timeTot=0,t0=0,sum=0;
	long *powers=NULL;
	int tag=0, i=0, power=0;
	MPI_Status status;
	
	/*
	Sincronizza tutti i processori per calcolare i tempi di calcolo della somma
	*/
	powers=computePowersOfTwo(nproc);
	
	MPI_Barrier(MPI_COMM_WORLD);
	t0=MPI_Wtime(); //calcolo del tempo di inizio, uguale per tutti per via della MPI_Barrier

	/*
	Ogni processore somma gli addendi che ha a disposizione
	*/
	for( i=0; i<nloc; i++ ) sum = sum+xloc[i];

	for( i=0; i<(int)logarithm; i++ ){ //passi di comunicazione

		if( menum%(int)powers[i] == 0 ){ //chi partecipa alla comunicazione
			
			tag = 80+i;
			
			if( menum%((int)powers[i+1]) == 0 ){ //chi riceve
				
				MPI_Recv(&sum_parz,1,MPI_DOUBLE,menum+powers[i],tag,MPI_COMM_WORLD,&status);
				
			}
			else{ //chi spedisce

				MPI_Send(&sum,1,MPI_DOUBLE,menum-powers[i],tag,MPI_COMM_WORLD);
			}
			sum = sum+sum_parz;
		}
	}

	t1=MPI_Wtime();
	myTime=t1-t0;
	
	if( processorID > -1 ){
		
		if( menum == processorID) printf("Sono il processore [%d] - la somma e' : %.2lf - ho impiegato %lf secondi\n", processorID, sum, myTime);
	}
	else if( processorID == -1 && menum != 0 ) printf("Sono il processore [%d] - la somma e' : %.2lf - ho impiegato %lf secondi\n", menum, sum, myTime);
	
	MPI_Reduce(&myTime,&timeTot,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
	if( menum == 0 ) printf("Sono il processore [%d] - la somma totale e' %.2lf - il tempo totale e' %lf secondi\n", menum, sum, timeTot);
}

/*
Ogni processore calcola la propria somma parziale.
Ad ogni passo, coppie distinte di processori comunicano contemporaneamente.
In ogni coppia, i processori si scambiano le proprie somme parziali.
Il risultato e' in tutti i processori.
*/
void thirdStrategy(int nproc, int menum, int processorID, double logarithm, int nloc, double *xloc){
	
	double sum_parz=0, t1=0, myTime=0, timeTot=0,sum=0,t0=0;
	long* powers=NULL;
	int tag=0, i=0, power=0;
	MPI_Status status;

	/*
	Sincronizza tutti i processori per calcolare i tempi di calcolo della somma
	*/
	powers=computePowersOfTwo(nproc);
	
	MPI_Barrier(MPI_COMM_WORLD);
	t0=MPI_Wtime(); //calcolo del tempo di inizio, uguale per tutti per via della MPI_Barrier

	/*
	Ogni processore somma gli addendi che ha a disposizione
	*/
	for( i=0; i<nloc; i++ ) sum = sum+xloc[i];
	
	//tutti partecipano alla comunicazione
	for( i=0; i<(int)logarithm; i++ ) { //passi di comunicazione
		
		tag = 80+i;
		//si decide solo a chi si invia e da chi si riceve
		if( menum%(int)powers[i+1] < (int)powers[i] ){
			
			MPI_Send(&sum,1,MPI_DOUBLE,menum+powers[i],tag,MPI_COMM_WORLD);
			MPI_Recv(&sum_parz,1,MPI_DOUBLE,menum+powers[i],tag,MPI_COMM_WORLD,&status);
		}
		else{
			
			MPI_Send(&sum,1,MPI_DOUBLE,menum-powers[i],tag,MPI_COMM_WORLD);
			MPI_Recv(&sum_parz,1,MPI_DOUBLE,menum-powers[i],tag,MPI_COMM_WORLD,&status);
		}
		sum = sum+sum_parz;
	}
		
	t1=MPI_Wtime();
	myTime=t1-t0;
	
	if( processorID > -1 ){
		
		if( menum == processorID) printf("Sono il processore [%d] - la somma e' : %.2lf - ho impiegato %lf secondi\n", processorID, sum, myTime);
	}
	else if( processorID == -1 && menum != 0 ) printf("Sono il processore [%d] - la somma e' : %.2lf - ho impiegato %lf secondi\n", menum, sum, myTime);
	
	MPI_Reduce(&myTime,&timeTot,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
	if( menum == 0 ) printf("Sono il processore [%d] - la somma totale e' %.2lf - il tempo totale e' %lf secondi\n", menum, sum, timeTot);
}


long* computePowersOfTwo(int nproc){
	
	long* powers = (long*)malloc((nproc + 1)*sizeof(long));
	int i = 0;
	
	for( i=0;i<=nproc; i++) powers[i] = pow(2,i);
	return powers;
}