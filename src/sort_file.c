#include "sort_file.h"
#include <stdio.h>
#include <stdlib.h>
#include "bf.h"
#include <string.h>
#include <math.h>

#define CALL_OR_DIE(call)     \
  {                           \
    SR_ErrorCode code = call; \
    if (code != SR_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

void QuickSort(char* a, int b, int f);



//------------------------------------------------------------------//
//------------------------Global Arrays-----------------------------//
//------------------------------------------------------------------//

BF_Block **bArray;
int* posArray, *noArray, *hasArray, *idArray, *toFree;
int SortCounter = 0;


SR_ErrorCode SR_Init() {

	return SR_OK;
}
SR_ErrorCode SR_CreateFile(const char *fileName) {
  

  int filedesc, counter;
  BF_Block *block;
  char* data;

  BF_Block_Init(&block);

  if(BF_CreateFile(fileName)!=SR_OK){
    printf("Error - File already exists\n");
    return SR_ERROR;
  }
  CALL_OR_DIE(BF_OpenFile(fileName, &filedesc));
  CALL_OR_DIE(BF_AllocateBlock(filedesc, block));
  data = BF_Block_GetData(block);
  memset(data, '2', 6);
  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));
  CALL_OR_DIE(BF_CloseFile(filedesc));

  return SR_OK;
}


SR_ErrorCode SR_OpenFile(const char *fileName, int *fileDesc) {
  BF_Block *block;

  BF_Block_Init(&block);

  CALL_OR_DIE(BF_OpenFile(fileName, fileDesc));
  CALL_OR_DIE(BF_GetBlock(*fileDesc, 0, block));
  char* data = BF_Block_GetData(block);

  CALL_OR_DIE(BF_UnpinBlock(block));

  if(atoi(data) == 222222){
    return SR_OK;
  }
  else{
    return SR_ERROR;
  }
}

SR_ErrorCode SR_CloseFile(int fileDesc) {
  
  CALL_OR_DIE(BF_CloseFile(fileDesc));

  return SR_OK;
}

SR_ErrorCode SR_InsertEntry(int fileDesc, Record record) {
  

  int counter;
  BF_Block *block, *newblock;
  char* data;

  BF_Block_Init(&block);
  BF_Block_Init(&newblock);

  CALL_OR_DIE(BF_GetBlockCounter(fileDesc, &counter));

  CALL_OR_DIE(BF_GetBlock(fileDesc, counter-1, block));

  data = BF_Block_GetData(block);

  if(data[0]==17 || atoi(data) == 222222){
    CALL_OR_DIE(BF_UnpinBlock(block));
    CALL_OR_DIE(BF_AllocateBlock(fileDesc, newblock));
    data = BF_Block_GetData(newblock);
    memset(data, 1, sizeof(int));
    memcpy(data+sizeof(int), &record, sizeof(record));
    BF_Block_SetDirty(newblock);
    CALL_OR_DIE(BF_UnpinBlock(newblock));
  }
  else{
    memcpy(data+sizeof(int)+data[0]*sizeof(record), &record, sizeof(record));
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
    data[0]++;  
  }

  return SR_OK;
}


