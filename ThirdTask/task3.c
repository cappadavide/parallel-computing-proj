#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

void getSuperiore(int *superiore, int *coordinate);
void getInferiore(int *inferiore, int *coordinate);
void creaMatrici(int dimensione, int *a, int *b);
void stampaMatrice(int *matrix, int dimensione, int flag);
void calcolaProdotto(int *a, int *b, int *c, int dimensione);
void creaGriglia(MPI_Comm *griglia, MPI_Comm *grigliar, int dimensione);
void distribuisciSubMatrix(int *a, int* localA, int dimensione, int dimensione_griglia);
void ricostruisciMatrice(int *c, int *localC, int dimensione_sub, int dimensione_griglia, int dimensione);

int main(int argc, char *argv[]){
	
	srand(time(NULL));
    int nproc=0, menum=0, dimensione=0, sup=0, inf=0, superiore[2], inferiore[2], *tmpMatrix, radiceBcast=0, index=0;
	double t0=0, t1=0, myTime=0, timeTot=0;
    MPI_Comm grigliar, griglia;
	MPI_Status status;
	
    if(argc != 2){
        printf("Numero di argomenti non sufficiente!\n");
        return 0;
    }

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD,&nproc);
    MPI_Comm_rank(MPI_COMM_WORLD,&menum);
	
	dimensione=atoi(argv[1]);
    int *coordinate = (int*)malloc(sizeof(int)*2);
	int dimensione_griglia = sqrt(nproc);
    
    if(dimensione % nproc != 0){
		
        printf("La quantità di numeri deve essere multiplo di %d\n", nproc);
        return 0;
    }
    
    if(dimensione_griglia*dimensione_griglia != nproc){
		
        printf("Il numero di processi non è un quadrato perfetto\n");
        return 0;
    } 

	int dimensione_sub = dimensione/(dimensione_griglia);
    
    /* Inizializzazione blocchi locali*/

    int *localA=(int *)calloc(dimensione_sub*dimensione_sub, sizeof(int)); 
    int *localB=(int *)calloc(dimensione_sub*dimensione_sub, sizeof(int));
    int *localC=(int *)calloc(dimensione_sub*dimensione_sub, sizeof(int));
	
	int *a=(int *)calloc(dimensione*dimensione, sizeof(int));
	int *b=(int *)calloc(dimensione*dimensione, sizeof(int));
	int *c=(int *)calloc(dimensione*dimensione, sizeof(int));
	
    if (menum == 0) creaMatrici(dimensione,a,b);

	distribuisciSubMatrix(a,localA,dimensione,dimensione_griglia);
	distribuisciSubMatrix(b,localB,dimensione,dimensione_griglia);
    
    creaGriglia(&griglia,&grigliar,dimensione_griglia);
	
	MPI_Barrier(MPI_COMM_WORLD);
	
    MPI_Cart_coords(griglia,menum,2,coordinate);   //ricava le coordinate per il processo corrente

    getInferiore(inferiore,coordinate);
    getSuperiore(superiore,coordinate);
    
    MPI_Cart_rank(griglia,superiore,&sup); //ricava l'id del processo in base alle coordinate indicate
    MPI_Cart_rank(griglia,inferiore,&inf);

    tmpMatrix=(int*)calloc(dimensione_sub*dimensione_sub, sizeof(int));
	
	t0=MPI_Wtime(); //calcolo tempo iniziale
	
    for(index=0; index<dimensione_griglia; index++){
		
        radiceBcast = (coordinate[0]+index)%dimensione_griglia;  //Calcolo del process id che deve inviare il proprio blocco agli altri processi della riga
		
        if(radiceBcast == coordinate[1]){
			
            MPI_Bcast(localA,dimensione_sub*dimensione_sub,MPI_INT,radiceBcast,grigliar); //Invio blocco ai processi della riga
            calcolaProdotto(localA,localB,localC,dimensione_sub);
        }
        else{
			
            MPI_Bcast(tmpMatrix,dimensione_sub*dimensione_sub,MPI_INT,radiceBcast,grigliar);
            calcolaProdotto(tmpMatrix, localB, localC,dimensione_sub);
        }
        //ciascun processore invia il proprio blocco della matrice B al processore situato nella stessa colonna e sulla riga precedente
        MPI_Sendrecv_replace(localB,dimensione_sub*dimensione_sub,MPI_INT,sup,0,inf,0,griglia,&status); 
    }

	MPI_Barrier(MPI_COMM_WORLD);
	
	ricostruisciMatrice(c,localC,dimensione_sub,dimensione_griglia,dimensione);
	
	t1=MPI_Wtime(); //calcolo tempofinale
	myTime=t1-t0; //calcolo del tempo effettivo
	
	MPI_Reduce(&myTime,&timeTot,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
	
	if(menum == 0) {
  
  	//stampaMatrice(a,dimensione,1);
  	//stampaMatrice(b,dimensione,2);
      //stampaMatrice(c,dimensione,0);
    printf("Processore [%d] - tempo impiegato [%lf]\n", menum, timeTot);
	}
    printf("\n");

    MPI_Finalize();
    return 0;    
}

/*Funzione di riempimento matrici*/
void creaMatrici(int dimensione, int *a, int *b){
	
	int index1=0, index2=0;

	for(index1=0; index1<dimensione*dimensione; index1++){
			
		a[index1]=rand()%20;
		b[index1]=rand()%20;
	}
}

