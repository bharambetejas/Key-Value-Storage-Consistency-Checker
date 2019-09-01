#include "Enclave.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdbool>
#include <ctime>
#include <iostream>
#include <cassert>
#include <sstream>
#include <leveldb/db.h>
//#include "utils.h"
using namespace std;
#define MIN(x,y) ((x) <= (y)) ? (x) : (y)
#define MAX(x,y) ((x>y)?x:y)
clock_t globalTime;
struct dict_t{
    const char *key;
    const char *value;
    long readStart;
    long writeStart;
    long writeFinish;
    struct dict_t *next;
};
struct dict_t *head=NULL;
int violationCount=0;
int lineCount=0;
void storeOperationsToState(const char *opkey,const char *opvalue, long Start, long finish, char *opType);
void addOperation(const char *opkey,const char *opvalue, long Start, long finish, char *opType);

void deleteList()
{   
    struct dict_t *ptr=head;
    struct dict_t *Next;
    while(ptr!=NULL)
    {   
        Next=ptr->next;
        free(ptr);
        ptr=Next;
    }
    head=NULL;
}
void addListtoDB()
{
    //cout<<"add list to db"<<endl;
    struct dict_t *ptr,*thru;
    ptr=head;
    leveldb::DB* db; 
    leveldb::Options options; 
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "/home/tejas/testcheckerDB", &db);
    assert(status.ok());
    while(ptr!=NULL)
    {
        std::string key=ptr->key + std::string("!") + ptr->value;
        //std::stringstream sstm;
        //sstm<<ptr->writeStart<<"!"<<ptr->writeFinish;
        std::string writeStartString=std::to_string(ptr->writeStart);
	std::string writeFinishString=std::to_string(ptr->writeFinish);
	std::string sstm=writeStartString + std::string("!") + writeFinishString;
	//std::string value=sstm.str();
	std::string value=sstm;
        db->Put(leveldb::WriteOptions(),key,value);
	ptr=ptr->next;
    }
    delete db;
}

void getOperation()
{
    struct dict_t *ptr=head;
    while(ptr!=NULL)
    {
        //cout<<ptr->key<<"|"<<ptr->value<<"|"<<ptr->readStart<<"|"<<ptr->writeStart<<"|"<<ptr->writeFinish<<"->"<<endl;
        /*
        ofstream LogFile;
        LogFile.open("Violation.Log",ios::app);
        LogFile.seekp(0,std::ios::end);
        LogFile<<"->"<<ptr->key<<"|"<<ptr->value<<"|"<<ptr->readStart<<"|"<<ptr->writeStart<<"|"<<ptr->writeFinish<<endl;
        LogFile.close();*/
        ptr=ptr->next;
        
    }
}
void getOplevel()
{
leveldb::DB* db;
leveldb::Options options;
options.create_if_missing = true;
leveldb::Status status = leveldb::DB::Open(options, "/home/tejas/testcheckerDB", &db);
assert(status.ok());
leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
for (it->SeekToFirst(); it->Valid(); it->Next()) {
  //cout << it->key().ToString() << ": "  << it->value().ToString() << endl;
}
assert(it->status().ok());  // Check for any errors found during the scan
delete it;
delete db;
}

