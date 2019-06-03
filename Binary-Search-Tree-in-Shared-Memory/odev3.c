/* @Author
* Student Name: <Yunus Emre BALAK  >
* Student ID : <150160509>
* Date: <12.05.2019> */

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
#define KEYSEM_SENK 1200 

struct node
{
    char* keyValue; // telefon no vb. gibi anahtar değeri
    int key; // düğümler için karşılık gelen anahtar 
    struct node *left, *right; // ağaçtaki düğümün sağ ve sol altındaki düğümleri gösteren pointerlar
};
 
// dugum olusturan fonksiyon
struct node *newNode(int item, char* keyValue)
{
    int shmid_1 = 0; // paylaşımlı bellek idsi

    // node struct'ı boyutunda paylaşımlı bellek alanı yarat ve temp'e bağla
    shmid_1 = shmget(KEYSHM1, sizeof(struct node), 0700 | IPC_CREAT); 
    struct node *temp = (struct node *)shmat(shmid_1, NULL, 0);

    temp->key = item;
    temp->keyValue = keyValue;
    temp->left = temp->right = NULL;

    return temp;
}

// Düğümü ağaca ekle
struct node* insert(struct node* node, int key, char* keyValue)
{
    /* Eger agac bossa direkt ekleme yapar (root = NULL ise) */
    if (node == NULL) return newNode(key, keyValue);
 
    // Eger anahtar degeri dugumun anahtar degerinden kucukse sol alt agacı degilse sağ alt agacı tara
    if (key < node->key)
        node->left  = insert(node->left, key, keyValue);
    else
        node->right = insert(node->right, key, keyValue);
 
    /* dugumu döndür*/
    return node;
}

// Ağaçtaki en küçük düğümü bul
struct node * minValueNode(struct node* node)
{
    struct node* current = node;
 
    //sol alt yaprağa ulaş
    while (current->left != NULL)
        current = current->left;
 
    return current;
}

// Düğümü sil ve yeni düğümü geri döndür
struct node* deleteNode(struct node* root, int key)
{
    // Hiç düğüm yoksa silme yapmaz
    if (root == NULL) return root;
 
    // düğüm root anahtar değerinden küçükse sol alta bak
    if (key < root->key)
        root->left = deleteNode(root->left, key);
 
    // düğüm root anahtar değerinden büyükse sağ alta bak
    else if (key > root->key)
        root->right = deleteNode(root->right, key);
 
    // root'u siler
    else
    {
        // Eğer düğümün hiç çocuğu yok ya da bir çocuğu var ise
        if (root->left == NULL)
        {
            struct node *temp = root->right;
            free(root);
            return temp;
        }
        else if (root->right == NULL)
        {
            struct node *temp = root->left;
            free(root);
            return temp;
        }
 
        // İki çocuğu olan düğümün sağ alt çocuğundaki en küçük elemanı bul
        struct node* temp = minValueNode(root->right);
 
        // Bulunan en küçük node root olur
        root->key = temp->key;
 
        // silme gerçekleşir
        root->right = deleteNode(root->right, temp->key);
    }
    return root;
}

