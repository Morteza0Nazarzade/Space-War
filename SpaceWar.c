#include <stdio.h>
#include <windows.h>
#include <wchar.h>
#include <io.h>
#include <fcntl.h>
#include <locale.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <mmsystem.h>
#pragma warning (disable:4996)
#pragma comment(lib, "winmm.lib")

#define HEART '+'
#define PORTAL 0x058D
#define BLACKHOLE '@'
#define BLOCK 0x2588
#define MIRROR 0x2551
#define SHIP_DOWN 0x25BC
#define SHIP_RIGHT 0x25BA
#define SHIP_UP 0x25B2
#define SHIP_LEFT 0x25C4
#define SPACE 0x0020
#define GHOST '*'
#define POWER_UP '#'
#define GERNADE 0x01A0

#define BLACK 30
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37

int count_id = 0;
int is_playing = 0;
float seconds = 0;
clock_t start = 0, end = 0;

typedef struct player_data
{
	char username[101];
	char password[50];
	char email[101];
	int id;
}data;
typedef struct object
{
	COORD position = { 0, 0 };
	wchar_t output;
	int color = WHITE;
}object;
typedef struct match_result
{
	int winner_id;
	int loser_id;
	int id1;
	int id2;
	int id3 = -1;
	char result[6];
}match;

BOOL is_pressed(int key)
{
	return GetAsyncKeyState(key);
}

void rewrite_account(data* player, char name[101], char pass[50], char email[50])
{
	FILE* fptr;
	data temp, sample = *player;
	strcpy(player->username, name);
	strcpy(player->password, pass);
	strcpy(player->email, email);
	fptr = fopen("saved_data.txt", "r+");
	int result, size = -1 * sizeof(data);

	if (fptr == NULL)
	{
		wprintf(L"Error");
		return;
	}

	while (!feof(fptr))
	{
		fread(&temp, sizeof(data), 1, fptr);

		if (!strcmp(sample.username, temp.username))
		{
			fseek(fptr, size, SEEK_CUR);
			fwrite(player, sizeof(data), 1, fptr);
			break;
		}
	}

	fclose(fptr);
}

void clearInputBuffer()
{
	while (_kbhit())
		_getch();
}

int is_email(char input[101])
{
	int index = -1;
	for (int i = 0; i < strlen(input); i++)
	{
		if (input[i] == '@')
		{
			index = i;
			break;
		}
	}

	if (index < 0)
		return 0;

	if ((!strcmp(&input[index + 1], "gmail.com")) || (!strcmp(&input[index + 1], "yahoo.com")))
		return 1;

	return 0;
}

void play_battle()
{
	is_playing = 1;
	PlaySound(TEXT("battle.wav"), NULL, SND_FILENAME | SND_ASYNC);

}