long getScore(long aMaxReadStart,long aWriteStart,long aWriteFinish,long bMaxReadStart,long bWriteStart,long bWriteFinish) {
    //sint file;
    //cout<<"evaluation"<<endl;
    bool aforward;
    bool bforward;
    long score = 0;
    if (aWriteFinish < aMaxReadStart)
        aforward = true;
    else
        aforward = false;

    if (bWriteFinish < bMaxReadStart)
        bforward = true;
    else
        bforward = false;


    if (aforward == true && bforward == true)//true for forward, false for backward
    {
        if (aMaxReadStart < bWriteFinish || bMaxReadStart < aWriteFinish) {
            score = 0;
        } else {
            score = MIN(aMaxReadStart - bWriteFinish, bMaxReadStart - aWriteFinish) / 2;
            //printf("forward-forward overlapp\n");

        }
    } else if (aforward == true && bforward == false) //Conflict only when forward encloses backward (a encloses b)
    {
        if (aMaxReadStart < bWriteFinish || bWriteStart < aWriteFinish) {
            score = 0;
        } else {
            score = MIN(aMaxReadStart - bWriteFinish, bWriteStart - aWriteFinish) / 2;

        }
    } else if (aforward == false && bforward == true)//(b encloses a)
    {
        if (bMaxReadStart < aWriteFinish || aWriteStart < bWriteFinish) {
            score = 0;
        } else {
            score = MIN(bMaxReadStart - aWriteFinish, aWriteStart - bWriteFinish) / 2;
        }
    } else {
        score = 0;
        //printf("hello2\n");
    }

    if (score > 0) {
        violationCount++;
    }
    return score;
}
void readStateDB()
{
    //cout<<"ScoreFunction"<<endl;
    long score;
    const char *key1;
    const char *value1;
    struct dict_t *ptr,*thru;
    ptr=head;
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "/home/tejas/testcheckerDB", &db);
    assert(status.ok());
    long aMaxReadStart;
    long awriteStart;
    long awriteFinish;
    long bMaxReadStart;
    long bwriteStart;
    long bwriteFinish;
    while(ptr->next!=NULL)
    {
            thru=ptr;
            key1=ptr->key;
            value1=ptr->value;
            aMaxReadStart=ptr->readStart;
            awriteStart=ptr->writeStart;
            awriteFinish=ptr->writeFinish;
            leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
            for (it->SeekToFirst(); it->Valid(); it->Next()) 
            {
                std::string keyValue=it->key().ToString();
                std::string okey=keyValue.substr(0,24);
		const char *key2=okey.c_str();
                std::string ovalue=keyValue.substr(25);
		const char *value2=ovalue.c_str();
                if(strcmp(key1,key2)==0 && strcmp(value1,value2)!=0)
                {
		    std::string writeValue=it->value().ToString();
                    std::string wst=writeValue.substr(0,19);
            	    bwriteStart=strtol(wst.c_str(),NULL,10);
                    std::string wet=writeValue.substr(20);
                    bwriteFinish=strtol(wet.c_str(),NULL,10);
	            bMaxReadStart=0;
                    score=getScore(aMaxReadStart,awriteStart,awriteFinish,bMaxReadStart,bwriteStart,bwriteFinish);
                }    
            }
            delete it;
            ptr=ptr->next;
    }
    delete db;

}
void readState()
{
    long score;
    struct dict_t *ptr,*thru;
    const char *key1;
    const char *avalue;
    long aMaxReadStart;
    long awriteStart;
    long awriteFinish;
    ptr=head;
    while(ptr->next!=NULL)
    {
        thru=ptr;
        key1=ptr->key;
        avalue=ptr->value;
        aMaxReadStart=ptr->readStart;
        awriteStart=ptr->writeStart;
        awriteFinish=ptr->writeFinish;
        while(thru->next!=NULL)
        {
            if(thru->key==key1)
            {
                score=getScore(aMaxReadStart,awriteStart,awriteFinish,thru->readStart,thru->writeStart,thru->writeFinish);
                //printf("%ld\n",score);
            }
            thru=thru->next;
        }
        ptr=ptr->next;
    }
}

