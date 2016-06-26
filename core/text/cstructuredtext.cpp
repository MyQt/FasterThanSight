#include "cstructuredtext.h"
#include "assert/advanced_assert.h"
#include "container/algorithms.h"

#include <algorithm>

CStructuredText::CStructuredText()
{

}

void CStructuredText::addChapter(const QString& name, const std::vector<Paragraph>& paragraphs)
{
	_chapters.push_back({name, paragraphs});
}

void CStructuredText::addChapter(const Chapter& chapter)
{
	_chapters.push_back(chapter);
}

Chapter& CStructuredText::addEmptyChapter(const QString& name)
{
	_chapters.emplace_back();
	_chapters.back().name = name;
	return _chapters.back();
}

void CStructuredText::clear()
{
	_chapters.clear();
}

bool CStructuredText::empty() const
{
	return totalFragmentsCount() == 0;
}

Chapter& CStructuredText::lastChapter()
{
	return _chapters.back();
}

Paragraph& CStructuredText::lastParagraph()
{
	return lastChapter()._paragraphs.back();
}

size_t CStructuredText::previousChapterStartIndex(size_t currentIndex) const
{
	auto chapterIterator = std::lower_bound(_chapters.cbegin(), _chapters.cend(), currentIndex, [](const Chapter& chapter, size_t index){
		return chapter.lastFragmentNumber() < index;
	});
	assert_and_return_r(chapterIterator != _chapters.cend(), currentIndex);

	if (chapterIterator != _chapters.cbegin())
		--chapterIterator;

	return chapterIterator->firstFragmentNumber();
}

size_t CStructuredText::previousParagraphStartIndex(size_t currentIndex) const
{
	const auto chapterIterator = std::lower_bound(_chapters.cbegin(), _chapters.cend(), currentIndex, [](const Chapter& chapter, size_t index){
		return chapter.lastFragmentNumber() < index;
	});
	assert_and_return_r(chapterIterator != _chapters.cend(), currentIndex);

	auto paragraphIterator = std::lower_bound(chapterIterator->_paragraphs.cbegin(), chapterIterator->_paragraphs.cend(), currentIndex, [](const Paragraph& paragraph, size_t index){
		return paragraph.lastFragmentNumber() < index;
	});
	assert_and_return_r(paragraphIterator != chapterIterator->_paragraphs.cend(), currentIndex);

	if (paragraphIterator != chapterIterator->_paragraphs.cbegin())
		--paragraphIterator;

	return paragraphIterator->firstFragmentNumber();
}

size_t CStructuredText::nextParagraphStartIndex(size_t currentIndex) const
{
	const auto chapterIterator = std::lower_bound(_chapters.cbegin(), _chapters.cend(), currentIndex, [](const Chapter& chapter, size_t index){
		return chapter.lastFragmentNumber() < index;
	});
	assert_and_return_r(chapterIterator != _chapters.cend(), currentIndex);

	auto paragraphIterator = std::lower_bound(chapterIterator->_paragraphs.cbegin(), chapterIterator->_paragraphs.cend(), currentIndex, [](const Paragraph& paragraph, size_t index){
		return paragraph.lastFragmentNumber() < index;
	});
	assert_and_return_r(paragraphIterator != chapterIterator->_paragraphs.cend(), currentIndex);

	++paragraphIterator;

	return paragraphIterator != chapterIterator->_paragraphs.cend() ? paragraphIterator->firstFragmentNumber() : currentIndex;
}

size_t CStructuredText::nextChapterStartIndex(size_t currentIndex) const
{
	auto chapterIterator = std::lower_bound(_chapters.cbegin(), _chapters.cend(), currentIndex, [](const Chapter& chapter, size_t index){
		return chapter.lastFragmentNumber() < index;
	});
	assert_and_return_r(chapterIterator != _chapters.cend(), currentIndex);

	++chapterIterator;

	return chapterIterator != _chapters.cend() ? chapterIterator->firstFragmentNumber() : currentIndex;
}

const std::vector<Chapter>& CStructuredText::chapters() const
{
	return _chapters;
}

size_t CStructuredText::chaptersCount() const
{
	return _chapters.size();
}

