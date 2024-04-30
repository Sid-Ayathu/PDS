#ifndef XBOX_ACCOUNT_H
#define XBOX_ACCOUNT_H

#define ACCOUNT_SUCCESS 0
#define ACCOUNT_FAILURE 1

struct XboxAccount
{
	int gamertag;
	char username[30];
	char phone[15];
};

extern struct PDS_RepoInfo *repoHandle;

// Add the given contact into the repository by calling put_rec_by_key
int add_account(struct XboxAccount *acc);

// Use get_rec_by_key function to retrieve contact
int search_account(int gamertag, struct XboxAccount *acc);

int search_account_by_phone(char *phone, struct XboxAccount *acc, int *io_count);

/* Return 0 if phone of the contact matches with phone parameter */
/* Return 1 if phone of the contact does NOT match */
/* Return > 1 in case of any other error */
int match_account_phone(void *rec, void *key);

int delete_account(int gamertag);

int get_games(int gamertag, int linked_games_result[], int *no_of_games);
#endif