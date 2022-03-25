#pragma warning(disable : 4996)
#include "FilterParser.h"

#include <cctype>
#include <cstring>
#include <regex>
#include <shlwapi.h>
#include <stdio.h>

using namespace std;

void PrintFormatError() {
	printf("\n***Error: input does not match expected format.***\n\n");
}

FilterParser::FilterParser() {
	memset(this->greenHash, this->defaultChar, 5);
	this->greenHash[5] = '\0';

	memset(this->yellowHash, this->defaultChar, 5 * 26);
	this->yellowHash[5 * 26] = '\0';

	memset(this->blackHash, this->defaultChar, 26);
	this->blackHash[26] = '\0';

	memset(this->greenAndYellowChars, this->defaultChar, 5);
	this->greenAndYellowChars[5] = '\0';

	memset(this->validAnswers, '\0', 180);
	memset(this->validGuesses, '\0', 180);
	this->UpdateFileNames();
}

FilterParser::~FilterParser() {}

void FilterParser::UpdateFileNames() {

	strcpy(this->validAnswers, "valid-answers");
	strcpy(this->validAnswers + 13, this->greenHash);
	strcpy(this->validAnswers + 13 + 5, this->yellowHash);
	strcpy(this->validAnswers + 13 + 5 + 130, this->blackHash);

	strcpy(this->validGuesses, "valid-guesses");
	strcpy(this->validGuesses + 13, this->greenHash);
	strcpy(this->validGuesses + 13 + 5, this->yellowHash);
	strcpy(this->validGuesses + 13 + 5 + 130, this->blackHash);
}

bool FilterParser::ParseUserInput() {

	char userInput[30];
	 
	char userWord[27];
	memset(userWord, this->defaultChar, 26);
	userWord[26] = '\0';

	printf("\nENTER WORDLE RESULT: ");
	scanf_s("%111s", userInput, 30);
	userInput[29] = '\0';

	if (!regex_match(userInput, regex("(([a-z]=[g|y|b])[,| ]?){1,5}"), regex_constants::match_any)) {
		PrintFormatError();
		return false;
	}

	int inputCharIndex = 0;
	int charCount = 0;
	while (inputCharIndex < (unsigned)_countof(userInput) && userInput[inputCharIndex] != NULL) {
		
		char inputChar = userInput[inputCharIndex];
		bool duplicateChar = inputChar == userWord[inputChar - 'a'];
		userWord[inputChar - 'a'] = inputChar;

		if (inputChar == ',' || inputChar == ' ') {
			inputCharIndex++;
			continue;
		}
		else if (!isalpha(inputChar)) {
			PrintFormatError();
			return false;
		}

		inputCharIndex++;
		if (userInput[inputCharIndex] == '=') {

			inputCharIndex++;
			char charColor = userInput[inputCharIndex];
			if (!isalpha(charColor)) {
				PrintFormatError();
				return false;
			}

			switch (charColor) {
			case 'g':
				this->greenHash[charCount] = inputChar;
				this->AddToGreenAndYellowChars(inputChar, duplicateChar);
				break;
			case 'y':
				this->yellowHash[(charCount * 26) + (inputChar - 'a')] = inputChar;
				this->AddToGreenAndYellowChars(inputChar, duplicateChar);
				break;
			case 'b':
				this->blackHash[inputChar - 'a'] = inputChar;
				break;
			default:
				PrintFormatError();
				return false;
			}

			inputCharIndex++;
		}
		else {
			PrintFormatError();
			return false;
		}

		charCount++;
	}
	return true;
}

void FilterParser::FilterAnswers() {
	this->FilterWordLists(this->validAnswers);
}

void FilterParser::FilterGuesses() {
	this->FilterWordLists(this->validGuesses);
}

void FilterParser::FilterWordLists(char* answersOrGuesses) {

	char fileName[183];
	char cachedFile[183];
	strcpy_s(fileName, answersOrGuesses);
	strcat_s(fileName, ".txt");
	char* rBuffer = NULL;
	char* wBuffer = NULL;

	if (!this->ParseUserInput()) {
		return;
	}
	this->UpdateFileNames();
	strcpy_s(cachedFile, answersOrGuesses);
	strcat_s(cachedFile, ".txt");

	printf("Looking for cached file: %s\n", cachedFile);
	wordListFile.open(cachedFile);
	if (wordListFile) {
		printf("Cached file found!");
		wordListFile.close();
		OpenFile(cachedFile);
		return;
	}
	printf("No cached file found.\n");

	try {
		int wordCount = ReadFile(fileName, rBuffer);
		if (rBuffer == NULL || wordCount == 0) {
			printf("***No words read***\n\n");
			return;
		}

		wBuffer = new char[wordCount * this->LINE_LEN];
		memset(wBuffer, '\0', wordCount * this->LINE_LEN);

		int filterCount = 0;
		for (int i = 0; i < wordCount; ++i) {
			char* currWord = rBuffer + (i * this->LINE_LEN);

			if (!this->ContainsAllGreenAndYellowChars(currWord) || !this->MatchesGreenHash(currWord) || !this->ContainsNoBlackChars(currWord) || !this->FitsYellowHash(currWord)) {
				continue;
			}

			int wIndex = filterCount * this->LINE_LEN;
			int rIndex = i * this->LINE_LEN;
			memcpy(wBuffer + wIndex, rBuffer + rIndex, LINE_LEN);
			filterCount++;
		}
		
		wBuffer[(wordCount * LINE_LEN) - 1] = '\0';
		printf("\nFiltered out %d words\n", wordCount - filterCount);
		printf("Filtered down to %d words\n", filterCount);

		newWordList.open(cachedFile);
		printf("Writing filtered words to %s\n", cachedFile);
		newWordList.write(wBuffer, strlen(wBuffer));
		newWordList.close();
		OpenFile(cachedFile);
	}
	catch (runtime_error& runtimeError) {
		printf("\n***Error: Runtime Error (FilterWordLists)***\n\n");
		if (newWordList.is_open()) {
			newWordList.close();
		}
	}

	if (rBuffer != NULL) {
		delete[] rBuffer;
		rBuffer = NULL;
	}
	if (wBuffer != NULL) {
		delete[] wBuffer;
		wBuffer = NULL;
	}
}

