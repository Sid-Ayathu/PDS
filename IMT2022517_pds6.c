#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pds_sample.h"
#include "bst.h"
#include "xbox_account.h"
#include "xbox_games.h"

struct PDS_RepoInfo repo_handle;

// pds_create
// Open the data file and index file in "wb" mode
// Initialize index file by storing "0" to indicate there are zero entries in index file
// close the files
int pds_create(char *repo_name, char *linked_repo_name)
{
	// creating empty repo data file
	char repo_filename[30];
	strcpy(repo_filename, repo_name);
	strcat(repo_filename, ".dat");
	FILE *data_fp = fopen(repo_filename, "wb");

	// creating empty index file(for the data file)
	char index_filename[30];
	strcpy(index_filename, repo_name);
	strcat(index_filename, ".ndx");
	FILE *index_fp = fopen(index_filename, "wb");

	// creating empty linked repo data file
	char linked_repo_filename[30];
	strcpy(linked_repo_filename, linked_repo_name);
	strcat(linked_repo_filename, ".dat");
	FILE *linked_repo_fp = fopen(linked_repo_filename, "wb");

	// creating empty link data file
	char link_data_filename[30];
	strcpy(link_data_filename, repo_name);
	strcat(link_data_filename, "_");
	strcat(link_data_filename, linked_repo_name);
	strcat(link_data_filename, ".dat");
	FILE *link_data_fp = fopen(link_data_filename, "wb");

	if (data_fp == NULL || index_fp == NULL || linked_repo_fp == NULL || link_data_fp == NULL)
	{
		return PDS_FILE_ERROR;
	}
	int zero = 0;
	fwrite(&zero, sizeof(int), 1, index_fp);
	fclose(data_fp);
	fclose(index_fp);
	fclose(linked_repo_fp);
	fclose(link_data_fp);
	return PDS_SUCCESS;
}

// pds_open - CHANGED
// Open the main data file and index file in rb+ mode
// If linked_repo_name is NOT NULL
//     Open the linked data file in rb+ mode (there is no index file for linked data)
//     Open the link file in rb+ mode
// end if
// Update the fields of PDS_RepoInfo appropriately
// Build BST and store in pds_bst by reading index entries from the index file
// Close only the index file
int pds_open(char *repo_name, char *linked_repo_name, int rec_size, int linked_rec_size)
{
	if (repo_handle.repo_status == PDS_REPO_OPEN || repo_handle.repo_status == PDS_REPO_ALREADY_OPEN)
	{
		repo_handle.repo_status = PDS_REPO_ALREADY_OPEN;
		return PDS_FILE_ERROR;
	}
	// opening the empty repo data file
	char repo_filename[30];
	strcpy(repo_filename, repo_name);
	strcat(repo_filename, ".dat");
	FILE *data_fp = fopen(repo_filename, "rb+");

	// opening the empty index file(for the data file)
	char index_filename[30];
	strcpy(index_filename, repo_name);
	strcat(index_filename, ".ndx");
	FILE *index_fp = fopen(index_filename, "rb+");

	if (linked_repo_name == NULL)
	{
		return PDS_REC_NOT_FOUND;
	}
	else
	{
		// opening the empty linked repo data file
		char linked_repo_filename[30];
		strcpy(linked_repo_filename, linked_repo_name);
		strcat(linked_repo_filename, ".dat");
		FILE *linked_repo_fp = fopen(linked_repo_filename, "rb+");

		// opening the empty link data file
		char link_data_filename[30];
		strcpy(link_data_filename, repo_name);
		strcat(link_data_filename, "_");
		strcat(link_data_filename, linked_repo_name);
		strcat(link_data_filename, ".dat");
		FILE *link_data_fp = fopen(link_data_filename, "rb+");

		// Updating the fields of PDS_RepoInfo
		strcpy(repo_handle.pds_name, repo_name);
		repo_handle.pds_data_fp = data_fp;
		repo_handle.pds_linked_data_fp = linked_repo_fp; // NEW
		repo_handle.pds_link_fp = link_data_fp;			 // NEW
		repo_handle.pds_ndx_fp = index_fp;
		repo_handle.repo_status = PDS_REPO_OPEN;
		repo_handle.rec_size = rec_size;			   // For fixed length records for main repo
		repo_handle.linked_rec_size = linked_rec_size; // NEW
		int pds_load_status = pds_load_ndx();
		int fclose_status = fclose(index_fp);
		if (pds_load_status == PDS_FILE_ERROR || fclose_status != 0)
		{
			return PDS_FILE_ERROR;
		}
		return PDS_SUCCESS;
	}
}

