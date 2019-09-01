# Key-Value-Storage-Consistency-Checker
This a a Data consistency checker for key value storages like Cassandra, leveldb.
This is a realtime consistency checkers
This program analyses consistency of the data store inside Intel SGX Enclave  and outputs the number of violations in the key value data store. For efficiency, this program makes use of leveldb to analyse the consistency of keys and their values.
### Steps you need to follow to use this program:
1)Download the file and copy it to your Enclave folder.

2)Go to your file inside enclave which registers get and put operations to the key value datastore.

3)After every put and get operation call addOperation function in the following format:
  #### void addOperation(key,Value, start time, finish time, operation type);// For operation type - 'R'(Get), 'W'(Put)
4)If you need to register the number of violations you can write it to a file. There are lines that are commented in the addOperation function.

There is a rough flow of the checker and example in the repository as well. Feel free of you have any questions or conscerns.
