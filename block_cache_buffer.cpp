
#include <cstddef>	//NULL
#include <iostream>
#include <thread>         // std::thread
#include <mutex>          // std::mutex
#include <functional>
#include <vector>         //for cashe buffer
#include <chrono>	  //for usleep() --to simulate the delaty 
#include <ctime> 	// for clock() and random
#include <cstdlib>

//BEGGINING OF BLOCK DATA STRUCTURE
using namespace std;

	class Block
	{

	public:

	Block();
	Block(int id);

	int getblockID();
	char readBlock();
	void writeBlock(char data);
	

	private:

	int increase_size[254]; //there to make the size of the block 1024 bytes
	
	int block_id;
	char block_data;

	};




//default constructer (should never be used)
Block::Block(){

	block_id = 0;
	block_data = '\0';

}


//constructor that takes in an ID to be set, sets data to zero to begin with
Block::Block(int id){

	block_id = id;
	block_data = '\0';

}

//return ID value
int Block:: getblockID(){

	return block_id;
}

//returns block data(char)
char Block:: readBlock(){


	return block_data;
	
}

//changes the block_data
void Block:: writeBlock(char data){

	block_data = data;


}


//END BLOCK DATA STRUCTURE



/*.................................................................................
...................................................................................
...............................................................................*/





//START OF DISK DATA STRUCTURE

	class Disk
	{

	public:

	Disk();
	void diskblockread(char *x, int blocknum);
	void diskblockwrite(char *x, int blocknum);
	
	Block* getBlock(int blocknum);
	void lockDisk();
	void unlockDisk();
	int get_disk_Reads();
	int get_disk_Writes();
	int get_ReadWrite_total();

	
	private:
	
	
	int disk_Reads;
	int disk_Writes;
	mutex lock1; //lock for disk
	Block* disk_data[4096]; //array of blocks
	int Disk_size; // int representing number of blocks currently on the disk

	};



//default constructor for disk
Disk::Disk(){

int i;
	
	//declare all pointers to start at null
	for(i = 0; i < 4096; i++){
		
		disk_data[i] = new Block(i);
	} 

	disk_Reads = 0;
	disk_Writes = 0;

	
	
}


void Disk:: diskblockread(char *x, int blocknum){

	lock1.lock(); //locks the disk for the threads use;


	this_thread::sleep_for(chrono::milliseconds(1)); //simulate 1ms latency
	
	
	if(disk_data[blocknum] == NULL){
		
		disk_data[blocknum] = new Block(blocknum);
		
		disk_data[blocknum] -> writeBlock('\0');
		
		*x = disk_data[blocknum] -> readBlock(); 
		
	}else{
		
		*x = disk_data[blocknum] -> readBlock();
	}

	disk_Reads++;
	
	
	
	lock1.unlock(); //unlock disk
	


}

void Disk:: diskblockwrite(char *x, int blocknum){


	lock1.lock();
	this_thread::sleep_for(chrono::milliseconds(1)); //simulate 1ms latency

		if(disk_data[blocknum] == NULL){
			disk_data[blocknum] = new Block(blocknum);
		}		
			
		disk_data[blocknum] -> writeBlock(*x);	

	disk_Writes++;
	
	lock1.unlock();

	
}


//function for returning a block address from the disk with the givin blocknum id
Block* Disk:: getBlock(int blocknum){
	return(disk_data[blocknum]);
}
	
void Disk:: lockDisk(){

	lock1.lock();
}

void Disk:: unlockDisk(){
	lock1.unlock();
}

int Disk:: get_disk_Reads(){

	return(disk_Reads);
}

int Disk:: get_disk_Writes(){

	return(disk_Writes);
}

int Disk:: get_ReadWrite_total(){

	return(disk_Writes + disk_Reads);
}


//END OF DISK DATA STRUCTURE



/*.................................................................................
...................................................................................
...............................................................................*/




/*note on the caches system of removing the least recently used block:

whenever a block is accessed, it is moved to the front of the vector,
so therefore if the block is full, the block that is least used is at the back.
whenever a new block is put in, it is also at the back */



