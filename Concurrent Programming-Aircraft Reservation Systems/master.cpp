#include <pthread.h>
#include <string.h>
#include <map>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <queue>

using namespace std;

int num_threads = 5;

map <string, int> get_num;


pthread_mutex_t mutex;
pthread_cond_t cond;

struct flight{
	string flight_name;
	int total_seats;
	int seats_booked;
	int seats_left;
	string status;
	pthread_mutex_t lock;
};

struct flight flight_details[10];

struct transaction{
	int query_no;
	string flight_name;
	string operation;
	int seats;
};

queue<struct transaction> transaction_details; 

bool get_status(struct transaction temp_transaction){
	int i = get_num[temp_transaction.flight_name];
	string s(flight_details[i].flight_name);

	if(i == 0 && temp_transaction.flight_name != flight_details[0].flight_name){	
		printf("For query no. - %d\nPlease check the flight name again.\n\n", temp_transaction.query_no);
		return false;
	}

	else{
		string temp = "Query number - " +  to_string(temp_transaction.query_no) + "\n";
		
		pthread_mutex_lock(&flight_details[i].lock);
		temp += "The flight " + s + " is - " + flight_details[i].status + "\n";
		pthread_mutex_unlock(&flight_details[i].lock);
		cout << temp;
		return true;
	}
}

bool book(struct transaction temp_transaction){
	int i = get_num[temp_transaction.flight_name];
	string s(flight_details[i].flight_name);
	string temp = "Query number - " +  to_string(temp_transaction.query_no) + "\n";

	if(i == 0 && temp_transaction.flight_name != flight_details[0].flight_name){	
		printf("For query no. - %d\nPlease check the flight name again.\n\n", temp_transaction.query_no);
		return false;
	}

	pthread_mutex_lock(&flight_details[i].lock);
	// temp += temp_transaction.seats + " " + to_string(flight_details[i].seats_booked) + "\n";
	if(temp_transaction.seats > flight_details[i].seats_left){
		temp += "Flight Name - " + s + "\nSeats required - " + to_string(temp_transaction.seats) + "\nSeats available - " + to_string(flight_details[i].seats_left) + "\nSeats cannot be booked.\n\n";
		pthread_mutex_unlock(&flight_details[i].lock);		
		cout << temp;
		return false;
	}
	else{
		flight_details[i].seats_booked = flight_details[i].seats_booked + temp_transaction.seats;
		flight_details[i].seats_left = flight_details[i].seats_left - temp_transaction.seats;
		
		if(temp_transaction.seats == flight_details[i].seats_left)
			flight_details[i].status = "full";

		temp += "Tickets booked successfully\n";
		temp += "Updated info about the Flight - " + s + "\nTotal Seats booked - " + to_string(flight_details[i].seats_booked) + "\nStatus - " + flight_details[i].status + "\n";
	}
	pthread_mutex_unlock(&flight_details[i].lock);

	cout << temp;
	return true;
}

bool cancel(struct transaction temp_transaction){
	int i = get_num[temp_transaction.flight_name];
	string s(flight_details[i].flight_name);
	string temp = "Query number - " +  to_string(temp_transaction.query_no) + "\n";

	if(i == 0 && temp_transaction.flight_name != flight_details[0].flight_name){	
		printf("For query no. - %d\nPlease check the flight name again.\n\n", temp_transaction.query_no);
		exit(EXIT_FAILURE);
		return false;
	}

	pthread_mutex_lock(&flight_details[i].lock);
	// temp += temp_transaction.seats + " " + to_string(flight_details[i].seats_booked) + "\n";
	if(temp_transaction.seats > flight_details[i].seats_booked){
		temp += "Flight Name - " + s + "\nSeats booked - " + to_string(flight_details[i].seats_booked) + "\nSeats to be cancelled - " + to_string(temp_transaction.seats) + "\nSeats cannot be cancelled.\n\n";
		pthread_mutex_unlock(&flight_details[i].lock);
		cout << temp;
		return false;
	}
	else{
		flight_details[i].seats_left = flight_details[i].seats_left + temp_transaction.seats;
		flight_details[i].seats_booked = flight_details[i].seats_booked - temp_transaction.seats;
		
		if(temp_transaction.seats == flight_details[i].seats_left)
			flight_details[i].status = "empty";

		temp += "Cancellation accomplished successfully\n";
		temp += "Updated info about the Flight - " + s + "\nSeats booked - " + to_string(flight_details[i].seats_booked) + "\nStatus - " + flight_details[i].status + "\n";
	}
	pthread_mutex_unlock(&flight_details[i].lock);

	cout << temp;
	//exit(EXIT_SUCCESS);
	return true;
}

