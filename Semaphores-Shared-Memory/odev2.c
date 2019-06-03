/* @Author
* Student Name: <Yunus Emre BALAK  >
* Student ID : <150160509>
* Date: <21.04.2019> */

// paylaşımlı bellek ve semaforlar için 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
//sinyal yakalama için
#include <signal.h>
//fork kulanmak için
#include <unistd.h>
//diğer gerekli kütüphaneler
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

//semaforlar için key değerleri
#define KEYSHM1 1000
#define KEYSHM2 1001
#define KEYSEM_T1_BOS 1100 
#define KEYSEM_T1_DOLU 1101 
#define KEYSEM_T2_BOS 1102 
#define KEYSEM_T2_DOLU 1103 
#define KEYSEM_SENK 1200 

//semafor arttırma operasyonu
void sem_signal(int semid, int val){
	struct sembuf semafor;
	semafor.sem_num = 0;
	semafor.sem_op = val;
	semafor.sem_flg = 1;
	semop(semid, &semafor, 1);
}

//semafor azaltma operasyonu
void sem_wait(int semid, int val){
	struct sembuf semafor;
	semafor.sem_num = 0;
	semafor.sem_op = (-1*val);
	semafor.sem_flg = 1;
	semop(semid, &semafor, 1);
}

// sinyal yakalama fonksiyonu
void mysignal(int signum){
	printf("Sinyal bu numarayla alındı: %d.\n", signum);
}

void mysigset(int num){
	struct sigaction mysigaction;
	mysigaction.sa_handler = (void*)mysignal;
	// sa_handler tarafından tanımlanan sinyal yakalama fonksiyonu
	mysigaction.sa_flags = 0;
	/* sigaction sistem çağrısı bir prosesin belirli bir 
	sinyal alınması üzerine çalışmasını değiştirmek amaçlı kullanılmıştır*/
	sigaction(num, &mysigaction, NULL);
}

