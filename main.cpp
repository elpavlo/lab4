#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <semaphore.h>
#include <fstream>

using namespace std;

/* variables */

/* threads */
pthread_t P1;
pthread_t P2;
pthread_t P3;
pthread_t P4;
pthread_t P5;
pthread_t P6;


pthread_mutex_t MCR1 = PTHREAD_MUTEX_INITIALIZER; // for CR1
pthread_mutex_t M_Sig21 = PTHREAD_MUTEX_INITIALIZER; // for Sig21
pthread_mutex_t M_Sig22 = PTHREAD_MUTEX_INITIALIZER; // for Sig22

sem_t SCR21;

pthread_cond_t Sig1 = PTHREAD_COND_INITIALIZER; // not full
pthread_cond_t Sig2 = PTHREAD_COND_INITIALIZER; // not empty
pthread_cond_t Sig21 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Sig22 = PTHREAD_COND_INITIALIZER;

int max_len = 3; // max length of CR1
int cur_len = 0;
/* counters for finish work */
unsigned times_full = 0;
unsigned times_empty = 0;
bool cond; // while cond == true all whiles are working

vector<int> CR1; // buffer CR1 stack
// stack functions
int push_CR1(){
	CR1.push_back(cur_len);
	return CR1[cur_len++];
}

int pop_CR1(){
	int buf = CR1[--cur_len];
	CR1.pop_back();
	return buf;
}

/* structure presents CR2 */
struct _CR2{
	int atom_int_1;
	int atom_int_2;
	unsigned atom_uns_1;
	unsigned atom_uns_2;
	long atom_long_1;
	long atom_long_2;
	unsigned long atom_uns_long_1;
	unsigned long atom_uns_long_2;
};
struct _CR2 CR2;

void use_CR2(){
	cout  << CR2.atom_int_1 << " " << CR2.atom_int_2 << " "
		  << CR2.atom_uns_1 << " " << CR2.atom_uns_2 << " "
		  << CR2.atom_long_1 << " " << CR2.atom_long_2 << " "
		  << CR2.atom_uns_long_1 << " " << CR2.atom_uns_long_2 << "\n";
}

void mod_CR2(){
	__sync_fetch_and_sub(&CR2.atom_int_1, 1);
	__sync_add_and_fetch(&CR2.atom_int_2, 2);
	__sync_fetch_and_xor(&CR2.atom_uns_1, 3);
//	__sync_fetch_and_nand(&CR2.atom_uns_2, 4);
	__sync_or_and_fetch(&CR2.atom_long_1, 5);
	__sync_and_and_fetch(&CR2.atom_long_2, 6);
	__sync_bool_compare_and_swap(&CR2.atom_uns_long_1, 1, 1);
	__sync_val_compare_and_swap(&CR2.atom_uns_long_2, 3, 2);
}