//BEGGINING OF CACHE DATA STRUCTURE

	class Cache
	{

	public:

	Cache(Disk* disk1); //the cache needs have a disk to refrence;
	void blockread(char *x, int blocknum);
	void blockwrite(char *x, int blocknum);
	
	int get_cache_Reads();
	int get_cache_Writes();
	int get_cache_ReadWrite_total();

	private:

	Disk* disk_used;
	vector<Block*> cache_buffer; //cache of max size 64
	int current_buffer_size; // the number of blocks currently in the buffer

	int cache_writes;
	int cache_reads;



	bool isInBuffer(int blocknum);//returns true if blocknum is in buffer, false if not
	void putInBuffer(int blocknum); //inserts block with id blocknum into buffer, removes one if full
	Block* getCacheBlock(int blocknum); //returns block address with id blocknum (reqires that the block be in the cache)

	int getLoacationInBuffer(int blocknum); //returns the index of that block

	};



//default constructor
Cache::Cache(Disk* disk1){

disk_used = disk1;


current_buffer_size = 0; 

}



//if block_id "blocknum" is in the cashe, set x = that blocknums block_data
//if block_id is not in chashe, 
void Cache:: blockread(char *x, int blocknum){
	
	Block* copyBlock = NULL;
	int location_in_buffer;

	//set copyBlock to point to the block;
	if(isInBuffer(blocknum)){
		location_in_buffer = getLoacationInBuffer(blocknum);
		copyBlock = getCacheBlock(blocknum);

		cache_buffer.erase(cache_buffer.begin() + location_in_buffer);
		cache_buffer.insert(cache_buffer.begin(), copyBlock);
	}else{
		
		disk_used -> diskblockread(x, blocknum);
		
		putInBuffer(blocknum);
		copyBlock = getCacheBlock(blocknum);
	}

	cache_reads++;
	*x = copyBlock -> readBlock();	
	
		

}

void Cache:: blockwrite(char *x, int blocknum){

	Block* copyBlock = NULL;
	int location_in_buffer;

	//set copyBlock to point to the block;
	if(isInBuffer(blocknum)){
 
		location_in_buffer = getLoacationInBuffer(blocknum);
		copyBlock = getCacheBlock(blocknum);

		cache_buffer.erase(cache_buffer.begin() + location_in_buffer);
		cache_buffer.insert(cache_buffer.begin(), copyBlock);
		*x = copyBlock -> readBlock();
	}else{
			
		disk_used -> diskblockwrite(x, blocknum);
		
		putInBuffer(blocknum);
		
	}

	cache_writes++;

}

bool Cache:: isInBuffer(int blocknum){

	int i;
	
	for(i=0; i < current_buffer_size; i++){
	
		if(cache_buffer[i] -> getblockID() == blocknum){
			return(true);
		}
	}

	return(false);

}

void Cache:: putInBuffer(int blocknum){

	disk_used -> lockDisk();
	
	if(current_buffer_size < 64){ //if buffer is not full

		cache_buffer.push_back(disk_used -> getBlock(blocknum));//insert at back
		
		current_buffer_size ++;
	
	}else{ //if buffer is full

		cache_buffer.pop_back(); //delete last element
		cache_buffer.push_back(disk_used -> getBlock(blocknum));// insert at back

	}
	disk_used -> unlockDisk();

	
			
}

Block* Cache:: getCacheBlock(int blocknum){

	int i;
	Block* returnBlock = NULL;
	
	for(i=0; i < current_buffer_size; i++){
	
		if(cache_buffer[i] -> getblockID() == blocknum){
			returnBlock =cache_buffer[i]; 
		}
	}
	
	return(returnBlock);
}

int Cache:: getLoacationInBuffer(int blocknum){
	unsigned int i;
	
	for(i=0; i < cache_buffer.size(); i++){
	
		if(cache_buffer[i] -> getblockID() == blocknum){
			return(i);
		}
	}

	return(0);
}

