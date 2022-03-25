#ifndef FILTERPARSER_H
#define FILTERPARSER_H

#include <fstream>
#include <map>
#include <set>

class FilterParser {

private:
	const int LINE_LEN = 6;

	char defaultChar = ' ';
	char greenHash[6];
	char yellowHash[131];
	char blackHash[27];
	char greenAndYellowChars[6];

	char validAnswers[180];
	char validGuesses[180];

	std::ifstream wordListFile;
	std::ofstream newWordList;

	void UpdateFileNames();

	bool ParseUserInput();

	void FilterWordLists(char* answersOrGuesses);

	int ReadFile(char* fileName, char*& buffer);

	void OpenFile(char* file);

	bool ContainsAllGreenAndYellowChars(const char* word);

	bool MatchesGreenHash(const char* word);

	bool ContainsNoBlackChars(const char* word);

	bool FitsYellowHash(const char* word);

	void AddToGreenAndYellowChars(const char rChar, bool duplicate);

public:

	FilterParser();

	~FilterParser();

	void FilterAnswers();

	void FilterGuesses();
};

#endif