void storeOperationsToState(const char *opkey,const char *opvalue, long Start, long finish, char *opType)
{
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "/home/tejas/testcheckerDB", &db);
    assert(status.ok());
    struct dict_t *ptr = (struct dict_t *) malloc(sizeof(struct dict_t));
    if (head == NULL) {
        //globalTime=clock();
        //cout<<"first head"<<endl;
        ptr->key = opkey;
        ptr->value = opvalue;
        if (strcmp(opType, "R") == 0) {
            ptr->readStart = Start;
            //cout<<"Entered Read"<<endl;
            //ptr->writeStart = 0;
            //ptr->writeFinish = 0;
            std::string writeKey=opkey + std::string("!") + opvalue;
            //cout<<"Fault here!"<<endl;
            std::string value;
            //cout<<"Fault here!"<<endl;
            leveldb::Status s=db->Get(leveldb::ReadOptions(), writeKey, &value);
            assert(s.ok());
            std::string wst=value.substr(0,19);
            ptr->writeStart=strtol(wst.c_str(),NULL,10);
            std::string wet=value.substr(20);
            ptr->writeFinish=strtol(wet.c_str(),NULL,10);
        }
        if (strcmp(opType, "W") == 0) {
            ptr->readStart = 0;
            ptr->writeStart = Start;
            ptr->writeFinish = finish;
        }
        ptr->next = NULL;
        head = ptr;
    }
    else {
        struct dict_t *current;
        current = head;
        while (current != NULL) {
            if (strcmp(current->key, opkey) == 0 && strcmp(current->value, opvalue) == 0) {
                if (strcmp(opType, "R") == 0) {
                    current->readStart = MAX(current->readStart, Start);
                }
                if (strcmp(opType, "W") == 0) {
                    if (current->writeStart < Start) {
                        current->writeStart = Start;
                        current->writeFinish = finish;
                    }
                }
                break;
            }
            current = current->next;
        }
        if (current == NULL) {
            current = head;
            while (current->next != NULL) {
                current = current->next;
            }
            ptr->key = opkey;
            ptr->value = opvalue;
            if (strcmp(opType, "R") == 0) {
                ptr->readStart = Start;
                //ptr->writeStart = 0;
                //ptr->writeFinish = 0;
                std::string writeKey=opkey + std::string("!") + opvalue;
                std::string value;
                db->Get(leveldb::ReadOptions(), writeKey, &value);
                std::string wst=value.substr(0,19);
                ptr->writeStart=strtol(wst.c_str(),NULL,10);
                std::string wet=value.substr(20);
                ptr->writeFinish=strtol(wet.c_str(),NULL,10);

            }
            if (strcmp(opType, "W") == 0) {
                ptr->readStart =0;//change
                ptr->writeStart = Start;
                ptr->writeFinish = finish;
            }
            ptr->next = NULL;
            current->next = ptr;
        }
    }
    delete db;
    //getOperation();
    lineCount++;   

}


void addOperation(const char *opkey,const char *opvalue, long Start, long finish, char *opType)
{
    //clock_t nowTime=clock();
    //double elapsed=(double)(nowTime - globalTime) * 1000.0 / CLOCKS_PER_SEC;
    //ofstream LogFile;
    storeOperationsToState(opkey,opvalue,Start,finish,opType);
    int firstFlag=0; 
    //cout<<"Line Count is:"<<lineCount<<endl;
    if(lineCount==3)
    {
        if(firstFlag==0){
                readState();
                firstFlag=1;
        }
        else{
        readStateDB();
        }
        //ScoreFunction();
        //getOperation();
        //LogFile.open("Violation.Log",ios::app);
        //LogFile.seekp(0,std::ios::end);
        //LogFile<<violationCount<<endl;
        //LogFile.close();
        printf("\nTotal number of consistency violations:%d\n",violationCount);
        violationCount=0;
        lineCount=0;
        addListtoDB();
        //cout<<"Add list to db complete"<<endl;
        getOplevel();
        deleteList();
        //cout<<"deleting complete"<<endl;
        //globalTime=clock();
    }
}
/*
int main()
{
    long score;
    //split();
    //score=getScore();
    addOperation("user12161962213042174405","field0=bbbbbbbbbb",1556833007613435147,1556833007614526914,"W");
    addOperation("user12161962213042174405","field0=vvvvvvvvvv",1556833025229800457,1556833025230878397,"W");
    addOperation("user12161962213042174405","field0=vvvvvvvvvv",1556833025231061954,1556833025232126091,"R");
    addOperation("user12161962213042174405","field0=bbbbbbbbbb",1556833025232152622,1556833025233242958,"R");
    addOperation("user12161962213042174405","field0=%%%%%%%%%%",1556833025234242958,1556833025235242958,"W");
    addOperation("user12161962213042174405","field0=%%%%%%%%%%",1556833025236061954,1556833025237126091,"R");
    //ScoreFunction();
    //getOperation();
    //cout<<"Total number of consistency violations:"<<violationCount<<endl;
}*/
