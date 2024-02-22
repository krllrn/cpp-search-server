#pragma once
#include <iterator>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : begin_(begin),
        end_(end)
    {}

    auto begin() const {
        return begin_;
    }

    auto end() const {
        return end_;
    }

    auto size() const {
        return distance(begin_, end_);
    }

private:
    Iterator begin_;
    Iterator end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, int p_size)
    {
        while (begin < end - p_size) {
            pages_.push_back(IteratorRange(begin, begin + p_size));
            advance(begin, p_size);
        }
        pages_.push_back(IteratorRange(begin, end));
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    auto size() const {
        return distance(pages_.begin(), pages_.end());
    }

    std::vector<IteratorRange<Iterator>> GetPages() {
        return pages_;
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, IteratorRange<Iterator> iterator_range) {
    for (auto it = iterator_range.begin(); it != iterator_range.end(); ++it) {
        std::cout << *it;
    }
    return output;
}