int main(){

	mysigset(12); // num = 12 ile sinyal yakalama

	int shmid_1 = 0, shmid_2 = 0; // paylaşımlı bellek idleri
	int *T1 = NULL, *T2 = NULL; // paylaşımlı bellek alanları
	int mainsem = 0, T1_BOS = 0, T1_DOLU = 0, T2_BOS = 0, T2_DOLU = 0; // semafor idleri
	int f; // fork() için return değeri
	int cocuk[3]; // cocuk proses idleri
	int i, benimSiram = 0; // calısan cocuk proses sırası

	// 3 cocuk yarat
	for (i = 0; i < 3; i++){
		f = fork();		//cocuk yaratıldı
		if (f == -1){
			printf("Fork hatası ....\n");
			exit(1);
		}
		// cocuk prosesse cocuk yapma
		if (f == 0)	
			break;	
		cocuk[i] = f;	// cocuklarin id'lerini cocuk arrayinde tut
	}

	if (f != 0){ // anne prosesse
		// anne ve cocuklar arasında senkronizasyon icin semafor yarat (anne kapanır)
		mainsem = semget(KEYSEM_SENK, 1, 0700 | IPC_CREAT); // 
		semctl(mainsem, 0, SETVAL, 0);

		// T1 bosluk kontrolu icin semafor yarat deger = 1
		T1_BOS = semget(KEYSEM_T1_BOS, 1, 0700 | IPC_CREAT); 
		semctl(T1_BOS, 0, SETVAL, 1);

		// T1 doluluk kontrolu icin semafor yarat deger = 0
		T1_DOLU = semget(KEYSEM_T1_DOLU, 1, 0700 | IPC_CREAT);
		semctl(T1_DOLU, 0, SETVAL, 0);

		// T2 bosluk kontrolu icin semafor yarat deger = 1
		T2_BOS = semget(KEYSEM_T2_BOS, 1, 0700 | IPC_CREAT);
		semctl(T2_BOS, 0, SETVAL, 1);

		// T1 doluluk kontrolu icin semafor yarat deger = 0
		T2_DOLU = semget(KEYSEM_T2_DOLU, 1, 0700 | IPC_CREAT); 
		semctl(T2_DOLU, 0, SETVAL, 0);

		// 1 integer boyutunda paylaşımlı bellek alanı yarat ve T1'e bağla
		shmid_1 = shmget(KEYSHM1, sizeof(int), 0700 | IPC_CREAT); 
		T1 = (int *)shmat(shmid_1, 0, 0);

		// 1 integer boyutunda paylaşımlı bellek alanı yarat ve T2'ye bağla
		shmid_2 = shmget(KEYSHM2, sizeof(int), 0700 | IPC_CREAT);
		T2 = (int *)shmat(shmid_2, 0, 0);

		// paylaşımlı bellekteki ilk değerler = -1
		*T1 = -1 , *T2 = -1; 

		sleep(2); // 2 saniye bekle

		printf("Anne kaynakları yarattı. \n");
		printf("Şimdi cocuk prosesleri başlatacak \n");
		for (i = 0; i < 3; i++)
			kill(cocuk[i], 12);

		// semafor değerini 3 azalt (diger cocukları bekle)
		sem_wait(mainsem, 3);

		// baglantilari kopar
		shmdt(T1); 
		shmdt(T2);
		// semaforlari sil
		semctl(mainsem, 0, IPC_RMID, 0);
		semctl(T1_BOS, 0, IPC_RMID, 0);
		semctl(T1_DOLU, 0, IPC_RMID, 0);
		semctl(T2_BOS, 0, IPC_RMID, 0);
		semctl(T2_DOLU, 0, IPC_RMID, 0);
		shmctl(shmid_1, IPC_RMID, 0);
		shmctl(shmid_2, IPC_RMID, 0);

		exit(0);
	}

	else{ // çocuk prosesse

		pause(); // anne semafor önce baslamalı
		
		// p1 = 0, p2 = 1, p3 = 2
		benimSiram = i; 

		// semaforlara ve paylaşılan bellek alanlarına ulaşma
		mainsem = semget(KEYSEM_SENK, 1, 0);
		T1_BOS = semget(KEYSEM_T1_BOS, 1, 0);
		T1_DOLU = semget(KEYSEM_T1_DOLU, 1, 0);
		T2_BOS = semget(KEYSEM_T2_BOS, 1, 0);
		T2_DOLU = semget(KEYSEM_T2_DOLU, 1, 0);
		shmid_1 = shmget(KEYSHM1, sizeof(int), 0);
		shmid_2 = shmget(KEYSHM2, sizeof(int), 0);
		T1 = (int *)shmat(shmid_1, 0, 0); 
		T2 = (int *)shmat(shmid_2, 0, 0);
		sleep(1);
		printf("Çocuk %d çalışmada. \n", benimSiram);
		sleep(2);

		if (benimSiram == 0){ // P1 prosesi
			int sayi = 1;
			while (sayi != 1000){
				sem_wait(T1_BOS, 1); // Boşsa çalış
				sleep(1);
				*T1 = sayi;
				printf("P1: %d T1 alanına yazıldı.\n", *T1);
				sem_signal(T1_DOLU, 1); // Dolu olarak kaydet
				sayi += 1;
				sleep(1);
			}
			sem_wait(T1_BOS, 1); //1000' se işaret bırak
			*T1 = 0;
			sem_signal(T1_DOLU, 1);

			sem_signal(mainsem, 1); // Anneye haber ver
			exit(0);
		}

		else if (benimSiram == 1){ // P2 prosesi
			int temp = 0;
			while (*T1 != 0){
				sem_wait(T1_DOLU, 1);
				sleep(1);
				temp = *T1;
				printf("P2: %d T1 alanından okundu.\n", temp);
				sem_signal(T1_BOS, 1);
				sleep(2);

				if (temp % 2 != 0){
					sem_wait(T2_BOS, 1); // Boşsa T2'ye yaz
					*T2 = temp;
					printf("P2: %d T2 alanına yazıldı.\n", *T2);
					sem_signal(T2_DOLU, 1);
				}
				sleep(1);
			}
			sem_wait(T2_BOS, 1); // T1 0'sa T2'ye işaret bırak
			*T2 = 0;
			sem_signal(T2_DOLU, 1);

			sem_signal(mainsem, 1); // Anneye haber ver
			exit(0);
		}
		else if (benimSiram == 2){ // P3 prosesi
			while (*T2 != 0){
				sem_wait(T2_DOLU, 1);
				sleep(1);
				printf("P3: %d T2 alanindan okundu.\n", *T2);
				if (*T2 % 3 != 0){
					printf("%d çıkışa aktarıldı\n", *T2);
				}
				sem_signal(T2_BOS, 1);
				sleep(1);
			}
			sem_signal(mainsem, 1); // Anne'ye haber ver
			exit(0);

		}
		// bağlantıları kopar
		shmdt(T1);
		shmdt(T2);
	}
	return 0;
}