SR_ErrorCode SR_SortedFile(
  const char* input_filename,
  const char* output_filename,
  int fieldNo,
  int bufferSize
) {

  	Record *test = malloc(sizeof(Record));
  	Record argu;
  	int counter = 1;
  	int i, j, fileDesc, blockNo, k, fd;
  	BF_Block *block, *new_file_block;
  	char* data, *sorted_data, *current, *last_rec, *temp_string;
  	int temp_int, flag=0;
  	char* intermediate = malloc(sizeof(output_filename)+10);

  	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	//------------------------Create intermediate file to put the sorted blocks------------------------------//
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

  	strcpy(intermediate, "sort_");				// Intermediate files have tha name "sort_'output_filename'"
  	strcat(intermediate, output_filename);

  	CALL_OR_DIE(SR_CreateFile(intermediate));
  	CALL_OR_DIE(SR_OpenFile(intermediate, &fd));


  	BF_Block_Init(&block);
  	BF_Block_Init(&new_file_block);
  	temp_string = malloc(sizeof(Record));

  	CALL_OR_DIE(SR_OpenFile(input_filename, &fileDesc));
  	CALL_OR_DIE(BF_GetBlockCounter(fileDesc, &blockNo));

  	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	//-----------Get as many blocks as the buffer size and sort them individually via quicksort--------------//
	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	while(counter<blockNo){

	  	for(i=0; i<bufferSize; i++){

	  		if(counter+i<blockNo){

	  			CALL_OR_DIE(BF_GetBlock(fileDesc, counter, block));
	  			data = BF_Block_GetData(block);

	  			CALL_OR_DIE(BF_AllocateBlock(fd, new_file_block));
	  			sorted_data = BF_Block_GetData(new_file_block);
	  			memcpy(sorted_data, data, BF_BLOCK_SIZE);

	  			QuickSort(sorted_data+sizeof(int), *sorted_data, fieldNo);

	  			counter++;

	  			BF_Block_SetDirty(block);
	  			BF_Block_SetDirty(new_file_block);
	  			CALL_OR_DIE(BF_UnpinBlock(new_file_block));
	  			CALL_OR_DIE(BF_UnpinBlock(block));
	  		}
	  		else{

	  			break;

	  		}

	  	}

	}

	int position = 0;
	int b_counter = 1;
	int fileCounter = 0;
	int tempFd;
	intermediate = malloc(12);
	sprintf(intermediate, "temp%d%d.db", SortCounter, fileCounter);	
	int consecutiveBlocksNum = 1;										// The step.
	int toCount = consecutiveBlocksNum * (bufferSize-1);
	int minID;
	char* minValueStr = malloc(sizeof(Record));							// minValueStr and minValueInt are to compare strings and integers.
	int minValueInt;
	int minPos = 0;
	int flag2 = 1;
	int recNum;
	int lastBlockRecs;
	CALL_OR_DIE(BF_GetBlockCounter(fd, &blockNo));




	CALL_OR_DIE(BF_GetBlock(fd, 1, block));				// The records that the first block has. This is the size of all 
														// the blocks apart from the last one that may be incomplete
	data = BF_Block_GetData(block);

	recNum = *data;

	CALL_OR_DIE(BF_UnpinBlock(block));



	CALL_OR_DIE(BF_GetBlock(fd, (blockNo-1), block));	// The records of the last block.
	data = BF_Block_GetData(block);

	lastBlockRecs = *data;

	CALL_OR_DIE(BF_UnpinBlock(block));




	bArray = (BF_Block **) malloc((bufferSize-1)*BF_BLOCK_SIZE);
	posArray = (int*) malloc((bufferSize-1) * sizeof(int));
	noArray = (int*) malloc((bufferSize-1) * sizeof(int));
	hasArray = (int*) malloc((bufferSize-1) * sizeof(int));
	idArray = (int*) malloc((bufferSize-1) * sizeof(int));
	toFree = (int*) malloc((bufferSize-1) * sizeof(int));

	for(i=0; i<(bufferSize-1); i++){

		BF_Block_Init(&bArray[i]);

	}

	while(consecutiveBlocksNum < (blockNo-1)){							// When this is false, then the sorting is complete.


		CALL_OR_DIE(SR_CreateFile(intermediate));
		CALL_OR_DIE(SR_OpenFile(intermediate, &tempFd));


		while(b_counter < blockNo){


			for(j=0; j<(bufferSize-1); j++){							// Initialize the bArray with the first blocks.

				if(b_counter >= blockNo){								// In this case, we have already reached the final blocks and
																		// therefore there are no blocks for this position.
					hasArray[j] = 0;
					b_counter+=consecutiveBlocksNum;
					toFree[j] = 0;

				}
				else{

					CALL_OR_DIE(BF_GetBlock(fd, b_counter, bArray[j]));

					posArray[j] = 0;
					noArray[j] = 1;
					hasArray[j] = 1;
					idArray[j] = b_counter;
					toFree[j] = 1;

					b_counter+=consecutiveBlocksNum;

				}

			}

			for(i=0; i<(recNum*(bufferSize-1)*consecutiveBlocksNum); i++){	

				k=0;

				while(!hasArray[k]){						// Find the first position of the array with an available block.

					k++;


				}	

				if(k>(bufferSize-2)){						// All array positions are done and therefore we need the next set of blocks.

					break;

				}

				data = BF_Block_GetData(bArray[k]);

				memcpy(test, data+sizeof(int)+((posArray[k])*sizeof(Record)), sizeof(Record)); // Comparisons.


				switch(fieldNo){

					case 0:
						minValueInt = test->id; 
						break;

					case 1:
						memcpy(minValueStr, test->name, sizeof(argu.name));
						break;

					case 2:
						memcpy(minValueStr, test->surname, sizeof(argu.surname));
						break;

					case 3:
						memcpy(minValueStr, test->city, sizeof(argu.city));


				}
				minPos = k;


				for(j=k; j<(bufferSize-1); j++){

					if(!hasArray[j]){
						continue;
					}

					data = BF_Block_GetData(bArray[j]);

					memcpy(test, data+sizeof(int)+(posArray[j]*sizeof(Record)), sizeof(Record));


					switch(fieldNo){

						case 0:
							if(test->id < minValueInt){

								minValueInt = test->id;
								minPos = j;

							}
							break;

						case 1:
							if(strcmp(test->name, minValueStr) < 0){

								memcpy(minValueStr, test->name, sizeof(argu.name));
								minPos = j;

							}
							break;

						case 2:
							if(strcmp(test->surname, minValueStr) < 0){

								memcpy(minValueStr, test->surname, sizeof(argu.surname));
								minPos = j;

							}
							break;

						case 3:
							if(strcmp(test->city, minValueStr) < 0){

								memcpy(minValueStr, test->city, sizeof(argu.city));
								minPos = j;

							}
							break;

					}

				}


				data = BF_Block_GetData(bArray[minPos]);
				memcpy(test, data+sizeof(int)+(posArray[minPos]*sizeof(Record)), sizeof(Record));


				memcpy(&argu, test, sizeof(Record));				// Argu is the record to be inserted.
				CALL_OR_DIE(SR_InsertEntry(tempFd, argu));

				posArray[minPos]++;


				if(posArray[minPos]>=recNum){						// If this is true then the minPos block has finished with its records.

					if(noArray[minPos]<consecutiveBlocksNum){

						CALL_OR_DIE(BF_UnpinBlock(bArray[minPos]));

						if(idArray[minPos]<(blockNo-1)){

							CALL_OR_DIE(BF_GetBlock(fd, (idArray[minPos]+1), bArray[minPos]));
							idArray[minPos]++;
							posArray[minPos] = 0;					
							noArray[minPos]++;

						}
						else{

							toFree[minPos] = 0;
							hasArray[minPos] = 0;

						}

					}
					else{

						hasArray[minPos] = 0;

					}
						
				}
				else if(idArray[minPos]==(blockNo-1) && posArray[minPos]==lastBlockRecs){			// Same case but for the lats block.

					hasArray[minPos] = 0;

				}


			}




			for(j=0; j<(bufferSize-1); j++){

				if(toFree[j]){
				
					CALL_OR_DIE(BF_UnpinBlock(bArray[j]));

				}

			}


		}

		consecutiveBlocksNum = consecutiveBlocksNum * (bufferSize-1);


		CALL_OR_DIE(SR_CloseFile(fd));
		fd = tempFd;
		fileCounter++;
		b_counter = 1;
		free(intermediate);
		intermediate = malloc(12);
		sprintf(intermediate, "temp%d%d.db", SortCounter, fileCounter);	
	
	}


	fileCounter--;	

	//--------------------------------------------------------------------------------//
	//-------------Copy of the last temp file to the output_filename file-------------//
	//--------------------------------------------------------------------------------//

	free(intermediate);
	intermediate = malloc(12);
	sprintf(intermediate, "temp%d%d.db", SortCounter, fileCounter);	

 	CALL_OR_DIE(SR_OpenFile(intermediate, &fd));

 	CALL_OR_DIE(SR_CreateFile(output_filename));
 	CALL_OR_DIE(SR_OpenFile(output_filename, &tempFd));


 	for(b_counter=1; b_counter<(blockNo); b_counter++){

 		CALL_OR_DIE(BF_GetBlock(fd, b_counter, block));
 		data = BF_Block_GetData(block);


 		CALL_OR_DIE(BF_AllocateBlock(tempFd, new_file_block));
 		current = BF_Block_GetData(new_file_block);

 		memcpy(current, data, BF_BLOCK_SIZE);

 		BF_Block_SetDirty(new_file_block);
 		CALL_OR_DIE(BF_UnpinBlock(new_file_block));
 		CALL_OR_DIE(BF_UnpinBlock(block));


 	}

 	SortCounter++;

 	CALL_OR_DIE(SR_CloseFile(fd));
 	CALL_OR_DIE(SR_CloseFile(tempFd));

 	free(bArray);
 	free(posArray);
 	free(noArray);
 	free(toFree);
 	free(hasArray);
 	free(idArray);

  	
	return SR_OK;

}