int Cache:: get_cache_Reads(){

	return(cache_reads);
}

int Cache:: get_cache_Writes(){

	return(cache_writes);
}

int Cache:: get_cache_ReadWrite_total(){

	return(cache_writes + cache_reads);
}
	


//END OF CACHE DATA STRUCTURE


/*.................................................................................
...................................................................................
...............................................................................*/




//main function that tests the program;
int main(void){
	
	thread workers_test1[40]; //threads for test 1
	thread workers_test2[40]; //threads for test 2

	char test1_char = 33; //char used for test 1
	char test2_char = 33; //char used for test 2
	

	char* test1_char_ptr = &test1_char; //char ptr for test 1
	char* test2_char_ptr = &test2_char; //char ptr for test 2
	

	Disk* test1_disk = new Disk; //test1 disk
	Cache * test1_cache = new Cache(test1_disk); //test1 cache

	Disk* test2_disk = new Disk; //test2 disk
	Cache * test2_cache = new Cache(test2_disk); //test2 cache
	
	//TEST 1

	clock_t begin1 = clock();
	
	for(int i = 0; i < 20; i++){
	
		test1_char = (test1_char +1);

			if(test1_char > 126){
				test1_char = 33;
			}
		workers_test1[i%20] = thread(&Cache:: blockwrite, test1_cache, test1_char_ptr, i);
		workers_test1[i%20].join();
		
		
	}

	for(int i = 0; i < 20; i++){
	
			
			workers_test1[(i%20) + 20] = thread(&Cache:: blockread, test1_cache, test1_char_ptr, i);
			workers_test1[(i%20) + 20].join();
		
		}

	
	
	clock_t end = clock();
	double elapsed_secs = double(end - begin1) / CLOCKS_PER_SEC;
	cout<<" Test 1: "<<endl;
	cout<<" \n cpu clock time for test 1: " <<elapsed_secs<<endl;
	cout<<" disk reads: "<<test1_disk -> get_disk_Reads()<<endl;
	cout<<" disk writes: "<<test1_disk -> get_disk_Writes()<<endl;
	cout<<" disk hits: "<<test1_disk -> get_ReadWrite_total()<<endl;

	cout<<"\n cache reads: "<<test1_cache -> get_cache_Reads()<<endl;
	cout<<" cache writes: "<<test1_cache -> get_cache_Writes()<<endl;
	cout<<" cache hits: "<<test1_cache -> get_cache_ReadWrite_total()<<endl;
	


	//Test2

	srand(time(NULL));
	int random_disk_location;

	clock_t begin2 = clock();
	for(int i = 0; i < 20; i++){

		random_disk_location = rand() % 4095;

		test2_char = (test2_char +1);

				if(test2_char > 126){
					test2_char = 33;
				}

	
		workers_test2[i] = thread(&Cache:: blockwrite, test2_cache, test2_char_ptr, random_disk_location);
		workers_test2[i].join();
		

		random_disk_location = rand() % 4095;

		workers_test2[i+20] = thread(&Cache:: blockread, test2_cache, test2_char_ptr, random_disk_location);
		workers_test2[i+20].join();
		
	}
	
	clock_t end2 = clock();
	double elapsed_secs2 = double(end2 - begin2) / CLOCKS_PER_SEC;

	
	cout<<" \n\n Test 2: "<<endl;
	cout<<" \n cpu clock time for test 2: " <<elapsed_secs2<<endl;
	cout<<" disk reads: "<<test2_disk -> get_disk_Reads()<<endl;
	cout<<" disk writes: "<<test2_disk -> get_disk_Writes()<<endl;
	cout<<" disk hits: "<<test2_disk -> get_ReadWrite_total()<<endl;

	cout<<"\n cache reads: "<<test2_cache -> get_cache_Reads()<<endl;
	cout<<" cache writes: "<<test2_cache -> get_cache_Writes()<<endl;
	cout<<" cache hits: "<<test2_cache -> get_cache_ReadWrite_total()<<endl;
	
	

		
	
	 

	
	
	

return 0;

}