void* worker(void *ptr){
	// int j = *(int*)ptr;
	while(!transaction_details.empty()){
		 // cout << "I am here1\n";
		/*
		while(transaction_details.empty()){
			pthread_cond_wait(&cond, &mutex);
					
		}
		*/
		 // cout << "I am here2\n";

		pthread_mutex_lock(&mutex);
		struct transaction transaction_first = transaction_details.front();
		transaction_details.pop();
		pthread_mutex_unlock(&mutex);
		
		// string temp = "Flight Name " + transaction_first.flight_name + " operation " + transaction_first.operation + " seats " + to_string(transaction_first.seats) + "\n\n";
		// cout << transaction_first.operation << " "; 
		// cout << transaction_first.seats << " \n";
		// cout << "I am here3\n";
		// cout << "transaction popped\n";
		 // cout << "I am here4\n";
		// cout << temp;
		// cout << transaction_first.flight_name << " ";
		// cout << transaction_first.operation << " "; 
		// cout << transaction_first.seats << " \n";
		bool return_value;
		if(transaction_first.operation == "status"){
			// cout << "status in";
			return_value = get_status(transaction_first);
			// cout << "status out";
		}

		else if(transaction_first.operation == "cancel"){
			 //cout << "Cancel in";
			 return_value = cancel(transaction_first);
			 //cout << "Cancel out";
		}

		else if(transaction_first.operation == "book"){
			// cout << "Book in";
			return_value= book(transaction_first);
			// cout << "Book out";
		}
		else{
			cout << "Query no. - " << to_string(transaction_first.query_no) << endl;
			cout << "Invalid Operation \"" << transaction_first.operation << "\"\n\n";
		}
	}
	pthread_exit(0);
}

int main (){

	pthread_t slave[5];

	FILE* dbp;
	char * line = NULL;
	size_t len = 0;
    ssize_t read;

    int i = 0;

	dbp = fopen("database", "r");
    if(dbp == NULL)
        exit(EXIT_FAILURE);

	while((read = getline(&line, &len, dbp)) != -1) {

	    char * pch;
		pch = strtok (line, " ");
		flight_details[i].flight_name = pch;

		pch = strtok (NULL, " ");
		flight_details[i].total_seats = atoi(pch);

		pch = strtok (NULL, " ");
		flight_details[i].seats_booked = atoi(pch);

		pch = strtok (NULL, " ");
		flight_details[i].status = pch;

		flight_details[i].seats_left = flight_details[i].total_seats - flight_details[i].seats_booked;

		get_num.insert(pair<string, int>(flight_details[i].flight_name, i));

		i++;
	}

    fclose(dbp);

 //    map<string,int>::iterator it = get_num.begin();
	// for (it=get_num.begin(); it!=get_num.end(); it++)
 //    	cout << it->first << " => " << it->second << '\n';

    // int j = get_num[flight_details[0].flight_name];

    // cout << j << endl;

  //   for (int j = 0; j < 5; j++){
		// pthread_create(&slave[j], NULL, worker, NULL);
		// pthread_detach(slave[j]);
  //   }

    FILE* fp;
    line = NULL;
    len = 0;

    struct transaction a_transaction;

    fp = fopen("transactions", "r");
    if(fp == NULL){
    	printf("No transactions file to play with!\n");
    	exit(EXIT_FAILURE);
    }

    while(1){

    	if((read = getline(&line, &len, fp)) != -1){

    		if(strlen(line) == 1)
    			continue;

	    	if(!strcmp(line, "END") || !strcmp(line, "END ")){
	    		break;
	    	}
	    	char * pch;

			pch = strtok (line, " ");
			a_transaction.query_no = atoi(pch);

			pch = strtok (NULL, " ");
			a_transaction.flight_name = pch;

			// cout << "Read Flight Name"<< a_transaction.flight_name << " "<< endl;
			
			pch = strtok (NULL, " ");
			a_transaction.operation = pch;

			// cout << "Read operation"<< a_transaction.operation << " "<< endl;

			pch = strtok (NULL, " ");

			if(a_transaction.operation == "status")
				a_transaction.seats = -1;
			else
				a_transaction.seats = atoi(pch);

			// cout << "Read seats"<< a_transaction.seats << " "<< endl;
			
			pthread_mutex_lock(&mutex);
			transaction_details.push(a_transaction);
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);

	    }
	}

		fclose(fp);
	  
	// while(!transaction_details.empty()){

 //    	struct transaction b_transaction = transaction_details.front();

 //    	cout << endl << endl << b_transaction.flight_name << " ";
 //    	cout << b_transaction.operation << " "; 
 //    	cout << b_transaction.seats << " \n\n\n";
 //    	transaction_details.pop();
 //    }  
	
	for (int i = 0; i < num_threads; i++){
		int j = i;
		pthread_create(&slave[i], NULL, worker, NULL);
		// pthread_detach(j);

	}    
	for (int i = 0; i < num_threads; i++)
		pthread_join(slave[i], NULL);

	exit(EXIT_SUCCESS);
}
