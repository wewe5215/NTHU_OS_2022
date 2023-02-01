#include "syscall.h"

// this is the testcase made by Peter Su 109062233 if you pass all the testcases you are good
int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";
    char validate_2[] = "bbb";
    char receive[26];
	int success= Create("file3.test");
    int second = Create("file4.test");
    int fid_index[25];
    char validate[] = "bbbaaa";
    char writing_limit[] = "aaa";
    char rec_fid2[30];
    char rec_fid3[30];

	OpenFileId fid;
    OpenFileId fid2;
    OpenFileId fid3;
	int i , j , count;
    /* this program will test 
    1.  超過20次寫入
    2.  關閉後寫入
    3. 基本開啟
    4. 開啟無法打開的檔案
    5. 開啟還沒被建立的檔案
    6. 寫入不存在的地方 -1 
    7. 寫入錯誤大小的地方
    8. 當只有3個字時寫入6個字應該會有的正確表現
    9. 寫入不存在的地方 valid number but not exist 
    10. 再刪除後寫入相同的fid
    11. 對於相同的檔案寫入
    12. 閱讀的size超過有的size
    */
    // opening file3 , 4
    // MSG("Hi, I think the problem is due to your implementation. - En Ming Huang");
    
	if (success != 1) MSG("Failed on creating file");
    if (second != 1 ) MSG("Failed on creating file");
    PrintInt(12345);
    // opening 20 files (b)
    for(j = 0 ; j < 2 ; j++){
        for(i = 0 ; i < 25 ; i++){
            fid_index[i] = Open("file3.test");
        }
        for(i = 0 ; i < 5 ; i++){
            if(fid_index[i+20] != -1){
                MSG("There's only twenty files available \n");
            }
        }
        for(i = 0 ; i < 20 ; i++){
            if(fid_index[i] == -1){
                MSG("You should be able to handle twenty open files ^^ \n");
            }
        }
        for(i = 0 ; i < 20 ; i++){
            Close(fid_index[i]);
        }
    }

    fid2 = Open("file3.test");
    if (fid2 < 0) MSG("Failed on opening file");
    fid3 = Open("file4.test");
    if (fid3 < 0) MSG("Failed on opening file");
    // check if it can handle (b) -> non-existing
	fid = Open("file5.test");
    if(fid != -1) MSG("Opening on none existing file");   

    // (c) not for sure
    // (d)
    // invalid write
    // write invalid block
    count = Write(test, 100, -1);
    if(count != -1 ) MSG("Writing on none existing file");
    // writing invalid size
    count = Write(test, -1, fid2);
    if(count != -1 ) MSG("Writing on invalid size");
    // return wrong value ->同學好:
    // Write(buffer, size, id)會寫入[buffer, buffer+size)區間內的值至file當中。
    // 因此只要沒有fail的情況，size是30，回傳值應為30。 
    
    count = Write(writing_limit, 100, fid2);
    if(count != 100) MSG("Writing incorrect size");
    // writing invalid block
    for(i = 5 ; i < 20 ; i++){
        if(i != fid2 && i != fid3){
            int count = Write(test, 1, i);
            if(count == 1) MSG("Writing on none existing file");
            break;
        }
    }
    // closing test should delete the file
    count  = Close(fid2);
    if(count != 1) MSG("Failed on closing file");
    count = Close(fid3);
    if(count != 1) MSG("Failed on closing file");
    // so if I write here it should be invalid
    count = Write(test, 1, fid2);
    if(count == 1) MSG("Writing on none existing file");
    // reopen the file 
    fid2 = Open("file3.test");
    fid3 = Open("file3.test");
    // finish upon here

    // writing test
    for (i = 0; i < 6; i++) {
		count = Write(test , 1, fid2) ;
		if (count != 1) MSG("Writing fault");
	}
    for(i = 0 ; i < 3 ; i++){
        count = Write(test + 1, 1, fid3) ;
		if (count != 1) MSG("Writing fault");
    }
    // close and reopen
    Close(fid2);
    Close(fid3);
    fid2 = Open("file3.test");
    fid3 = Open("file3.test");
    count = Read(rec_fid2 , 6 , fid2);
    if(count != 6) MSG("Failed on reading file");

    count = Read(rec_fid3 , 6 , fid3);
    if(count != 6) MSG("Failed on reading file");

    for(i = 0 ; i < 6 ; i ++){
        if(rec_fid2[i] != rec_fid3[i]){
            MSG("Failed on reading correctly (same file with different content)");
        }
        if(rec_fid2[i] != validate[i]){
            MSG("Failed on reading correctly");
        }
    }

    // reopen the file 
    Close(fid3);
    fid3 = Open("file4.test");
    if (fid3 < 0) MSG("Failed on opening file");

    for(i = 0 ; i < 3 ; i++){
        int count = Write(test + 1, 1, fid3) ;
		if (count != 1) MSG("Writing fault");
    }

    Close(fid3);
    fid3 = Open("file4.test");
    count = Read(rec_fid3 , 6 , fid3); // read the extra length
    if(count != 3) MSG("Reading the wrong size");
    for(i = 0 ; i < 3 ; i ++){
        if(rec_fid3[i] != validate_2[i]){
            MSG("Failed on reading correctly");
        }
    }
    Close(fid3);

    MSG("Passed! ^_^ it's so hard to design ouo ");
	Halt();
}