size_t CStructuredText::totalFragmentsCount() const
{
	for (auto chapter = _chapters.rbegin(), endChapter = _chapters.rend(); chapter != endChapter; ++chapter)
		for (auto paragraph = chapter->_paragraphs.rbegin(), endParagraph = chapter->_paragraphs.rend(); paragraph != endParagraph; ++paragraph)
			if (!paragraph->_fragments.empty())
				return paragraph->lastFragmentNumber() + 1;

	return 0;
}

const TextFragment& CStructuredText::fragment(size_t fragmentIndex) const
{
	static const TextFragment dummy;

	const auto chapterIterator = std::lower_bound(_chapters.cbegin(), _chapters.cend(), fragmentIndex, [](const Chapter& chapter, size_t index){
		return chapter.lastFragmentNumber() < index;
	});
	assert_and_return_r(chapterIterator != _chapters.cend(), dummy);

	const auto paragraphIterator = std::lower_bound(chapterIterator->_paragraphs.cbegin(), chapterIterator->_paragraphs.cend(), fragmentIndex, [](const Paragraph& paragraph, size_t index){
		return paragraph.lastFragmentNumber() < index;
	});
	assert_and_return_r(paragraphIterator != chapterIterator->_paragraphs.cend(), dummy);

	const auto fragmentIterator = std::lower_bound(paragraphIterator->_fragments.cbegin(), paragraphIterator->_fragments.cend(), fragmentIndex, [](const IndexedFragment& fragment, size_t index) {
		return fragment.fragmentIndex < index;
	});
	assert_and_return_r(fragmentIterator != paragraphIterator->_fragments.cend(), dummy);

	return fragmentIterator->fragment;
}

void CStructuredText::removeEmptyItems()
{
	for (auto& chapter: _chapters)
	{
		ContainerAlgorithms::erase_if(chapter._paragraphs, [](const Paragraph& p){
			return p._fragments.empty();
		});
	}

	ContainerAlgorithms::erase_if(_chapters, [](const Chapter& chapter){
		return chapter._paragraphs.empty();
	});
}

CStructuredText::const_iterator CStructuredText::begin() const
{
	const_iterator it;
	it._chapterIterator = forward_iterator_wrapper::cbegin(_chapters);

	if (it._chapterIterator != _chapters.cend())
	{
		it._paragraphIterator = forward_iterator_wrapper::cbegin(it._chapterIterator->_paragraphs);

		if (it._paragraphIterator != it._chapterIterator->_paragraphs.cend())
			it._fragmentIterator = forward_iterator_wrapper::cbegin(it._paragraphIterator->_fragments);
	}

	return it;
}

CStructuredText::const_iterator CStructuredText::end() const
{
	const_iterator it;
	it._chapterIterator = forward_iterator_wrapper::cend(_chapters);

	return it;
}

CStructuredText::const_iterator& CStructuredText::const_iterator::operator++()
{
	++_fragmentIterator;
	if (_fragmentIterator.endReached())
	{
		++_paragraphIterator;
		if (_paragraphIterator.endReached())
		{
			++_chapterIterator; // If _chapterIterator.endReached() is true, that will ensure it compares true to CStructuredText::end() (see const_iterator::operator==)
			if (!_chapterIterator.endReached())
			{
				_paragraphIterator = forward_iterator_wrapper::cbegin(_chapterIterator->_paragraphs);
				_fragmentIterator = forward_iterator_wrapper::cbegin(_paragraphIterator->_fragments);
			}
		}
		else
			_fragmentIterator = forward_iterator_wrapper::cbegin(_paragraphIterator->_fragments);
	}

	return *this;
}

const IndexedFragment& CStructuredText::const_iterator::operator*() const
{
	return *_fragmentIterator;
}

IndexedFragment const* CStructuredText::const_iterator::operator->() const
{
	return &(*_fragmentIterator);
}

bool CStructuredText::const_iterator::operator==(const const_iterator& other) const
{
	return
		(_chapterIterator.endReached() && _chapterIterator == other._chapterIterator) || // If _chapterIterator.endReached() is true, that will ensure it compares true to CStructuredText::end()
		(_chapterIterator == other._chapterIterator && _paragraphIterator == other._paragraphIterator && _fragmentIterator == other._fragmentIterator);
}

bool CStructuredText::const_iterator::operator!=(const const_iterator& other) const
{
	return !(*this == other);
}
