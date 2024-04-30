#ifndef XBOX_GAMES_H
#define XBOX_GAMES_H

#define GAME_SUCCESS 0
#define GAME_FAILURE 1

struct XboxGame
{
	int game_id;
	char title[30];
	char ISBN[15];
};

extern struct PDS_RepoInfo *repoHandle;

// Add the given contact into the repository by calling put_rec_by_key
int add_game(struct XboxGame *g);

// Use get_rec_by_key function to retrieve contact
int search_game(int game_id, struct XboxGame *g);

#endif