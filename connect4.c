#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SIZE_X 9
#define SIZE_Y 9

//diplays the entire grid
void displayGrid(char grid[SIZE_X][SIZE_Y]);

//ask user for which X to put chip in, or set userX to -1 at end of input
void askUserX(int *userX, char chip);

//fills grid with '-'
void clearGrid(char grid[SIZE_X][SIZE_Y]);

//modifies directs to contain the empty cells surrounding x,y
void checkChip(char grid[SIZE_X][SIZE_Y], int directs[8], int x, int y, char chip);

//modifies x and y based on the direction given
//1-north, 2-northeast, 3-east, 4-southeast, 5-south, 6-southwest, 7-west, 8-northwest
void moveCoord(int *x, int *y, int direction);

//places chip on grid on the given X axis and returns the Y
int placeChip(char grid[SIZE_X][SIZE_Y], int userX, char chip);

//returns 1 to continue the game
//returns 0 to end the game
int checkWin(char grid[SIZE_X][SIZE_Y], int userX, int lastY, char *chip);

//switch chipNum for the other side
void switchChip(char *chip);

int main()
{
	char grid[SIZE_X][SIZE_Y], chip = 'Y';
	int userX = 0, lastY = 0, moves = 0;
	
	printf("Connect four chips and win!\n");
	printf("Y are yellow chips and R are red chips.\n");
	printf("ONLY ENTER IN NUMBERS!!!\n\n");

	clearGrid(grid); //fill grid with '-'
	displayGrid(grid); //grid should be empty

	do {
		do {
			askUserX(&userX, chip);
			if (userX == -1)
				return 0;
			lastY = placeChip(grid, userX, chip);
			displayGrid(grid);
			if (lastY == -1)
				printf("The col is full!\n");
		} while (lastY == -1);
		moves++;
		if (!checkWin(grid, userX, lastY, &chip))
			break;
		if (moves == SIZE_X * SIZE_Y) {
			printf("Draw!\n");
			return 0;
		}
	} while (1);

	printf("%c wins\n", chip);

	return 0;
}

void displayGrid(char grid[SIZE_X][SIZE_Y])
{
	for (int y = 0; y < SIZE_Y; y++){ //loops through entire grid
		for (int x = 0; x < SIZE_X; x++)
			printf("%c", grid[x][y]);
		printf("\n");
	}

	for (int i = 0; i < SIZE_X; i++)
		if (!(i % 2))
			printf("%d", i + 1);
		else
			printf(" ");
	printf("\n");
}

void askUserX(int *userX, char chip)
{
	long tempX = 0;
	char input[64], *end;
	int next;
	do {
		printf("%c turn, enter row: ", chip);
		if (scanf("%63s", input) != 1) {
			*userX = -1;
			return;
		}
		tempX = strtol(input, &end, 10);
		if (*end)
			tempX = 0;
		next = getchar();
		if (next != EOF && !isspace(next)) { //discard the rest of a long input
			tempX = 0;
			do {
				next = getchar();
			} while (next != EOF && !isspace(next));
		}
		printf("\n");
	} while (tempX <= 0 || tempX > SIZE_X); //makes sure the input is within the bounds of the grid
	*userX = tempX - 1; //converts human understanding of cols to computer understanding
						//start from one vs start from zero 
}

void clearGrid(char grid[SIZE_X][SIZE_Y])
{
	for (int y = 0; y < SIZE_Y; y++)
		for (int x = 0; x < SIZE_X; x++)
			grid[x][y] = '-';
}

void checkChip(char grid[SIZE_X][SIZE_Y], int directs[8], int x, int y, char chip)
{
	int i = 0;

	if (y > 0 && grid[x][y - 1] == chip) {
		directs[i] = 1; //north
		i++;
	}
	if (x + 1 < SIZE_X && y > 0 && grid[x + 1][y - 1] == chip) {
		directs[i] = 2; //northeast
		i++;
	}
	if (x + 1 < SIZE_X && grid[x + 1][y] == chip) {
		directs[i] = 3; //east
		i++;
	}
	if (x + 1 < SIZE_X && y + 1 < SIZE_Y && grid[x + 1][y + 1] == chip) {
		directs[i] = 4; //southeast
		i++;
	}
	if (y + 1 < SIZE_Y && grid[x][y + 1] == chip) {
		directs[i] = 5; //south
		i++;
	}
	if (x > 0 && y + 1 < SIZE_Y && grid[x - 1][y + 1] == chip) {
		directs[i] = 6; //southwest
		i++;
	}
	if (x > 0 && grid[x - 1][y] == chip) {
		directs[i] = 7; //west
		i++;
	}
	if (x > 0 && y > 0 && grid[x - 1][y - 1] == chip) {
		directs[i] = 8; //northwest
		i++;
	}
}

void moveCoord(int *x, int *y, int direction)
{
	switch (direction)
	{
	case 1: //north
		*y -= 1;
		break;

	case 2: //northeast
		*x += 1;
		*y -= 1;
		break;

	case 3: //east
		*x += 1;
		break;

	case 4: //southeast
		*x += 1;
		*y += 1;
		break;

	case 5: //south
		*y += 1;
		break;

	case 6: //southwest
		*x -= 1;
		*y += 1;
		break;

	case 7: //west
		*x -= 1;
		break;

	case 8: //northwest
		*x -= 1;
		*y -= 1;
		break;
	default:
		printf("moveCoords oops!");
		break;
	}
}

int placeChip(char grid[SIZE_X][SIZE_Y], int userX, char chip)
{
	for (int y = SIZE_Y - 1; y >= 0; y--) //begins at bottom of grid
	{
		if (grid[userX][y] == '-') {//place chip on empty space
			grid[userX][y] = chip;
			return y; //return y coord of chip
		}
	}
	return -1; //place chip fails
}

//returns 0 if theres four chipNum in a row
//returns 1 if anything else
int checkWin(char grid[SIZE_X][SIZE_Y], int userX, int lastY, char *chip)
{
	int chipNum = 0, x = userX, y = lastY;
	int directions[8] = { 0 };

	checkChip(grid, directions, userX, lastY, *chip); //modifies directions to give clear directions

	for (int i = 0; i < 8 && directions[i]; i++) { //stop if direction has zero
		while (x >= 0 && x < SIZE_X && y >= 0 && y < SIZE_Y && grid[x][y] == *chip) { //checks in a straight line with the directions
			moveCoord(&x, &y, directions[i]); //move in direction of found chip
			chipNum++;
			if (chipNum >= 4) {
				return 0; //row of 4 found
			}
		}
		x = userX;
		y = lastY;
		moveCoord(&x, &y, (directions[i] + 3) % 8 + 1); //check the opposite side too
		while (x >= 0 && x < SIZE_X && y >= 0 && y < SIZE_Y && grid[x][y] == *chip) {
			chipNum++;
			if (chipNum >= 4)
				return 0;
			moveCoord(&x, &y, (directions[i] + 3) % 8 + 1);
		}
		x = userX; //return to orginial 
		y = lastY;
		chipNum = 0;
	}

	switchChip(chip);
	return 1; //fails to find a row of 4
}

//swtich chipNum
void switchChip(char *chip)
{
	if (*chip == 'Y') {
		*chip = 'R';
	} else if (*chip == 'R') {
		*chip = 'Y';
	}
}