// pds_load_ndx - Internal function
// Load the index entries into the BST ndx_root by calling bst_add_node repeatedly for each
// index entry. Unlike array, for BST, you need read the index file one by one in a loop
int pds_load_ndx()
{
	char index_filename[30];
	strcpy(index_filename, repo_handle.pds_name);
	strcat(index_filename, ".ndx");
	FILE *index_fp = fopen(index_filename, "rb+");
	int rec_read_status = fread(&repo_handle.rec_count, sizeof(int), 1, index_fp);

	struct BST_Node *ndx_root = NULL;
	repo_handle.pds_bst = ndx_root;
	// int i2 = 0;
	for (int i = 0; i < repo_handle.rec_count; i++)
	{
		struct PDS_NdxInfo *temp_index = (struct PDS_NdxInfo *)malloc(sizeof(struct PDS_NdxInfo));
		fread(temp_index, sizeof(struct PDS_NdxInfo), 1, index_fp);
		bst_add_node(&repo_handle.pds_bst, temp_index->key, temp_index);
	}
	fclose(index_fp);

	if (rec_read_status != 1)
		return PDS_FILE_ERROR;

	return PDS_SUCCESS;
}

// put_rec_by_key
// Seek to the end of the data file
// Create an index entry with the current data file location using ftell
// Add index entry to BST by calling bst_add_node
// Increment record count
// Write the record at the end of the file
// Return failure in case of duplicate key
int put_rec_by_key(int key, void *rec)
{
	if (repo_handle.repo_status != PDS_REPO_OPEN && repo_handle.repo_status != PDS_REPO_ALREADY_OPEN)
		return PDS_ADD_FAILED;
	if (repo_handle.rec_count == MAX_NDX_SIZE)
		return PDS_ADD_FAILED;

	if (bst_search(repo_handle.pds_bst, key) != NULL)
	{
		struct BST_Node *node_found = bst_search(repo_handle.pds_bst, key);

		struct PDS_NdxInfo *node_data = (struct PDS_NdxInfo *)malloc(sizeof(struct PDS_NdxInfo));
		node_data = (struct PDS_NdxInfo *)node_found->data;
		if (node_data->is_deleted == 0)
		{
			return PDS_ADD_FAILED;
		}
		else
		{
			node_data->is_deleted = 0;
			return PDS_SUCCESS;
		}
	}

	FILE *data_fp = repo_handle.pds_data_fp;
	fseek(data_fp, 0, SEEK_END);

	int *location = (int *)malloc(sizeof(int));
	*location = ftell(data_fp);
	int number_of_records = repo_handle.rec_count;

	struct BST_Node *ndx_record = (struct BST_Node *)malloc(sizeof(struct BST_Node));

	struct PDS_NdxInfo *pds_data = (struct PDS_NdxInfo *)malloc(sizeof(struct PDS_NdxInfo));
	pds_data->key = key;
	pds_data->offset = *location;
	pds_data->is_deleted = 0;

	ndx_record->data = pds_data;
	int add_node_status = bst_add_node(&repo_handle.pds_bst, key, pds_data);
	if (add_node_status != BST_SUCCESS)
	{
		return PDS_ADD_FAILED;
	}

	int fwrite1_status = fwrite(&key, sizeof(int), 1, data_fp);
	int fwrite2_status = fwrite(rec, repo_handle.rec_size, 1, data_fp);

	repo_handle.rec_count++;

	if (fwrite1_status != 1 || fwrite2_status != 1)
	{
		return PDS_ADD_FAILED;
	}
	return PDS_SUCCESS;
}