/* check if CR1 is full */
int is_full() {
	if (cur_len >= max_len)
		cout << "Buffer is filled " << ++times_full << " times\n";
	return cur_len >= max_len;
}
/* check if CR1 is empty */
int is_empty() {
	if (cur_len <= 0)
		cout << "Buffer is emptied " << ++times_empty << " times\n";
	return cur_len <= 0;
}
void killall(){
	cond = false;
}
// done
void* P1_thread(void *) {
	while (cond){

		pthread_mutex_lock(&M_Sig21);
		cout << "P1 waits Sig21\n";
		pthread_cond_wait(&Sig21, &M_Sig21);
		cout << "P1 got Sig21\n";
		pthread_mutex_unlock(&M_Sig21);

		cout << "P1 use CR2\n";
		use_CR2();

		cout << "P1 opened SCR21\n";
		sem_post(&SCR21);

		pthread_mutex_lock(&M_Sig22);
		cout << "P1 waits Sig22\n";
		pthread_cond_wait(&Sig22, &M_Sig22);
		cout << "P1 got Sig22\n";
		pthread_mutex_unlock(&M_Sig22);

		cout << "P1 use CR2\n";
		use_CR2();

		if (times_full >=2 && times_empty >= 2){
			killall();
		}
	}
	cout << "P1 ends\n";
	sem_post(&SCR21);
	return 0;
}
// done
void* P2_thread(void *) {
	while (cond){
		pthread_mutex_lock(&M_Sig21);
		cout << "P2 waits Sig21\n";
		pthread_cond_wait(&Sig21, &M_Sig21);
		cout << "P2 got Sig21\n";
		pthread_mutex_unlock(&M_Sig21);


		cout << "P2 use CR2\n";
		use_CR2();

		pthread_mutex_lock(&MCR1);
		while (is_empty()){
			cout << "CR1 is empty. P2 waits\n";
			pthread_cond_wait(&Sig2, &MCR1);
		}
		cout << "P2 took " << pop_CR1() << "\n";
		pthread_cond_broadcast(&Sig1);
		pthread_mutex_unlock(&MCR1);


		if (times_full >= 2 && times_empty >= 2) {
			killall();
		}
	}
	cout << "P2 ends\n";
	if (!is_empty()) pop_CR1();
	return 0;
}
// done
void* P3_thread(void *) {
	while (cond){
		pthread_mutex_lock(&MCR1);
		while (is_empty()){
		cout << "CR1 is empty. P3 waits\n";
		pthread_cond_wait(&Sig2, &MCR1);
		}
		cout << "P3 took " << pop_CR1() << "\n";
		pthread_cond_broadcast(&Sig1);
		pthread_mutex_unlock(&MCR1);

		/*	here is deadlock */
/*		pthread_mutex_lock(&M_Sig22);
		cout << "P3 waits Sig22\n";
		pthread_cond_wait(&Sig22, &M_Sig22);
		cout << "P3 got Sig22\n";
		pthread_mutex_unlock(&M_Sig22);
*/
		cout << "P3 use CR2\n";
		use_CR2();

		if (times_empty >= 2 && times_full >= 2) {
			killall();
		}
	}
	if (!is_empty()) pop_CR1();
	return 0;
}
// done
void* P4_thread(void *) {
	while (cond){
		cout << "P4 mod CR2\n";
		mod_CR2();

		pthread_mutex_lock(&M_Sig21);
		pthread_cond_broadcast(&Sig21);
		cout << "P4 send Sig21\n";
		pthread_mutex_unlock(&M_Sig21);

		cout << "P4 waits SCR21\n";
		sem_wait(&SCR21);

		cout << "P4 mod CR2\n";
		mod_CR2();

		pthread_mutex_lock(&M_Sig22);
		cout << "P4 sent Sig22\n";
		pthread_cond_broadcast(&Sig22);
		pthread_mutex_unlock(&M_Sig22);

		cout << "P4 close SCR21\n";
		sem_close(&SCR21);

		if (times_full >= 2 && times_empty >= 2) {
			killall();
		}
	}
	cout << "P4 ends\n";
	pthread_cond_broadcast(&Sig21);
	pthread_cond_signal(&Sig22);
	return 0;
}
// done
void* P5_thread(void *) {
	while (cond){
		pthread_mutex_lock(&MCR1);
		while (is_empty()){
			cout << "CR1 is empty. P5 waits\n";
			pthread_cond_wait(&Sig2, &MCR1);
		}
		cout << "P5 took " << pop_CR1() << "\n";
		pthread_cond_broadcast(&Sig1);
		pthread_mutex_unlock(&MCR1);

		pthread_mutex_lock(&M_Sig21);
		cout << "P5 waits Sig21\n";
		pthread_cond_wait(&Sig21, &M_Sig21);
		cout << "P5 got Sig21\n";
		pthread_mutex_unlock(&M_Sig21);


		cout << "P5 use CR2\n";
		use_CR2();

		if (times_empty >= 2 && times_full >= 2) {
			killall();
		}
	}
	cout << "P5 ends\n";
	if (!is_empty()) pop_CR1();
	return 0;
}
// done
void* P6_thread(void *) {
	while (cond){
		pthread_mutex_lock(&MCR1);
		while (is_full()){
			cout << "CR1 is full. P6 waits\n";
			pthread_cond_wait(&Sig1, &MCR1);
		}
		cout << "P6 added " << push_CR1() << "\n";
		pthread_cond_broadcast(&Sig2);
		pthread_mutex_unlock(&MCR1);


		if (times_empty >= 2 && times_full >= 2){
			killall();
		}
	}
	cout << "P6 ends\n";
	//if (!is_full()) push_CR1();
	//if (!is_full()) push_CR1();
	//if (!is_full()) push_CR1();
	return 0;
}

int main(){

	sem_init(&SCR21, 0, 0);
	cond = true;
	pthread_create(&P1, 0, &P1_thread, 0);
	pthread_create(&P2, 0, &P2_thread, 0);
	pthread_create(&P3, 0, &P3_thread, 0);
	pthread_create(&P4, 0, &P4_thread, 0);
	pthread_create(&P5, 0, &P5_thread, 0);
	pthread_create(&P6, 0, &P6_thread, 0);


	pthread_join(P6, 0);



	cout << "The end !\n";
	return 0;
}
