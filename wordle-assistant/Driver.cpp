#include "FilterParser.h"

using namespace std;

int main() {

	printf("WELCOME TO WORDLE ASSISTANT CLI!!!\n\n");
	FilterParser fp;
	
	while (true) {
		fp.FilterAnswers();

		char continueInput[2];
		printf("\n\nContinue..? (y/n)");
		scanf_s("%2s", &continueInput, 2);
		if (continueInput[0] == 'n') {
			break;
		}
	}
}