// put_linked_rec_by_key - NEW
// Seek to the end of the linked data file
// No need to create index entry
// Write the key at the current data file location
// Write the record after writing the key
int put_linked_rec_by_key(int key, void *rec)
{
	if (repo_handle.pds_linked_data_fp == NULL)
	{
		return PDS_REC_NOT_FOUND;
	}
	else
	{
		FILE *linked_repo_fp = repo_handle.pds_linked_data_fp;
		int fseek_status = fseek(linked_repo_fp, 0, SEEK_END);

		if (fseek_status != 0)
			return PDS_ADD_FAILED;

		// writing into the linked_data file
		int fwrite1_status = fwrite(&key, sizeof(int), 1, linked_repo_fp);
		int fwrite2_status = fwrite(rec, repo_handle.rec_size, 1, linked_repo_fp);

		if (fwrite1_status != 1 || fwrite2_status != 1)
		{
			return PDS_ADD_FAILED;
		}
		return PDS_SUCCESS;
	}
}

// get_rec_by_key
// Search for index entry in BST by calling bst_search
// Seek to the file location based on offset in index entry
// Read the record from the current location
int get_rec_by_ndx_key(int key, void *rec)
{
	struct BST_Node *node = bst_search(repo_handle.pds_bst, key);
	if (node == NULL)
	{
		return PDS_REC_NOT_FOUND;
	}
	else
	{
		struct PDS_NdxInfo *data_temp = (struct PDS_NdxInfo *)malloc(sizeof(struct PDS_NdxInfo));
		data_temp = (struct PDS_NdxInfo *)node->data;

		int offset_temp = data_temp->offset;

		if (data_temp->is_deleted == 1)
		{
			return PDS_REC_NOT_FOUND;
		}

		FILE *data_fp = repo_handle.pds_data_fp;
		fseek(data_fp, offset_temp, SEEK_SET);
		int temp_key;
		fread(&temp_key, sizeof(int), 1, data_fp);
		fread(rec, repo_handle.rec_size, 1, data_fp);
		return PDS_SUCCESS;
	}
}

// Brute-force retrieval using an arbitrary search key
// io_count = 0
// Loop through data file till EOF
//	fread the key and record
//	io_count++
//      Invoke the matcher with the current record and input non-ndx search key
//	if mathcher returns success, return the current record, else continue the loop
// end loop
int get_rec_by_non_ndx_key(void *non_ndx_key, void *rec, int (*matcher)(void *rec, void *non_ndx_key), int *io_count)
{
	FILE *fp = repo_handle.pds_data_fp;
	fseek(repo_handle.pds_data_fp, 0, SEEK_SET);
	int key;
	char phone[15];
	*io_count = 0;
	strcpy(phone, non_ndx_key);
	for (int i = 0; i < repo_handle.rec_count; i++)
	{
		*io_count += 1;

		fread(&key, sizeof(int), 1, repo_handle.pds_data_fp);
		fread(rec, repo_handle.rec_size, 1, repo_handle.pds_data_fp);
		if (matcher(rec, non_ndx_key) == 0)
		{
			return PDS_SUCCESS;
		}
	}
	return PDS_REC_NOT_FOUND;
}

// get_linked_rec_by_key - NEW
// Do a linear search of the given key in the linked data file
int get_linked_rec_by_key(int key, void *rec)
{
	if (repo_handle.pds_linked_data_fp == NULL)
	{
		return PDS_REC_NOT_FOUND;
	}
	else
	{
		FILE *linked_repo_fp = repo_handle.pds_linked_data_fp;
		int linkfile_key;
		for (int i = 0; i < repo_handle.linked_rec_size; i++)
		{

			fread(&linkfile_key, sizeof(int), 1, linked_repo_fp);
			if (key == linkfile_key)
			{
				fread(rec, repo_handle.rec_size, 1, linked_repo_fp);
				return PDS_SUCCESS;
			}
		}
	}
}

void getPreOrder(struct BST_Node *node, FILE *index_file)
{
	if (node == NULL)
		return;

	struct PDS_NdxInfo *ndx_info = (struct PDS_NdxInfo *)node->data;

	fwrite(ndx_info, sizeof(struct PDS_NdxInfo), 1, index_file);

	getPreOrder(node->left_child, index_file);

	getPreOrder(node->right_child, index_file);
}