int FilterParser::ReadFile(char* fileName, char* &buffer) {
	try {
		printf("Opening file: %s\n", fileName);
		this->wordListFile.open(fileName);
		if (this->wordListFile.good()) {

			this->wordListFile.ignore((numeric_limits<streamsize>::max)());
			int length = 1 + (int) this->wordListFile.gcount();
			this->wordListFile.clear();
			this->wordListFile.seekg(0, this->wordListFile.beg);

			buffer = new char[length];
			int readLen = length - 1;
			buffer[readLen] = '\0';

			this->wordListFile.read(buffer, readLen);
			printf("Closing file: %s\n", fileName);
			this->wordListFile.close();

			int wordCount = length / this->LINE_LEN;

			printf("Read %d bytes, %d words\n", length, wordCount);

			return wordCount;
		}
		else {
			printf("Closing file: %s\n", fileName);
			this->wordListFile.close();
		}

	}
	catch (runtime_error& runtimeError) {
		printf("\n***Error: Runtime Error (ReadFile)***\n\n");
		printf("Closing file: %s\n", fileName);
		this->wordListFile.close();
	}

	/*if (buffer != NULL) {
		delete[] buffer;
		buffer = NULL;
	}*/

	printf("\n***Error: failed to read file***\n\n");
	return 0;
}

void FilterParser::OpenFile(char* file) {
	ShellExecuteA(NULL, "open", file, NULL, NULL, SW_SHOW);
}

bool FilterParser::ContainsAllGreenAndYellowChars(const char* word) {
	/// <summary>
	/// ContainsAllGreenAndYellowChars checks to see that all valid letters are included in the word. If all characters in the 
	/// greenAndYellowChars list are present in the passed word, the function returns true. Otherwise, it returns false. The function ignores 
	/// all default characters present in the list and only compares valid charaters to those in the word. The function expects the list to be
	/// sequential in that all characters are added sequentially in the first open spot. No characters should ever be removed, moved, or
	/// replaced. The function expects the passed paramter to be a 5 letter word and will only compare the first five letters. If there are
	/// more letters, they will be ignored. If there are less, the application will explode.
	/// </summary>
	/// <param name="word"> Char pointer to a five letter word. </param>
	/// <returns> The function returns a boolean signalling if the passed word matches the hash. </returns>
	char wordCopy[6];
	memcpy(wordCopy, word, 5);
	wordCopy[5] = '\0';

	for (int i = 0; i < 5; ++i) {
		if (this->greenAndYellowChars[i] == this->defaultChar) {
			break;
		}
		bool charFound = false;
		for (int j = 0; j < 5; ++j) {
			if (this->greenAndYellowChars[i] == wordCopy[j]) {
				wordCopy[j] = defaultChar;
				charFound = true;
				break;
			}
		}

		if (!charFound) {
			return false;
		}
	}

	return true;
}

bool FilterParser::MatchesGreenHash(const char* word) {
	/// <summary>
	/// 
	/// </summary>
	/// <param name="word"></param>
	/// <returns></returns>
	for (int i = 0; i < 5; ++i) {
		if (this->greenHash[i] == this->defaultChar) {
			continue;
		}
		else if (this->greenHash[i] != word[i]) {
			return false;
		}
	}
	return true;
}

bool FilterParser::ContainsNoBlackChars(const char* word) {
	/// <summary>
	/// 
	/// </summary>
	/// <param name="word"></param>
	/// <returns></returns>
	for (int i = 0; i < 5; ++i) {
		char rChar = word[i];
		if (this->blackHash[rChar - 'a'] != defaultChar) {
			return false;
		}
	}
	return true;
}

bool FilterParser::FitsYellowHash(const char* word) {
	/// <summary>
	/// 
	/// </summary>
	/// <param name="word"></param>
	/// <returns></returns>
	for (int i = 0; i < 5; ++i) {
		char rChar = word[i];
		if (this->yellowHash[(i * 26) + rChar - 'a'] != defaultChar) {
			return false;
		}
	}
	return true;
}

void FilterParser::AddToGreenAndYellowChars(const char rChar, bool duplicate) {
	for (int i = 0; i < 5; ++i) {
		if (this->greenAndYellowChars[i] != this->defaultChar) {
			if (this->greenAndYellowChars[i] == rChar && !duplicate) {
				break;
			}

			continue;
		}
		else {
			this->greenAndYellowChars[i] = rChar;
			break;
		}
	}
}