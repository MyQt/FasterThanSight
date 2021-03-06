#include "ctextfragment.h"

#include <cmath>
#include <algorithm>

// The declaration is fully specialized (does not depend on template parameter), so it may not go into a header file
// http://stackoverflow.com/a/4445772/634821

template<>
const std::vector<TextFragment::Delimiter::EnumItem> TextFragment::Delimiter::_items = {
	{TextFragment::NoDelimiter, "No delimiter"},
	{TextFragment::Space, "Space"},
	{TextFragment::Newline, "Newline"},
	{TextFragment::Dash, "Dash"},
	{TextFragment::Bracket, "Bracket"},
	{TextFragment::Quote, "Quote"},
	{TextFragment::Comma, "Comma"},
	{TextFragment::Colon, "Colon"},
	{TextFragment::Semicolon, "Semicolon"},
	{TextFragment::Point, "Point"},
	{TextFragment::Ellipsis, "Ellipsis"},
	{TextFragment::ExclamationMark, "Exclamation mark"},
	{TextFragment::QuestionMark, "Question mark"}
};

TextFragment::TextFragment(const QString& word, const QString& punctuation, Delimiter delimiter /*= NoDelimiter*/) : _word(word), _punctuationText(punctuation), _delimitier(delimiter)
{
}

#ifdef __ANDROID__
namespace std {
	// Fix for MinGW 4.9.2 bug - std::log2 is missing there
	template <typename T>
	T log2 (T value)
	{
		static const T l2 = std::log(T(2));
		return std::log(value) / l2;
	}
}
#endif

inline int pivot(TextFragment::PivotCalculationMethod method, int wordLength)
{
	switch (method)
	{
	case TextFragment::pcmMagic:
		switch (wordLength)
		{
		case 1:
			return 0; // First letter
		case 2:
		case 3:
		case 4:
		case 5:
			return 1; // Second letter
		case 6:
		case 7:
		case 8:
		case 9:
			return 2; // Third letter
		case 10:
		case 11:
		case 12:
		case 13:
			return 3; // Fourth letter
		case 0:
			return -1;
		default:
			return 4; // Fifth letter
		}
	case TextFragment::pcmMiddle:
		return std::max((int)round(wordLength / 2.0f) - 1, 0);
	case TextFragment::pcmQuarter:
		return std::max((int)round(wordLength / 4.0f) - 1, 0);
	case TextFragment::pcmSquareRoot:
		return std::max((int)round(std::sqrt(wordLength)) - 1, 0);
	case TextFragment::pcmCubicRoot:
		return std::max((int)round(std::pow(wordLength, 0.33333f)) - 1, 0);
	case TextFragment::pcmLogarithm:
		return std::max((int)round(std::log2(wordLength)) - 1, 0);
	default:
		return std::max(wordLength / 2 - 1, 0);
	}
}

int TextFragment::pivotLetterIndex(PivotCalculationMethod method) const
{
	// Ignoring quotes and other possible non-alphanumeric characters in the beginning
	int wordStartIndex = 0, wordLength = 0;
	bool wordStarted = false;
	for (QChar ch : _word)
	{
		if (ch.isLetterOrNumber())
		{
			wordStarted = true;
			++wordLength;
		}
		else if (!wordStarted)
			++wordStartIndex;
	}

	const int pivotIndex = wordStartIndex + pivot(method, wordLength);

	if (pivotIndex == -1)
		return -1;
	else if (wordLength > 1 && !_word[pivotIndex].isLetterOrNumber())
	{
		if (pivotIndex > wordLength/2)
			return pivotIndex - 1;
		else if (pivotIndex < wordLength/2)
			return pivotIndex + 1;
		else
			return pivotIndex - 1;
	}
	else
		return pivotIndex;
}

QString TextFragment::text() const
{
	return _word + _punctuationText;
}

QString TextFragment::word() const
{
	return _word;
}

QString TextFragment::punctuation() const
{
	return _punctuationText;
}

TextFragment::Delimiter TextFragment::delimiter() const
{
	return _delimitier;
}

bool TextFragment::isEndOfSentence() const
{
	return _delimitier == Point || _delimitier == QuestionMark || _delimitier == ExclamationMark || _delimitier == Newline || _delimitier == Ellipsis;
}

bool TextFragment::isEmpty() const
{
	return _word.isEmpty() && _punctuationText.isEmpty();
}
