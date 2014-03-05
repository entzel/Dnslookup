void* ReqThreads(void*);
//the ReqThread function opens an input file and scans through it to find 
//each hostname in the file, to place each hostname individually in the queue.
//It checks if the queue is full, if so it waits a random time from 1-100
//microseconds, giving the resolver thread time to take an item out of it
//when the queue has an available spot the thread that uses that function
//creates a mutex lock to restrict other threads from accessing the queue
//and pushes the hostname onto the queue



void* ResThreads(void*);
//ResThreads checks if the queue is empty or if the SearchComplete is true
//The Search is complete if all the input files have been scanned through
//by ReqThreads, this is only true if ReqThreads has scanned each hostname
//in every inputfile given. If the queue is not empty or the search is not
//complete, lock the queue to be the only thread with access to it, then
//pop the first item and obtain it, unlock the queue to allow the ReqThreads
//access to push, and take the item and perform dns search on it to find
//the correct url for the hostname, then write the name to the output file.
