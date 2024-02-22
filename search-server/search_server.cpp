#include "search_server.h"

int MAX_RESULT_DOCUMENT_COUNT = 5;
double EPSILON = 1e-6;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document ID < 0");
    }
    else if (documents_.count(document_id) != 0) {
        throw std::invalid_argument("Document ID presents in documents.");
    }
    else {
        ids_.push_back(document_id);
        const std::vector<std::string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{ ComputeAverageRating(ratings), status });
    }
}

int SearchServer::GetDocumentCount() const { 
    return SearchServer::documents_.size(); 
}

int SearchServer::GetDocumentId(int index) const {
    return ids_.at(index);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
    const std::string& raw_query, int document_id) const {
    std::tuple<std::vector<std::string>, DocumentStatus> result;
    GetDocumentId(document_id);
    Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    result = { matched_words, documents_.at(document_id).status };

    return result;
}


bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Special character in document words: " + word);
        }
        else {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }

    }
    return words;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ' && c != '-';
        });
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

// В эту функцию приходит любое слово (минус или плюс)
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    if (text.empty()) {
        throw std::invalid_argument("Word is empty.");
    }
    QueryWord query_word;
    bool is_minus = false;
    if (text[0] == '-') { 
        is_minus = true;
        text = text.substr(1);
        if (text[0] == '-' || text.empty() || !IsValidWord(text)) {
            throw std::invalid_argument("Bad minus word: " + text);
        }
        else {
            query_word = { text, is_minus, IsStopWord(text) };
        }
    }
    else {
        if (!IsValidWord(text)) {
            throw std::invalid_argument("Bad plus word: " + text);
        }
        else {
            query_word = { text, is_minus, IsStopWord(text) };
        }
    }
    return query_word;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }

    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 /
        word_to_document_freqs_.at(word).size());
}