void play_menu()
{
	is_playing = 1;
	PlaySound(TEXT("menu.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

void stop_playing()
{
	is_playing = 0;
	PlaySound(NULL, NULL, 0);
}

int pass_compare(char* str1, char* str2)
{
	if (strlen(str1) - 1 != strlen(str2))
		return 0;

	for (int i = 0; i < strlen(str1) - 1; i++)
	{
		if (str1[i] != str2[i])
			return 0;
	}

	return 1;
}

void add_history(match this_game, data winner, data loser, int wins_winner, int wins_loser)
{
	match new_match;
	char result[6];
	sprintf(result, "%d : %d", wins_winner, wins_loser);
	new_match = { winner.id, loser.id };
	strcpy(new_match.result, result);
	new_match.id1 = this_game.id1;
	new_match.id2 = this_game.id2;
	new_match.id3 = this_game.id3;


	FILE* fptr;
	fptr = fopen("history.txt", "a");

	if (fptr == NULL)
	{
		wprintf(L"Error");
		return;
	}

	fwrite(&new_match, sizeof(match), 1, fptr);
	fclose(fptr);
}

void update_UI(HANDLE hSdtout, data player1, data player2, int lives1, int lives2, int wins1, int wins2)
{
	COORD UI = { 80, 21 };
	SetConsoleCursorPosition(hSdtout, UI);
	if (lives1 < 0)
		lives1 = 0;
	if (lives2 < 0)
		lives2 = 0;
	_setmode(_fileno(stdout), 0x4000);
	printf("\r\t\033[1;%dm%s : %d%d\t\t\t%d\033[0m : \033[1;%dm%d\t\t\t%s : %d%d\033[0m", BLUE, player1.username, lives1 / 10, lives1 % 10, wins1, RED, wins2, player2.username, lives2 / 10, lives2 % 10);
	_setmode(_fileno(stdout), 0x20000);
}

void player_death(HANDLE hStdout, object* ship, COORD pos, int* lives)
{
	SetConsoleCursorPosition(hStdout, ship->position);
	wprintf(L"\033[1;%dm!\033[0m", ship->color);
	ship->position = pos;
	ship->output = ship->position.X  <= 20 ? SHIP_RIGHT : SHIP_LEFT;
	*lives -= 1;
}

void show_screen(HANDLE hStdout, COORD position, wchar_t new_char, int color)
{
	SetConsoleCursorPosition(hStdout, position);
	wprintf(L"\033[1;%dm%lc\033[0m", color, new_char);
}

int mask(char pass[50])
{
	char str[50];
	char input;
	int i = 0;

	input = _getch();
	while (input != '\r')
	{
		str[i] = input;
		i++;
		wprintf(L"*");
		input = _getch();
	}
	str[i] = '\0';

	if (i < 8)
	{
		if (!strcmp(str, "-1"))
			return -1;
		else
			return 0;
	}
	strcpy(pass, str);
	return 1;
}

void update_id()
{
	FILE* fptr;
	data temp;
	fptr = fopen("saved_data.txt", "r");

	if (fptr == NULL)
		return;

	while (fread(&temp, sizeof(data), 1, fptr))
		count_id++;

	fclose(fptr);
}

int check_if_repeated(char name[51])
{
	FILE* fptr;
	data temp;
	fptr = fopen("saved_data.txt", "r");

	if (fptr == NULL)
	{
		return 0;
	}

	while (!feof(fptr))
	{
		fread(&temp, sizeof(data), 1, fptr);

		if (!strcmp(temp.username, name))
		{
			fclose(fptr);
			return 1;
		}
	}
	fclose(fptr);
	return 0;
}

int find(data* player, char name[51])
{
	FILE* fptr;
	data temp;
	fptr = fopen("saved_data.txt", "r");

	if (fptr == NULL)
		return 0;

	while (!feof(fptr))
	{
		fread(&temp, sizeof(data), 1, fptr);

		if (!strcmp(temp.username, name))
		{
			*player = temp;
			fclose(fptr);
			return 1;
		}
	}

	fclose(fptr);
	return 0;

}

int find_id(data* player, int my_id)
{
	FILE* fptr;
	fptr = fopen("saved_data.txt", "r");
	data temp;

	if (fptr == NULL)
		return 0;

	while (fread(&temp, sizeof(temp), 1, fptr))
	{
		if (temp.id == my_id)
		{
			*player = temp;
			fclose(fptr);
			return 1;
		}
	}
	fclose(fptr);
	return 0;
}

void add(data new_player)
{
	FILE* fptr;
	fptr = fopen("saved_data.txt", "a");

	if (fptr == NULL)
	{
		wprintf(L"Error");
	}

	fwrite(&new_player, sizeof(data), 1, fptr);
	fclose(fptr);
	return;
}

void info(data* player);

void with_email();

void sign_in();

void main_menu();

void game3(match* this_game, data* player1, data* player2, int lives, int wins1, int wins2)
{
	system("cls||clear");
	_setmode(_fileno(stdout), 0x20000);
	setvbuf(stdin, NULL, _IONBF, 0);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	wprintf(L"\n\n\n\n\n\t\t\t\033[1;%dm╔═══════════════╗\n", YELLOW);
	wprintf(L"\t\t\t║\033[1;%dm    ROUND 3    \033[1;%dm║\n", MAGENTA, YELLOW);
	wprintf(L"\t\t\t╚═══════════════╝\n");
	Sleep(1000);
	system("cls||clear");

	int b1_up = 0, b1_down = 0, b1_left = 0, b1_right = 0, b2_up = 0, b2_down = 0, b2_left = 0, b2_right = 0;
	int lives1 = lives, lives2 = lives, power_shot1 = 0, power_shot2 = 0, gernade1 = 0, gernade2 = 0, ghost1 = -1, ghost2 = -1;

	object map[20][80];
	object ship1 = { {8, 10}, SHIP_RIGHT, BLUE };
	object ship2 = { {72, 10}, SHIP_LEFT, RED };
	object bullet1 = { ship1.position, L'.', WHITE };
	object bullet2 = { ship2.position, L'.', WHITE };

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 80; x++)
		{
			if (x == 0 || y == 0 || x == 79 || y == 19)
				map[y][x].output = BLOCK;
			else
				map[y][x].output = SPACE;
			map[y][x].position.X = x;
			map[y][x].position.Y = y;
		}
	}

	map[2][37].output = map[2][38].output = map[2][39].output = map[2][40].output = map[2][41].output = map[5][5].output = map[5][6].output = map[5][7].output = map[5][8].output = map[5][9].output = map[5][10].output = map[5][11].output = map[5][34].output = map[5][35].output = map[5][43].output = map[5][44].output = map[5][69].output = map[5][70].output = map[5][71].output = map[5][72].output = map[5][73].output = map[5][74].output = map[5][68].output = BLOCK;
	map[6][11].output = map[6][34].output = map[6][35].output = map[6][43].output = map[6][44].output = map[6][68].output = map[7][22].output = map[7][23].output = map[7][24].output = map[7][25].output = map[7][26].output = map[7][27].output = map[7][28].output = map[7][29].output = map[7][30].output = map[7][31].output = map[7][32].output = map[7][33].output = map[7][34].output = map[7][35].output = map[7][43].output = map[7][44].output = map[7][45].output = map[7][56].output = map[7][46].output = map[7][47].output = map[7][48].output = map[7][49].output = map[7][50].output = map[7][51].output = map[7][52].output = map[7][53].output = map[7][54].output = map[7][55].output = BLOCK;
	map[12][22].output = map[12][23].output = map[12][24].output = map[12][25].output = map[12][26].output = map[12][27].output = map[12][28].output = map[12][29].output = map[12][30].output = map[12][31].output = map[12][32].output = map[12][33].output = map[12][34].output = map[12][35].output = map[12][43].output = map[12][44].output = map[12][45].output = map[12][46].output = map[12][47].output = map[12][48].output = map[12][49].output = map[12][50].output = map[12][51].output = map[12][52].output = map[12][53].output = map[12][54].output = map[12][55].output = map[12][56].output = map[13][11].output = map[13][34].output = map[13][35].output = map[13][43].output = map[13][44].output = map[13][68].output = BLOCK;
	map[14][5].output = map[14][6].output = map[14][7].output = map[14][8].output = map[14][9].output = map[14][10].output = map[14][11].output = map[14][34].output = map[14][35].output = map[14][43].output = map[14][44].output = map[14][68].output = map[14][69].output = map[14][70].output = map[14][71].output = map[14][72].output = map[14][73].output = map[14][74].output = BLOCK;

	map[1][37].output = map[1][41].output = map[7][11].output = map[7][68].output = map[8][11].output = map[8][28].output = map[8][49].output = map[8][68].output = map[11][11].output = map[11][28].output = map[11][49].output = map[11][68].output = map[12][11].output = map[12][68].output = MIRROR;

	map[2][20].output = map[2][59].output = map[16][39].output = GERNADE;
	map[2][20].color = map[2][59].color = map[16][39].color = GREEN;
	object gernade = { {39, 16}, GERNADE, GREEN };

	map[1][39].output = map[10][39].output = HEART;
	map[1][39].color = map[10][39].color = RED;
	object heart = { {39, 1}, HEART, RED };

	map[10][37].output = map[10][41].output = BLACKHOLE;

	map[9][52].output = map[9][24].output = POWER_UP;
	map[9][52].color = map[9][24].color = YELLOW;
	object power_up = { {24, 9}, POWER_UP, YELLOW };

	map[17][7].output = map[17][74].output = PORTAL;
	map[17][7].color = map[17][74].color = YELLOW;

	map[4][39].output = GHOST;
	object ghost = { {39, 4}, GHOST, WHITE };

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 80; x++)
		{
			wprintf(L"\033[1;%dm%lc\033[0m", map[y][x].color, map[y][x].output);
		}
		wprintf(L"\n");
	}

	update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
	wprintf(L"\n\t\t\t\t\t 3rd");

	show_screen(hStdout, ship1.position, ship1.output, ship1.color);
	show_screen(hStdout, ship2.position, ship2.output, ship2.color);

	while (lives1 > 0 && lives2 > 0)
	{
		if (seconds >= 142)
		{
			play_battle();
			seconds = 0;
		}
		start = end = 0;
		start = clock();
		if (map[heart.position.Y][heart.position.X].output == SPACE && map[10][39].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					heart.position = { (short)x, (short)y };
					map[y][x].output = HEART;
					map[y][x].color = RED;
					show_screen(hStdout, heart.position, heart.output, heart.color);
					count++;
				}
			}
			else
				count++;
		}
		if (map[power_up.position.Y][power_up.position.X].output == SPACE && map[9][52].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					power_up.position = { (short)x, (short)y };
					map[y][x].output = POWER_UP;
					map[y][x].color = YELLOW;
					show_screen(hStdout, power_up.position, power_up.output, power_up.color);
					count++;
				}
			}
			else
				count++;
		}
		if (map[gernade.position.Y][gernade.position.X].output == SPACE && map[2][20].output == SPACE && map[2][59].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					gernade.position = { (short)x, (short)y };
					map[y][x].output = GERNADE;
					map[y][x].color = GREEN;
					show_screen(hStdout, gernade.position, gernade.output, gernade.color);
					count++;
				}
			}
			else
				count++;
		}
		if (map[ghost.position.Y][ghost.position.X].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK && map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					ghost.position = { (short)x, (short)y };
					map[y][x].output = GHOST;
					map[y][x].color = WHITE;
					show_screen(hStdout, ghost.position, ghost.output, ghost.color);
					count++;
				}
			}
			else
				count++;
		}
		if (_kbhit())
		{
			//ship1
			if (is_pressed('W') && ship1.position.Y > 1)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X != ship2.position.X || ship1.position.Y - 1 != ship2.position.Y)
				{
					switch (map[ship1.position.Y - 1][ship1.position.X].output)
					{
					case HEART:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y -= 1;
							ship1.output = SHIP_UP;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 74, 16 };
							ship1.output = SHIP_UP;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 7, 16 };
							ship1.output = SHIP_UP;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y -= 1;
						}
						ship1.output = SHIP_UP;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_UP;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}

			}
			if (is_pressed('S') && ship1.position.Y < 18)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X != ship2.position.X || ship1.position.Y + 1 != ship2.position.Y)
				{
					switch (map[ship1.position.Y + 1][ship1.position.X].output)
					{
					case HEART:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y += 1;
							ship1.output = SHIP_DOWN;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 74, 18 };
							ship1.output = SHIP_DOWN;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 7, 18 };
							ship1.output = SHIP_DOWN;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y += 1;
						}
						ship1.output = SHIP_DOWN;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_DOWN;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}
			}
			if (is_pressed('D') && ship1.position.X < 78)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X + 1 != ship2.position.X || ship1.position.Y != ship2.position.Y)
				{
					switch (map[ship1.position.Y][ship1.position.X + 1].output)
					{
					case HEART:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X += 1;
							ship1.output = SHIP_RIGHT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 6)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 75, 17 };
							ship1.output = SHIP_RIGHT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 8, 17 };
							ship1.output = SHIP_RIGHT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X += 1;
						}
						ship1.output = SHIP_RIGHT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_RIGHT;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}
			}
			if (is_pressed('A') && ship1.position.X > 1)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X - 1 != ship2.position.X || ship1.position.Y != ship2.position.Y)
				{
					switch (map[ship1.position.Y][ship1.position.X - 1].output)
					{
					case HEART:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X -= 1;
							ship1.output = SHIP_LEFT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 8)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 73, 17 };
							ship1.output = SHIP_LEFT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 6, 17 };
							ship1.output = SHIP_LEFT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X -= 1;
						}
						ship1.output = SHIP_LEFT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_LEFT;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}
			}
			if (is_pressed('C'))
			{
				if (b1_up + b1_down + b1_right + b1_left == 0)
				{
					if (ghost1 > 0)
					{
						ghost1 -= 1;
						ship1.color = WHITE;
					}
					else
						ship1.color = BLUE;
					bullet1.position = ship1.position;
					if (gernade1 == 1)
						bullet1.output = GERNADE;
					else
					{
						if (power_shot1 == 0)
							bullet1.color = WHITE;
						else
						{
							power_shot1 -= 1;
							bullet1.color = BLUE;
						}
					}
					switch (ship1.output)
					{
					case SHIP_UP:
						if (map[ship1.position.Y - 1][ship1.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y - 1][ship1.position.X].output != BLOCK)
						{
							b1_up = 1;
							bullet1.position.Y -= 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_up = 1;
							bullet1.position.Y -= 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					case SHIP_DOWN:
						if (map[ship1.position.Y + 1][ship1.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y + 1][ship1.position.X].output != BLOCK)
						{
							b1_down = 1;
							bullet1.position.Y += 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_down = 1;
							bullet1.position.Y += 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					case SHIP_RIGHT:
						if (map[ship1.position.Y][ship1.position.X + 1].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y][ship1.position.X + 1].output != BLOCK)
						{
							b1_right = 1;
							bullet1.position.X += 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_right = 1;
							bullet1.position.X += 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					case SHIP_LEFT:
						if (map[ship1.position.Y][ship1.position.X - 1].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y][ship1.position.X - 1].output != BLOCK)
						{
							b1_left = 1;
							bullet1.position.X -= 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_left = 1;
							bullet1.position.X -= 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					}
				}
				else
				{
					b1_up = b1_down = b1_right = b1_left = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
				}
			}

			//ship2
			if (is_pressed('I') && ship2.position.Y > 1)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X != ship1.position.X || ship2.position.Y - 1 != ship1.position.Y)
				{
					switch (map[ship2.position.Y - 1][ship2.position.X].output)
					{
					case HEART:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y -= 1;
							ship2.output = SHIP_UP;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 74, 16 };
							ship2.output = SHIP_UP;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 7, 16 };
							ship2.output = SHIP_UP;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y -= 1;
						}
						ship2.output = SHIP_UP;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_UP;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('K') && ship2.position.Y < 18)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X != ship1.position.X || ship2.position.Y + 1 != ship1.position.Y)
				{
					switch (map[ship2.position.Y + 1][ship2.position.X].output)
					{
					case HEART:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y += 1;
							ship2.output = SHIP_DOWN;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 74, 18 };
							ship2.output = SHIP_DOWN;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 7, 18 };
							ship2.output = SHIP_DOWN;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y += 1;
						}
						ship2.output = SHIP_DOWN;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_DOWN;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('L') && ship2.position.X < 78)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X != ship1.position.X || ship2.position.Y != ship1.position.Y)
				{
					switch (map[ship2.position.Y][ship2.position.X + 1].output)
					{
					case HEART:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X += 1;
							ship2.output = SHIP_RIGHT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 6)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 75, 17 };
							ship2.output = SHIP_RIGHT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 8, 17 };
							ship2.output = SHIP_RIGHT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X += 1;
						}
						ship2.output = SHIP_RIGHT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_RIGHT;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('J') && ship2.position.X > 1)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X - 1 != ship1.position.X || ship2.position.Y != ship1.position.Y)
				{
					switch (map[ship2.position.Y][ship2.position.X - 1].output)
					{
					case HEART:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X -= 1;
							ship2.output = SHIP_LEFT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 8)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 73, 17 };
							ship2.output = SHIP_LEFT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 6, 17 };
							ship2.output = SHIP_LEFT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X -= 1;
						}
						ship2.output = SHIP_LEFT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_LEFT;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('N'))
			{
				if (b2_up + b2_down + b2_right + b2_left == 0)
				{
					if (ghost2 > 0)
					{
						ghost2 -= 1;
						ship2.color = WHITE;
					}
					bullet2.position = ship2.position;
					if (gernade2 == 1)
						bullet2.output = GERNADE;
					else
					{
						if (power_shot2 == 0)
							bullet2.color = WHITE;
						else
						{
							power_shot2 -= 1;
							bullet2.color = RED;
						}
					}
					switch (ship2.output)
					{
					case SHIP_UP:
						if (map[ship2.position.Y - 1][ship2.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y - 1][ship2.position.X].output != BLOCK)
						{
							b2_up = 1;
							bullet2.position.Y -= 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost2 > 0)
						{
							b2_up = 1;
							bullet2.position.Y -= 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					case SHIP_DOWN:
						if (map[ship2.position.Y + 1][ship2.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y + 1][ship2.position.X].output != BLOCK)
						{
							b2_down = 1;
							bullet2.position.Y += 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost2 > 0)
						{
							b2_down = 1;
							bullet2.position.Y += 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					case SHIP_RIGHT:
						if (map[ship2.position.Y][ship2.position.X + 1].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y][ship2.position.X + 1].output != BLOCK)
						{
							b2_right = 1;
							bullet2.position.X += 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost2 > 0)
						{
							b2_right = 1;
							bullet2.position.X += 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					case SHIP_LEFT:
						if (map[ship2.position.Y][ship2.position.X - 1].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y][ship2.position.X - 1].output != BLOCK)
						{
							b2_left = 1;
							bullet2.position.X -= 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost2 > 0)
						{
							b2_left = 1;
							bullet2.position.X -= 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					}
				}
				else
				{
					b2_up = b2_down = b2_right = b2_left = 0;
					show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
				}
			}

			//quit the game
			if (is_pressed(VK_END))
			{
				info(player1);
				stop_playing();
				seconds = 0;
				return;
			}

		}

		//bullets
		if (b1_up + b1_down + b1_left + b1_right > 0)
		{
			if (b1_up == 1)
			{
				if (ghost1 > 0)
				{
					if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y - 1 == ship2.position.Y))
					{
						b1_up = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.Y > 1)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.Y -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_up = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y - 1][bullet1.position.X].output == BLOCK)
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
									{
										SetConsoleCursorPosition(hStdout, ship1.position);
										wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
									}
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.Y == ship2.position.Y || bullet1.position.Y - 1 == ship2.position.Y) && bullet1.position.X == ship2.position.X)
					{
						b1_up = 0;
						show_screen(hStdout, bullet1.position, SPACE, WHITE);
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship1.position.Y - ship2.position.Y < 3)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y - 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, { bullet1.position.X, bullet1.position.Y }, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
							show_screen(hStdout, bullet1.position, SPACE, WHITE);
						bullet1.position.Y -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						bullet1.position.Y -= 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y - 1][bullet1.position.X].output == BLOCK)
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y - 1 == ship2.position.Y))
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y - 1][bullet1.position.X].output == MIRROR)
					{
						b1_up = 0;
						b1_down = 1;
					}
					else if (bullet1.position.X == ship1.position.X && (bullet1.position.Y == ship1.position.Y || bullet1.position.Y - 1 == ship1.position.Y))
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y - 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, { bullet1.position.X , bullet1.position.Y }, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.Y -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.Y -= 1;
					}
				}
			}
			if (b1_down == 1)
			{
				if (ghost1 > 0)
				{
					if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y + 1 == ship2.position.Y))
					{
						b1_down = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.Y < 18)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.Y += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_down = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y + 1][bullet1.position.X].output == BLOCK)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
									{
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									}
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
									{
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									}
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.Y == ship2.position.Y || bullet1.position.Y + 1 == ship2.position.Y) && bullet1.position.X == ship2.position.X)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship2.position.Y - ship1.position.Y < 3)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y + 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
						{
							show_screen(hStdout, { bullet1.position.X , bullet1.position.Y }, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.Y += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.Y += 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y + 1][bullet1.position.X].output == BLOCK)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y + 1 == ship2.position.Y))
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y + 1][bullet1.position.X].output == MIRROR)
					{
						b1_down = 0;
						b1_up = 1;
					}
					else if (bullet1.position.X == ship1.position.X && (bullet1.position.Y == ship1.position.Y || bullet1.position.Y + 1 == ship1.position.Y))
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y + 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.Y += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.Y += 1;
					}
				}
			}
			if (b1_right == 1)
			{
				if (ghost1 > 0)
				{
					if ((bullet1.position.X == ship2.position.X || bullet1.position.X + 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_right = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.X < 78)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.X += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_right = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives2 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X + 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship1.position.X - ship2.position.X < 5)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X + 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X += 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y][bullet1.position.X + 1].output == BLOCK || bullet1.position.X == 79)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (map[bullet1.position.Y][bullet1.position.X + 1].output == MIRROR)
					{
						b1_right = 0;
						b1_left = 1;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X + 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet1.position.X == ship1.position.X || bullet1.position.X + 1 == ship1.position.X) && bullet1.position.Y == ship1.position.Y)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y][bullet1.position.X + 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X += 1;
					}
				}
			}
			if (b1_left == 1)
			{
				if (ghost1 > 0)
				{
					if ((bullet1.position.X == ship2.position.X || bullet1.position.X - 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_left = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.X > 1)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.X -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_left = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X - 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship2.position.Y - ship1.position.Y < 3)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X - 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X -= 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y][bullet1.position.X - 1].output == BLOCK || bullet1.position.X == 0)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (map[bullet1.position.Y][bullet1.position.X - 1].output == MIRROR)
					{
						b1_left = 0;
						b1_right = 1;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X - 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet1.position.X == ship1.position.X || bullet1.position.X - 1 == ship1.position.X) && bullet1.position.Y == ship1.position.Y)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y][bullet1.position.X - 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X -= 1;
					}
				}
			}
		}
		if (b2_up + b2_down + b2_left + b2_right > 0)
		{
			if (b2_up == 1)
			{
				if (ghost2 > 0)
				{
					if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_up = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.Y > 1)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.Y -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_up = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship2.position.Y - ship1.position.Y < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y - 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y -= 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y - 1][bullet2.position.X].output == BLOCK || bullet2.position.Y == 0)
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y - 1][bullet2.position.X].output == MIRROR)
					{
						b2_up = 0;
						b2_down = 1;
					}
					else if (bullet2.position.X == ship2.position.X && (bullet2.position.Y == ship2.position.Y || bullet2.position.Y - 1 == ship2.position.Y))
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y - 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y -= 1;
					}
				}
			}
			if (b2_down == 1)
			{
				if (ghost2 > 0)
				{
					if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y + 1 == ship1.position.Y))
					{
						b2_down = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.Y < 19)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.Y += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_down = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y + 1 == ship1.position.Y))
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship1.position.Y - ship2.position.Y < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y + 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y += 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y + 1][bullet2.position.X].output == BLOCK || bullet2.position.Y > 18)
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (map[bullet2.position.Y + 1][bullet2.position.X].output == MIRROR)
					{
						b2_up = 1;
						b2_down = 0;
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y + 1 == ship1.position.Y))
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.X == ship2.position.X && (bullet2.position.Y == ship2.position.Y || bullet2.position.Y + 1 == ship2.position.Y))
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y + 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y += 1;
					}
				}
			}
			if (b2_right == 1)
			{
				if (ghost2 > 0)
				{
					if ((bullet2.position.X == ship1.position.X || bullet2.position.X + 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_right = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.X < 79)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.X += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_right = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X + 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship2.position.X - ship1.position.X < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X + 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X += 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y][bullet2.position.X + 1].output == BLOCK || bullet2.position.X == 79)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (map[bullet2.position.Y][bullet2.position.X + 1].output == MIRROR)
					{
						b2_right = 0;
						b2_left = 1;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X + 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet2.position.X == ship2.position.X || bullet2.position.X + 1 == ship2.position.X) && bullet2.position.Y == ship2.position.Y)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y][bullet2.position.X + 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X += 1;
					}
				}
			}
			if (b2_left == 1)
			{
				if (ghost2 > 0)
				{
					if ((bullet2.position.X == ship1.position.X || bullet2.position.X - 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_left = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.X > 1)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.X -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_left = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X - 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship1.position.X - ship2.position.X < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X - 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X -= 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y][bullet2.position.X - 1].output == BLOCK || bullet2.position.X == 0)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (map[bullet2.position.Y][bullet2.position.X - 1].output == MIRROR)
					{
						b2_left = 0;
						b2_right = 1;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X - 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet2.position.X == ship2.position.X || bullet2.position.X - 1 == ship2.position.X) && bullet2.position.Y == ship2.position.Y)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y][bullet2.position.X - 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X -= 1;
					}
				}
			}

		}

		SetConsoleCursorPosition(hStdout, { 0, 22 });
		Sleep(100);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
	}
	char winner[101];
	int color;

	if (lives1 <= 0)
	{
		color = RED;
		this_game->id3 = player2->id;
		wins2++;
		strcpy(winner, player2->username);
	}
	else
	{
		color = BLUE;
		this_game->id3 = player1->id;
		wins1++;
		strcpy(winner, player1->username);
	}
	update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
	_setmode(_fileno(stdout), 0x4000);
	printf("\n\n\t\t\t\t*** \033[1;%dm %s WINS! \033[0m***\n", color, winner);
	_setmode(_fileno(stdout), 0x20000);
	Sleep(1500);

	system("cls||clear");
	wprintf(L"\t \033[1;%dm████████████\n", GREEN);
	wprintf(L"\t ██        ██\n");
	wprintf(L"\t ██ █    █ ██\n");
	wprintf(L"\t ██  └──┘  ██\n");
	wprintf(L"\t ████████████\n");
	wprintf(L"\t ██ ██  ██ ██\n");
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");


	if (wins1 == 1)
	{
		add_history(*this_game, *player2, *player1, wins2, wins1);
		_setmode(_fileno(stdout), 0x4000);
		printf("\n\t\t\033[1;%dm THE WINNER IS:\n\t\t  ***%s***", CYAN, player2->username);
	}
	else if (wins2 == 1)
	{
		add_history(*this_game, *player1, *player2, wins1, wins2);
		_setmode(_fileno(stdout), 0x4000);
		printf("\n\t\t\033[1;%dm THE WINNER IS:\n\t\t  ***%s***", CYAN, player1->username);
	}
	Sleep(1500);
	stop_playing();
	info(player1);
}

