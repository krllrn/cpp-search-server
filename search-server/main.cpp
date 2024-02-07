#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
inline static constexpr int INVALID_DOCUMENT_ID = -1;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id), relevance(relevance), rating(rating) {}

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& stop_word : stop_words_) {
            if (!IsValidWord(stop_word)) {
                throw invalid_argument("Special character in stop words."s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
        for (const auto& stop_word : stop_words_) {
            if (!IsValidWord(stop_word)) {
                throw invalid_argument("Special character in stop words."s);
            }
        }
    }

    void AddDocument(int document_id, const string& document,
        DocumentStatus status,
        const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id) != 0) {
            throw invalid_argument("Document ID < 0 or presents in documents."s);
        }
        else {
            ids_.push_back(document_id);
            const vector<string> words = SplitIntoWordsNoStop(document);
            for (const string& word : words) {
                if (!IsValidWord(word)) {
                    throw invalid_argument("Special character in document words."s);
                }
            }
            const double inv_word_count = 1.0 / words.size();
            for (const string& word : words) {
                word_to_document_freqs_[word][document_id] += inv_word_count;
            }
            documents_.emplace(document_id,
                DocumentData{ ComputeAverageRating(ratings), status });
        }
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        vector<Document> result;
        try {
            Query query = ParseQuery(raw_query);
            result = FindAllDocuments(query, document_predicate);
            sort(result.begin(), result.end(),
                [](const Document& lhs, const Document& rhs) {
                    if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                        return lhs.rating > rhs.rating;
                    }
                    else {
                        return lhs.relevance > rhs.relevance;
                    }
                });
            if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
                result.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
        }
        catch (const invalid_argument& ex) {
            throw ex;
        }

        return result;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status,
                int rating) { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const { return documents_.size(); }

    int GetDocumentId(int index) const {
        if (index < 0 || index > ids_.size()) {
            throw out_of_range("Index is out of range."s);
        }
        else {
            return ids_[index];
        }
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(
        const string& raw_query, int document_id) const {
        tuple<vector<string>, DocumentStatus> result;
        try {
            GetDocumentId(document_id);
            Query query = ParseQuery(raw_query);
            vector<string> matched_words;
            for (const string& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }
            for (const string& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.clear();
                    break;
                }
            }
            result = { matched_words, documents_.at(document_id).status };
        }
        catch (const out_of_range& ex) {
            throw ex;
        }
        catch (const invalid_argument& ex) {
            throw ex;
        }

        return result;
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> ids_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        bool is_valid = true;
        for (size_t j = 0; j < word.size(); ++j) {
            for (size_t i = 0; i < 31; ++i) {
                int char_num = int(word[j]);
                if (char_num == i || (char_num == 13 && !isalpha(word[j + 1]))) {
                    is_valid = false;
                }
            }
        }
        return is_valid;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        QueryWord query_word;
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
            if (text[0] == '-' || text.empty() || !IsValidWord(text)) {
                throw invalid_argument("Bad minus word."s);
            }
            else {
                query_word = { text, is_minus, IsStopWord(text) };
            }
        }
        else {
            if (text[0] == '-' || text.empty() || !IsValidWord(text)) {
                throw invalid_argument("Bad plus word."s);
            }
            else {
                query_word = { text, is_minus, IsStopWord(text) };
            }
        }
        return query_word;
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        try {
            for (const string& word : SplitIntoWords(text)) {
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
        }
        catch (const invalid_argument& ex) {
            throw ex;
        }

        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 /
            word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(
        const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// ==================== для примера =========================

int main() {
    try {
        SearchServer search_server("и в на"s);
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        search_server.FindTopDocuments("--пушистый"s);
    }
    catch (const invalid_argument& ex) {
        cout << "Error: "s << ex.what() << endl;
    }
}