/*funzione per ricostruire la matrice finale contenente tutti i prodotti*/
void ricostruisciMatrice(int *c, int *localC, int dimensione_sub, int dimensione_griglia, int dimensione){
	
    MPI_Datatype tipoBlocco, tipoBlocco2;
	
    MPI_Type_vector(dimensione_sub,dimensione_sub,dimensione,MPI_INT,&tipoBlocco2);
    MPI_Type_create_resized(tipoBlocco2,0,sizeof(int),&tipoBlocco);
    MPI_Type_commit(&tipoBlocco);
	
	int nproc=dimensione_griglia*dimensione_griglia;
    int spostamenti[nproc], numeroElementi[nproc], index=0, index1=0;
	
    //setta lo stride per fare scatter
    for (index=0; index<dimensione_griglia; index++){
		
        for (index1=0; index1<dimensione_griglia; index1++){
			
            spostamenti[index*(dimensione_griglia)+index1]=(index*dimensione*dimensione_sub)+(index1*dimensione_sub);
            numeroElementi[index*(dimensione_griglia)+index1]=1;
        }
    }
    MPI_Gatherv(localC,dimensione_sub*dimensione_sub,MPI_INT,c,numeroElementi,spostamenti,tipoBlocco,0,MPI_COMM_WORLD);
}

/*Funzione per effettuare la distribuzione di sottomatrici quadrate tra i processori*/
void distribuisciSubMatrix(int *a, int* localA, int dimensione, int dimensione_griglia){
	
    int index=0, index1=0, dimensione_sub = dimensione/dimensione_griglia, nproc = dimensione_griglia*dimensione_griglia;
    MPI_Datatype tipoBlocco; //Definizione di un tipo di dato MPI
    MPI_Datatype tipoBlocco2; //Definizione di un tipo di dato MPI

    /*Inizializzazione di tipoBlocco tramite la funzione MPI_Type_vector che crea un tipo vettore */
    MPI_Type_vector(dimensione_sub,dimensione_sub,dimensione,MPI_INT,&tipoBlocco2); 
    MPI_Type_create_resized(tipoBlocco2,0,sizeof(int),&tipoBlocco); //Inizializza tipoBlocco copiando tipoBlocco2
    MPI_Type_commit(&tipoBlocco); //Salva permanentemente il tipo di dato

    int spostamenti[nproc];
    int numeroElementi[nproc];

    for (index=0; index<dimensione_griglia; index++){
		
        for (index1=0; index1<dimensione_griglia; index1++){
			
            spostamenti[index*(dimensione_griglia)+index1] = (index*dimensione*dimensione_sub)+(index1*dimensione_sub);
            numeroElementi[index*(dimensione_griglia)+index1] = 1;
        }
    }
	
    MPI_Scatterv(a,numeroElementi,spostamenti,tipoBlocco,localA,dimensione_sub*dimensione_sub,MPI_INT,0,MPI_COMM_WORLD); //Distribuzione dei blocchi tra i processi
}

/*Funzione per creare una griglia bidimensionale periodica di dimensione pari a sqrt(nproc)^2*/
void creaGriglia(MPI_Comm *griglia, MPI_Comm *grigliar, int dimensione){  

    int dim=2, *ndim, reorder=0, *period, vc[2];
	
    ndim=(int*)calloc(dim, sizeof(int));
    ndim[0]=ndim[1]=dimensione;
    
	period=(int*)calloc(dim, sizeof(int));
    period[0]= period[1]= 1;
	
	vc[0] = 0;
    vc[1] = 1;
    /*Creazione griglia bidimensionale e conseguente creazione di un nuovo communicator griglia*/
   	MPI_Cart_create(MPI_COMM_WORLD,dim,ndim,period,reorder,griglia);
    MPI_Cart_sub(*griglia,vc,grigliar); //Creazione di un sottocomunicatore per le righe 
}

/*Funzione per ottenere le coordinate del processore che si trova sopra a quello corrente all'interno della griglia*/
void getSuperiore(int *superiore, int *coordinate){
	
   superiore[0] = coordinate[0] - 1;
   superiore[1] = coordinate[1];
   
}
/*Funzione per ottenere le coordinate del processore che si trova sotto a quello corrente all'interno della griglia*/
void getInferiore(int *inferiore, int *coordinate){
	
   inferiore[0] = coordinate[0] + 1;
   inferiore[1] = coordinate[1] ;
}

/*Calcolo prodotto righe per colonne */
void calcolaProdotto(int *a, int *b, int *c, int dimensione){
	
    int i=0, j=0, k=0;

    for(i=0; i<dimensione; i++){
		
        for(j=0; j<dimensione; j++){
			
            for(k=0; k<dimensione; k++){
				
                c[dimensione*i+j] = c[dimensione*i+j]+a[dimensione*i+k]*b[dimensione*k+j];
            }
        }
	}    
}

void stampaMatrice(int *matrix, int dimensione, int flag){
	
    int i=0, j=0;
	
	if(flag == 0) printf("MATRICE C\n");
	else if (flag == 1) printf("MATRICE A\n");
	else if (flag == 2) printf("MATRICE B\n");
    for(i=0; i<dimensione; i++){
		
        for(j=0; j<dimensione; j++){
			
            printf("[%d]", matrix[dimensione*i+j]);
        }
        printf("\n");
    }
}