void game2(match* this_game, data* player1, data* player2, int lives, int wins1, int wins2)
{
	system("cls||clear");
	_setmode(_fileno(stdout), 0x20000);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	wprintf(L"\n\n\n\n\n\t\t\t\033[1;%dm╔═══════════════╗\n", YELLOW);
	wprintf(L"\t\t\t║\033[1;%dm    ROUND 2    \033[1;%dm║\n", MAGENTA, YELLOW);
	wprintf(L"\t\t\t╚═══════════════╝\n");
	Sleep(1000);
	system("cls||clear");

	int b1_up = 0, b1_down = 0, b1_left = 0, b1_right = 0, b2_up = 0, b2_down = 0, b2_left = 0, b2_right = 0;
	int lives1 = lives, lives2 = lives, power_shot1 = 0, power_shot2 = 0, gernade1 = 0, gernade2 = 0, ghost1 = -1, ghost2 = -1;

	object map[20][80];
	object ship1 = { {8, 10}, SHIP_RIGHT, BLUE };
	object ship2 = { {72, 10}, SHIP_LEFT, RED };
	object bullet1 = { ship1.position, L'.', WHITE };
	object bullet2 = { ship2.position, L'.', WHITE };

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 80; x++)
		{
			if (x == 0 || y == 0 || x == 79 || y == 19)
				map[y][x].output = BLOCK;
			else
				map[y][x].output = SPACE;
			map[y][x].position.X = x;
			map[y][x].position.Y = y;
		}
	}

	map[2][24].output = map[2][56].output = map[3][24].output = map[3][56].output = map[4][38].output = map[4][39].output = map[4][40].output = map[4][41].output = map[4][42].output = map[6][30].output = map[6][40].output = map[6][50].output = map[7][29].output = map[7][37].output = map[7][38].output = map[7][39].output = map[7][40].output = map[7][51].output = BLOCK;
	map[8][28].output = map[8][37].output = map[8][52].output = map[9][27].output = map[9][35].output = map[9][36].output = map[9][37].output = map[9][44].output = map[9][45].output = map[9][46].output = map[9][53].output = map[10][28].output = map[10][44].output = map[10][52].output = map[11][29].output = map[11][40].output = map[11][41].output = map[11][42].output = map[11][43].output = map[11][44].output = map[11][51].output = BLOCK;
	map[12][30].output = map[12][40].output = map[12][50].output = map[15][38].output = map[15][39].output = map[15][40].output = map[15][41].output = map[15][42].output = map[16][24].output = map[16][56].output = map[17][24].output = map[17][56].output = BLOCK;

	map[2][25].output = map[3][55].output = map[7][12].output = map[7][69].output = map[8][12].output = map[8][69].output = map[8][51].output = map[9][12].output = map[9][69].output = map[9][38].output = map[10][29].output = map[10][12].output = map[10][69].output = map[11][12].output = map[11][69].output = map[12][12].output = map[12][69].output = map[16][25].output = map[17][55].output = MIRROR;

	map[2][40].output = GHOST;
	object ghost = { {40, 2}, GHOST, WHITE };

	map[9][40].output = HEART;
	map[9][40].color = RED;
	object heart = { {40, 9}, HEART, RED };

	map[17][7].output = map[17][74].output = PORTAL;
	map[17][7].color = map[17][74].color = YELLOW;

	map[14][40].output = GERNADE;
	map[14][40].color = GREEN;
	object gernade = { {40, 14}, GERNADE, GREEN };

	map[7][24].output = map[7][56].output = POWER_UP;
	map[7][24].color = map[7][56].color = YELLOW;
	object power_up = { {24, 7}, POWER_UP, YELLOW };

	map[8][42].output = map[10][38].output = map[13][40].output = BLACKHOLE;

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 80; x++)
		{
			wprintf(L"\033[1;%dm%lc\033[0m", map[y][x].color, map[y][x].output);
		}
		wprintf(L"\n");
	}

	update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
	wprintf(L"\n\t\t\t\t\t 2nd");

	show_screen(hStdout, ship1.position, ship1.output, ship1.color);
	show_screen(hStdout, ship2.position, ship2.output, ship2.color);

	while (lives1 > 0 && lives2 > 0)
	{
		if (seconds >= 142)
		{
			play_battle();
			seconds = 0;
		}
		start = end = 0;
		start = clock();
		if (map[heart.position.Y][heart.position.X].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					heart.position = { (short)x, (short)y };
					map[y][x].output = HEART;
					map[y][x].color = RED;
					show_screen(hStdout, heart.position, heart.output, heart.color);
					count++;
				}
			}
			else
				count++;
		}
		if (map[power_up.position.Y][power_up.position.X].output == SPACE && map[7][56].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					power_up.position = { (short)x, (short)y };
					map[y][x].output = POWER_UP;
					map[y][x].color = YELLOW;
					show_screen(hStdout, power_up.position, power_up.output, power_up.color);
					count++;
				}
			}
			else
				count++;
		}
		if (map[ghost.position.Y][ghost.position.X].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					ghost.position = { (short)x, (short)y };
					map[y][x].output = GHOST;
					map[y][x].color = WHITE;
					show_screen(hStdout, ghost.position, ghost.output, ghost.color);
					count++;
				}
			}
			else
				count++;
		}
		if (map[gernade.position.Y][gernade.position.X].output == SPACE)
		{
			static int count = 1;
			if (count % 200 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					gernade.position = { (short)x, (short)y };
					map[y][x].output = GERNADE;
					map[y][x].color = GREEN;
					show_screen(hStdout, gernade.position, gernade.output, gernade.color);
					count++;
				}
			}
			else
				count++;
		}
		if (_kbhit())
		{
			//ship1
			if (is_pressed('W') && ship1.position.Y > 1)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X != ship2.position.X || ship1.position.Y - 1 != ship2.position.Y)
				{
					switch (map[ship1.position.Y - 1][ship1.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y -= 1;
							ship1.output = SHIP_UP;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 74, 16 };
							ship1.output = SHIP_UP;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 7, 16 };
							ship1.output = SHIP_UP;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y -= 1;
						}
						ship1.output = SHIP_UP;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_UP;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}

			}
			if (is_pressed('S') && ship1.position.Y < 18)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X != ship2.position.X || ship1.position.Y + 1 != ship2.position.Y)
				{
					switch (map[ship1.position.Y + 1][ship1.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y += 1;
							ship1.output = SHIP_DOWN;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 74, 18 };
							ship1.output = SHIP_DOWN;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 7, 18 };
							ship1.output = SHIP_DOWN;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.Y += 1;
						}
						ship1.output = SHIP_DOWN;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_DOWN;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}
			}
			if (is_pressed('D') && ship1.position.X < 78)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X + 1 != ship2.position.X || ship1.position.Y != ship2.position.Y)
				{
					switch (map[ship1.position.Y][ship1.position.X + 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X += 1;
							ship1.output = SHIP_RIGHT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 6)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 75, 17 };
							ship1.output = SHIP_RIGHT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 8, 17 };
							ship1.output = SHIP_RIGHT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X += 1;
						}
						ship1.output = SHIP_RIGHT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_RIGHT;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}
			}
			if (is_pressed('A') && ship1.position.X > 1)
			{
				if (ghost1 > 0)
				{
					ghost1 -= 1;
					ship1.color = WHITE;
				}
				else
					ship1.color = BLUE;
				if (ship1.position.X - 1 != ship2.position.X || ship1.position.Y != ship2.position.Y)
				{
					switch (map[ship1.position.Y][ship1.position.X - 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X -= 1;
							ship1.output = SHIP_LEFT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship1.position.X == 8)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 73, 17 };
							ship1.output = SHIP_LEFT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 6, 17 };
							ship1.output = SHIP_LEFT;
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						power_shot1 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						gernade1 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						ship1.color = WHITE;
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						ghost1 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost1 > 0)
						{
							show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
							ship1.position.X -= 1;
						}
						ship1.output = SHIP_LEFT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					default:
						show_screen(hStdout, ship1.position, map[ship1.position.Y][ship1.position.X].output, map[ship1.position.Y][ship1.position.X].color);
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_LEFT;
					show_screen(hStdout, ship1.position, ship1.output, ship1.color);
				}
			}
			if (is_pressed('C'))
			{
				if (b1_up + b1_down + b1_right + b1_left == 0)
				{
					if (ghost1 > 0)
					{
						ghost1 -= 1;
						ship1.color = WHITE;
					}
					else
						ship1.color = BLUE;
					bullet1.position = ship1.position;
					if (gernade1 == 1)
						bullet1.output = GERNADE;
					else
					{
						if (power_shot1 == 0)
							bullet1.color = WHITE;
						else
						{
							power_shot1 -= 1;
							bullet1.color = BLUE;
						}
					}
					switch (ship1.output)
					{
					case SHIP_UP:
						if (map[ship1.position.Y - 1][ship1.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y - 1][ship1.position.X].output != BLOCK)
						{
							b1_up = 1;
							bullet1.position.Y -= 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_up = 1;
							bullet1.position.Y -= 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					case SHIP_DOWN:
						if (map[ship1.position.Y + 1][ship1.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y + 1][ship1.position.X].output != BLOCK)
						{
							b1_down = 1;
							bullet1.position.Y += 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_down = 1;
							bullet1.position.Y += 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					case SHIP_RIGHT:
						if (map[ship1.position.Y][ship1.position.X + 1].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y][ship1.position.X + 1].output != BLOCK)
						{
							b1_right = 1;
							bullet1.position.X += 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_right = 1;
							bullet1.position.X += 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					case SHIP_LEFT:
						if (map[ship1.position.Y][ship1.position.X - 1].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y][ship1.position.X - 1].output != BLOCK)
						{
							b1_left = 1;
							bullet1.position.X -= 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						else if (ghost1 > 0)
						{
							b1_left = 1;
							bullet1.position.X -= 1;
							show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
						}
						break;
					}
				}
				else
				{
					b1_up = b1_down = b1_right = b1_left = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
				}
			}

			//ship2
			if (is_pressed('I') && ship2.position.Y > 1)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X != ship1.position.X || ship2.position.Y - 1 != ship1.position.Y)
				{
					switch (map[ship2.position.Y - 1][ship2.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y -= 1;
							ship2.output = SHIP_UP;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 74, 16 };
							ship2.output = SHIP_UP;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 7, 16 };
							ship2.output = SHIP_UP;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y -= 1;
						}
						ship2.output = SHIP_UP;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_UP;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('K') && ship2.position.Y < 18)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X != ship1.position.X || ship2.position.Y + 1 != ship1.position.Y)
				{
					switch (map[ship2.position.Y + 1][ship2.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y += 1;
							ship2.output = SHIP_DOWN;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 74, 18 };
							ship2.output = SHIP_DOWN;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 7, 18 };
							ship2.output = SHIP_DOWN;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.Y += 1;
						}
						ship2.output = SHIP_DOWN;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_DOWN;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('L') && ship2.position.X < 78)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X != ship1.position.X || ship2.position.Y != ship1.position.Y)
				{
					switch (map[ship2.position.Y][ship2.position.X + 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X += 1;
							ship2.output = SHIP_RIGHT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 6)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 75, 17 };
							ship2.output = SHIP_RIGHT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 8, 17 };
							ship2.output = SHIP_RIGHT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X += 1;
						}
						ship2.output = SHIP_RIGHT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_RIGHT;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('J') && ship2.position.X > 1)
			{
				if (ghost2 > 0)
				{
					ghost2 -= 1;
					ship2.color = WHITE;
				}
				else
					ship2.color = RED;
				if (ship2.position.X - 1 != ship1.position.X || ship2.position.Y != ship1.position.Y)
				{
					switch (map[ship2.position.Y][ship2.position.X - 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X -= 1;
							ship2.output = SHIP_LEFT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives2 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						}
						break;
					case PORTAL:
						if (ship2.position.X == 8)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 73, 17 };
							ship2.output = SHIP_LEFT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 6, 17 };
							ship2.output = SHIP_LEFT;
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						break;
					case POWER_UP:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_UP;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						power_shot2 = 5;
						break;
					case GERNADE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						gernade2 = 1;
						break;
					case GHOST:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						ship2.color = WHITE;
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						ghost2 = 7;
						break;
					case BLOCK:
					case MIRROR:
						if (ghost2 > 0)
						{
							show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
							ship2.position.X -= 1;
						}
						ship2.output = SHIP_LEFT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					default:
						show_screen(hStdout, ship2.position, map[ship2.position.Y][ship2.position.X].output, map[ship2.position.Y][ship2.position.X].color);
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_LEFT;
					show_screen(hStdout, ship2.position, ship2.output, ship2.color);
				}
			}
			if (is_pressed('N'))
			{
				if (b2_up + b2_down + b2_right + b2_left == 0)
				{
					if (ghost2 > 0)
					{
						ghost2 -= 1;
						ship2.color = WHITE;
					}
					else
						ship2.color = RED;
					bullet2.position = ship2.position;
					if (gernade2 == 1)
						bullet2.output = GERNADE;
					else
					{
						if (power_shot2 == 0)
							bullet2.color = WHITE;
						else
						{
							power_shot2 -= 1;
							bullet2.color = RED;
						}
					}
					switch (ship2.output)
					{
					case SHIP_UP:
						if (map[ship2.position.Y - 1][ship2.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y - 1][ship2.position.X].output != BLOCK)
						{
							b2_up = 1;
							bullet2.position.Y -= 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost1 > 0)
						{
							b2_up = 1;
							bullet2.position.Y -= 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					case SHIP_DOWN:
						if (map[ship2.position.Y + 1][ship2.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y + 1][ship2.position.X].output != BLOCK)
						{
							b2_down = 1;
							bullet2.position.Y += 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost1 > 0)
						{
							b2_down = 1;
							bullet2.position.Y += 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					case SHIP_RIGHT:
						if (map[ship2.position.Y][ship2.position.X + 1].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y][ship2.position.X + 1].output != BLOCK)
						{
							b2_right = 1;
							bullet2.position.X += 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost1 > 0)
						{
							b2_right = 1;
							bullet2.position.X += 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					case SHIP_LEFT:
						if (map[ship2.position.Y][ship2.position.X - 1].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y][ship2.position.X - 1].output != BLOCK)
						{
							b2_left = 1;
							bullet2.position.X -= 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						else if (ghost1 > 0)
						{
							b2_left = 1;
							bullet2.position.X -= 1;
							show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
						}
						break;
					}
				}
				else
				{
					b2_up = b2_down = b2_right = b2_left = 0;
					show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
				}
			}

			//quit the game
			if (is_pressed(VK_END))
			{
				info(player1);
				stop_playing();
				seconds = 0;
				return;
			}
		}


		//bullets
		if (b1_up + b1_down + b1_left + b1_right > 0)
		{
			if (b1_up == 1)
			{
				if (ghost1 > 0)
				{
					if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y - 1 == ship2.position.Y))
					{
						b1_up = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.Y > 1)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.Y -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_up = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y - 1][bullet1.position.X].output == BLOCK)
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
									{
										SetConsoleCursorPosition(hStdout, ship1.position);
										wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
									}
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.Y == ship2.position.Y || bullet1.position.Y - 1 == ship2.position.Y) && bullet1.position.X == ship2.position.X)
					{
						b1_up = 0;
						show_screen(hStdout, bullet1.position, SPACE, WHITE);
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship1.position.Y - ship2.position.Y < 3)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y - 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, { bullet1.position.X, bullet1.position.Y }, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
							show_screen(hStdout, bullet1.position, SPACE, WHITE);
						bullet1.position.Y -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.Y -= 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y - 1][bullet1.position.X].output == BLOCK)
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y - 1 == ship2.position.Y))
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y - 1][bullet1.position.X].output == MIRROR)
					{
						b1_up = 0;
						b1_down = 1;
					}
					else if (bullet1.position.X == ship1.position.X && (bullet1.position.Y == ship1.position.Y || bullet1.position.Y - 1 == ship1.position.Y))
					{
						b1_up = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y - 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, { bullet1.position.X , bullet1.position.Y }, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.Y -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.Y -= 1;
					}
				}
			}
			if (b1_down == 1)
			{
				if (ghost1 > 0)
				{
					if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y + 1 == ship2.position.Y))
					{
						b1_down = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.Y < 18)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.Y += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_down = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y + 1][bullet1.position.X].output == BLOCK)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
									{
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									}
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
									{
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									}
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.Y == ship2.position.Y || bullet1.position.Y + 1 == ship2.position.Y) && bullet1.position.X == ship2.position.X)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship2.position.Y - ship1.position.Y < 3)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y + 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
						{
							show_screen(hStdout, { bullet1.position.X , bullet1.position.Y }, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.Y += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.Y += 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y + 1][bullet1.position.X].output == BLOCK)
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y + 1 == ship2.position.Y))
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y + 1][bullet1.position.X].output == MIRROR)
					{
						b1_down = 0;
						b1_up = 1;
					}
					else if (bullet1.position.X == ship1.position.X && (bullet1.position.Y == ship1.position.Y || bullet1.position.Y + 1 == ship1.position.Y))
					{
						b1_down = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y + 1][bullet1.position.X].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.Y += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.Y += 1;
					}
				}
			}
			if (b1_right == 1)
			{
				if (ghost1 > 0)
				{
					if ((bullet1.position.X == ship2.position.X || bullet1.position.X + 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_right = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.X < 78)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.X += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_right = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives2 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X + 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship1.position.X - ship2.position.X < 5)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X + 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X += 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y][bullet1.position.X + 1].output == BLOCK || bullet1.position.X == 79)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (map[bullet1.position.Y][bullet1.position.X + 1].output == MIRROR)
					{
						b1_right = 0;
						b1_left = 1;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X + 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet1.position.X == ship1.position.X || bullet1.position.X + 1 == ship1.position.X) && bullet1.position.Y == ship1.position.Y)
					{
						b1_right = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y][bullet1.position.X + 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X += 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X += 1;
					}
				}
			}
			if (b1_left == 1)
			{
				if (ghost1 > 0)
				{
					if ((bullet1.position.X == ship2.position.X || bullet1.position.X - 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_left = 0;
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet1.position.X > 1)
					{
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						bullet1.position.X -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						b1_left = 0;
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					}
				}
				else if (bullet1.output == GERNADE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X].output == MIRROR)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						map[bullet1.position.Y][bullet1.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship2.position.X == bullet1.position.X + i && ship2.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship1.position.X == bullet1.position.X + i && ship1.position.Y == bullet1.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X - 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						if (ship2.position.Y - ship1.position.Y < 3)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							if (lives1 > 0)
								show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet1.position.Y][bullet1.position.X - 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X -= 1;
					}
				}
				else
				{
					if (map[bullet1.position.Y][bullet1.position.X - 1].output == BLOCK || bullet1.position.X == 0)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					else if (map[bullet1.position.Y][bullet1.position.X - 1].output == MIRROR)
					{
						b1_left = 0;
						b1_right = 1;
					}
					else if ((bullet1.position.X == ship2.position.X || bullet1.position.X - 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot1 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet1.position.X == ship1.position.X || bullet1.position.X - 1 == ship1.position.X) && bullet1.position.Y == ship1.position.Y)
					{
						b1_left = 0;
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot1 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet1.position.Y][bullet1.position.X - 1].output == SPACE)
					{
						if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
							show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L" ");
						}
						bullet1.position.X -= 1;
						show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
						bullet1.position.X -= 1;
					}
				}
			}

		}
		if (b2_up + b2_down + b2_left + b2_right > 0)
		{
			if (b2_up == 1)
			{
				if (ghost2 > 0)
				{
					if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_up = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.Y > 1)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.Y -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_up = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet1.output = '.';
						gernade1 = 0;
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship2.position.Y - ship1.position.Y < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y - 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y -= 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y - 1][bullet2.position.X].output == BLOCK || bullet2.position.Y == 0)
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y - 1][bullet2.position.X].output == MIRROR)
					{
						b2_up = 0;
						b2_down = 1;
					}
					else if (bullet2.position.X == ship2.position.X && (bullet2.position.Y == ship2.position.Y || bullet2.position.Y - 1 == ship2.position.Y))
					{
						b2_up = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y - 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y -= 1;
					}
				}
			}
			if (b2_down == 1)
			{
				if (ghost2 > 0)
				{
					if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y + 1 == ship1.position.Y))
					{
						b2_down = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.Y < 19)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.Y += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_down = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship1.position.Y - ship2.position.Y < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y + 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y += 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y + 1][bullet2.position.X].output == BLOCK || bullet2.position.Y > 18)
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (map[bullet2.position.Y + 1][bullet2.position.X].output == MIRROR)
					{
						b2_up = 1;
						b2_down = 0;
					}
					else if (bullet2.position.X == ship1.position.X && (bullet2.position.Y == ship1.position.Y || bullet2.position.Y - 1 == ship1.position.Y))
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.X == ship2.position.X && (bullet2.position.Y == ship2.position.Y || bullet2.position.Y + 1 == ship2.position.Y))
					{
						b2_down = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y + 1][bullet2.position.X].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.Y += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.Y += 1;
					}
				}
			}
			if (b2_right == 1)
			{
				if (ghost2 > 0)
				{
					if ((bullet2.position.X == ship1.position.X || bullet2.position.X + 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_right = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.X < 79)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.X += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_right = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X + 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship2.position.X - ship1.position.X < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X + 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X += 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y][bullet2.position.X + 1].output == BLOCK || bullet2.position.X == 79)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (map[bullet2.position.Y][bullet2.position.X + 1].output == MIRROR)
					{
						b2_right = 0;
						b2_left = 1;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X + 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet2.position.X == ship2.position.X || bullet2.position.X + 1 == ship2.position.X) && bullet2.position.Y == ship2.position.Y)
					{
						b2_right = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y][bullet2.position.X + 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X += 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X += 1;
					}
				}
			}
			if (b2_left == 1)
			{
				if (ghost2 > 0)
				{
					if ((bullet2.position.X == ship1.position.X || bullet2.position.X - 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_left = 0;
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (bullet2.position.X > 1)
					{
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						bullet2.position.X -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						b2_left = 0;
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					}
				}
				else if (bullet2.output == GERNADE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L"%lc", BLOCK);
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X].output == MIRROR)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						map[bullet2.position.Y][bullet2.position.X].output = SPACE;
						for (int i = -4; i < 5; i++)
						{
							for (int j = -2; j < 3; j++)
							{
								if (ship1.position.X == bullet2.position.X + i && ship1.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship1, { 8, 10 }, &lives1);
									if (lives1 > 0)
										show_screen(hStdout, ship1.position, ship1.output, ship1.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
								if (ship2.position.X == bullet2.position.X + i && ship2.position.Y == bullet2.position.Y + j)
								{
									player_death(hStdout, &ship2, { 72, 10 }, &lives2);
									if (lives2 > 0)
										show_screen(hStdout, ship2.position, ship2.output, ship2.color);
									update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
								}
							}
						}
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X - 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						if (ship1.position.X - ship2.position.X < 3)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							if (lives1 > 0)
								show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						bullet2.output = '.';
						gernade2 = 0;
					}
					else if (map[bullet2.position.Y][bullet2.position.X - 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X -= 1;
					}
				}
				else
				{
					if (map[bullet2.position.Y][bullet2.position.X - 1].output == BLOCK || bullet2.position.X == 0)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					else if (map[bullet2.position.Y][bullet2.position.X - 1].output == MIRROR)
					{
						b2_left = 0;
						b2_right = 1;
					}
					else if ((bullet2.position.X == ship1.position.X || bullet2.position.X - 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship1, { 8, 10 }, &lives1);
						if (power_shot2 > 0)
							lives1 -= 1;
						if (lives1 > 0)
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if ((bullet2.position.X == ship2.position.X || bullet2.position.X - 1 == ship2.position.X) && bullet2.position.Y == ship2.position.Y)
					{
						b2_left = 0;
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						player_death(hStdout, &ship2, { 72, 10 }, &lives2);
						if (power_shot2 > 0)
							lives2 -= 1;
						if (lives2 > 0)
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
					}
					else if (map[bullet2.position.Y][bullet2.position.X - 1].output == SPACE)
					{
						if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
							show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
						else
						{
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L" ");
						}
						bullet2.position.X -= 1;
						show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
					}
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
						bullet2.position.X -= 1;
					}
				}
			}

		}

		SetConsoleCursorPosition(hStdout, { 0, 22 });
		seconds += 0.1;
		Sleep(100);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;

	}
	char winner[101];
	int color;


	if (lives1 <= 0)
	{
		color = RED;
		this_game->id2 = player2->id;
		wins2++;
		strcpy(winner, player2->username);
	}
	else
	{
		color = BLUE;
		this_game->id2 = player1->id;
		wins1++;
		strcpy(winner, player1->username);
	}
	update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
	_setmode(_fileno(stdout), 0x4000);
	printf("\n\n\t\t\t\t*** \033[1;%dm %s WINS! \033[0m***\n", color, winner);
	_setmode(_fileno(stdout), 0x20000);
	Sleep(1250);

	if (wins1 == 0)
	{
		add_history(*this_game, *player2, *player1, wins2, wins1);
		system("cls||clear");
		wprintf(L"\t \033[1;%dm████████████\n", GREEN);
		wprintf(L"\t ██        ██\n");
		wprintf(L"\t ██ █    █ ██\n");
		wprintf(L"\t ██  └──┘  ██\n");
		wprintf(L"\t ████████████\n");
		wprintf(L"\t ██ ██  ██ ██\n");
		wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
		_setmode(_fileno(stdout), 0x4000);
		printf("\n\t\t\033[1;%dm THE WINNER IS:\n\t\t  ***%s***", CYAN, player2->username);
		Sleep(1500);
		stop_playing();
		info(player1);
	}
	else if (wins2 == 0)
	{
		add_history(*this_game, *player1, *player2, wins1, wins2);
		system("cls||clear");
		wprintf(L"\t \033[1;%dm████████████\n", GREEN);
		wprintf(L"\t ██        ██\n");
		wprintf(L"\t ██ █    █ ██\n");
		wprintf(L"\t ██  └──┘  ██\n");
		wprintf(L"\t ████████████\n");
		wprintf(L"\t ██ ██  ██ ██\n");
		wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
		_setmode(_fileno(stdout), 0x4000);
		printf("\n\t\t\033[1;%dm THE WINNER IS:\n\t\t  ***%s***", CYAN, player1->username);
		Sleep(1500);
		stop_playing();
		info(player1);
	}
	else
	{
		game3(this_game, player1, player2, lives, wins1, wins2);
		return;
	}
}