// anahtar ve anahtar değerlerine bakarak ağaçta arama yapar
int  search(struct node* root, int key, char* keyValue)
{
    if (root->keyValue == keyValue)    // root anahtar değerine eşittir. root anahtarını döndürür.
        return root->key;

        while(root->left == NULL || root->right == NULL){   
            // anahtar root anahtarından küçükse solundan devam et 
            if(key < root->key){
                root = root->left;
                if (root->keyValue == keyValue)
                return root->key; // bulunan düğümünn anahtarını döndür.
            }
            // anahtar root anahtarından büyükse sağından devam et
            if(root->key < key){
                root = root->right;
                if (root->keyValue == keyValue)
                return root->key; // bulunan düğümünn anahtarını döndür.
            }
        } 
        return -1; // anahtarla eşleşen tamsayı değeri yoksa -1 değerini geri getirir.
    }   

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

    int f; // fork() için return değeri
    int i, benimSiram = 0; // calısan cocuk proses sırası
    int cocuk[3]; // cocuk proses idleri
    int shmid_1 = 0; // paylaşımlı bellek idsi
    int mainsem = 0, T1_BOS = 0, T1_DOLU = 0;
    struct node *root = NULL;

    int keyvalues = 1, nodecount = 0;

    // 3 cocuk yarat
    for (i = 0; i < 3; i++){
        f = fork();     //cocuk yaratıldı
        if (f == -1){
            printf("Fork hatası ....\n");
            exit(1);
        }
        // cocuk prosesse cocuk yapma
        if (f == 0) 
            break;  
        cocuk[i] = f;   // cocuklarin id'lerini cocuk arrayinde tut
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

        sleep(2); // 2 saniye bekle

        printf("Anne kaynakları yarattı. \n");
        printf("Şimdi cocuk prosesleri başlatacak \n");
        for (i = 0; i < 3; i++)
            kill(cocuk[i], 12);

        // semafor değerini 3 azalt (diger cocukları bekle)
        sem_wait(mainsem, 3);

        // baglantilari kopar
        shmdt(root); 

        // semaforlari ve belleği sil
        shmctl(shmid_1, IPC_RMID, 0);
        semctl(mainsem, 0, IPC_RMID, 0);
        semctl(T1_BOS, 0, IPC_RMID, 0);
        semctl(T1_DOLU, 0, IPC_RMID, 0);

        exit(0);
    }

    else{ // çocuk prosesse

        pause(); // anne semafor önce baslamalı

        // p1 = 0, p2 = 1, p3 = 2
        benimSiram = i; 
        root = NULL;

        // semaforlara ve bellek alanına erişim
        mainsem = semget(KEYSEM_SENK, 1, 0);
        T1_BOS = semget(KEYSEM_T1_BOS, 1, 0);
        T1_DOLU = semget(KEYSEM_T1_DOLU, 1, 0);

        shmid_1 = shmget(KEYSHM1, sizeof(struct node), 0);
        struct node *rootptr = (struct node *)shmat(shmid_1, NULL, 0);

        sleep(1);
        printf("Çocuk %d çalışmada. \n", benimSiram);
        sleep(2);

        if (benimSiram == 0){ // P1 prosesi (EKLE)

            // ilk 2 düğüm eklendi
            root = insert(root, 1, "ahmet");
            printf("P1 %d anahtarlı değeri ağaca ekledi\n", root->key);
            root = insert(root, 2, "mehmet");
            printf("P1 %d anahtarlı değeri ağaca ekledi\n", root->key);
            sleep(1);

            sem_signal(T1_DOLU, 1); //dolu diye sinyal ver

            sem_wait(T1_BOS, 1); //boşsa eklemeye devam et
            sleep(1);
            root = insert(root, 3, "veli");
            printf("P1 %d anahtarlı değeri ağaca ekledi\n", root->key);

            sem_signal(T1_DOLU, 1); //dolu diye sinyal ver
            sleep(1);

            sem_signal(mainsem, 1); // Anneye haber ver
            exit(0);
        }

        else if (benimSiram == 1){ // P2 prosesi (SİL)

            sem_wait(T1_DOLU, 1); // en az 2 düğüm eklendiyse başla
            sleep(1);
            printf("P2 silme işlemine başlıyor\n");
            
            root = deleteNode(root, 1); // 1 anahtar değerine sahip düğümü sil
            printf("P2 %d anahtarlı değeri ağaçtan sildi\n", root->key);

            sem_signal(T1_DOLU, 1); // silme gerçekleşti
            sleep(1);

            sem_wait(T1_DOLU, 1); 
            sleep(1);
            root = deleteNode(root, 2); // 2 anahtar değerine sahip düğümü sil

            sem_signal(mainsem, 1); // Anneye haber ver
            exit(0);
        }

        else if (benimSiram == 2){ // P3 prosesi
            
            sem_wait(T1_BOS, 1); // ekleme ve arama aynı anda olabilir
            sleep(1);
            printf("%d\n",search(root, 1,"ahmet"));
            printf("arandı\n");
            sem_signal(T1_DOLU, 1); // dolu sinyali ver
            sleep(1);

            sem_wait(T1_BOS, 1); // ekleme ve arama aynı anda olabilir
            sleep(1);
            printf("%d\n",search(root, 2,"mehmet"));
            sem_signal(T1_DOLU, 1); // dolu sinyali ver
            sleep(1);

            sem_wait(T1_BOS, 1); // ekleme ve arama aynı anda olabilir
            sleep(1);
            printf("%d\n",search(root, 2,"veli"));
            sem_signal(T1_DOLU, 1); // dolu sinyali ver
            sleep(1);

            sem_signal(mainsem, 1); // Anne'ye haber ver
            exit(0);

        }

    }

}