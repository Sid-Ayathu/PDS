#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pds_sample.h"
#include "xbox_account.h"
#include "xbox_games.h"

int add_account(struct XboxAccount *acc)
{
	int status;

	status = put_rec_by_key(acc->gamertag, acc);

	if (status != PDS_SUCCESS)
	{
		fprintf(stderr, "Unable to add account with key %d. Error %d", acc->gamertag, status);
		return ACCOUNT_FAILURE;
	}
	return status;
}

// Use get_rec_by_key function to retrieve contact
int search_account(int gamertag, struct XboxAccount *acc)
{
	return get_rec_by_ndx_key(gamertag, acc);
}

int search_account_by_phone(char *phone, struct XboxAccount *acc, int *io_count){
	int status;
	status = get_rec_by_non_ndx_key(phone, acc, match_account_phone, io_count);
	//printf("%s \n",acc->phone);
	if (status != PDS_SUCCESS)
	{
		return ACCOUNT_FAILURE;
	}
	return ACCOUNT_SUCCESS;
	// Call function
}

/* Return 0 if phone of the contact matches with phone parameter */
/* Return 1 if phone of the contact does NOT match */
/* Return > 1 in case of any other error */
int match_account_phone(void *rec, void *key)
{
	struct XboxAccount *acc = rec;
	char phone[15];
	strcpy(phone, key);
	if (strcmp(acc->phone, phone) == 0)
		return 0;
	else if (strcmp(acc->phone, phone) != 0)
		return 1;
	else
	{
		return 2;
	}

	// Store the rec in a struct contact pointer
	// Store the key in a char pointer
	// Compare the phone values in key and record
	// Return 0,1,>1 based on above condition
}

int delete_account(int gamertag)
{
	int status = delete_rec_by_ndx_key(gamertag);
	return status;
}
int get_games(int gamertag, int linked_games_result[], int *no_of_games){
	int status = pds_get_linked_rec(gamertag, linked_games_result, no_of_games);
	return status;
}