int delete_rec_by_ndx_key(int key)
{
	struct BST_Node *node = bst_search(repo_handle.pds_bst, key);
	if (node == NULL)
		return PDS_FILE_ERROR;
	else
	{
		struct PDS_NdxInfo *pds_data = (struct PDS_NdxInfo *)malloc(sizeof(struct PDS_NdxInfo));
		pds_data = node->data;
		if (pds_data->is_deleted == 1)
		{
			return PDS_FILE_ERROR;
		}
		else
		{
			pds_data->is_deleted = 1;
		}
		return PDS_SUCCESS;
	}
}

// pds_link_rec - NEW
// Create PDS_link_info instance based on key1 and key2
// Go to the end of the link file
// Store the PDS_link_info record
int pds_link_rec(int key1, int key2)
{
	if (repo_handle.pds_link_fp == NULL)
	{
		return PDS_REC_NOT_FOUND;
	}
	else
	{
		struct XboxAccount* temp_acc = (struct XboxAccount* )malloc(sizeof(struct XboxAccount));
		int parent_status = get_rec_by_ndx_key(key1, temp_acc);

		if(parent_status != PDS_SUCCESS){
			return  PDS_FILE_ERROR;
		}
		else{
			struct PDS_link_info *temp_link = (struct PDS_link_info *)malloc(sizeof(struct PDS_link_info));
			temp_link->parent_key = key1;
			temp_link->child_key = key2;

			FILE *datalink_fp = repo_handle.pds_link_fp;
			int fseek_status = fseek(datalink_fp, 0, SEEK_END);

			if (fseek_status != 0)
			{
				return PDS_FILE_ERROR;
			}
			else
			{
				int write_status = fwrite(temp_link, sizeof(struct PDS_link_info), 1, datalink_fp);

				if (write_status != 1)
					return PDS_FILE_ERROR;

				return PDS_SUCCESS;
			}
		}
		
	}
}

// pds_get_linked_rec - NEW
// Go to the beginning of the link file
// Reset result_set_size to 0
// Do a linear search of all link_info records for matching the given parent_key
// Store the matching linked key in linked_keys_result array
// Keep updating the result_set_size
int pds_get_linked_rec(int parent_key, int linked_keys_result[], int *result_set_size)
{
	if (repo_handle.pds_link_fp == NULL)
	{
		return PDS_REC_NOT_FOUND;
	}
	else
	{
		FILE *datalink_fp = repo_handle.pds_link_fp;
		int fseek_status = fseek(datalink_fp, 0, SEEK_SET);

		if (fseek_status != 0)
			return PDS_FILE_ERROR;

		*result_set_size = 0; // resetting set_size to zero
		struct PDS_link_info *temp_datalink = (struct PDS_link_info *)malloc(sizeof(struct PDS_link_info));
		while (fread(temp_datalink, sizeof(struct PDS_link_info), 1, datalink_fp))
		{
			int temp_p_key = temp_datalink->parent_key;
			int temp_c_key = temp_datalink->child_key;
			//printf("%d \n", temp_c_key);
			if (temp_p_key == parent_key)
			{
				
				linked_keys_result[*result_set_size] = temp_c_key;
				(*result_set_size)++;
				//printf("%d \n",*result_set_size);
			}
		}
		return PDS_SUCCESS;
	}
}

// pds_close - CHANGED
// Open the index file in wb mode (write mode, not append mode)
// Unload the BST into the index file by traversing it in PRE-ORDER (overwrite the entire index file)
// Free the BST by call bst_destroy()
// Close the index file, data file and linked data file
int pds_close()
{
	if (repo_handle.repo_status == PDS_REPO_OPEN || repo_handle.repo_status == PDS_REPO_ALREADY_OPEN)
	{
		char index_filename[30];
		strcpy(index_filename, repo_handle.pds_name);
		strcat(index_filename, ".ndx");
		FILE *index_fp = fopen(index_filename, "wb");

		fwrite(&repo_handle.rec_count, sizeof(int), 1, index_fp);
		getPreOrder(repo_handle.pds_bst, index_fp);
		fclose(index_fp);
		fclose(repo_handle.pds_data_fp);
		fclose(repo_handle.pds_linked_data_fp);
		fclose(repo_handle.pds_link_fp);
		repo_handle.repo_status = PDS_REPO_CLOSED;

		bst_free(repo_handle.pds_bst);
		repo_handle.pds_bst = NULL;

		return PDS_SUCCESS;
	}
	else
	{
		return PDS_NDX_SAVE_FAILED;
	}
}