void game1(data* player1, data* player2, int lives)
{
	system("cls||clear");
	_setmode(_fileno(stdout), 0x20000);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	play_battle();
	wprintf(L"\n\n\n\n\n\t\t\t\033[1;%dm╔═══════════════╗\n", YELLOW);
	wprintf(L"\t\t\t║\033[1;%dm    ROUND 1    \033[1;%dm║\n", MAGENTA, YELLOW);
	wprintf(L"\t\t\t╚═══════════════╝\n");
	Sleep(1000);
	system("cls||clear");
	seconds = 0;

	int b1_up = 0, b1_down = 0, b1_left = 0, b1_right = 0, b2_up = 0, b2_down = 0, b2_left = 0, b2_right = 0;
	int lives1 = lives, lives2 = lives, wins1 = 0, wins2 = 0;
	match this_game;
	object map[20][80];
	object ship1 = { {8, 10}, SHIP_RIGHT, BLUE };
	object ship2 = { {72, 10}, SHIP_LEFT, RED };
	object bullet1 = { ship1.position, L'.', WHITE };
	object bullet2 = { ship2.position, L'.', WHITE };

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 80; x++)
		{
			if (x == 0 || y == 0 || x == 79 || y == 19)
				map[y][x].output = BLOCK;
			else
				map[y][x].output = SPACE;
			map[y][x].position.X = x;
			map[y][x].position.Y = y;
		}
	}

	map[2][24].output = map[2][56].output = map[3][24].output = map[3][56].output = map[5][6].output = map[5][7].output = map[5][8].output = map[5][9].output = map[5][10].output = map[5][11].output = map[5][12].output = map[5][68].output = map[5][69].output = map[5][70].output = map[5][71].output = map[5][72].output = map[5][73].output = map[5][74].output = map[6][12].output = map[6][40].output = map[6][68].output = map[7][12].output = map[7][40].output = map[7][45].output = map[7][46].output = map[7][47].output = map[7][48].output = map[7][49].output = map[7][50].output = map[7][51].output = map[7][68].output = BLOCK;
	map[8][12].output = map[8][40].output = map[8][68].output = map[9][12].output = map[9][40].output = map[9][68].output = map[10][68].output = map[10][12].output = map[10][68].output = map[11][12].output = map[11][28].output = map[11][29].output = map[11][30].output = map[11][31].output = map[11][32].output = map[11][33].output = map[11][34].output = map[11][40].output = map[11][68].output = map[12][12].output = map[12][40].output = map[12][68].output = map[13][12].output = map[13][40].output = map[13][68].output = map[14][6].output = map[14][7].output = map[14][8].output = map[14][9].output = map[14][10].output = map[14][11].output = map[14][12].output = map[14][40].output = map[14][68].output = map[14][69].output = map[14][70].output = map[14][71].output = map[14][72].output = map[14][73].output = map[14][74].output = BLOCK;
	map[16][24].output = map[16][56].output = map[17][24].output = map[17][56].output = BLOCK;

	map[2][25].output = map[3][25].output = map[6][13].output = map[6][67].output = map[7][13].output = map[7][67].output = map[10][40].output = map[12][13].output = map[12][67].output = map[13][13].output = map[13][67].output = map[16][55].output = map[17][55].output = MIRROR;

	map[4][40].output = HEART;
	map[4][40].color = RED;
	object heart = { {40, 4}, HEART, RED };

	map[4][63].output = map[15][17].output = BLACKHOLE;

	map[17][7].output = map[17][74].output = PORTAL;
	map[17][7].color = map[17][74].color = YELLOW;

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 80; x++)
		{
			wprintf(L"\033[1;%dm%lc\033[0m", map[y][x].color, map[y][x].output);
		}
		wprintf(L"\n");
	}

	update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
	wprintf(L"\n\t\t\t\t\t 1st");

	SetConsoleCursorPosition(hStdout, ship1.position);
	wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
	SetConsoleCursorPosition(hStdout, ship2.position);
	wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);

	while (lives1 > 0 && lives2 > 0)
	{
		if (seconds >= 142)
		{
			play_battle();
			seconds = 0;
		}
		start = end = 0;
		start = clock();
		if (map[heart.position.Y][heart.position.X].output == SPACE)
		{
			static int count = 1;
			if (count % 300 == 0)
			{
				int x, y;
				srand(time(NULL));
				x = rand() % 77 + 2;
				y = rand() % 17 + 2;
				if (map[y][x].output == SPACE && map[y][x + 1].output != BLOCK || map[y][x - 1].output != BLOCK && map[y + 1][x].output != BLOCK && map[y - 1][x].output != BLOCK)
				{
					heart.position = { (short)x, (short)y };
					map[y][x].output = HEART;
					map[y][x].color = RED;
					show_screen(hStdout, heart.position, heart.output, heart.color);
					count++;
				}
			}
			else
				count++;
		}
		if (_kbhit())
		{
			//ship1
			if (is_pressed('W') && ship1.position.Y > 0 && map[ship1.position.Y - 1][ship1.position.X].output != BLOCK && map[ship1.position.Y - 1][ship1.position.X].output != MIRROR)
			{
				if (ship1.position.X != ship2.position.X || ship1.position.Y - 1 != ship2.position.Y)
				{
					switch (map[ship1.position.Y - 1][ship1.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						SetConsoleCursorPosition(hStdout, ship1.position);
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm!\033[0m", ship1.color);
						ship1.position = { 8, 10 };
						ship1.output = SHIP_RIGHT;
						lives1 -= 1;
						if (lives1 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship1.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 74, 16 };
							ship1.output = SHIP_UP;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 7, 16 };
							ship1.output = SHIP_UP;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y -= 1;
						ship1.output = SHIP_UP;
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_UP;
					SetConsoleCursorPosition(hStdout, ship1.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
				}

			}
			if (is_pressed('S') && ship1.position.Y < 19 && map[ship1.position.Y + 1][ship1.position.X].output != BLOCK && map[ship1.position.Y + 1][ship1.position.X].output != MIRROR)
			{
				if (ship1.position.X != ship2.position.X || ship1.position.Y + 1 != ship2.position.Y)
				{
					switch (map[ship1.position.Y + 1][ship1.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						SetConsoleCursorPosition(hStdout, ship1.position);
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm!\033[0m", ship1.color);
						ship1.position = { 8, 10 };
						ship1.output = SHIP_RIGHT;
						lives1 -= 1;
						if (lives1 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship1.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 74, 18 };
							ship1.output = SHIP_DOWN;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 7, 18 };
							ship1.output = SHIP_DOWN;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.Y += 1;
						ship1.output = SHIP_DOWN;
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_DOWN;
					SetConsoleCursorPosition(hStdout, ship1.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
				}
			}
			if (is_pressed('D') && ship1.position.X < 80 && map[ship1.position.Y][ship1.position.X + 1].output != BLOCK && map[ship1.position.Y][ship1.position.X + 1].output != MIRROR)
			{
				if (ship1.position.X + 1 != ship2.position.X || ship1.position.Y != ship2.position.Y)
				{
					switch (map[ship1.position.Y][ship1.position.X + 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						SetConsoleCursorPosition(hStdout, ship1.position);
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm!\033[0m", ship1.color);
						ship1.position = { 8, 10 };
						ship1.output = SHIP_RIGHT;
						lives1 -= 1;
						if (lives1 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship1.position.X == 6)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 75, 17 };
							ship1.output = SHIP_RIGHT;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 8, 17 };
							ship1.output = SHIP_RIGHT;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X += 1;
						ship1.output = SHIP_RIGHT;
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_RIGHT;
					SetConsoleCursorPosition(hStdout, ship1.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
				}
			}
			if (is_pressed('A') && ship1.position.X > 0 && map[ship1.position.Y][ship1.position.X - 1].output != BLOCK && map[ship1.position.Y][ship1.position.X - 1].output != MIRROR)
			{
				if (ship1.position.X - 1 != ship2.position.X || ship1.position.Y != ship2.position.Y)
				{
					switch (map[ship1.position.Y][ship1.position.X - 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						SetConsoleCursorPosition(hStdout, ship1.position);
						map[ship1.position.Y][ship1.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						lives1 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm!\033[0m", ship1.color);
						ship1.position = { 8, 10 };
						ship1.output = SHIP_RIGHT;
						lives1 -= 1;
						if (lives1 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship1.position.X == 8)
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 73, 17 };
							ship1.output = SHIP_LEFT;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L" ");
							ship1.position = { 6, 17 };
							ship1.output = SHIP_LEFT;
							SetConsoleCursorPosition(hStdout, ship1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L" ");
						ship1.position.X -= 1;
						ship1.output = SHIP_LEFT;
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
						break;
					}
				}
				else
				{
					ship1.output = SHIP_LEFT;
					SetConsoleCursorPosition(hStdout, ship1.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
				}
			}
			if (is_pressed('C'))
			{
				if (b1_up + b1_down + b1_right + b1_left == 0)
				{
					bullet1.position = ship1.position;
					switch (ship1.output)
					{
					case SHIP_UP:
						if (map[ship1.position.Y - 1][ship1.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y - 1][ship1.position.X].output != BLOCK)
						{
							b1_up = 1;
							bullet1.position.Y -= 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						break;
					case SHIP_DOWN:
						if (map[ship1.position.Y + 1][ship1.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y + 1][ship1.position.X].output != BLOCK)
						{
							b1_down = 1;
							bullet1.position.Y += 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						break;
					case SHIP_RIGHT:
						if (map[ship1.position.Y][ship1.position.X + 1].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y][ship1.position.X + 1].output != BLOCK)
						{
							b1_right = 1;
							bullet1.position.X += 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						break;
					case SHIP_LEFT:
						if (map[ship1.position.Y][ship1.position.X - 1].output == MIRROR)
						{
							player_death(hStdout, &ship1, { 8, 10 }, &lives1);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship1.position, ship1.output, ship1.color);
						}
						else if (map[ship1.position.Y][ship1.position.X - 1].output != BLOCK)
						{
							b1_left = 1;
							bullet1.position.X -= 1;
							SetConsoleCursorPosition(hStdout, bullet1.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet1.color, bullet1.output);
						}
						break;
					}
				}
				else
				{
					b1_up = b1_down = b1_right = b1_left = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
				}
			}

			//ship2
			if (is_pressed('I') && ship2.position.Y > 0 && map[ship2.position.Y - 1][ship2.position.X].output != BLOCK && map[ship2.position.Y - 1][ship2.position.X].output != MIRROR)
			{
				if (ship2.position.X != ship1.position.X || ship2.position.Y - 1 != ship1.position.Y)
				{
					switch (map[ship2.position.Y - 1][ship2.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						SetConsoleCursorPosition(hStdout, ship2.position);
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm!\033[0m", ship2.color);
						ship2.position = { 72, 10 };
						ship2.output = SHIP_LEFT;
						lives2 -= 1;
						if (lives2 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship2.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 74, 16 };
							ship2.output = SHIP_UP;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 7, 16 };
							ship2.output = SHIP_UP;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y -= 1;
						ship2.output = SHIP_UP;
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_UP;
					SetConsoleCursorPosition(hStdout, ship2.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
				}
			}
			if (is_pressed('K') && ship2.position.Y < 19 && map[ship2.position.Y + 1][ship2.position.X].output != BLOCK && map[ship2.position.Y + 1][ship2.position.X].output != MIRROR)
			{
				if (ship2.position.X != ship1.position.X || ship2.position.Y + 1 != ship1.position.Y)
				{
					switch (map[ship2.position.Y + 1][ship2.position.X].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						SetConsoleCursorPosition(hStdout, ship2.position);
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm!\033[0m", ship2.color);
						ship2.position = { 72, 10 };
						ship2.output = SHIP_LEFT;
						lives2 -= 1;
						if (lives2 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship2.position.X == 7)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 74, 18 };
							ship2.output = SHIP_DOWN;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 7, 18 };
							ship2.output = SHIP_DOWN;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.Y += 1;
						ship2.output = SHIP_DOWN;
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_DOWN;
					SetConsoleCursorPosition(hStdout, ship2.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
				}
			}
			if (is_pressed('L') && ship2.position.X < 80 && map[ship2.position.Y][ship2.position.X + 1].output != BLOCK && map[ship2.position.Y][ship2.position.X + 1].output != MIRROR)
			{
				if (ship2.position.X + 1 != ship1.position.X || ship2.position.Y != ship1.position.Y)
				{
					switch (map[ship2.position.Y][ship2.position.X + 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						SetConsoleCursorPosition(hStdout, ship2.position);
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm!\033[0m", ship2.color);
						ship2.position = { 72, 10 };
						ship2.output = SHIP_LEFT;
						lives2 -= 1;
						if (lives2 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship2.position.X == 6)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 75, 17 };
							ship2.output = SHIP_RIGHT;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 8, 17 };
							ship2.output = SHIP_RIGHT;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X += 1;
						ship2.output = SHIP_RIGHT;
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_RIGHT;
					SetConsoleCursorPosition(hStdout, ship2.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
				}
			}
			if (is_pressed('J') && ship2.position.X > 0 && map[ship2.position.Y][ship2.position.X - 1].output != BLOCK && map[ship2.position.Y][ship2.position.X - 1].output != MIRROR)
			{
				if (ship2.position.X - 1 != ship1.position.X || ship2.position.Y != ship1.position.Y)
				{
					switch (map[ship2.position.Y][ship2.position.X - 1].output)
					{
					case HEART:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						SetConsoleCursorPosition(hStdout, ship2.position);
						map[ship2.position.Y][ship2.position.X].output = SPACE;
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						lives2 += 5;
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case BLACKHOLE:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm!\033[0m", ship2.color);
						ship2.position = { 72, 10 };
						ship2.output = SHIP_LEFT;
						lives2 -= 1;
						if (lives2 > 0)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
						break;
					case PORTAL:
						if (ship2.position.X == 8)
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 73, 17 };
							ship2.output = SHIP_LEFT;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						else
						{
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L" ");
							ship2.position = { 6, 17 };
							ship2.output = SHIP_LEFT;
							SetConsoleCursorPosition(hStdout, ship2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						}
						break;
					default:
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L" ");
						ship2.position.X -= 1;
						ship2.output = SHIP_LEFT;
						SetConsoleCursorPosition(hStdout, ship2.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
						break;
					}
				}
				else
				{
					ship2.output = SHIP_LEFT;
					SetConsoleCursorPosition(hStdout, ship2.position);
					wprintf(L"\033[1;%dm%lc\033[0m", ship2.color, ship2.output);
				}
			}
			if (is_pressed('N'))
			{
				if (b2_up + b2_down + b2_right + b2_left == 0)
				{
					bullet2.position = ship2.position;
					switch (ship2.output)
					{
					case SHIP_UP:
						if (map[ship2.position.Y - 1][ship2.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y - 1][ship2.position.X].output != BLOCK)
						{
							b2_up = 1;
							bullet2.position.Y -= 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						break;
					case SHIP_DOWN:
						if (map[ship2.position.Y + 1][ship2.position.X].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y + 1][ship2.position.X].output != BLOCK)
						{
							b2_down = 1;
							bullet2.position.Y += 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						break;
					case SHIP_RIGHT:
						if (map[ship2.position.Y][ship2.position.X + 1].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y][ship2.position.X + 1].output != BLOCK)
						{
							b2_right = 1;
							bullet2.position.X += 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						break;
					case SHIP_LEFT:
						if (map[ship2.position.Y][ship2.position.X - 1].output == MIRROR)
						{
							player_death(hStdout, &ship2, { 72, 10 }, &lives2);
							update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
							show_screen(hStdout, ship2.position, ship2.output, ship2.color);
						}
						else if (map[ship2.position.Y][ship2.position.X - 1].output != BLOCK)
						{
							b2_left = 1;
							bullet2.position.X -= 1;
							SetConsoleCursorPosition(hStdout, bullet2.position);
							wprintf(L"\033[1;%dm%lc\033[0m", bullet2.color, bullet2.output);
						}
						break;
					}
				}
				else
				{
					b2_up = b2_down = b2_right = b2_left = 0;
					show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
				}
			}

			//quit the game
			if (is_pressed(VK_END))
			{
				info(player1);
				stop_playing();
				seconds = 0;
				return;
			}
		}

		//bullets
		if (b1_up + b1_down + b1_right + b1_left > 0)
		{
			if (b1_up == 1)
			{
				if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK || bullet1.position.Y == 0)
				{
					b1_up = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
				}
				else if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y - 1 == ship2.position.Y))
				{
					b1_up = 0;
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet1.position.Y - 1][bullet1.position.X].output == MIRROR)
				{
					b1_up = 0;
					b1_down = 1;
				}
				else if (bullet1.position.X == ship1.position.X && (bullet1.position.Y == ship1.position.Y || bullet1.position.Y - 1 == ship1.position.Y))
				{
					b1_up = 0;
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					player_death(hStdout, &ship1, { 8, 10 }, &lives1);
					if (lives1 > 0)
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet1.position.Y - 1][bullet1.position.X].output == SPACE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					bullet1.position.Y -= 1;
					show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					bullet1.position.Y -= 1;
				}
			}
			if (b1_down == 1)
			{
				if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK || bullet1.position.Y > 18)
				{
					b1_down = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
				}
				else if (map[bullet1.position.Y + 1][bullet1.position.X].output == MIRROR)
				{
					b1_up = 1;
					b1_down = 0;
				}
				else if (bullet1.position.X == ship2.position.X && (bullet1.position.Y == ship2.position.Y || bullet1.position.Y + 1 == ship2.position.Y))
				{
					b1_down = 0;
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (bullet1.position.X == ship1.position.X && (bullet1.position.Y == ship1.position.Y || bullet1.position.Y + 1 == ship1.position.Y))
				{
					b1_down = 0;
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					player_death(hStdout, &ship1, { 8, 10 }, &lives1);
					if (lives1 > 0)
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet1.position.Y + 1][bullet1.position.X].output == SPACE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					bullet1.position.Y += 1;
					show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					bullet1.position.Y += 1;
				}
			}
			if (b1_right == 1)
			{
				if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK || bullet1.position.X == 80)
				{
					b1_right = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
				}
				else if (map[bullet1.position.Y][bullet1.position.X + 1].output == MIRROR)
				{
					b1_right = 0;
					b1_left = 1;
				}
				else if ((bullet1.position.X == ship2.position.X || bullet1.position.X + 1 == ship2.position.X) && bullet1.position.Y == ship2.position.Y)
				{
					b1_right = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if ((bullet1.position.X == ship1.position.X || bullet1.position.X + 1 == ship1.position.X) && bullet1.position.Y == ship1.position.Y)
				{
					b1_right = 0;
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					player_death(hStdout, &ship1, { 8, 10 }, &lives1);
					if (lives1 > 0)
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet1.position.Y][bullet1.position.X + 1].output == SPACE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					bullet1.position.X += 1;
					show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					bullet1.position.X += 1;
				}
			}
			if (b1_left == 1)
			{
				if (map[bullet1.position.Y][bullet1.position.X].output == BLOCK || bullet1.position.X == 0)
				{
					b1_left = 0;
					show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
				}
				else if (map[bullet1.position.Y][bullet1.position.X - 1].output == MIRROR)
				{
					b1_left = 0;
					b1_right = 1;
				}
				else if ((bullet1.position.X == ship2.position.X || bullet1.position.X - 1 == ship2.position.X) && (bullet1.position.Y == ship2.position.Y))
				{
					b1_left = 0;
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if ((bullet1.position.X == ship1.position.X || bullet1.position.X - 1 == ship1.position.X) && bullet1.position.Y == ship1.position.Y)
				{
					b1_left = 0;
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					player_death(hStdout, &ship1, { 8, 10 }, &lives1);
					if (lives1 > 0)
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet1.position.Y][bullet1.position.X - 1].output == SPACE)
				{
					if (map[bullet1.position.Y][bullet1.position.X].output != SPACE)
						show_screen(hStdout, bullet1.position, map[bullet1.position.Y][bullet1.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet1.position);
						wprintf(L" ");
					}
					bullet1.position.X -= 1;
					show_screen(hStdout, bullet1.position, bullet1.output, bullet1.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet1.position);
					wprintf(L" ");
					bullet1.position.X -= 1;
				}
			}

		}
		if (b2_up + b2_down + b2_right + b2_left > 0)
		{
			if (b2_up == 1)
			{
				if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK || bullet2.position.Y == 0)
				{
					b2_up = 0;
					show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
				}
				else if ((bullet2.position.Y - 1 == ship1.position.Y || bullet2.position.Y == ship1.position.Y) && bullet2.position.X == ship1.position.X)
				{
					b2_up = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					player_death(hStdout, &ship1, { 8, 10 }, &lives1);
					if (lives1 > 0)
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet2.position.Y - 1][bullet2.position.X].output == MIRROR)
				{
					b2_up = 0;
					b2_down = 1;
				}
				else if ((bullet2.position.Y - 1 == ship2.position.Y || bullet2.position.Y == ship2.position.Y) && bullet2.position.X == ship2.position.X)
				{
					b2_up = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet2.position.Y - 1][bullet2.position.X].output == SPACE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					bullet2.position.Y -= 1;
					show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					bullet2.position.Y -= 1;
				}
			}
			if (b2_down == 1)
			{
				if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK || bullet2.position.Y > 18)
				{
					b2_down = 0;
					show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
				}
				else if (map[bullet2.position.Y + 1][bullet2.position.X].output == MIRROR)
				{
					b2_up = 1;
					b2_down = 0;
				}
				else if ((bullet2.position.Y == ship1.position.Y || bullet2.position.Y + 1 == ship1.position.Y) && bullet2.position.X == ship1.position.X)
				{
					b2_down = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					player_death(hStdout, &ship1, { 8, 10 }, &lives1);
					if (lives1 > 0)
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);

				}
				else if ((bullet2.position.Y + 1 == ship2.position.Y || bullet2.position.Y == ship2.position.Y) && bullet2.position.X == ship2.position.X)
				{
					b2_down = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet2.position.Y + 1][bullet2.position.X].output == SPACE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet1.position.Y][bullet1.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					bullet2.position.Y += 1;
					show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					bullet2.position.Y += 1;
				}
			}
			if (b2_right == 1)
			{
				if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK || bullet2.position.X == 80)
				{
					b2_right = 0;
					show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
				}
				else if (map[bullet2.position.Y][bullet2.position.X + 1].output == MIRROR)
				{
					b2_right = 0;
					b2_left = 1;
				}
				else if ((bullet2.position.X == ship1.position.X || bullet2.position.X + 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
				{
					b2_right = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					SetConsoleCursorPosition(hStdout, ship1.position);
					wprintf(L"\033[1;%dm!\033[0m", ship1.color);
					ship1.position = { 8, 10 };
					ship1.output = SHIP_RIGHT;
					lives1 -= 1;
					if (lives1 > 0)
					{
						SetConsoleCursorPosition(hStdout, ship1.position);
						wprintf(L"\033[1;%dm%lc\033[0m", ship1.color, ship1.output);
					}
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if ((bullet2.position.X == ship2.position.X || bullet2.position.X + 1 == ship2.position.X) && bullet2.position.Y == ship2.position.Y)
				{
					b2_right = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet2.position.Y][bullet2.position.X + 1].output == SPACE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					bullet2.position.X += 1;
					show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					bullet2.position.X += 1;
				}
			}
			if (b2_left == 1)
			{
				if (map[bullet2.position.Y][bullet2.position.X].output == BLOCK || bullet2.position.X == 0)
				{
					b2_left = 0;
					show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
				}
				else if (map[bullet2.position.Y][bullet2.position.X - 1].output == MIRROR)
				{
					b2_left = 0;
					b2_right = 1;
				}
				else if ((bullet2.position.X == ship1.position.X || bullet2.position.X - 1 == ship1.position.X) && bullet2.position.Y == ship1.position.Y)
				{
					b2_left = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					player_death(hStdout, &ship1, { 8, 10 }, &lives1);
					if (lives1 > 0)
						show_screen(hStdout, ship1.position, ship1.output, ship1.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if ((bullet2.position.X == ship2.position.X || bullet2.position.X - 1 == ship2.position.X) && bullet2.position.Y == ship2.position.Y)
				{
					b2_left = 0;
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					player_death(hStdout, &ship2, { 72, 10 }, &lives2);
					if (lives2 > 0)
						show_screen(hStdout, ship2.position, ship2.output, ship2.color);
					update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
				}
				else if (map[bullet2.position.Y][bullet2.position.X - 1].output == SPACE)
				{
					if (map[bullet2.position.Y][bullet2.position.X].output != SPACE)
						show_screen(hStdout, bullet2.position, map[bullet2.position.Y][bullet2.position.X].output, map[bullet2.position.Y][bullet2.position.X].color);
					else
					{
						SetConsoleCursorPosition(hStdout, bullet2.position);
						wprintf(L" ");
					}
					bullet2.position.X -= 1;
					show_screen(hStdout, bullet2.position, bullet2.output, bullet2.color);
				}
				else
				{
					SetConsoleCursorPosition(hStdout, bullet2.position);
					wprintf(L" ");
					bullet2.position.X -= 1;
				}
			}

		}

		SetConsoleCursorPosition(hStdout, { 0, 22 });
		Sleep(100);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
	}

	char winner[101];

	if (lives1 == 0)
	{
		this_game.id1 = player2->id;
		wins2++;
		strcpy(winner, player2->username);
	}
	else
	{
		this_game.id1 = player1->id;
		wins1++;
		strcpy(winner, player1->username);
	}
	update_UI(hStdout, *player1, *player2, lives1, lives2, wins1, wins2);
	_setmode(_fileno(stdout), 0x4000);
	printf("\n\n\t\t\t\t*** \033[1;%dm %s WINS! \033[0m***\n", wins1 > wins2 ? BLUE : RED, winner);
	_setmode(_fileno(stdout), 0x20000);

	Sleep(1000);

	game2(&this_game, player1, player2, lives, wins1, wins2);
}

void game_settings(data* player1)
{
	system("cls||clear");
	start = end = 0;
	start = clock();
	clearInputBuffer();
	setvbuf(stdin, NULL, _IONBF, 0);
	data player2;
	int result;
	char input[101];
	char check[151];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔═════════════════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║                                                 ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║  \033[1;%dmPlayer 2, please enter your name and password\033[1;%dm  ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║              \033[1;%dm To cancel, enter -1               \033[1;%dm║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚═════════════════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"Enter your name: ");
	gets_s(input);

	if (!strcmp("-1", input))
	{
		info(player1);
		return;
	}
	if (!find(&player2, input))
	{
		wprintf(L"\n\033[1;%dmThere is no player with this name!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		game_settings(player1);
		return;
	}
	if (!strcmp(player1->username, input))
	{
		wprintf(L"\n\033[1;%dmThis account is already in use!\033[0m", RED);
		Sleep(750);
		clearInputBuffer();
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		game_settings(player1);
		return;
	}

	wprintf(L"Enter your password(must contain at least 8 characters): ");
	result = mask(check);
	if (result == -1)
	{
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		info(player1);
		return;
	}
	if (result == 0)
	{
		wprintf(L"\033[1;%dm\nInvalid password!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		game_settings(player1);
		return;
	}
	if (strcmp(player2.password, check))
	{
		wprintf(L"\033[1;%dm\nWrong password!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		game_settings(player1);
		return;
	}

	wprintf(L"\n\033[1;%dmHow many lives must each player have?\033[0m   ", CYAN);
	scanf("%d", &result);
	if (result == -1)
	{
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		info(player1);
		return;
	}
	else if (result < 0)
	{
		wprintf(L"\033[1;%dm\nInvalid value!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		game_settings(player1);
		return;
	}

	system("cls||clear");

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║              \033[1;%dmAll set!\033[1;%dm                ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██  └──┘  ██\033[1;%dm\t    ║         \033[1;%dmLet the match BEGIN!\033[1;%dm         ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");

	Sleep(2000);
	system("cls||clear");
	end = clock();
	seconds += (float)(end - start) / CLOCKS_PER_SEC;
	clearInputBuffer();
	game1(player1, &player2, result);

}

void change_name(data* player)
{
	system("cls||clear");
	start = end = 0;
	start = clock();
	clearInputBuffer();

	char input[101];
	char previous[101];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\     ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t     ║        \033[1;%dm  To cnacel enter -1          \033[1;%dm║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t     ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\n", GREEN);
	wprintf(L"\t ████████████\n");
	wprintf(L"\t ██ ██  ██ ██\n");
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"Enter your new name:");
	gets_s(input);

	if (!strcmp("-1", input))
	{
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		info(player);
		return;
	}
	if (check_if_repeated(input))
	{
		wprintf(L"\n\033[1;%dmThis name is already taken!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		change_name(player);
		return;
	}

	strcpy(previous, player->username);
	rewrite_account(player, input, player->password, player->email);

	system("cls||clear");

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║       \033[1;%dm***CHANGED SUCCESSFULLY***\033[1;%dm     ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██  ╚══╝  ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");

	Sleep(2000);
	end = clock();
	seconds += (float)(end - start) / CLOCKS_PER_SEC;
	info(player);


}

void change_password(data* player, int state)
{
	system("cls||clear");
	start = end = 0;
	start = clock();
	clearInputBuffer();

	int result;
	char input[101];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\     ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t     ║        \033[1;%dm  To cnacel enter -1          \033[1;%dm║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t     ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\n", GREEN);
	wprintf(L"\t ████████████\n");
	wprintf(L"\t ██ ██  ██ ██\n");
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"Enter your new password(must contain at least 8 characters): ");
	result = mask(player->password);
	if (result == -1)
	{
		if (state == 1)
			info(player);
		else
			with_email();
		return;
	}
	if (result == 0)
	{
		wprintf(L"\033[1;%dm\nInvalid password!\033[0m", RED);
		Sleep(750);
		change_password(player, state);
		return;
	}

	wprintf(L"\nPlease repeat your password: ");
	result = mask(input);
	if (result == -1)
	{
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		if (state == 1)
			info(player);
		else
			with_email();
		return;
	}
	if (strcmp(player->password, input))
	{
		wprintf(L"\033[1;%dm\nWrong password!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		change_password(player, state);
		return;
	}

	rewrite_account(player, player->username, input, player->email);

	system("cls||clear");

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║       \033[1;%dm***CHANGED SUCCESSFULLY***\033[1;%dm     ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██  ╚══╝  ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");

	Sleep(2000);
	end = clock();
	seconds += (float)(end - start) / CLOCKS_PER_SEC;
	if (state == 1)
		info(player);
	else
		main_menu();
}

void change_email(data* player)
{
	system("cls||clear");
	start = end = 0;
	start = clock();
	clearInputBuffer();

	char input[101];
	int result;

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\     ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t     ║        \033[1;%dm  To cnacel enter -1          \033[1;%dm║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t     ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\n", GREEN);
	wprintf(L"\t ████████████\n");
	wprintf(L"\t ██ ██  ██ ██\n");
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"Enter your email: ");
	gets_s(input);
	result = is_email(input);
	if (!strcmp("-1", input))
	{
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		info(player);
		return;
	}
	if (!result)
	{
		wprintf(L"\033[1;%dmInvalid email!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		change_email(player);
		return;
	}

	rewrite_account(player, player->username, player->password, input);

	system("cls||clear");

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║       \033[1;%dm***CHANGED SUCCESSFULLY***\033[1;%dm     ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██  ╚══╝  ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");

	Sleep(2000);
	end = clock();
	seconds += (float)(end - start) / CLOCKS_PER_SEC;
	info(player);
}

void info(data* player);

void change(data* player)
{
	system("cls||clear");
	_setmode(_fileno(stdout), 0x20000);
	start = end = 0;
	start = clock();
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	object choose = { {0, 9},  0x25BA };

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\     ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t     ║   \033[1;%dm  What do you want to change?      \033[1;%dm║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t     ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\n", GREEN);
	wprintf(L"\t ████████████\n");
	wprintf(L"\t ██ ██  ██ ██\n");
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"\n\tUSERNAME\n");
	wprintf(L"\tPASSWORD\n");
	wprintf(L"\tE_MAIL\n");
	wprintf(L"\tCancel");
	SetConsoleCursorPosition(hStdout, choose.position);
	wprintf(L"%lc", choose.output);

	while (1)
	{
		if (seconds >= 120)
		{
			play_menu();
			seconds = 0;
		}
		if (is_pressed(VK_UP) && (choose.position.Y > 9))
		{
			wprintf(L"\r ");
			choose.position.Y -= 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_DOWN) && (choose.position.Y < 12))
		{
			wprintf(L"\r ");
			choose.position.Y += 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_INSERT))
		{

			switch (choose.position.Y)
			{
			case 9:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				change_name(player);
				return;
			case 10:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				change_password(player, 1);
				return;
			case 11:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				change_email(player);
				return;
			case 12:
				info(player);
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				return;
			}
		}
		Sleep(100);
	}

}

void show_history(data* player)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	start = end = 0;
	start = clock();
	int count = 1;
	int wins = 0, losses = 0;
	match temp;

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██╔═╦══╦═╗██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██╚═╝  ╚═╝██\033[1;%dm\t    ║          \033[1;%dm***YOUR HISTORY***\033[1;%dm          ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██   ──   ██\033[1;%dm\t    ║          \033[1;%dmPress Esc to return         \033[1;%dm║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	FILE* fptr;
	fptr = fopen("history.txt", "r");
	data second, lev1, lev2, lev3;

	_setmode(_fileno(stdout), 0x4000);

	printf("\n\t___________________________________\n");

	if (fptr == NULL)
	{
		printf("\n\t \033[1;%dmThere is no history of you!\n\033[0m", RED);
	}
	else
	{
		while (fread(&temp, sizeof(match), 1, fptr) == 1)
		{
			if (temp.winner_id == player->id)
			{
				find_id(player, temp.winner_id);
				find_id(&second, temp.loser_id);
				find_id(&lev1, temp.id1);
				find_id(&lev2, temp.id2);
				if (temp.id3 > -1)
					find_id(&lev3, temp.id3);
				printf(" %d-\t\033[1;%dm%s %s %s\033[0m\n", count, GREEN, player->username, temp.result, second.username);
				printf("   \t\033[1;%dm1: %s\n \t2: %s\n   \t3: %s\033[0m\n", CYAN, lev1.username, lev2.username, temp.id3 != -1 ? lev3.username : "-");
				count++;
				wins++;
			}
			else if (temp.loser_id == player->id)
			{
				find_id(player, temp.loser_id);
				find_id(&second, temp.winner_id);
				find_id(&lev1, temp.id1);
				find_id(&lev2, temp.id2);
				if (temp.id3 > -1)
					find_id(&lev3, temp.id3);
				printf(" %d-\t\033[1;%dm%s %s %s\033[0m\n", count, RED, second.username, temp.result, player->username);
				printf("   \t\033[1;%dm1: %s\n \t2: %s\n   \t3: %s\033[0m\n", CYAN, lev1.username, lev2.username, temp.id3 != -1 ? lev3.username : "-");
				count++;
				losses++;
			}
		}

		if (count == 1)
			printf("\n\t \033[1;%dmThere is no history of you!\n\033[0m", RED);
		fclose(fptr);
	}

	printf("\t___________________________________\n");
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, 8 });
	printf("\033[1;%dm%s :   (wins : %d    losses : %d)\033[0m\n", CYAN, player->username, wins, losses);

	while (1)
	{
		if (seconds >= 120)
		{
			play_menu();
			seconds = 0;
		}
		if (is_pressed(VK_ESCAPE))
		{
			info(player);
			end = clock();
			seconds += (float)(end - start);
			return;
		}
	}
}

void info(data* player)
{
	system("cls||clear");

	_setmode(_fileno(stdout), 0x20000);
	clearInputBuffer();
	setvbuf(stdin, NULL, _IONBF, 0);

	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	start = end = 0;
	start = clock();
	if (is_playing == 0)
		play_menu();

	object choose = { {0, 10},  0x25BA };


	char input[51];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t╔══════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t║     \033[1;%dmWELCOME!\033[1;%dm     ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t╚══════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██   └─   ██  ██\n", GREEN);
	wprintf(L"\t ████████████ ███\n");
	wprintf(L"\t ██ ██  ██  ███\n");
	wprintf(L"\t ██ ██  ██\033[0m\n\n");
	_setmode(_fileno(stdout), 0x4000);
	printf("\033[1;%dmPlayer : %s\n\033[0m", CYAN, player->username);
	_setmode(_fileno(stdout), 0x20000);
	wprintf(L"\n\tStart game\n");
	wprintf(L"\tChange info\n");
	wprintf(L"\tHistory\n");
	wprintf(L"\tReturn to main menu");
	SetConsoleCursorPosition(hStdout, choose.position);
	wprintf(L"%lc", choose.output);

	while (1)
	{
		if (seconds >= 120)
		{
			play_menu();
			seconds = 0;
		}
		if (is_pressed(VK_UP) && (choose.position.Y > 10))
		{
			wprintf(L"\r ");
			choose.position.Y -= 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_DOWN) && (choose.position.Y < 13))
		{
			wprintf(L"\r ");
			choose.position.Y += 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_INSERT))
		{

			switch (choose.position.Y)
			{
			case 10:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				game_settings(player);
				return;
			case 11:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				change(player);
				return;
			case 12:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				show_history(player);
				return;
			case 13:
				main_menu();
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				return;
			}
		}
		Sleep(100);
	}
}

void with_password()
{
	system("cls||clear");
	start = end = 0;
	start = clock();
	_setmode(_fileno(stdout), 0x20000);

	clearInputBuffer();

	int result;

	char input[51];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║ \033[1;%dmin order to sign in, please enter:\033[1;%dm   ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║         \033[1;%dmyour name, password\033[1;%dm          ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║         \033[1;%dmto cancel enter -1\033[1;%dm           ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");

	wprintf(L"Enter your name: ");
	data player1;
	gets_s(input);
	if (!strcmp("-1", input))
	{
		main_menu();
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		return;
	}
	if (!find(&player1, input))
	{
		wprintf(L"\033[1;%dmA player with this username does not exist!", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		with_password();
		return;
	}
	wprintf(L"Enter your password:");
	result = mask(input);
	if (result == -1)
	{
		main_menu();
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		return;
	}
	if (strcmp(player1.password, input) || result == 0)
	{
		wprintf(L"\033[1;%dm\nWrong password!", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		with_password();
		return;
	}



	system("cls||clear");

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║     \033[1;%dm***SIGNED IN SUCCESSFULLY***\033[1;%dm     ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██  └──┘  ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");

	Sleep(2000);
	end = clock();
	seconds += (float)(end - start) / CLOCKS_PER_SEC;
	clearInputBuffer();
	info(&player1);

}

void with_email()
{
	system("cls||clear");
	start = end = 0;
	start = clock();
	_setmode(_fileno(stdout), 0x20000);

	clearInputBuffer();


	int result;
	char input[51];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔════════════════════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║  \033[1;%dmin order to change your password, please enter:\033[1;%dm   ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║               \033[1;%dmyour name and email\033[1;%dm                  ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║               \033[1;%dmto cancel enter -1\033[1;%dm                   ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚════════════════════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"Enter your name: ");
	data player1;
	gets_s(input);
	if (!strcmp("-1", input))
	{
		main_menu();
		return;
	}
	if (!find(&player1, input))
	{
		wprintf(L"\033[1;%dmA player with this username does not exist!", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		with_email();
		return;
	}
	wprintf(L"Enter your email:");
	gets_s(input);
	if (!strcmp("-1", input))
	{
		main_menu();
		end = clock();
		seconds += (float)(end - start);
		return;
	}
	if (strcmp(player1.email, input))
	{
		wprintf(L"\033[1;%dmWrong email!", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		with_email();
		return;
	}

	end = clock();
	seconds += (float)(end - start) / CLOCKS_PER_SEC;
	change_password(&player1, 0);
}

void sign_in()
{
	system("cls||clear");
	_setmode(_fileno(stdout), 0x20000);
	clearInputBuffer();
	start = end = 0;
	start = clock();
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	object choose = { {0, 8},  0x25BA };


	char input[51];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\     ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t     ║  \033[1;%dm  Do you remember your password?    \033[1;%dm║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t     ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\n", GREEN);
	wprintf(L"\t ████████████\n");
	wprintf(L"\t ██ ██  ██ ██\n");
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"\tYes\n");
	wprintf(L"\tNo");
	SetConsoleCursorPosition(hStdout, choose.position);
	wprintf(L"%lc", choose.output);


	while (1)
	{
		if (seconds >= 120)
		{
			play_menu();
			seconds = 0;
		}
		if (is_pressed(VK_UP) && choose.position.Y > 8)
		{
			wprintf(L"\r ");
			choose.position.Y -= 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_DOWN) && choose.position.Y < 9)
		{
			wprintf(L"\r ");
			choose.position.Y += 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_INSERT))
		{
			switch (choose.position.Y)
			{
			case 8:
				with_password();
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				return;
			case 9:
				with_email();
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				return;
			}
		}
		Sleep(100);
	}

}

void sign_up()
{
	system("cls||clear");
	start = end = 0;
	start = clock();
	_setmode(_fileno(stdout), 0x20000);
	clearInputBuffer();
	setvbuf(stdin, NULL, _IONBF, 0);

	int result;

	char input[51];
	char check[51];

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║ \033[1;%dmin order to sign up, please enter:\033[1;%dm   ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║ \033[1;%dmyour name, password(twice) and email\033[1;%dm ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║         \033[1;%dmto cancel enter -1\033[1;%dm           ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");
	wprintf(L"Enter your name: ");
	data new_player;
	gets_s(input);

	if (!strcmp("-1", input))
	{
		main_menu();
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		return;
	}
	if (check_if_repeated(input))
	{
		wprintf(L"\n\033[1;%dmThis name is already taken!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		sign_up();
		return;
	}

	strcpy(new_player.username, input);

	wprintf(L"Enter your password(must contain at least 8 characters): ");
	result = mask(new_player.password);
	if (!strcmp("-1", input))
	{
		main_menu();
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		return;
	}
	if (result == 0)
	{
		wprintf(L"\033[1;%dm\nInvalid password!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		sign_up();
		return;
	}

	wprintf(L"\nPlease repeat your password: ");
	result = mask(check);
	if (result == -1)
	{
		main_menu();
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		return;
	}
	if (strcmp(new_player.password, check))
	{
		wprintf(L"\033[1;%dm\nWrong password!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		sign_up();
		return;
	}

	wprintf(L"\nEnter your email: ");
	gets_s(input);
	result = is_email(input);
	if (!strcmp("-1", input))
	{
		main_menu();
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		return;
	}
	if (!result)
	{
		wprintf(L"\033[1;%dmInvalid email!\033[0m", RED);
		Sleep(750);
		end = clock();
		seconds += (float)(end - start) / CLOCKS_PER_SEC;
		sign_up();
		return;
	}

	//can be modified
	strcpy(new_player.email, input);

	new_player.id = count_id;
	count_id++;

	add(new_player);

	system("cls||clear");

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t\    ╔══════════════════════════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t    ║     \033[1;%dm***SIGNED UP SUCCESSFULLY***\033[1;%dm     ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██  └──┘  ██\033[1;%dm\t    ║                                      ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t    ╚══════════════════════════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██ ██  ██ ██\n", GREEN);
	wprintf(L"\t ██ ██  ██ ██\n\n\033[0m");

	Sleep(2000);
	end = clock();
	seconds += (float)(end - start) / CLOCKS_PER_SEC;
	main_menu();


}

void main_menu()
{
	_setmode(_fileno(stdout), 0x20000);
	setlocale(LC_CTYPE, "");
	system("cls||clear");
	setvbuf(stdin, NULL, _IONBF, 0);
	clearInputBuffer();
	char input[50];
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	start = end = 0;
	start = clock();
	if (is_playing == 0)
		play_menu();

	object choose = { {0, 8},  0x25BA };

	system("cls||clear");

	wprintf(L"\t \033[1;%dm████████████\033[1;%dm\t╔══════════════════╗\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██\033[1;%dm\t║     \033[1;%dmWELCOME!\033[1;%dm     ║\n", GREEN, YELLOW, MAGENTA, YELLOW);
	wprintf(L"\t \033[1;%dm██ █    █ ██\033[1;%dm\t╚══════════════════╝\n", GREEN, YELLOW);
	wprintf(L"\t \033[1;%dm██        ██  ██\n", GREEN);
	wprintf(L"\t ████████████ ███\n");
	wprintf(L"\t ██ ██  ██  ███\n");
	wprintf(L"\t ██ ██  ██\033[0m\n");
	wprintf(L"\n\tSIGN UP\n");
	wprintf(L"\tSIGN IN\n");
	wprintf(L"\tEXIT");
	SetConsoleCursorPosition(hStdout, choose.position);
	wprintf(L"%lc", choose.output);

	while (1)
	{
		if (seconds >= 120)
		{
			play_menu();
			seconds = 0;
		}
		if (is_pressed(VK_UP) && (choose.position.Y > 8))
		{
			wprintf(L"\r ");
			choose.position.Y -= 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_DOWN) && (choose.position.Y < 10))
		{
			wprintf(L"\r ");
			choose.position.Y += 1;
			SetConsoleCursorPosition(hStdout, choose.position);
			wprintf(L"%lc", choose.output);
		}
		if (is_pressed(VK_INSERT))
		{

			switch (choose.position.Y)
			{
			case 8:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				sign_up();
				return;
			case 9:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				sign_in();
				return;
			case 10:
				system("cls||clear");
				end = clock();
				seconds += (float)(end - start) / CLOCKS_PER_SEC;
				exit(1);
				break;
			}
		}
		Sleep(100);
	}


}

void title_screen()
{
	PlaySound(TEXT("title.wav"), NULL, SND_FILENAME | SND_ASYNC);
	_setmode(_fileno(stdout), 0x20000);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	wprintf(L"\n\n\t    \033[1;%dm████████                                             \n", RED);
	wprintf(L"\t    ██                                                   \n");
	wprintf(L"\t    ████████                                             \n");
	wprintf(L"\t          ██                                             \n");
	wprintf(L"\t    ████████\033[0m                                             \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\t           \033[1;%dm████████                                   \n", GREEN);
	wprintf(L"\t           ██    ██                                      \n");
	wprintf(L"\t           ████████                                      \n");
	wprintf(L"\t           ██                                            \n");
	wprintf(L"\t           ██\033[0m                                            \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\t                        \033[1;%dm████                             \n", YELLOW);
	wprintf(L"\t                       ██  ██                            \n");
	wprintf(L"\t                      ████████                           \n");
	wprintf(L"\t                      ██    ██                           \n");
	wprintf(L"\t                      ██    ██\033[0m                           \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\t                                     \033[1;%dm█████████           \n", BLUE);
	wprintf(L"\t                                     ███                 \n");
	wprintf(L"\t                                     ███                 \n");
	wprintf(L"\t                                     ███                 \n");
	wprintf(L"\t                                     █████████\033[0m           \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\t                                                 \033[1;%dm████████    \n", MAGENTA);
	wprintf(L"\t                                                 ███     \n");
	wprintf(L"\t                                                 ████████\n");
	wprintf(L"\t                                                 ███     \n");
	wprintf(L"\t                                                 ████████\033[0m\n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t          \033[1;%dm██    ██    ██                                 \n", CYAN);
	wprintf(L"\t          ██    ██    ██                                 \n");
	wprintf(L"\t           ██  ████  ██                                  \n");
	wprintf(L"\t            ████  ████                                   \n");
	wprintf(L"\t             ██    ██\033[0m                                    \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                             \033[1;%dm████                        \n", WHITE);
	wprintf(L"\t                            ██  ██                       \n");
	wprintf(L"\t                           ████████                      \n");
	wprintf(L"\t                           ██    ██                      \n");
	wprintf(L"\t                           ██    ██\033[0m                      \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                      \033[1;%dm████████           \n", BLACK);
	wprintf(L"\t                                      ██    ██           \n");
	wprintf(L"\t                                      ████████           \n");
	wprintf(L"\t                                      ██  ███            \n");
	wprintf(L"\t                                      ██   ███\033[0m           \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                                                      \033[1;%dm████████████ \n", GREEN);
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ██ █    █ ██ \n");
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ████████████ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                                                      ████████████  \n");
	wprintf(L"\t                                                                      ██        ██  \n");
	wprintf(L"\t                                                                      ██ █    █ ██  \n");
	wprintf(L"\t                                                                      ████████████  \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██  \n");
	wprintf(L"\t                                                                     ██ ██    ██ ██ \n");
	wprintf(L"\t                                                                    ██  ██    ██  ██\n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                                                      ████████████ \n");
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ██ █    █ ██ \n");
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ████████████ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                                                      ████████████  \n");
	wprintf(L"\t                                                                      ██        ██  \n");
	wprintf(L"\t                                                                      ██ █    █ ██  \n");
	wprintf(L"\t                                                                      ████████████  \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██  \n");
	wprintf(L"\t                                                                     ██ ██    ██ ██ \n");
	wprintf(L"\t                                                                    ██  ██    ██  ██\n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                                                      \033[1;%dm████████████ \n", GREEN);
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ██ █    █ ██ \n");
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ████████████ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                                                      ████████████  \n");
	wprintf(L"\t                                                                      ██        ██  \n");
	wprintf(L"\t                                                                      ██ █    █ ██  \n");
	wprintf(L"\t                                                                      ████████████  \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██  \n");
	wprintf(L"\t                                                                     ██ ██    ██ ██ \n");
	wprintf(L"\t                                                                    ██  ██    ██  ██\n");

	Sleep(350);
	system("cls||clear");

	wprintf(L"\n\n\n\n\n\n\n\n\n\t                                                                      \033[1;%dm████████████ \n", GREEN);
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ██ █    █ ██ \n");
	wprintf(L"\t                                                                      ██        ██ \n");
	wprintf(L"\t                                                                      ████████████ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");
	wprintf(L"\t                                                                      ██ ██  ██ ██ \n");

	Sleep(500);
	system("cls||clear");

	wprintf(L"\033[1;%dm*   *   *   *   *   *   *   *   *   *   *     *   *   *   *   *   *   *   \033[0m\n", WHITE);
	wprintf(L"\033[1;%dm  *   *\033[1;%dm╔═══════════════════════════════════════════════════════╗ \033[1;%dm  *   *\n", WHITE, YELLOW, WHITE);
	wprintf(L"\033[1;%dm*   *  \033[1;%dm║ \033[1;%dm████████   ████████     ████     █████████   ████████ \033[1;%dm║ \033[1;%dm*   *  \n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm  *   *\033[1;%dm║ \033[1;%dm██         ██    ██    ██  ██    ███         ███      \033[1;%dm║ \033[1;%dm  *   *\n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm*   *  \033[1;%dm║ \033[1;%dm████████   ████████   ████████   ███         ████████ \033[1;%dm║ \033[1;%dm*   *  \n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm  *   *\033[1;%dm║       \033[1;%dm██   ██         ██    ██   ███         ███      \033[1;%dm║ \033[1;%dm  *   *\n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm*   *  \033[1;%dm║ \033[1;%dm████████   ██         ██    ██   █████████   ████████ \033[1;%dm║ \033[1;%dm*   *  \n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm  *   *\033[1;%dm║                                                       ║ \033[1;%dm  *   *\n", WHITE, YELLOW, WHITE);
	wprintf(L"\033[1;%dm*   *  \033[1;%dm║       \033[1;%dm██    ██    ██     ████     ████████            \033[1;%dm║ \033[1;%dm*   *  \n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm  *   *\033[1;%dm║       \033[1;%dm██    ██    ██    ██  ██    ██    ██            \033[1;%dm║ \033[1;%dm  *   *\n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm*   *  \033[1;%dm║        \033[1;%dm██  ████  ██    ████████   ████████            \033[1;%dm║ \033[1;%dm*   *  \n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm  *   *\033[1;%dm║         \033[1;%dm████  ████     ██    ██   ██  ███             \033[1;%dm║ \033[1;%dm  *   *\n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm*   *  \033[1;%dm║          \033[1;%dm██    ██      ██    ██   ██   ███            \033[1;%dm║ \033[1;%dm*   *  \n", WHITE, YELLOW, MAGENTA, YELLOW, WHITE);
	wprintf(L"\033[1;%dm  *   *\033[1;%dm╚═══════════════════════════════════════════════════════╝ \033[1;%dm  *   *\n", WHITE, YELLOW, WHITE);
	wprintf(L"\033[1;%dm*   *   *   *   *   *   *   *   *   *   *     *   *   *   *   *   *   *   \033[0m\n", WHITE);

	Sleep(500);

	wprintf(L"\n\t\t\t\tPress Enter\n");

	while (1)
	{
		if (is_pressed(VK_RETURN))
		{
			system("cls||clear");
			main_menu();
			return;
		}
	}

}

int main()
{
	update_id();
	title_screen();
}