SR_ErrorCode SR_PrintAllEntries(int fileDesc) {
  
  int i, j, counter;
  BF_Block *block;
  char *data, *name, *city, *surname;
  Record *record;

  record = malloc(sizeof(Record));

  BF_Block_Init(&block);

  CALL_OR_DIE(BF_GetBlockCounter(fileDesc, &counter));

  for (i=1; i<counter; i++){
    CALL_OR_DIE(BF_GetBlock(fileDesc, i, block));
    data = BF_Block_GetData(block);
    for (j=0; j<*data; j++){
      memcpy(record, (data+sizeof(int)+j*sizeof(Record)), sizeof(Record));
      printf("%d,\"%s\",\"%s\",\"%s\"\n", record->id, record->name, record->surname, record->city);
    }
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  return SR_OK;
}

void QuickSort(char* current, int rec_num, int fieldNo){

	if(rec_num<=1){
		return;
	}

	Record *test = malloc(sizeof(Record));  		// Create a record so we can get the fields' sizes dynamically

	int pivot = rec_num/2;
	int j, left_no, right_no, k, left_id, right_id, pivot_id;
	char* temp_string = malloc(sizeof(Record));
	char* left_string, *right_string, *pivot_string;
	char* last_rec;
	char* start = current;
	char *left, *right;

	last_rec = current+(rec_num-1)*sizeof(Record);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	//-------------------------------Move the pivot records to the end---------------------------------------//
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	for(j=0; j<pivot; j++){
		current+=sizeof(Record);
	}

	memcpy(temp_string, last_rec, sizeof(Record));
	memcpy(last_rec, current, sizeof(Record));
	memcpy(current, temp_string, sizeof(Record));

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	//---------------------Compare and move records depending on the selected value--------------------------//
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	left = start;
	right = last_rec-sizeof(Record);
	left_no = 0;
	right_no = rec_num-2;

	switch(fieldNo){

		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--------------------------------------Sort by record's id (INT)----------------------------------------//
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		case 0:

			memcpy(&left_id, left, sizeof(int));
			memcpy(&right_id, right, sizeof(int));
			memcpy(&pivot_id, last_rec, sizeof(int));

			while(left_no<right_no){

				while(left_id<pivot_id&&left_no<right_no){
					left+=sizeof(Record);
					left_no++;
					memcpy(&left_id, left, sizeof(int));
				}
				while(right_id>=pivot_id&&left_no<right_no){
					right-=sizeof(Record);
					right_no--;
					memcpy(&right_id, right, sizeof(int));
				}


				if(left_no<right_no){
					memcpy(temp_string, left, sizeof(Record));
					memcpy(left, right, sizeof(Record));
					memcpy(right, temp_string, sizeof(Record));
					left+=sizeof(Record);
					right-=sizeof(Record);
					left_no++;
					right_no--;
					memcpy(&left_id, left, sizeof(int));
					memcpy(&right_id, right, sizeof(int));
				}


			}

			memcpy(&left_id, left, sizeof(int));
			memcpy(&right_id, right, sizeof(int));
			if(right_id<pivot_id){
				right+=sizeof(Record);
				right_no++;
			}

			memcpy(temp_string, right, sizeof(Record));
			memcpy(right, last_rec, sizeof(Record));
			memcpy(last_rec, temp_string, sizeof(Record));

			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			//--------------------Sort recursively the two parts that the table got divided to-----------------------//
			///////////////////////////////////////////////////////////////////////////////////////////////////////////


			QuickSort(start, right_no, fieldNo);
			QuickSort(right+sizeof(Record), rec_num-right_no-1, fieldNo);

			break;


		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		//----------------------------------Sort by record's name (char[15])-------------------------------------//
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		case 1:

			left_string = malloc(sizeof(test->name));
			right_string = malloc(sizeof(test->name));
			pivot_string = malloc(sizeof(test->name));

			memcpy(left_string, left+sizeof(test->id), sizeof(test->name));
			memcpy(right_string, right+sizeof(test->id), sizeof(test->name));
			memcpy(pivot_string, last_rec+sizeof(test->id), sizeof(test->name));

			while(left_no<right_no){

				while(strcmp(left_string, pivot_string)<0&&left_no<right_no){
					left+=sizeof(Record);
					left_no++;
					memcpy(left_string, left+sizeof(test->id), sizeof(test->name));
				}
				while(strcmp(right_string, pivot_string)>=0&&left_no<right_no){
					right-=sizeof(Record);
					right_no--;
					memcpy(right_string, right+sizeof(test->id), sizeof(test->name));
				}


				if(left_no<right_no){
					memcpy(temp_string, left, sizeof(Record));
					memcpy(left, right, sizeof(Record));
					memcpy(right, temp_string, sizeof(Record));
					left+=sizeof(Record);
					right-=sizeof(Record);
					left_no++;
					right_no--;
					memcpy(left_string, left+sizeof(test->id), sizeof(test->name));
					memcpy(right_string, right+sizeof(test->id), sizeof(test->name));
				}


			}

			memcpy(left_string, left+sizeof(test->id), sizeof(test->name));
			memcpy(right_string, right+sizeof(test->id), sizeof(test->name));
			if(strcmp(right_string, pivot_string)<0){
				right+=sizeof(Record);
				right_no++;
			}

			memcpy(temp_string, right, sizeof(Record));
			memcpy(right, last_rec, sizeof(Record));
			memcpy(last_rec, temp_string, sizeof(Record));

			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			//--------------------Sort recursively the two parts that the table got divided to-----------------------//
			///////////////////////////////////////////////////////////////////////////////////////////////////////////

			QuickSort(start, right_no, fieldNo);
			QuickSort(right+sizeof(Record), rec_num-right_no-1, fieldNo);

			break;


		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--------------------------------Sort by record's surname (char[20])------------------------------------//
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		case 2:

			left_string = malloc(sizeof(test->surname));
			right_string = malloc(sizeof(test->surname));
			pivot_string = malloc(sizeof(test->surname));

			memcpy(left_string, left+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
			memcpy(right_string, right+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
			memcpy(pivot_string, last_rec+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));

			while(left_no<right_no){

				while(strcmp(left_string, pivot_string)<0&&left_no<right_no){
					left+=sizeof(Record);
					left_no++;
					memcpy(left_string, left+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
				}
				while(strcmp(right_string, pivot_string)>=0&&left_no<right_no){
					right-=sizeof(Record);
					right_no--;
					memcpy(right_string, right+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
				}


				if(left_no<right_no){
					memcpy(temp_string, left, sizeof(Record));
					memcpy(left, right, sizeof(Record));
					memcpy(right, temp_string, sizeof(Record));
					left+=sizeof(Record);
					right-=sizeof(Record);
					left_no++;
					right_no--;
					memcpy(left_string, left+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
					memcpy(right_string, right+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
				}


			}

			memcpy(left_string, left+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
			memcpy(right_string, right+sizeof(test->id)+sizeof(test->name), sizeof(test->surname));
			if(strcmp(right_string, pivot_string)<0){
				right+=sizeof(Record);
				right_no++;
			}

			memcpy(temp_string, right, sizeof(Record));
			memcpy(right, last_rec, sizeof(Record));
			memcpy(last_rec, temp_string, sizeof(Record));

			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			//--------------------Sort recursively the two parts that the table got divided to-----------------------//
			///////////////////////////////////////////////////////////////////////////////////////////////////////////

			QuickSort(start, right_no, fieldNo);
			QuickSort(right+sizeof(Record), rec_num-right_no-1, fieldNo);

			break;


		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		//----------------------------------Sort by record's city (char[20])-------------------------------------//
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		case 3:

			left_string = malloc(sizeof(test->city));
			right_string = malloc(sizeof(test->city));
			pivot_string = malloc(sizeof(test->city));

			memcpy(left_string, left+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
			memcpy(right_string, right+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
			memcpy(pivot_string, last_rec+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));

			while(left_no<right_no){

				while(strcmp(left_string, pivot_string)<0&&left_no<right_no){
					left+=sizeof(Record);
					left_no++;
					memcpy(left_string, left+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
				}
				while(strcmp(right_string, pivot_string)>=0&&left_no<right_no){
					right-=sizeof(Record);
					right_no--;
					memcpy(right_string, right+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
				}


				if(left_no<right_no){
					memcpy(temp_string, left, sizeof(Record));
					memcpy(left, right, sizeof(Record));
					memcpy(right, temp_string, sizeof(Record));
					left+=sizeof(Record);
					right-=sizeof(Record);
					left_no++;
					right_no--;
					memcpy(left_string, left+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
					memcpy(right_string, right+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
				}


			}

			memcpy(left_string, left+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
			memcpy(right_string, right+sizeof(test->id)+sizeof(test->name)+sizeof(test->surname), sizeof(test->city));
			if(strcmp(right_string, pivot_string)<0){
				right+=sizeof(Record);
				right_no++;
			}

			memcpy(temp_string, right, sizeof(Record));
			memcpy(right, last_rec, sizeof(Record));
			memcpy(last_rec, temp_string, sizeof(Record));

			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			//--------------------Sort recursively the two parts that the table got divided to-----------------------//
			///////////////////////////////////////////////////////////////////////////////////////////////////////////

			QuickSort(start, right_no, fieldNo);
			QuickSort(right+sizeof(Record), rec_num-right_no-1, fieldNo);

			break;

	}

	return;
}