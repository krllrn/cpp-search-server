#pragma once
#include "document.h"
#include "string_processing.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <numeric>

using namespace std;

extern int MAX_RESULT_DOCUMENT_COUNT;
extern double EPSILON;
#define INACCURACY(l_relevance, r_relevance) std::abs(l_relevance - r_relevance) < EPSILON

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& stop_word : stop_words_) {
            if (!IsValidWord(stop_word)) {
                throw std::invalid_argument("Special character in stop words.");
            }
        }
    }

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
        std::vector<Document> result;
        Query query = ParseQuery(raw_query);
        result = FindAllDocuments(query, document_predicate);
        sort(result.begin(), result.end(),
            [](const Document& lhs, const Document& rhs) {
                if (INACCURACY(lhs.relevance, rhs.relevance)) {
                    return lhs.rating > rhs.rating;
                }
                return lhs.relevance > rhs.relevance;
            });
        if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
            result.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return result;
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status,
                int rating) { return document_status == status; });
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> ids_;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static bool IsValidWord(const std::string& word);

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string text) const;

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const Query& query, DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] :
                word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status,
                    document_data.rating)) {
                    document_to_relevance[document_id] +=
                        term_freq * inverse_document_freq;
                }
            }
        }

        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};