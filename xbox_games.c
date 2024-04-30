#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pds_sample.h"
#include "xbox_games.h"

// Add the given contact into the repository by calling put_rec_by_key
int add_game(struct XboxGame *g)
{
	int status;

	status = put_linked_rec_by_key(g->game_id, g);

	if (status != PDS_SUCCESS)
	{
		fprintf(stderr, "Unable to add game with id %d. Error %d", g->game_id, status);
		return GAME_FAILURE;
	}
	return status;
}

int search_game(int game_id, struct XboxGame *g)
{
	return get_rec_by_ndx_key(game_id, g);
}