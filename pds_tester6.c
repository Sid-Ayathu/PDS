#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pds_sample.h"
#include "xbox_account.h"
#include "xbox_games.h"

#define TREPORT(a1, a2) printf("Status: %s - %s\n\n", a1, a2);

void process_line(char *test_case);

int main(int argc, char *argv[])
{
	FILE *cfptr;
	char test_case[50];

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s testcasefile\n", argv[0]);
		exit(1);
	}

	cfptr = (FILE *)fopen(argv[1], "r");
	while (fgets(test_case, sizeof(test_case) - 1, cfptr))
	{
		// printf("line:%s",test_case);
		if (!strcmp(test_case, "\n") || !strcmp(test_case, ""))
			continue;
		process_line(test_case);
	}
}

void process_line(char *test_case)
{
	char repo_name[30], linked_repo_name[30], phone[15];
	char command[15], param1[15], param2[15], param3[15], info[1024];
	int gamertag, game_id, status, rec_size, linked_rec_size, expected_status;

	struct XboxAccount *testAccount = (struct XboxAccount *)malloc(sizeof(struct XboxAccount));
	struct XboxGame *testGame = (struct XboxGame *)malloc(sizeof(struct XboxGame));

	rec_size = sizeof(struct XboxAccount);
	linked_rec_size = sizeof(struct XboxGame);

	sscanf(test_case, "%s%s%s%s", command, param1, param2, param3);
	printf("Test case: %s", test_case);

	if (!strcmp(command, "CREATE"))
	{
		// printf("test1");
		strcpy(repo_name, param1);
		strcpy(linked_repo_name, param2);
		if (!strcmp(param3, "0"))
			expected_status = ACCOUNT_SUCCESS;
		else
			expected_status = ACCOUNT_FAILURE;

		status = pds_create(repo_name, linked_repo_name);
		if (status == PDS_SUCCESS)
			status = ACCOUNT_SUCCESS;
		else
			status = ACCOUNT_FAILURE;
		if (status == expected_status)
		{
			// printf("hello");
			TREPORT("PASS", "");
		}
		else
		{
			// printf("hello1");
			sprintf(info, "pds_create returned status %d", status);
			TREPORT("FAIL", info);
		}
	}

	else if (!strcmp(command, "OPEN"))
	{
		strcpy(repo_name, param1);
		strcpy(linked_repo_name, param2);
		if (!strcmp(param3, "0"))
			expected_status = ACCOUNT_SUCCESS;
		else
			expected_status = ACCOUNT_FAILURE;

		status = pds_open(repo_name, linked_repo_name, rec_size, linked_rec_size);
		if (status == PDS_SUCCESS)
			status = ACCOUNT_SUCCESS;
		else
			status = ACCOUNT_FAILURE;
		if (status == expected_status)
		{
			TREPORT("PASS", "");
		}
		else
		{
			sprintf(info, "pds_open returned status %d", status);
			TREPORT("FAIL", info);
		}
	}

	else if (!strcmp(command, "STORE"))
	{
		if (!strcmp(param3, "0"))
			expected_status = ACCOUNT_SUCCESS;
		else
			expected_status = ACCOUNT_FAILURE;

		sscanf(param1, "%d", &gamertag);
		sscanf(param2, "%s", phone);
		testAccount->gamertag = gamertag;
		strcpy(testAccount->phone, phone);
		sprintf(testAccount->phone, "Task_%d", gamertag);
		sprintf(testAccount->username, "Username-of-%d", gamertag);
		status = add_account(testAccount);
		if (status == PDS_SUCCESS)
			status = ACCOUNT_SUCCESS;
		else
			status = ACCOUNT_FAILURE;
		if (status == expected_status)
		{
			TREPORT("PASS", "");
		}
		else
		{
			sprintf(info, "add_account returned status %d", status);
			TREPORT("FAIL", info);
		}
	}

	else if (!strcmp(command, "ADD_MEMBER"))
	{
		if (!strcmp(param2, "0"))
			expected_status = GAME_SUCCESS;
		else
			expected_status = GAME_FAILURE;

		sscanf(param1, "%d", &game_id);
		testGame->game_id = game_id;
		sprintf(testGame->title, "Video game Title-of-%d", game_id);
		sprintf(testGame->ISBN, "ISBN-of-%d", game_id);
		status = add_game(testGame);
		if (status == PDS_SUCCESS)
			status = GAME_SUCCESS;
		else
			status = GAME_FAILURE;
		if (status == expected_status)
		{
			TREPORT("PASS", "");
		}
		else
		{
			sprintf(info, "add_game returned status %d", status);
			TREPORT("FAIL", info);
		}
	}

	else if (!strcmp(command, "NDX_SEARCH"))
	{
		if (!strcmp(param2, "0"))
			expected_status = ACCOUNT_SUCCESS;
		else
			expected_status = ACCOUNT_FAILURE;

		sscanf(param1, "%d", &gamertag);
		testAccount->gamertag = -1;
		status = search_account(gamertag, testAccount);
		if (status == PDS_SUCCESS)
			status = ACCOUNT_SUCCESS;
		else
			status = ACCOUNT_FAILURE;
		if (status != expected_status)
		{
			sprintf(info, "search tag: %d; Got status %d", gamertag, status);
			TREPORT("FAIL", info);
		}
		else
		{
			// Check if the retrieved values match
			char expected_phone[30];
			sprintf(expected_phone, "Task_%d", gamertag);
			char expected_username[30];
			sprintf(expected_username, "Username-of-%d", gamertag);
			if (expected_status == 0)
			{
				if (testAccount->gamertag == gamertag &&
					strcmp(testAccount->username, expected_username) == 0 &&
					strcmp(testAccount->phone, expected_phone) == 0)
				{
					TREPORT("PASS", "");
				}
				else
				{
					sprintf(info, "Account data not matching... Expected:{%d,%s,%s} Got:{%d,%s,%s}\n",
							gamertag, expected_username, expected_phone,
							testAccount->gamertag, testAccount->username, testAccount->phone);
					TREPORT("FAIL", info);
				}
			}
			else
				TREPORT("PASS", "");
		}
	}

	else if (!strcmp(command, "NON_NDX_SEARCH"))
	{
		char phone_num[30], expected_username[30], expected_phone[30];
		int expected_io, actual_io;

		if (strcmp(param2, "-1") == 0)
			expected_status = ACCOUNT_FAILURE;
		else
			expected_status = ACCOUNT_SUCCESS;

		//sscanf(param1, "%s", phone_num);
		strcpy(phone_num,param1);
		//printf("param1 %s \n",param1);
		sscanf(param2, "%d", &expected_io);
		testAccount->gamertag = -1;
		actual_io = 0;
		status = search_account_by_phone(phone_num, testAccount, &actual_io);
		if (status == PDS_SUCCESS)
			status = ACCOUNT_SUCCESS;
		else
			status = ACCOUNT_FAILURE;
		if (status != expected_status)
		{
			sprintf(info, "search tag: %d; Got status %d", gamertag, status);
			TREPORT("FAIL", info);
		}
		else
		{
			// Check if the retrieved values match
			// Check if num block accesses match too
			// Extract the expected contact_id from the phone number
			sscanf(phone_num, "Task_%d", &gamertag);
			//printf("%s \n",phone_num);
			sprintf(expected_username, "Username-of-%d", gamertag);
			sprintf(expected_phone, "Task_%d", gamertag);
			//printf("%d \n", gamertag);
			if (expected_status == 0)
			{
				if (testAccount->gamertag == gamertag &&
					strcmp(testAccount->username, expected_username) == 0 &&
					strcmp(testAccount->phone, expected_phone) == 0)
				{
					
					if (expected_io == actual_io)
					{
						TREPORT("PASS", "");
					}
					else
					{
						sprintf(info, "Num I/O not matching for contact %d... Expected:%d Got:%d\n",
								gamertag, expected_io, actual_io);
						TREPORT("FAIL", info);
					}
				}
				else
				{
					sprintf(info, "ACcount data not matching... Expected:{%d,%s,%s} Got:{%d,%s,%s}\n",
							gamertag, expected_username, expected_phone,
							testAccount->gamertag, testAccount->username, testAccount->phone);
					TREPORT("FAIL", info);
				}
			}
			else
				TREPORT("PASS", "");
		}
	}

	else if (!strcmp(command, "LINKED_REC_SEARCH"))
	{
		if (!strcmp(param2, "0"))
			expected_status = GAME_SUCCESS;
		else
			expected_status = GAME_FAILURE;

		sscanf(param1, "%d", &game_id);
		testGame->game_id = -1;
		status = search_game(game_id, testGame);
		if (status == PDS_SUCCESS)
			status = GAME_SUCCESS;
		else
			status = GAME_FAILURE;
		if (status != expected_status)
		{
			sprintf(info, "search id: %d; Got status %d", game_id, status);
			TREPORT("FAIL", info);
		}
		else
		{
			// Check if the retrieved values match
			char expected_isbn[30];
			sprintf(expected_isbn, "isbn-of-%d", game_id);
			char expected_title[30];
			sprintf(expected_title, "title-of-%d", game_id);
			if (expected_status == 0)
			{
				if (testGame->game_id == game_id &&
					strcmp(testGame->title, expected_title) == 0 &&
					strcmp(testGame->ISBN, expected_isbn) == 0)
				{
					TREPORT("PASS", "");
				}
				else
				{
					sprintf(info, "Game data not matching... Expected:{%d,%s,%s} Got:{%d,%s,%s}\n",
							game_id, expected_title, expected_isbn,
							testGame->game_id, testGame->title, testGame->ISBN);
					TREPORT("FAIL", info);
				}
			}
			else
				TREPORT("PASS", "");
		}
	}

	else if (!strcmp(command, "NDX_DELETE"))
	{
		if (strcmp(param2, "0") == 0)
			expected_status = ACCOUNT_SUCCESS;
		else
			expected_status = ACCOUNT_FAILURE;

		sscanf(param1, "%d", &gamertag);
		testAccount->gamertag = -1;
		status = delete_account(gamertag);
		if (status != expected_status)
		{
			sprintf(info, "delete tag: %d; Got status %d", gamertag, status);
			TREPORT("FAIL", info);
		}
		else
		{
			TREPORT("PASS", "");
		}
	}

	else if (!strcmp(command, "ADD_LINK"))
	{ // LINK gamertag game_id status
		if (strcmp(param3, "0") == 0)
			expected_status = ACCOUNT_SUCCESS;
		else
			expected_status = ACCOUNT_FAILURE;
		sscanf(param1, "%d", &gamertag);
		sscanf(param2, "%d", &game_id);
		status = pds_link_rec(gamertag, game_id);
		if (status != expected_status)
		{
			sprintf(info, "link tag and id: %d & %d; Got status %d", gamertag, game_id, status);
			TREPORT("FAIL", info);
		}
		else
		{
			TREPORT("PASS", "");
		}
	}

	else if (!strcmp(command, "GET_MEMBERS"))
	{
		int linked_games_result[50];
		int no_of_games = 0;
		int games;
		// printf("%d", no_of_games);
		int verify_status;
		struct XboxGame *temp_game = (struct XboxGame *)malloc(sizeof(struct XboxGame));
		// printf("hi");
		
		
		sscanf(param1, "%d", &gamertag);
		sscanf(param2 , "%d", &games);

		status = get_games(gamertag, linked_games_result, &no_of_games);
		//printf("number_of_games %d and games %d",no_of_games,games );
		if (no_of_games != games)
		{
			sprintf(info, "getlink tag : %d; Got status %d", gamertag, status);
			TREPORT("FAIL", info);
		}
		else
		{
			TREPORT("PASS", "");
			if (verify_status != PDS_SUCCESS)
			{
				sprintf(info, "getlink tag : %d ; Got status %d", gamertag, status);
				TREPORT("FAIL", info);
			}
		}
	}

	else if (!strcmp(command, "CLOSE"))
	{
		if (!strcmp(param1, "0"))
			expected_status = ACCOUNT_SUCCESS;
		else
			expected_status = ACCOUNT_FAILURE;

		status = pds_close();
		if (status == PDS_SUCCESS)
			status = ACCOUNT_SUCCESS;
		else
			status = ACCOUNT_FAILURE;
		if (status == expected_status)
		{
			TREPORT("PASS", "");
		}
		else
		{
			sprintf(info, "pds_close returned status %d", status);
			TREPORT("FAIL", info);
		}
	}
}
